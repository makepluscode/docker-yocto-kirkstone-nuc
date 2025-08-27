# RAUC Bundle Creation Study

## 1. Manifest Creation

### 1.1 Manifest File Structure
```ini
[update]
compatible=intel-i7-x64-nuc-rauc    # 시스템 호환성 식별자
version=0.0.1                       # 번들 버전
description=Intel NUC               # 번들 설명
build=20250823220912               # 빌드 타임스탬프

[image.rootfs]
sha256=a4a45f4e7935e9fd02f7a71228398628f99a7ea44e001d1991d1111f5cd888f3
size=5268045824                    # 파일 크기 (5.3 GB)
filename=nuc-image-qt5-intel-corei7-64.ext4
```

### 1.2 Manifest Generation Process
```python
# bundle.bbclass의 write_manifest() 함수
def write_manifest(d):
    manifest = open('%s/manifest.raucm' % bundle_path, 'w')
    
    # 업데이트 정보
    manifest.write('[update]\n')
    manifest.write(d.expand('compatible=${RAUC_BUNDLE_COMPATIBLE}\n'))
    manifest.write(d.expand('version=${RAUC_BUNDLE_VERSION}\n'))
    manifest.write(d.expand('description=${RAUC_BUNDLE_DESCRIPTION}\n'))
    
    # 슬롯 정보
    for slot in slots:
        manifest.write('[image.%s]\n' % slotname)
        manifest.write("filename=%s\n" % imgname)
```

## 2. Image File Copy

### 2.1 Copy Process
```python
# DEPLOY_DIR_IMAGE에서 빌드된 이미지를 번들 디렉토리로 복사
searchpath = d.expand("${DEPLOY_DIR_IMAGE}/%s") % imgsource
if os.path.isfile(searchpath):
    shutil.copy(searchpath, bundle_imgpath)
```

### 2.2 Bundle Directory Structure
```
${BUNDLE_DIR}/
├── manifest.raucm                    # 메타데이터 파일
└── nuc-image-qt5-intel-corei7-64.ext4  # 루트 파일시스템 이미지
```

## 3. Hash Calculation (SHA256)

### 3.1 Hash Generation
```c
// RAUC 내부에서 SHA256 해시 계산
SHA256(이미지_파일_내용) = 해시값

// 예시 해시
sha256=a4a45f4e7935e9fd02f7a71228398628f99a7ea44e001d1991d1111f5cd888f3
```

### 3.2 Hash Verification
```c
// bundle.c의 check_manifest_internal() 함수
if (!check_manifest_internal(manifest, &ierror)) {
    g_propagate_prefixed_error(
        error, ierror,
        "cannot sign bundle containing inconsistent manifest: ");
    return NULL;
}
```

## 4. Digital Signature Creation

### 4.1 Signature Generation Process

#### 4.1.1 Key Loading
```c
// signature.c의 cms_sign() 함수
signcert = load_cert(certfile, &ierror);  // development-1.cert.pem
pkey = load_key(keyfile, &ierror);        // development-1.key.pem
```

#### 4.1.2 Certificate Information
```bash
# 개발용 서명 인증서 상세 정보
Subject: C=US, ST=State, O=Example Org, CN=Example Org Development-1
Issuer: C=US, ST=State, L=City, O=Example Org, CN=Example Org RAUC CA Development
Public Key: RSA 2048-bit
Signature Algorithm: sha256WithRSAEncryption
Validity: 2025-08-20 ~ 2026-08-20
```

#### 4.1.3 CMS Signature Creation
```c
// signature.c의 cms_sign() 함수
cms = CMS_sign(signcert, pkey, intercerts, incontent, flags);

// 플래그 설정
int flags = CMS_BINARY | CMS_NOSMIMECAP;
if (detached) flags |= CMS_DETACHED;
```

### 4.2 RSA Signature Algorithm

#### 4.2.1 Mathematical Process
```bash
# 1. 해시 계산
SHA256(번들_파일_내용) = 해시값

# 2. PKCS#1 v1.5 패딩
패딩된_해시 = 00 01 FF FF ... FF 00 || 해시값

# 3. RSA 암호화
서명값 = 패딩된_해시^d mod n
# (d는 개인키, n은 공개키의 모듈러스)
```

#### 4.2.2 Actual Signature Value
```bash
# 256바이트 RSA 서명 (2048-bit 키)
08:8a:fb:50:e3:7f:37:90:a8:db:e5:3f:c3:87:5a:68:a0:09:
eb:11:61:06:e3:cc:96:c8:79:9d:0d:49:8f:78:d4:24:0c:35:
0f:52:ac:22:00:eb:1b:ff:33:2a:83:88:47:b3:a9:8e:75:dd:
...
```

### 4.3 CMS (Cryptographic Message Syntax) Structure

```
CMS_ContentInfo
├── contentType: signedData
└── content: SignedData
    ├── version: 1
    ├── digestAlgorithms: [sha256]
    ├── encapContentInfo
    │   ├── eContentType: data
    │   └── eContent: (번들 파일 내용)
    ├── certificates: [개발용 인증서, CA 인증서]
    ├── signerInfos
    │   └── SignerInfo
    │       ├── version: 1
    │       ├── sid: (서명자 식별자)
    │       ├── digestAlgorithm: sha256
    │       ├── signedAttrs: (서명된 속성들)
    │       ├── signatureAlgorithm: sha256WithRSAEncryption
    │       └── signature: (실제 서명 값)
    └── unsignedAttrs: (없음)
```

### 4.4 Signed Attributes
```
SignedAttributes ::= SET OF Attribute
├── contentType: 1.2.840.113549.1.7.1 (data)
├── messageDigest: (SHA256 해시값)
└── signingTime: (서명 생성 시간)
```

## 5. Bundle Assembly

### 5.1 Signature Appending
```c
// bundle.c의 append_signature_to_bundle() 함수
if (!output_stream_write_bytes_all(bundleoutstream, sig, NULL, &ierror)) {
    g_propagate_prefixed_error(
        error, ierror,
        "failed to append signature to bundle: ");
    return FALSE;
}
```

### 5.2 Signature Size Appending
```c
offset = g_seekable_tell(G_SEEKABLE(bundlestream)) - offset;
if (!output_stream_write_uint64_all(bundleoutstream, offset, NULL, &ierror)) {
    g_propagate_prefixed_error(
        error, ierror,
        "failed to append signature size to bundle: ");
    return FALSE;
}
```

### 5.3 Final Bundle Structure
```
bundle.raucb (SquashFS 파일시스템)
├── manifest.raucm                    # 메타데이터
├── nuc-image-qt5-intel-corei7-64.ext4  # 루트 파일시스템
└── [서명 데이터]                     # CMS 서명 + 크기 정보
    ├── CMS_Signature (DER 인코딩)
    └── Signature_Size (8바이트)
```

## 6. Security Features

### 6.1 Cryptographic Security
- **RSA-2048**: 충분한 키 길이로 안전성 보장
- **SHA-256**: 강력한 해시 알고리즘
- **CMS 표준**: 업계 표준 서명 포맷

### 6.2 Key Management
- **개인키 보호**: 빌드 시스템에서만 접근 가능
- **인증서 체인**: CA에서 서명된 신뢰 체인
- **유효기간**: 1년으로 제한된 개발용 인증서

### 6.3 Integrity Verification
- **전체 파일 해시**: 번들 전체에 대한 해시 검증
- **매니페스트 검증**: 메타데이터 무결성 확인
- **서명 검증**: CA 인증서로 서명자 신뢰성 확인

## 7. Bundle Verification Process

### 7.1 Target Verification
```bash
# 1. 번들 압축 해제
rauc extract bundle.raucb /tmp/extract

# 2. 서명 검증
rauc verify --keyring=/etc/rauc/ca.cert.pem bundle.raucb

# 3. 검증 과정
# - CMS 서명 파싱
# - 인증서 체인 검증
# - 해시 값 비교
# - 서명 수학적 검증
```

### 7.2 Verification Steps
1. **번들 압축 해제**: SquashFS 번들을 임시 디렉토리로 추출
2. **서명 데이터 추출**: 번들 끝에서 서명 크기와 서명 데이터 읽기
3. **CMS 파싱**: DER 인코딩된 CMS 구조 파싱
4. **인증서 체인 검증**: CA 인증서로 서명자 신뢰성 확인
5. **해시 검증**: 매니페스트와 이미지 파일의 해시 값 검증
6. **서명 검증**: RSA 서명의 수학적 검증

## 8. Build Process Integration

### 8.1 Yocto Build Integration
```bash
# bundle.bbclass의 do_bundle() 함수
do_bundle() {
    if [ -z "${RAUC_KEY_FILE}" ]; then
        bbfatal "'RAUC_KEY_FILE' not set. Please set to a valid key file location."
    fi

    if [ -z "${RAUC_CERT_FILE}" ]; then
        bbfatal "'RAUC_CERT_FILE' not set. Please set to a valid certificate file location."
    fi

    ${STAGING_BINDIR_NATIVE}/rauc bundle \
        --debug \
        --cert="${RAUC_CERT_FILE}" \
        --key="${RAUC_KEY_FILE}" \
        ${BUNDLE_ARGS} \
        ${BUNDLE_DIR} \
        ${B}/bundle.raucb
}
```

### 8.2 Key Configuration
```bash
# site.conf 설정
RAUC_KEYRING_FILE="${BUILDDIR}/example-ca/ca.cert.pem"
RAUC_KEY_FILE="${BUILDDIR}/example-ca/private/development-1.key.pem"
RAUC_CERT_FILE="${BUILDDIR}/example-ca/development-1.cert.pem"
```

## 9. Bundle Usage

### 9.1 Installation Process
```bash
# 타겟 시스템에서 번들 설치
sudo rauc install /data/nuc-image-qt5-bundle-intel-corei7-64.raucb
```

### 9.2 A/B Boot System
- **슬롯 A**: 현재 활성 루트 파일시스템
- **슬롯 B**: 업데이트 대상 루트 파일시스템
- **자동 롤백**: 부팅 실패 시 이전 슬롯으로 복구

## 10. Summary

RAUC 번들 생성 과정은 다음과 같은 단계로 구성됩니다:

1. **매니페스트 생성**: 메타데이터 및 설정 정보 작성
2. **이미지 파일 복사**: 루트 파일시스템 이미지 포함
3. **해시 계산**: SHA256 체크섬 생성
4. **서명 생성**: 개발용 개인키로 디지털 서명
5. **번들 조립**: 모든 파일을 SquashFS로 압축

이 과정을 통해 **완전한 보안과 무결성을 갖춘 업데이트 패키지**가 생성됩니다. 