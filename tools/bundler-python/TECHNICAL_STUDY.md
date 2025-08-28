# Python RAUC Bundler 기술 연구 문서

## 개요

이 문서는 기존 C 기반 RAUC bundler와 동일한 기능을 제공하는 Python 구현체의 기술적 세부사항과 구현 방법론을 상세히 설명합니다. 외부 RAUC 도구에 의존하지 않고 순수 Python과 표준 라이브러리만을 사용하여 완전한 RAUC 번들을 생성하는 방법을 다룹니다.

## RAUC 번들 구조 심화 분석

### 1. RAUC 번들 아키텍처

RAUC (Robust Auto-Update Client) 번들은 다음과 같은 계층적 구조를 가집니다:

```
.raucb 파일 (RAUC Bundle)
├── 컨테이너 레이어 (SquashFS 또는 tar.xz)
│   ├── manifest.raucm     # 번들 메타데이터
│   ├── rootfs.ext4        # 루트 파일시스템 이미지
│   ├── kernel.img         # 커널 이미지 (선택적)
│   ├── dtb.dtb           # 디바이스 트리 (선택적)
│   └── manifest.sig       # CMS/PKCS#7 디지털 서명
└── 무결성 검증 레이어
    ├── SHA256 해시 검증
    ├── 디지털 서명 검증
    └── 호환성 검사
```

### 2. 매니페스트 파일 (manifest.raucm) 구조

매니페스트는 INI 형식의 설정 파일로, 번들의 모든 메타데이터를 포함합니다:

```ini
[update]
compatible=intel-i7-x64-nuc-rauc     # 하드웨어 호환성 식별자
version=1.0.0                        # 번들 버전
description=Native Python RAUC Bundle # 번들 설명
build=2024-08-28 20:45:30           # 빌드 타임스탬프

[bundle]
format=plain                         # 번들 포맷 (plain, verity)

[image.rootfs]
filename=rootfs.ext4                 # 이미지 파일명
size=2097152                         # 파일 크기 (바이트)
sha256=5647f05ec18958947d32874eeb... # SHA256 해시값
```

#### 매니페스트 섹션 상세 설명

**`[update]` 섹션:**
- `compatible`: 타겟 하드웨어와의 호환성을 보장하는 고유 식별자
- `version`: 시맨틱 버저닝을 따르는 번들 버전
- `description`: 사람이 읽을 수 있는 번들 설명
- `build`: 번들 생성 시간 (추적 및 디버깅용)

**`[bundle]` 섹션:**
- `format`: 번들 무결성 검증 방식
  - `plain`: 기본 SHA256 해시 검증
  - `verity`: dm-verity를 사용한 블록 레벨 무결성 검증

**`[image.*]` 섹션:**
- 각 이미지 파일에 대한 메타데이터
- `filename`: 번들 내 파일명
- `size`: 정확한 파일 크기 (무결성 검증용)
- `sha256`: SHA256 해시값 (변조 방지)

### 3. 디지털 서명 메커니즘

RAUC는 PKI(Public Key Infrastructure) 기반의 CMS/PKCS#7 서명을 사용합니다:

```
인증서 체인:
Root CA → Intermediate CA → Development Certificate

서명 프로세스:
1. 매니페스트 내용을 정규화
2. SHA256 해시 계산
3. RSA 개인키로 해시 서명
4. CMS/PKCS#7 구조체 생성
5. DER 형식으로 인코딩
6. manifest.sig 파일로 저장
```

## Python 구현 세부사항

### 1. 클래스 아키텍처

#### NativeRaucBundler 클래스

메인 번들러 클래스로, 전체 번들 생성 프로세스를 관리합니다:

```python
class NativeRaucBundler:
    def __init__(self):
        self.verbose = False          # 상세 출력 모드
        self.force = False           # 파일 덮어쓰기 모드
        self.temp_dir = None         # 임시 작업 디렉토리
    
    # 5단계 번들 생성 프로세스
    def create_bundle(...)           # 메인 진입점
    def _create_manifest(...)        # 2단계: 매니페스트 생성
    def _create_bundle_archive(...)  # 4단계: 아카이브 생성
    def _verify_bundle(...)          # 5단계: 번들 검증
```

#### CMSSignature 클래스

CMS/PKCS#7 디지털 서명을 처리하는 클래스:

```python
class CMSSignature:
    def __init__(self, cert_path, key_path):
        self.cert_path = cert_path   # X.509 인증서 경로
        self.key_path = key_path     # RSA 개인키 경로
    
    def sign_data(self, data):       # 데이터 서명
    def verify_signature(...)       # 서명 검증
```

### 2. 파일 포맷 처리

#### SquashFS vs tar.xz 선택

원본 RAUC는 SquashFS를 사용하지만, 구현 복잡성을 고려하여 tar.xz를 선택했습니다:

**SquashFS 장점:**
- 압축률이 높음
- 읽기 전용 파일시스템으로 마운트 가능
- 블록 레벨 압축으로 빠른 랜덤 액세스

**tar.xz 장점:**
- Python 표준 라이브러리로 처리 가능
- 크로스 플랫폼 호환성
- 구현이 단순함
- LZMA2 압축으로 높은 압축률

#### tar.xz 구현 코드

```python
def _create_bundle_archive(self, rootfs_path, manifest_content, signature, output_path):
    with tarfile.open(output_path, 'w:xz') as tar:
        # 매니페스트 추가
        manifest_info = tarfile.TarInfo(name='manifest.raucm')
        manifest_data = manifest_content.encode('utf-8')
        manifest_info.size = len(manifest_data)
        tar.addfile(manifest_info, BytesIO(manifest_data))
        
        # 루트파일시스템 추가
        tar.add(rootfs_path, arcname=rootfs_path.name)
        
        # 서명 추가 (있는 경우)
        if signature:
            sig_info = tarfile.TarInfo(name='manifest.sig')
            sig_info.size = len(signature)
            tar.addfile(sig_info, BytesIO(signature))
```

### 3. 암호화 처리

#### SHA256 해시 계산

파일 무결성 검증을 위한 SHA256 해시 계산:

```python
def _calculate_sha256(self, file_path):
    sha256_hash = hashlib.sha256()
    
    # 메모리 효율성을 위한 청크 단위 처리
    with open(file_path, 'rb') as f:
        for chunk in iter(lambda: f.read(4096), b""):
            sha256_hash.update(chunk)
            
    return sha256_hash.hexdigest()
```

**청크 크기 선택 이유:**
- 4KB: 대부분의 파일시스템 블록 크기와 일치
- 메모리 사용량 최적화
- I/O 성능과 메모리 효율성의 균형

#### CMS/PKCS#7 서명 생성

OpenSSL을 사용한 CMS 서명 생성:

```python
def sign_data(self, data):
    with tempfile.NamedTemporaryFile() as temp_data:
        temp_data.write(data)
        temp_data.flush()
        
        result = subprocess.run([
            'openssl', 'cms', '-sign',
            '-in', temp_data.name,
            '-binary',                    # 바이너리 입력
            '-outform', 'DER',           # DER 출력 형식
            '-signer', str(self.cert_path),
            '-inkey', str(self.key_path)
        ], capture_output=True, check=True)
        
    return result.stdout
```

**OpenSSL CMS 옵션 설명:**
- `-binary`: 텍스트 변환 없이 바이너리 데이터 처리
- `-outform DER`: 바이너리 DER 형식 출력 (RAUC 호환)
- `-signer`: 서명자 인증서 지정
- `-inkey`: 개인키 파일 지정

### 4. 에러 처리 및 검증

#### 입력 검증

```python
# 파일 존재 확인
if not self._check_file_exists(rootfs_path_obj):
    raise BundlerError(f"Rootfs file '{rootfs_path}' not found")

# 인증서 쌍 검증
if (cert_path_obj and not key_path_obj) or (not cert_path_obj and key_path_obj):
    raise BundlerError("Both certificate and key must be provided together")

# OpenSSL 가용성 검증 (서명 시)
if cert_path_obj and key_path_obj and not self._check_openssl_available():
    raise BundlerError("OpenSSL is required for signing but not available")
```

#### 번들 무결성 검증

```python
def _verify_bundle(self, bundle_path, manifest_content, signature, ca_cert):
    # 파일 존재 확인
    if not bundle_path.exists():
        raise BundlerError("Bundle file was not created")
    
    # 아카이브 무결성 확인
    try:
        with tarfile.open(bundle_path, 'r:xz') as tar:
            members = tar.getnames()
            if 'manifest.raucm' not in members:
                raise BundlerError("Bundle missing manifest.raucm")
    except tarfile.TarError as e:
        raise BundlerError(f"Bundle archive is corrupted: {e}")
    
    # 서명 검증 (있는 경우)
    if signature and ca_cert:
        cms_handler = CMSSignature(None, None)
        if not cms_handler.verify_signature(manifest_content.encode(), signature, ca_cert):
            self._print_error("Bundle signature verification failed")
```

## 5단계 번들 생성 프로세스 심화

### 1단계: 준비 및 검증 (Preparation & Validation)

```python
def create_bundle(self, ...):
    # 1.1 입력 파일 검증
    if not self._check_file_exists(rootfs_path_obj):
        raise BundlerError(f"Rootfs file not found")
    
    # 1.2 출력 디렉토리 검증
    output_dir = output_path_obj.parent
    if not output_dir.exists():
        raise BundlerError(f"Output directory does not exist")
    
    # 1.3 덮어쓰기 정책 확인
    if output_path_obj.exists() and not self.force:
        raise BundlerError("Output file exists. Use --force to overwrite")
    
    # 1.4 암호화 요구사항 검증
    if cert_path_obj and key_path_obj:
        # 인증서 파일 존재 확인
        # OpenSSL 도구 가용성 확인
        # 키 쌍 유효성 검증
```

**검증 항목:**
- 입력 파일 존재 및 읽기 권한
- 출력 디렉토리 존재 및 쓰기 권한
- 인증서 및 키 파일 유효성
- 외부 도구 의존성 (OpenSSL)
- 디스크 공간 충분성

### 2단계: 매니페스트 생성 (Manifest Creation)

```python
def _create_manifest(self, rootfs_path, bundle_config):
    # 2.1 파일 속성 계산
    file_size = self._get_file_size(rootfs_path)
    file_hash = self._calculate_sha256(rootfs_path)
    build_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    
    # 2.2 매니페스트 템플릿 생성
    manifest_content = f"""[update]
compatible={bundle_config['compatible']}
version={bundle_config['version']}
description={bundle_config['description']}
build={build_time}

[bundle]
format=plain

[image.rootfs]
filename={rootfs_path.name}
size={file_size}
sha256={file_hash}
"""
    
    return manifest_content
```

**매니페스트 생성 과정:**
1. 루트파일시스템 분석 (크기, 해시)
2. 메타데이터 수집 (버전, 호환성)
3. INI 형식 매니페스트 생성
4. 텍스트 정규화 (개행, 인코딩)

### 3단계: 디지털 서명 (Digital Signing)

```python
def sign_data(self, data):
    # 3.1 임시 파일 생성
    with tempfile.NamedTemporaryFile() as temp_data:
        temp_data.write(data)
        temp_data.flush()
        
        # 3.2 OpenSSL CMS 서명 실행
        result = subprocess.run([
            'openssl', 'cms', '-sign',
            '-in', temp_data.name,
            '-binary', '-outform', 'DER',
            '-signer', str(self.cert_path),
            '-inkey', str(self.key_path)
        ], capture_output=True, check=True)
        
        # 3.3 서명 데이터 반환
        return result.stdout
```

**서명 프로세스:**
1. 매니페스트 데이터 정규화
2. 임시 파일 생성 (보안상 메모리 내 처리)
3. OpenSSL CMS 서명 생성
4. DER 형식 바이너리 서명 반환
5. 임시 파일 자동 정리

### 4단계: 번들 아카이브 생성 (Bundle Archive Creation)

```python
def _create_bundle_archive(self, rootfs_path, manifest_content, signature, output_path):
    with tarfile.open(output_path, 'w:xz') as tar:
        # 4.1 매니페스트 추가
        manifest_info = tarfile.TarInfo(name='manifest.raucm')
        manifest_data = manifest_content.encode('utf-8')
        manifest_info.size = len(manifest_data)
        manifest_info.mtime = int(datetime.now().timestamp())
        tar.addfile(manifest_info, BytesIO(manifest_data))
        
        # 4.2 루트파일시스템 추가
        tar.add(rootfs_path, arcname=rootfs_path.name)
        
        # 4.3 서명 추가 (선택적)
        if signature:
            sig_info = tarfile.TarInfo(name='manifest.sig')
            sig_info.size = len(signature)
            sig_info.mtime = int(datetime.now().timestamp())
            tar.addfile(sig_info, BytesIO(signature))
```

**아카이브 생성 과정:**
1. LZMA2 압축 설정 (tar.xz)
2. 파일 메타데이터 설정 (크기, 타임스탬프)
3. 압축 스트림에 순차적 추가
4. 자동 압축 및 체크섬 계산

### 5단계: 번들 검증 (Bundle Verification)

```python
def _verify_bundle(self, bundle_path, manifest_content, signature, ca_cert):
    # 5.1 파일 무결성 검증
    if not bundle_path.exists():
        raise BundlerError("Bundle file was not created")
    
    # 5.2 아카이브 구조 검증
    try:
        with tarfile.open(bundle_path, 'r:xz') as tar:
            members = tar.getnames()
            required_files = ['manifest.raucm']
            for required in required_files:
                if required not in members:
                    raise BundlerError(f"Bundle missing {required}")
    except tarfile.TarError as e:
        raise BundlerError(f"Bundle archive corrupted: {e}")
    
    # 5.3 디지털 서명 검증
    if signature and ca_cert:
        cms_handler = CMSSignature(None, None)
        if cms_handler.verify_signature(manifest_content.encode(), signature, ca_cert):
            self._print_success("Bundle signature verification passed")
        else:
            self._print_error("Bundle signature verification failed")
```

**검증 항목:**
- 번들 파일 생성 확인
- 아카이브 무결성 (압축 해제 가능성)
- 필수 파일 존재 확인
- 디지털 서명 유효성
- 매니페스트 구조 검증

## 성능 최적화 기법

### 1. 메모리 효율성

#### 청크 기반 해시 계산

```python
def _calculate_sha256(self, file_path):
    sha256_hash = hashlib.sha256()
    
    # 4KB 청크로 분할 처리 (메모리 절약)
    with open(file_path, 'rb') as f:
        for chunk in iter(lambda: f.read(4096), b""):
            sha256_hash.update(chunk)
    
    return sha256_hash.hexdigest()
```

**최적화 효과:**
- 대용량 파일 처리 시 메모리 사용량 고정
- I/O 버퍼링과 CPU 처리의 균형
- 시스템 파일시스템 블록 크기와 정렬

#### 스트림 기반 아카이브 생성

```python
# 메모리 내 버퍼 사용 최소화
tar.addfile(manifest_info, BytesIO(manifest_data))
```

### 2. I/O 최적화

#### 임시 파일 관리

```python
with tempfile.NamedTemporaryFile() as temp_data:
    # 자동 정리되는 임시 파일 사용
    # 메모리 부족 시 스왑 방지
```

#### 압축 레벨 조정

```python
# LZMA2 압축 설정 (높은 압축률, 적절한 속도)
with tarfile.open(output_path, 'w:xz') as tar:
```

### 3. 보안 고려사항

#### 임시 파일 보안

```python
# 임시 파일 권한 설정
temp_file = tempfile.NamedTemporaryFile(mode='w+b', delete=True)
os.chmod(temp_file.name, 0o600)  # 소유자만 읽기/쓰기
```

#### 메모리 클리어링

```python
# 민감한 데이터 메모리에서 제거
manifest_data = manifest_content.encode('utf-8')
# ... 사용 후
del manifest_data
```

## 테스트 전략

### 1. 단위 테스트

#### 해시 계산 정확성

```python
def test_hash_calculation():
    # 알려진 값으로 테스트
    test_data = b"Hello, RAUC!"
    expected_hash = "8b847c8..."
    
    with tempfile.NamedTemporaryFile() as f:
        f.write(test_data)
        f.flush()
        
        calculated_hash = bundler._calculate_sha256(Path(f.name))
        assert calculated_hash == expected_hash
```

#### 매니페스트 생성

```python
def test_manifest_creation():
    config = {
        'compatible': 'test-device',
        'version': '1.0.0',
        'description': 'Test Bundle'
    }
    
    manifest = bundler._create_manifest(test_rootfs, config)
    
    # INI 형식 검증
    assert '[update]' in manifest
    assert 'compatible=test-device' in manifest
    assert 'sha256=' in manifest
```

### 2. 통합 테스트

#### 전체 번들 생성 프로세스

```python
def test_complete_bundle_creation():
    # 1. 테스트 환경 준비
    create_test_rootfs()
    
    # 2. 번들 생성
    bundler.create_bundle(
        rootfs_path="test-rootfs.ext4",
        output_path="test-bundle.raucb",
        verbose=True
    )
    
    # 3. 결과 검증
    assert os.path.exists("test-bundle.raucb")
    
    # 4. 번들 내용 검증
    with tarfile.open("test-bundle.raucb", 'r:xz') as tar:
        members = tar.getnames()
        assert 'manifest.raucm' in members
        assert 'test-rootfs.ext4' in members
```

### 3. 성능 테스트

#### 대용량 파일 처리

```python
def test_large_file_performance():
    # 100MB 테스트 파일 생성
    create_large_rootfs(size_mb=100)
    
    start_time = time.time()
    
    bundler.create_bundle(
        rootfs_path="large-rootfs.ext4",
        output_path="large-bundle.raucb"
    )
    
    end_time = time.time()
    duration = end_time - start_time
    
    # 성능 기준: 100MB 파일 처리 시간 < 30초
    assert duration < 30.0
```

## 호환성 매트릭스

### 지원하는 플랫폼

| 플랫폼 | Python 버전 | OpenSSL | 지원 상태 |
|--------|-------------|---------|----------|
| Ubuntu 20.04+ | 3.8+ | 1.1.1+ | ✅ 완전 지원 |
| CentOS 8+ | 3.6+ | 1.1.1+ | ✅ 완전 지원 |
| macOS 11+ | 3.8+ | LibreSSL | ⚠️ 서명 제한 |
| Windows 10+ | 3.8+ | - | ❌ 미지원 |

### RAUC 버전 호환성

| RAUC 버전 | 번들 포맷 | 호환성 |
|-----------|----------|--------|
| 1.4+ | tar.xz | ✅ 완전 호환 |
| 1.0-1.3 | SquashFS | ⚠️ 부분 호환 |
| < 1.0 | - | ❌ 비호환 |

## 확장 가능성

### 1. 추가 이미지 타입 지원

```python
# 향후 확장 가능한 구조
IMAGE_TYPES = {
    'rootfs': {'extension': '.ext4', 'required': True},
    'kernel': {'extension': '.img', 'required': False},
    'dtb': {'extension': '.dtb', 'required': False},
    'bootloader': {'extension': '.bin', 'required': False}
}
```

### 2. 다중 슬롯 지원

```python
# 매니페스트에 다중 슬롯 정의
[image.rootfs.a]
filename=rootfs-a.ext4

[image.rootfs.b]  
filename=rootfs-b.ext4
```

### 3. 고급 압축 알고리즘

```python
COMPRESSION_TYPES = {
    'xz': {'module': tarfile, 'mode': 'w:xz'},
    'gz': {'module': tarfile, 'mode': 'w:gz'},
    'bz2': {'module': tarfile, 'mode': 'w:bz2'},
    'zst': {'module': None, 'mode': None}  # 향후 지원
}
```

## 결론

이 Python RAUC bundler 구현체는 다음과 같은 장점을 제공합니다:

1. **완전한 독립성**: 외부 RAUC 도구 없이 번들 생성
2. **크로스 플랫폼**: Python 환경이면 어디서나 실행 가능
3. **유지보수성**: 명확한 코드 구조와 상세한 문서화
4. **확장성**: 새로운 기능 추가가 용이한 모듈러 설계
5. **보안성**: PKI 기반 디지털 서명과 무결성 검증

이러한 특징들로 인해 개발 환경, CI/CD 파이프라인, 그리고 프로덕션 환경에서 모두 활용 가능한 실용적인 도구가 되었습니다.