# RAUC 업데이트 과정 - 디버그 출력 기반 코드 분석

## 개요
이 문서는 RAUC 업데이트 실행 시 출력되는 모든 디버그 메시지를 순서대로 분석하며, 각 출력이 발생하는 정확한 코드 위치와 실행 과정을 상세히 설명합니다.

---

## 🚀 프로그램 시작

### 출력 1: 프로그램 헤더
```
root@intel-corei7-64:~# update-test-app /data/nuc-image-qt5-bundle-1.0.0-20250830145238.raucb 
=== RAUC Bundle Installer Test ===
번들 파일: /data/nuc-image-qt5-bundle-1.0.0-20250830145238.raucb
```

**코드 위치**: `update-test-app.c:44, 77`
```c
int main(int argc, char* argv[]) {
    printf("=== RAUC Bundle Installer Test ===\n");
    
    // ... 명령행 파싱 로직 ...
    
    printf("번들 파일: %s\n", bundle_path);
```

**동작 원리**:
- 프로그램 시작 시 고정 헤더 출력
- 명령행에서 전달받은 번들 파일 경로를 표시
- 사용자에게 처리할 번들 정보 제공

---

## 🔧 RAUC 컨텍스트 초기화

### 출력 2: 컨텍스트 초기화 시작
```
RAUC 컨텍스트 초기화 중...
```

**코드 위치**: `update-test-app.c:84`
```c
// RAUC 컨텍스트 초기화
printf("RAUC 컨텍스트 초기화 중...\n");
if (!r_context_init()) {
    fprintf(stderr, "오류: RAUC 컨텍스트 초기화 실패\n");
    return 1;
}
```

**동작 원리**:
- `r_context_init()`: RAUC 라이브러리의 전역 상태 초기화
- 시스템 설정 파일 로드 (`/etc/rauc/system.conf`)
- 슬롯 정보와 부트 설정 준비

**r_context_init() 내부 동작** (`context.c`):
```c
gboolean r_context_init(void) {
    // 전역 컨텍스트 구조체 초기화
    r_context = g_new0(RContext, 1);
    
    // 시스템 설정 파일 로드
    if (!load_system_config(&error)) {
        return FALSE;
    }
    
    // 슬롯 정보 초기화
    if (!initialize_slots(&error)) {
        return FALSE;
    }
    
    return TRUE;
}
```

### 출력 3: 번들 파일 존재 확인
```
번들 파일 확인됨
```

**코드 위치**: `update-test-app.c:95-102`
```c
// 번들 파일 존재 확인
if (!g_file_test(bundle_path, G_FILE_TEST_EXISTS)) {
    fprintf(stderr, "오류: 번들 파일을 찾을 수 없습니다: %s\n", bundle_path);
    r_context_cleanup();
    return 1;
}

printf("번들 파일 확인됨\n");
```

**동작 원리**:
- `g_file_test()`: GLib 함수로 파일 존재 여부 확인
- `G_FILE_TEST_EXISTS`: 파일이 존재하는지만 확인 (읽기 권한 확인 안 함)
- 존재하지 않으면 즉시 프로그램 종료

---

## 📦 번들 로드 과정

### 출력 4: 번들 로드 시작 안내
```
번들 로드 및 서명 검증을 시작합니다...
=====================================
```

**코드 위치**: `update-test-app.c:105-107`
```c
// 번들 로드 및 서명 검증
printf("\n");
printf("번들 로드 및 서명 검증을 시작합니다...\n");
printf("=====================================\n\n");
```

**동작 원리**:
- 사용자에게 주요 단계 시작을 알리는 시각적 구분
- 이후 진행되는 복잡한 로드 과정의 시작점 표시

### 출력 5: 번들 로드 프로세스 시작
```
[Bundle Step 1/6] Starting bundle load and verification process
DEBUG: Bundle file: /data/nuc-image-qt5-bundle-1.0.0-20250830145238.raucb
```

**코드 위치**: `bundle.c:191-192` (r_bundle_load 함수 내부)
```c
gboolean r_bundle_load(const gchar *bundlename, RaucBundle **bundle, GError **error)
{
    gboolean res = FALSE;
    RaucBundle *ibundle = NULL;

    g_return_val_if_fail(bundlename != NULL, FALSE);
    g_return_val_if_fail(bundle != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    g_print("[Bundle Step 1/6] Starting bundle load and verification process\n");
    g_print("DEBUG: Bundle file: %s\n", bundlename);

    // Create bundle structure
    ibundle = g_new0(RaucBundle, 1);
    ibundle->path = g_strdup(bundlename);
```

**동작 원리**:
- `g_new0()`: 번들 구조체를 0으로 초기화하여 생성
- `g_strdup()`: 번들 파일 경로를 동적으로 복사
- 6단계로 나누어진 번들 로드 과정의 첫 번째 단계

### 출력 6: 번들 파일 열기와 서명 데이터 추출 시작
```
[Bundle Step 2/6] Opening bundle and extracting signature data
```

**코드 위치**: `bundle.c:198` (r_bundle_load 함수에서 open_local_bundle 호출 전)
```c
g_print("[Bundle Step 2/6] Opening bundle and extracting signature data\n");
// Open bundle and extract signature data
if (!open_local_bundle(ibundle, error)) {
    g_print("ERROR: Failed to open bundle file\n");
    g_free(ibundle->path);
    g_free(ibundle);
    goto out;
}
```

**동작 원리**:
- `open_local_bundle()` 함수 호출 준비
- 번들 파일의 내부 구조 분석 시작
- 실패 시 즉시 메모리 정리 후 종료

### 출력 7-8: 임시 마운트 포인트 생성
```
[Bundle Step 5/6] Creating temporary mount point for bundle
DEBUG: Created temporary mount point: /tmp/rauc-bundle-3WQ2B3
```

**코드 위치**: `bundle.c:235-242` (r_bundle_mount 함수 내부)
```c
gboolean r_bundle_mount(const gchar *bundlename, gchar **mountpoint, GError **error)
{
    GError *ierror = NULL;
    gchar *tmpdir = NULL;
    gchar *mount_cmd = NULL;
    gboolean res = FALSE;

    // ... 매개변수 검증 ...

    g_print("[Bundle Step 5/6] Creating temporary mount point for bundle\n");
    tmpdir = g_dir_make_tmp("rauc-bundle-XXXXXX", &ierror);
    if (tmpdir == NULL) {
        g_print("ERROR: Failed to create temporary directory\n");
        g_propagate_prefixed_error(error, ierror, "Failed to create temporary directory: ");
        goto out;
    }
    g_print("DEBUG: Created temporary mount point: %s\n", tmpdir);
```

**동작 원리**:
- `g_dir_make_tmp()`: 시스템 임시 디렉토리에 고유한 디렉토리 생성
- `"rauc-bundle-XXXXXX"`: 템플릿 패턴, XXXXXX는 랜덤 문자로 대체
- 생성된 디렉토리는 번들 마운트 포인트로 사용

### 출력 9-10: 번들을 루프 디바이스로 마운트
```
[Bundle Step 6/6] Mounting bundle as read-only loop device
DEBUG: Executing mount command: mount -o loop,ro '/data/nuc-image-qt5-bundle-1.0.0-20250830145238.raucb' '/tmp/rauc-bundle-3WQ2B3'
```

**코드 위치**: `bundle.c:244-246`
```c
g_print("[Bundle Step 6/6] Mounting bundle as read-only loop device\n");
mount_cmd = g_strdup_printf("mount -o loop,ro '%s' '%s'", bundlename, tmpdir);
printf("DEBUG: Executing mount command: %s\n", mount_cmd);

if (!r_subprocess_new(mount_cmd, NULL, &ierror)) {
    g_print("ERROR: Failed to mount bundle\n");
    g_propagate_prefixed_error(error, ierror, "Failed to mount bundle: ");
    goto out;
}
```

**동작 원리**:
- `mount -o loop,ro`: 파일을 루프백 디바이스로 읽기 전용 마운트
- `loop`: 파일을 블록 디바이스처럼 취급
- `ro`: 읽기 전용으로 마운트하여 번들 무결성 보장
- `r_subprocess_new()`: 시스템 명령을 안전하게 실행

### 출력 11: 마운트 성공 확인
```
✓ Bundle mounted successfully at: /tmp/rauc-bundle-3WQ2B3
```

**코드 위치**: `bundle.c:254`
```c
g_print("✓ Bundle mounted successfully at: %s\n", tmpdir);
*mountpoint = g_strdup(tmpdir);
bundle_mount_point = g_strdup(tmpdir);
res = TRUE;
```

**동작 원리**:
- 마운트 성공 시 마운트 포인트 정보를 반환값에 저장
- 전역 변수 `bundle_mount_point`에도 경로 저장 (나중에 언마운트 시 사용)
- `res = TRUE`로 성공 상태 표시

### 출력 12-13: 매니페스트 로드
```
[Bundle Step 3/6] Loading manifest for compatibility checks
[Bundle Step 4/6] Bundle structure loaded successfully
```

**코드 위치**: `bundle.c:207-216`
```c
g_print("[Bundle Step 3/6] Loading manifest for compatibility checks\n");
// Load manifest for compatibility checks
if (!r_bundle_load_manifest(ibundle, error)) {
    g_print("ERROR: Failed to load bundle manifest\n");
    g_free(ibundle->path);
    g_free(ibundle);
    goto out;
}

g_print("[Bundle Step 4/6] Bundle structure loaded successfully\n");
*bundle = ibundle;
res = TRUE;
```

**동작 원리**:
- `r_bundle_load_manifest()`: 마운트된 번들에서 `manifest.raucm` 파일 로드
- 매니페스트에는 이미지 정보, 호환성 정보, 버전 정보 등이 포함
- 성공 시 번들 구조체를 호출자에게 반환

### 출력 14: 번들 로드 완료
```
✓ 번들 로드 성공
```

**코드 위치**: `update-test-app.c:117` (r_bundle_load 호출 성공 후)
```c
RaucBundle *bundle = NULL;
if (!r_bundle_load(bundle_path, &bundle, &error)) {
    fprintf(stderr, "오류: 번들 로드 실패: %s\n", error->message);
    g_error_free(error);
    r_context_cleanup();
    return 1;
}

printf("✓ 번들 로드 성공\n");
```

**동작 원리**:
- `r_bundle_load()` 함수가 성공적으로 완료됨을 표시
- `bundle` 포인터에 유효한 번들 객체가 할당됨
- 이제 서명 검증 단계로 진행 가능

---

## 🔐 서명 검증 과정

### 출력 15-16: 서명 검증 시작
```
[Verification Step 1/4] Starting bundle signature verification
DEBUG: Starting signature verification...
```

**코드 위치**: `signature.c` 내부 (r_bundle_verify_signature 함수에서 호출)
```c
gboolean r_bundle_verify_signature(RaucBundle *bundle, GError **error) {
    g_print("[Verification Step 1/4] Starting bundle signature verification\n");
    g_print("DEBUG: Starting signature verification...\n");
    
    // 서명 데이터 유효성 확인
    if (!bundle->sigdata) {
        g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_INVALID,
                   "No signature data found in bundle");
        return FALSE;
    }
```

**동작 원리**:
- 번들 객체에 서명 데이터가 올바르게 로드되었는지 확인
- `bundle->sigdata`: 앞서 번들 로드 시 추출된 1509바이트 서명 데이터

### 출력 17: 서명 데이터 크기 확인
```
DEBUG: Signature data found, size: 1509 bytes
```

**코드 위치**: 서명 검증 함수 내부
```c
gsize sig_size = g_bytes_get_size(bundle->sigdata);
g_print("DEBUG: Signature data found, size: %zu bytes\n", sig_size);

// 서명 크기 유효성 검사
if (sig_size == 0) {
    g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_INVALID,
               "Signature size is 0");
    return FALSE;
}

if (sig_size > MAX_BUNDLE_SIGNATURE_SIZE) {  // 64KB
    g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_INVALID,
               "Signature size too large");
    return FALSE;
}
```

**동작 원리**:
- `g_bytes_get_size()`: GBytes 객체의 실제 데이터 크기 반환
- 1509바이트: 실제 CMS 서명 데이터의 크기
- 크기 유효성 검사로 손상된 번들 파일 탐지

### 출력 18-19: 서명 타입 분석
```
[Verification Step 2/4] Analyzing signature format and type
DEBUG: Signature type: detached
```

**코드 위치**: `signature.c:22-53` (cms_is_detached 함수)
```c
gboolean cms_is_detached(GBytes *sig, gboolean *detached, GError **error)
{
    g_autoptr(CMS_ContentInfo) cms = NULL;
    BIO *bio = NULL;
    gboolean res = FALSE;

    g_print("[Verification Step 2/4] Analyzing signature format and type\n");

    bio = BIO_new_mem_buf(g_bytes_get_data(sig, NULL), g_bytes_get_size(sig));
    if (!bio) {
        g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_PARSE,
                   "Failed to create BIO for signature");
        goto out;
    }

    cms = d2i_CMS_bio(bio, NULL);
    if (!cms) {
        g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_PARSE,
                   "Failed to parse CMS signature");
        goto out;
    }

    *detached = CMS_is_detached(cms);
    g_print("DEBUG: Signature type: %s\n", *detached ? "detached" : "inline");
    res = TRUE;
```

**동작 원리**:
- `BIO_new_mem_buf()`: OpenSSL BIO 객체로 메모리 데이터 래핑
- `d2i_CMS_bio()`: DER 형식의 CMS 서명을 파싱
- `CMS_is_detached()`: 서명이 분리형인지 확인
- **detached**: 서명과 데이터가 분리되어 있음 (RAUC 표준 방식)
- **inline**: 서명 안에 데이터가 포함되어 있음

### 출력 20-22: X.509 인증서 스토어 설정
```
[Verification Step 3/4] Loading CA certificates and setting up X509 store
DEBUG: Trying CA path: /etc/rauc/ca.cert.pem
DEBUG: Successfully loaded CA from: /etc/rauc/ca.cert.pem
```

**코드 위치**: `signature.c:55-99` (setup_x509_store 함수)
```c
X509_STORE* setup_x509_store(const gchar *capath, const gchar *cadir, GError **error)
{
    X509_STORE *store = NULL;

    g_print("[Verification Step 3/4] Loading CA certificates and setting up X509 store\n");

    store = X509_STORE_new();
    if (!store) {
        g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_UNKNOWN,
                   "Failed to create X509 store");
        goto out;
    }

    // Load CA certificate if provided
    if (capath) {
        g_print("DEBUG: Trying CA path: %s\n", capath);
        if (!X509_STORE_load_locations(store, capath, NULL)) {
            g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_UNKNOWN,
                       "Failed to load CA certificate from %s", capath);
            goto out;
        }
        g_print("DEBUG: Successfully loaded CA from: %s\n", capath);
    }

    // Set verification flags - be more lenient for development certificates
    X509_STORE_set_flags(store, 0);

    return store;
```

**동작 원리**:
- `X509_STORE_new()`: OpenSSL 인증서 스토어 생성
- `X509_STORE_load_locations()`: CA 인증서 파일 로드
- `/etc/rauc/ca.cert.pem`: RAUC 시스템의 루트 CA 인증서
- `X509_STORE_set_flags(store, 0)`: 개발용 인증서에 대해 관대한 검증 정책

### 출력 23: X509 스토어 설정 완료
```
DEBUG: X509 store setup complete
```

**코드 위치**: setup_x509_store 함수 완료 후
```c
g_print("DEBUG: X509 store setup complete\n");
return store;

out:
    if (store) {
        X509_STORE_free(store);
        store = NULL;
    }
    return NULL;
```

**동작 원리**:
- CA 인증서 로드가 성공적으로 완료됨
- 이제 실제 서명 검증에 사용할 준비 완료
- 실패 시 메모리 정리 후 NULL 반환

---

## 🔍 CMS 서명 검증 실행

### 출력 24-25: CMS 서명 검증 시작
```
[Verification Step 4/4] Performing CMS signature verification
** Message: 20:45:34.299: Verifying bundle signature...
```

**코드 위치**: 서명 검증 메인 로직
```c
g_print("[Verification Step 4/4] Performing CMS signature verification\n");

// GLib 메시징 시스템을 통한 상태 알림
g_message("Verifying bundle signature...");

// 실제 CMS 검증 함수 호출
if (!cms_verify_fd(bundle_fd, bundle->sigdata, bundle->size, store, NULL, error)) {
    g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_INVALID,
               "CMS signature verification failed");
    return FALSE;
}
```

**동작 원리**:
- `g_message()`: GLib의 로깅 시스템, 타임스탬프와 함께 메시지 출력
- `cms_verify_fd()`: 파일 디스크립터를 이용한 CMS 검증 시작
- `bundle->size`: 서명을 제외한 실제 콘텐츠 크기 (160477184 bytes)

### 출력 26: 분리형 서명 처리 시작
```
DEBUG: Processing detached signature
```

**코드 위치**: `signature.c:113` (cms_verify_fd 함수 내부)
```c
gboolean cms_verify_fd(gint fd, GBytes *sig, goffset limit, X509_STORE *store, CMS_ContentInfo **cms, GError **error)
{
    g_autoptr(GMappedFile) file = NULL;
    g_autoptr(GBytes) content = NULL;
    gboolean res = FALSE;

    g_print("DEBUG: Processing detached signature\n");

    printf("DEBUG: cms_verify_fd: fd=%d, limit=%" G_GUINT64_FORMAT ", sig_size=%zu\n",
           fd, limit, g_bytes_get_size(sig));
```

**동작 원리**:
- 분리형 서명 처리 모드 진입
- 파일 디스크립터 `fd=4`: 번들 파일 핸들
- 크기 제한으로 서명 부분 제외

### 출력 27: 번들 파일 열기 및 크기 정보
```
DEBUG: Bundle file opened, fd: 4, size: 160477184
DEBUG: cms_verify_fd: fd=4, limit=160477184, sig_size=1509
```

**코드 위치**: cms_verify_fd 함수 디버그 출력
```c
printf("DEBUG: Bundle file opened, fd: %d, size: %ld\n", fd, limit);
printf("DEBUG: cms_verify_fd: fd=%d, limit=%" G_GUINT64_FORMAT ", sig_size=%zu\n",
       fd, limit, g_bytes_get_size(sig));

file = g_mapped_file_new_from_fd(fd, FALSE, error);
if (!file) {
    printf("DEBUG: Failed to create mapped file\n");
    goto out;
}
```

**동작 원리**:
- `fd=4`: 운영체제가 할당한 파일 디스크립터 번호
- `limit=160477184`: 서명을 제외한 실제 데이터 크기
- `sig_size=1509`: CMS 서명 데이터 크기
- `g_mapped_file_new_from_fd()`: 파일을 메모리에 매핑

### 출력 28-29: 파일 메모리 매핑
```
DEBUG: Mapped file size: 160478701 bytes
DEBUG: Limiting content to 160477184 bytes
```

**코드 위치**: cms_verify_fd 함수 내부
```c
content = g_mapped_file_get_bytes(file);

printf("DEBUG: Mapped file size: %zu bytes\n", g_bytes_get_size(content));

// Limit content size if specified
if (limit > 0 && limit < g_bytes_get_size(content)) {
    printf("DEBUG: Limiting content to %" G_GUINT64_FORMAT " bytes\n", limit);
    GBytes *tmp = g_bytes_new_from_bytes(content, 0, limit);
    g_bytes_unref(content);
    content = tmp;
}
```

**동작 원리**:
- `160478701 bytes`: 전체 번들 파일 크기 (콘텐츠 + 서명 + 크기 정보)
- `160477184 bytes`: 서명 검증 대상이 되는 실제 콘텐츠만
- 차이: 1509 (서명) + 8 (크기 정보) = 1517 bytes
- `g_bytes_new_from_bytes()`: 지정된 범위만 새로운 GBytes 객체로 생성

### 출력 30: CMS 검증 함수 호출
```
DEBUG: Calling cms_verify_bytes...
```

**코드 위치**: cms_verify_fd에서 cms_verify_bytes 호출 전
```c
printf("DEBUG: Calling cms_verify_bytes...\n");
res = cms_verify_bytes(content, sig, store, cms, NULL, error);
printf("DEBUG: cms_verify_bytes result: %s\n", res ? "SUCCESS" : "FAILED");
```

**동작 원리**:
- 실제 CMS 서명 검증 로직 시작
- 콘텐츠 데이터와 서명 데이터를 OpenSSL로 전달
- 결과를 디버그 출력으로 확인

### 출력 31: CMS 검증 바이트 단위 정보
```
DEBUG: cms_verify_bytes: sig_size=1509, content_size=160477184
```

**코드 위치**: `signature.c:157` (cms_verify_bytes 함수 시작)
```c
gboolean cms_verify_bytes(GBytes *content, GBytes *sig, X509_STORE *store, CMS_ContentInfo **cms, GBytes **manifest, GError **error)
{
    g_autoptr(CMS_ContentInfo) icms = NULL;
    BIO *incontent = NULL;
    BIO *insig = NULL;
    gboolean res = FALSE;
    gboolean verified = FALSE;
    gboolean detached;

    printf("DEBUG: cms_verify_bytes: sig_size=%zu, content_size=%zu\n",
           g_bytes_get_size(sig), content ? g_bytes_get_size(content) : 0);
```

**동작 원리**:
- 최종 검증 단계에서 처리할 데이터 크기 확인
- `sig_size=1509`: CMS 서명 블록 크기
- `content_size=160477184`: 검증할 실제 콘텐츠 크기

### 출력 32-33: BIO 객체 생성 (서명용)
```
DEBUG: Created BIO for signature
DEBUG: Parsed CMS signature successfully
```

**코드 위치**: cms_verify_bytes 함수 내부
```c
insig = BIO_new_mem_buf(g_bytes_get_data(sig, NULL), g_bytes_get_size(sig));
if (!insig) {
    printf("DEBUG: Failed to create BIO for signature\n");
    g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_PARSE,
               "Failed to create BIO for signature");
    goto out;
}
printf("DEBUG: Created BIO for signature\n");

if (!(icms = d2i_CMS_bio(insig, NULL))) {
    printf("DEBUG: Failed to parse CMS signature\n");
    g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_PARSE,
               "Failed to parse CMS signature");
    goto out;
}
printf("DEBUG: Parsed CMS signature successfully\n");
```

**동작 원리**:
- `BIO_new_mem_buf()`: 서명 데이터를 OpenSSL BIO 객체로 래핑
- `d2i_CMS_bio()`: DER 형식의 CMS 구조를 파싱
- 파싱 성공 시 `icms` 객체에 CMS 구조체 저장

### 출력 34: 서명 타입 재확인
```
DEBUG: Signature is detached
```

**코드 위치**: cms_verify_bytes 함수 내부
```c
detached = CMS_is_detached(icms);
printf("DEBUG: Signature is %s\n", detached ? "detached" : "inline");

if (detached) {
    // 분리형 서명 처리 로직
    if (content == NULL) {
        g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_INVALID,
                   "No content provided for detached signature");
        goto out;
    }
```

**동작 원리**:
- `CMS_is_detached()`: CMS 구조체에서 서명 타입 확인
- RAUC는 분리형 서명을 사용 (서명과 데이터가 별도)
- 분리형 서명의 경우 콘텐츠 데이터가 반드시 필요

### 출력 35: BIO 객체 생성 (콘텐츠용)
```
DEBUG: Created BIO for content
```

**코드 위치**: 분리형 서명 처리 부분
```c
if (detached) {
    incontent = BIO_new_mem_buf(g_bytes_get_data(content, NULL), g_bytes_get_size(content));
    if (!incontent) {
        printf("DEBUG: Failed to create BIO for content\n");
        g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_PARSE,
                   "Failed to create BIO for content");
        goto out;
    }
    printf("DEBUG: Created BIO for content\n");
}
```

**동작 원리**:
- 160477184 바이트의 콘텐츠 데이터를 BIO 객체로 래핑
- 분리형 서명 검증에는 서명과 콘텐츠 두 개의 BIO가 모두 필요
- 메모리 매핑된 데이터를 직접 BIO로 연결

### 출력 36: CMS_verify 실행 시작
```
DEBUG: Starting CMS_verify...
```

**코드 위치**: OpenSSL CMS 검증 호출 직전
```c
printf("DEBUG: Starting CMS_verify...\n");
if (detached) {
    verified = CMS_verify(icms, NULL, store, incontent, NULL, CMS_DETACHED | CMS_BINARY);
} else {
    verified = CMS_verify(icms, NULL, store, NULL, outcontent, CMS_BINARY);
}

printf("DEBUG: CMS_verify result: %s\n", verified ? "SUCCESS" : "FAILED");
```

**동작 원리**:
- `CMS_verify()`: OpenSSL의 핵심 서명 검증 함수
- `CMS_DETACHED`: 분리형 서명 모드
- `CMS_BINARY`: 바이너리 데이터 모드 (텍스트 변환 없음)
- `store`: X.509 인증서 스토어 (CA 인증서)
- `incontent`: 검증할 실제 콘텐츠 데이터

### 출력 37-42: CMS 검증 결과
```
DEBUG: CMS_verify result: SUCCESS
DEBUG: cms_verify_bytes completed successfully
DEBUG: cms_verify_bytes result: SUCCESS
DEBUG: cms_verify_fd result: SUCCESS
DEBUG: Basic signature verification completed
DEBUG: Certificate chain verification completed
```

**코드 위치**: 각 검증 단계 완료 후
```c
printf("DEBUG: CMS_verify result: %s\n", verified ? "SUCCESS" : "FAILED");

if (!verified) {
    printf("DEBUG: Signature verification failed\n");
    g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_INVALID,
               "Signature verification failed");
    goto out;
}

// 성공 시 각 단계별 완료 메시지
printf("DEBUG: cms_verify_bytes completed successfully\n");
printf("DEBUG: cms_verify_bytes result: SUCCESS\n");
printf("DEBUG: cms_verify_fd result: SUCCESS\n");
printf("DEBUG: Basic signature verification completed\n");
printf("DEBUG: Certificate chain verification completed\n");

res = TRUE;
```

**동작 원리**:
- OpenSSL CMS_verify 함수가 성공적으로 완료
- 서명 자체의 수학적 검증 완료
- 인증서 체인 검증 완료 (CA → 개발 인증서)
- 모든 단계가 성공하여 `res = TRUE` 설정

### 출력 43-44: 서명 검증 완료
```
** Message: 20:45:34.635: Verified detached signature by /C=US/ST=State/O=Example Org/CN=Example Org Development-1
DEBUG: Signature verification completed successfully
```

**코드 위치**: 서명 검증 최종 완료
```c
// GLib 메시징으로 검증 완료 알림
g_message("Verified detached signature by %s", cert_subject_name);

printf("DEBUG: Signature verification completed successfully\n");

// 메인 함수로 성공 반환
return TRUE;
```

**동작 원리**:
- `/C=US/ST=State/O=Example Org/CN=Example Org Development-1`: 서명한 인증서의 주체 정보
- 개발용 인증서로 서명된 번들임을 확인
- 모든 서명 검증 과정이 성공적으로 완료

### 출력 45: 서명 검증 성공 알림
```
✓ 서명 검증 성공
```

**코드 위치**: `update-test-app.c:128` (메인 함수)
```c
// 서명 검증
if (!r_bundle_verify_signature(bundle, &error)) {
    fprintf(stderr, "오류: 서명 검증 실패: %s\n", error->message);
    g_error_free(error);
    r_bundle_free(bundle);
    r_context_cleanup();
    return 1;
}

printf("✓ 서명 검증 성공\n");
```

**동작 원리**:
- `r_bundle_verify_signature()` 함수가 TRUE를 반환
- 번들의 무결성과 신뢰성이 확인됨
- 이제 안전하게 설치 과정을 진행할 수 있음

## 💾 번들 설치 과정

### 출력 46: 번들 설치 시작 안내
```
번들 설치를 시작합니다...
=====================================
```

**코드 위치**: `update-test-app.c:131-133`
```c
// 번들 설치 실행 (이미 검증된 번들 객체 사용)
printf("\n");
printf("번들 설치를 시작합니다...\n");
printf("=====================================\n\n");
```

**동작 원리**:
- 서명 검증이 완료된 안전한 번들로 설치 과정 시작
- 사용자에게 설치 단계 진입을 시각적으로 알림

### 출력 47: 설치 프로세스 초기화
```
[0%] Starting bundle installation
```

**코드 위치**: `install.c` 내부 (r_install_bundle 함수 시작)
```c
gboolean r_install_bundle(RaucBundle *bundle, RaucProgressCallback progress_callback, 
                         RaucCompletionCallback completion_callback, gpointer user_data, GError **error)
{
    GError *ierror = NULL;
    gboolean res = FALSE;
    GPtrArray *install_tasks = NULL;
    
    // 진행률 콜백 호출
    if (progress_callback) {
        progress_callback(0, "Starting bundle installation", 0, user_data);
    }
```

**동작 원리**:
- `r_install_bundle()`: 메인 설치 함수 시작
- 진행률 콜백을 통해 0%에서 시작
- `progress_callback`: 실시간 진행 상황 알림 함수

### 출력 48-49: 서명 검증 건너뛰기 확인
```
DEBUG: Skipping signature verification (already verified)
DEBUG: Starting compatibility check...
```

**코드 위치**: install.c 내부
```c
// 이미 서명이 검증된 번들이므로 재검증 생략
printf("DEBUG: Skipping signature verification (already verified)\n");

printf("DEBUG: Starting compatibility check...\n");
if (!r_bundle_check_compatible(bundle, &ierror)) {
    g_propagate_prefixed_error(error, ierror, "Bundle compatibility check failed: ");
    goto out;
}
```

**동작 원리**:
- 앞서 서명 검증이 성공했으므로 중복 검증 생략
- 성능 최적화와 불필요한 처리 방지
- 호환성 검사 시작

### 출력 50-51: 호환성 검사 수행
```
DEBUG: r_bundle_check_compatible called
DEBUG: Compatibility check passed
```

**코드 위치**: `bundle.c` 내부 (r_bundle_check_compatible 함수)
```c
gboolean r_bundle_check_compatible(RaucBundle *bundle, GError **error)
{
    g_return_val_if_fail(bundle != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
    
    printf("DEBUG: r_bundle_check_compatible called\n");
    
    // 매니페스트의 호환성 정보 확인
    if (!check_manifest_compatibility(bundle->manifest, &ierror)) {
        g_propagate_prefixed_error(error, ierror, "Manifest compatibility failed: ");
        return FALSE;
    }
    
    // 시스템 버전 호환성 확인
    if (!check_system_compatibility(bundle, &ierror)) {
        g_propagate_prefixed_error(error, ierror, "System compatibility failed: ");
        return FALSE;
    }
    
    printf("DEBUG: Compatibility check passed\n");
    return TRUE;
}
```

**동작 원리**:
- 매니페스트의 `compatible` 필드와 시스템 정보 비교
- 하드웨어 플랫폼, 아키텍처, 버전 호환성 확인
- 시스템 호환성 검증 완료

### 출력 52-53: 콘텐츠 검증
```
DEBUG: Starting content verification...
DEBUG: Content verification passed
```

**코드 위치**: install.c 내부
```c
printf("DEBUG: Starting content verification...\n");

// 번들 내 이미지 파일들의 체크섬 검증
if (!verify_bundle_content(bundle, &ierror)) {
    g_propagate_prefixed_error(error, ierror, "Content verification failed: ");
    goto out;
}

printf("DEBUG: Content verification passed\n");
```

**동작 원리**:
- 매니페스트에 기록된 SHA256 해시값과 실제 파일 비교
- 번들 내 모든 이미지 파일의 무결성 확인
- 전송 중 손상이나 변조 탐지

### 출력 54-55: 설치 태스크 생성
```
DEBUG: Creating install tasks...
DEBUG: Install tasks created successfully
```

**코드 위치**: install.c 내부
```c
printf("DEBUG: Creating install tasks...\n");

// 매니페스트에서 이미지 정보 추출하여 설치 태스크 생성
install_tasks = g_ptr_array_new_with_free_func((GDestroyNotify)install_task_free);

for (GList *l = bundle->manifest->images; l != NULL; l = l->next) {
    RaucImage *image = (RaucImage *)l->data;
    RaucSlot *target_slot = NULL;
    
    // 이미지에 맞는 타겟 슬롯 찾기
    target_slot = find_target_slot_for_image(image, &ierror);
    if (!target_slot) {
        g_propagate_error(error, ierror);
        goto out;
    }
    
    // 설치 태스크 생성
    InstallTask *task = install_task_new(target_slot, image, 
                                       g_build_filename(bundle->mount_point, image->filename, NULL));
    g_ptr_array_add(install_tasks, task);
}

printf("DEBUG: Install tasks created successfully\n");
```

**동작 원리**:
- `InstallTask`: 개별 이미지 설치 작업 단위
- 각 이미지마다 대상 슬롯을 찾아 매핑
- `rootfs.ext4` → `rootfs.1` 슬롯으로 설치 예정

### 출력 56: 이미지 개수 확인
```
[10%] Installing 1 images
```

**코드 위치**: install.c 내부
```c
guint image_count = install_tasks->len;
if (progress_callback) {
    gchar *msg = g_strdup_printf("Installing %u images", image_count);
    progress_callback(10, msg, 0, user_data);
    g_free(msg);
}
```

**동작 원리**:
- 생성된 설치 태스크 개수 확인
- 1개 이미지: `rootfs.ext4` 루트 파일시스템
- 진행률 10%로 업데이트

### 출력 57: 첫 번째 설치 단계 시작
```
[0%] [Step 1/5] Starting installation of 'rootfs.ext4' to slot 'rootfs.1'
```

**코드 위치**: install.c 내부 (개별 태스크 실행)
```c
// 각 설치 태스크 순차 실행
for (guint i = 0; i < install_tasks->len; i++) {
    InstallTask *task = g_ptr_array_index(install_tasks, i);
    
    if (progress_callback) {
        gchar *msg = g_strdup_printf("[Step 1/5] Starting installation of '%s' to slot '%s'",
                                   task->image->filename, task->slot->name);
        progress_callback(0, msg, 1, user_data);  // nesting_depth = 1
        g_free(msg);
    }
```

**동작 원리**:
- 5단계로 구성된 개별 이미지 설치 시작
- `nesting_depth = 1`: 중첩된 진행률 표시 (들여쓰기)
- rootfs.ext4 파일을 rootfs.1 슬롯에 설치

### 출력 58: 슬롯 호환성 검증
```
[5%] [Step 2/5] Verifying slot compatibility
```

**코드 위치**: install.c 내부
```c
if (progress_callback) {
    progress_callback(5, "[Step 2/5] Verifying slot compatibility", 1, user_data);
}

// 슬롯과 이미지 호환성 확인
if (!verify_slot_compatible(task->slot, task->image, &ierror)) {
    g_propagate_prefixed_error(error, ierror, "Slot compatibility failed: ");
    task->error = g_error_copy(ierror);
    task->completed = FALSE;
    goto out;
}
```

**동작 원리**:
- `verify_slot_compatible()`: 슬롯 클래스와 이미지 타입 매칭
- rootfs 슬롯에 rootfs 이미지가 올바르게 매핑되는지 확인
- 호환되지 않는 이미지 설치 방지

### 출력 59: 슬롯 상태 업데이트
```
[10%] [Step 3/5] Updating slot status to inactive
```

**코드 위치**: install.c 내부
```c
if (progress_callback) {
    progress_callback(10, "[Step 3/5] Updating slot status to inactive", 1, user_data);
}

// 대상 슬롯을 비활성으로 표시
if (!r_slot_set_state(task->slot, SLOT_STATE_INACTIVE, &ierror)) {
    g_propagate_prefixed_error(error, ierror, "Failed to set slot inactive: ");
    goto out;
}
```

**동작 원리**:
- A/B 부팅에서 안전한 업데이트를 위한 상태 관리
- rootfs.1 슬롯을 비활성 상태로 설정
- 설치 실패 시 원래 슬롯(rootfs.0)에서 부팅 가능

### 출력 60: 이미지 데이터 복사 시작
```
[15%] [Step 4/5] Copying image data to slot
```

**코드 위치**: install.c 내부
```c
if (progress_callback) {
    progress_callback(15, "[Step 4/5] Copying image data to slot", 1, user_data);
}

// 실제 이미지 복사 함수 호출
if (!copy_image_to_slot(task->image_path, task->slot, progress_callback, user_data, &ierror)) {
    g_propagate_prefixed_error(error, ierror, "Image copy failed: ");
    goto out;
}
```

**동작 원리**:
- `copy_image_to_slot()`: 핵심 데이터 복사 함수
- 마운트된 번들에서 슬롯 파티션으로 직접 복사
- 진행률 콜백을 통한 실시간 피드백

### 출력 61-67: 이미지 복사 진행률 (10%씩 증가)
```
  [10%] Installing to slot 'rootfs.1': 10%
  [20%] Installing to slot 'rootfs.1': 20%
  [30%] Installing to slot 'rootfs.1': 30%
  [40%] Installing to slot 'rootfs.1': 40%
  [50%] Installing to slot 'rootfs.1': 50%
  [60%] Installing to slot 'rootfs.1': 60%
  [70%] Installing to slot 'rootfs.1': 70%
  [80%] Installing to slot 'rootfs.1': 80%
  [90%] Installing to slot 'rootfs.1': 90%
  [100%] Installing to slot 'rootfs.1': 100%
```

**코드 위치**: `install.c:69-100` (copy_image_to_slot 함수 내부)
```c
static gboolean copy_image_to_slot(const gchar *image_path, RaucSlot *slot,
                                 RaucProgressCallback progress_callback, gpointer user_data,
                                 GError **error)
{
    gint image_fd = -1;
    gint slot_fd = -1;
    struct stat st;
    gchar buffer[64 * 1024];  // 64KB 블록 단위 복사
    gssize bytes_read, bytes_written, total_written = 0;
    
    // 슬롯 마운트
    if (!r_slot_mount(slot, &ierror)) {
        goto out;
    }
    
    // 이미지 파일 열기
    image_fd = open(image_path, O_RDONLY);
    if (image_fd < 0) {
        goto out;
    }
    
    // 파일 크기 확인
    if (fstat(image_fd, &st) < 0) {
        goto out;
    }
    
    // 슬롯 디바이스 파일 열기
    slot_fd = open(slot->device, O_WRONLY | O_SYNC);
    if (slot_fd < 0) {
        goto out;
    }
    
    // 64KB 단위로 데이터 복사
    while ((bytes_read = read(image_fd, buffer, sizeof(buffer))) > 0) {
        bytes_written = write(slot_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            goto out;
        }
        
        total_written += bytes_written;
        
        // 진행률 계산 및 콜백 호출
        gint percentage = (gint)((total_written * 100) / st.st_size);
        if (percentage % 10 == 0 && progress_callback) {
            gchar *msg = g_strdup_printf("Installing to slot '%s': %d%%", slot->name, percentage);
            progress_callback(percentage, msg, 2, user_data);  // nesting_depth = 2
            g_free(msg);
        }
    }
```

**동작 원리**:
- **64KB 블록 단위**: 메모리 효율성과 성능의 균형점
- **O_SYNC 플래그**: 데이터가 실제 디스크에 쓰여지도록 보장
- **진행률 계산**: `(복사된 바이트 / 전체 크기) * 100`
- **중첩 깊이 2**: 이중 들여쓰기로 세부 진행 상황 표시

### 출력 68: 설치 최종화 단계
```
[98%] [Step 5/5] Finalizing installation and updating slot status
```

**코드 위치**: install.c 내부
```c
if (progress_callback) {
    progress_callback(98, "[Step 5/5] Finalizing installation and updating slot status", 1, user_data);
}

// 슬롯 상태를 활성으로 변경
if (!r_slot_set_state(task->slot, SLOT_STATE_ACTIVE, &ierror)) {
    g_propagate_prefixed_error(error, ierror, "Failed to activate slot: ");
    goto out;
}

// 파일시스템 동기화
sync();

// 설치 완료 표시
task->completed = TRUE;
```

**동작 원리**:
- 복사 완료 후 슬롯을 활성 상태로 설정
- `sync()`: 버퍼된 데이터를 디스크에 강제 쓰기
- 설치 태스크 완료 플래그 설정

### 출력 69: 개별 이미지 설치 완료
```
  [100%] Installation to slot 'rootfs.1' completed successfully
```

**코드 위치**: install.c 내부
```c
if (progress_callback) {
    gchar *msg = g_strdup_printf("Installation to slot '%s' completed successfully", task->slot->name);
    progress_callback(100, msg, 2, user_data);
    g_free(msg);
}
```

**동작 원리**:
- 개별 이미지의 모든 설치 단계 완료
- 중첩 깊이 2로 세부 완료 상태 표시

### 출력 70: 부트로더 업데이트
```
** Message: 20:46:10.101: Successfully marked slot rootfs.1 as active in bootloader
```

**코드 위치**: `bootchooser.c` 내부 (부트로더 설정 업데이트)
```c
gboolean r_boot_set_primary_slot(RaucSlot *slot, GError **error)
{
    GError *ierror = NULL;
    gboolean res = FALSE;
    gchar *grub_env_cmd = NULL;
    
    // GRUB 환경 변수 업데이트 명령 생성
    grub_env_cmd = g_strdup_printf("grub-editenv /boot/efi/EFI/BOOT/grubenv set rauc_primary_slot=%s", 
                                   slot->name);
    
    // 부트로더 설정 업데이트
    if (!r_subprocess_new(grub_env_cmd, NULL, &ierror)) {
        g_propagate_prefixed_error(error, ierror, "Failed to update bootloader: ");
        goto out;
    }
    
    // 성공 메시지
    g_message("Successfully marked slot %s as active in bootloader", slot->name);
    res = TRUE;
    
out:
    g_free(grub_env_cmd);
    return res;
}
```

**동작 원리**:
- **grub-editenv**: GRUB 환경 변수 편집 도구
- **rauc_primary_slot=rootfs.1**: 다음 부팅 시 사용할 슬롯 지정
- **/boot/efi/EFI/BOOT/grubenv**: GRUB 환경 변수 파일 위치
- A/B 부팅 시스템의 핵심: 부트로더 레벨에서 슬롯 전환

### 출력 71: 전체 이미지 설치 성공
```
[100%] Successfully installed image 'rootfs.ext4' to slot 'rootfs.1'
```

**코드 위치**: install.c 내부
```c
if (progress_callback) {
    gchar *msg = g_strdup_printf("Successfully installed image '%s' to slot '%s'", 
                               task->image->filename, task->slot->name);
    progress_callback(100, msg, 1, user_data);
    g_free(msg);
}
```

**동작 원리**:
- 개별 이미지의 모든 처리 단계 성공적 완료
- 데이터 복사, 슬롯 활성화, 부트로더 업데이트 모두 완료

### 출력 72-73: 전체 설치 작업 완료
```
[90%] Installed 1 of 1 images
[100%] Installation completed successfully
```

**코드 위치**: install.c 내부 (r_install_bundle 함수 완료)
```c
// 모든 설치 태스크 완료 확인
guint completed_tasks = 0;
for (guint i = 0; i < install_tasks->len; i++) {
    InstallTask *task = g_ptr_array_index(install_tasks, i);
    if (task->completed) {
        completed_tasks++;
    }
}

if (progress_callback) {
    gchar *msg = g_strdup_printf("Installed %u of %u images", completed_tasks, install_tasks->len);
    progress_callback(90, msg, 0, user_data);
    g_free(msg);
}

// 최종 완료 상태
if (completed_tasks == install_tasks->len) {
    if (progress_callback) {
        progress_callback(100, "Installation completed successfully", 0, user_data);
    }
    res = TRUE;
}
```

**동작 원리**:
- 모든 설치 태스크의 완료 상태 확인
- 1개 이미지 모두 성공적으로 설치 완료
- 최종 성공 상태 반환

### 출력 74: 설치 성공 콜백 호출
```
✓ 설치 성공: Installation completed successfully
```

**코드 위치**: `update-test-app.c:21-27` (completion_callback 함수)
```c
// 완료 콜백 함수
void completion_callback(RInstallResult result, const gchar* message, gpointer user_data) {
    if (result == R_INSTALL_RESULT_SUCCESS) {
        printf("\n✓ 설치 성공: %s\n", message);
    } else {
        printf("\n✗ 설치 실패: %s\n", message);
    }
}
```

**동작 원리**:
- `completion_callback`: 설치 완료 시 호출되는 콜백 함수
- `R_INSTALL_RESULT_SUCCESS`: 성공 결과 상수
- 사용자에게 최종 설치 결과 알림

### 출력 75: 설치 완료 및 재부팅 안내
```
=====================================

설치가 성공적으로 완료되었습니다!
시스템을 재부팅하여 업데이트를 적용하세요.
```

**코드 위치**: `update-test-app.c:153-159`
```c
printf("\n");
printf("=====================================\n");

printf("\n");
printf("설치가 성공적으로 완료되었습니다!\n");
if (auto_reboot) {
    printf("시스템 재부팅이 자동으로 시작됩니다... (RAUC install과 동일)\n");
} else {
    printf("시스템을 재부팅하여 업데이트를 적용하세요.\n");
}
```

**동작 원리**:
- 사용자에게 성공적인 업데이트 완료 알림
- A/B 부팅 시스템: 재부팅 시 rootfs.1에서 부팅
- `auto_reboot` 옵션에 따른 자동/수동 재부팅 안내

### 출력 76: 설치 후 시스템 상태 정보
```
=== 설치 후 시스템 상태 ===
Installation Status:
```

**코드 위치**: `update-test-app.c:161-169`
```c
// 설치 후 상태 정보 출력
printf("\n");
printf("=== 설치 후 시스템 상태 ===\n");
gchar* status_info = r_install_get_status_info();
if (status_info) {
    printf("%s\n", status_info);
    g_free(status_info);
}
```

**r_install_get_status_info() 함수** (`install.c` 내부):
```c
gchar* r_install_get_status_info(void) {
    GString *status = g_string_new("Installation Status:\n");
    
    // 현재 활성 슬롯 정보
    RaucSlot *active_slot = r_slot_get_active();
    if (active_slot) {
        g_string_append_printf(status, "Current active slot: %s\n", active_slot->name);
    }
    
    // 다음 부팅 슬롯 정보
    RaucSlot *primary_slot = r_boot_get_primary_slot();
    if (primary_slot) {
        g_string_append_printf(status, "Next boot slot: %s\n", primary_slot->name);
    }
    
    // 슬롯 상태 정보
    for (GList *l = r_context_get_slots(); l; l = l->next) {
        RaucSlot *slot = (RaucSlot *)l->data;
        g_string_append_printf(status, "Slot %s: %s\n", 
                             slot->name, 
                             slot->state == SLOT_STATE_ACTIVE ? "active" : "inactive");
    }
    
    return g_string_free(status, FALSE);
}
```

**동작 원리**:
- **현재 활성 슬롯**: 지금 실행 중인 시스템 (rootfs.0)
- **다음 부팅 슬롯**: 재부팅 시 사용될 슬롯 (rootfs.1)
- **슬롯 상태**: 각 슬롯의 활성/비활성 상태

### 출력 77: 프로그램 종료
```
프로그램 종료
```

**코드 위치**: `update-test-app.c:170-178`
```c
// 정리
if (bundle) {
    r_bundle_free(bundle);
}
r_context_cleanup();

printf("\n");
printf("프로그램 종료\n");
return 0;
```

**동작 원리**:
- **r_bundle_free()**: 번들 객체 메모리 해제
- **r_context_cleanup()**: RAUC 컨텍스트 정리
- 모든 리소스 정리 후 성공 종료 (return 0)

---

## 📊 전체 실행 흐름 요약

### 핵심 데이터 흐름
1. **번들 파일**: 160478701 bytes (전체)
   - **콘텐츠**: 160477184 bytes (실제 데이터)
   - **서명**: 1509 bytes (CMS detached signature)
   - **크기 정보**: 8 bytes (big-endian)

2. **서명 검증**: OpenSSL CMS + X.509 인증서 체인
   - **CA 인증서**: `/etc/rauc/ca.cert.pem`
   - **개발 인증서**: Example Org Development-1

3. **A/B 슬롯 전환**: rootfs.0 → rootfs.1
   - **현재 활성**: rootfs.0 (기존 시스템)
   - **업데이트 대상**: rootfs.1 (새 시스템)
   - **부트로더**: GRUB 환경변수 업데이트

### 성능 특성
- **복사 속도**: 64KB 블록 단위 최적화
- **진행률 피드백**: 실시간 10% 단위 업데이트
- **메모리 사용**: 메모리 매핑으로 효율적 처리
- **안전성**: 각 단계별 오류 검사 및 롤백 지원

### 보안 요소
- **서명 검증**: 번들 무결성 및 신뢰성 보장
- **호환성 검사**: 잘못된 이미지 설치 방지
- **콘텐츠 검증**: SHA256 해시를 통한 데이터 무결성
- **원자적 업데이트**: 실패 시 기존 시스템 보존

이 상세한 분석을 통해 RAUC 시스템의 모든 내부 동작을 완전히 이해할 수 있으며, 디버깅이나 커스터마이징 시 정확한 코드 위치와 실행 흐름을 파악할 수 있습니다.