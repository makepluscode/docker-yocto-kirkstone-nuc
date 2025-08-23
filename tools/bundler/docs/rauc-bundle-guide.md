# RAUC Bundle Creation Files

이 디렉토리는 Yocto 빌드 시스템에서 추출한 RAUC 번들 생성 컴포넌트들을 포함합니다. 이 파일들을 사용하여 Yocto 빌드 시스템 없이도 RAUC 번들을 생성할 수 있습니다.

## 파일 구조

```
files/
├── bundle.bbclass              # RAUC 번들 생성 클래스 (Yocto에서 추출)
├── rauc-bundle-recipe.bb       # 샘플 RAUC 번들 레시피
├── create-example-keys.sh      # RAUC 인증서/키 생성 스크립트
├── rauc-bundle-manual.sh       # 수동 RAUC 번들 생성 스크립트
└── README.md                   # 이 파일
```

## 1. bundle.bbclass

Yocto의 `meta-rauc/classes/bundle.bbclass`에서 추출한 단순화된 버전입니다.

### 주요 기능:
- **매니페스트 생성**: RAUC 번들 매니페스트 파일 자동 생성
- **이미지 파일 처리**: 루트파일시스템, 커널, DTB 등 이미지 파일 처리
- **해시 계산**: SHA256 해시 자동 계산
- **번들 생성**: RAUC 번들 (.raucb) 파일 생성
- **서명**: 디지털 서명 적용

### 사용법:
```bitbake
# 레시피에서 bundle 클래스 상속
inherit bundle

# 번들 메타데이터 설정
RAUC_BUNDLE_COMPATIBLE = "my-device-rauc"
RAUC_BUNDLE_VERSION = "1.0.0"
RAUC_BUNDLE_DESCRIPTION = "My Device Update Bundle"

# 슬롯 정의
RAUC_BUNDLE_SLOTS = "rootfs"
RAUC_SLOT_rootfs = "core-image-minimal"
RAUC_SLOT_rootfs[fstype] = "ext4"
```

## 2. rauc-bundle-recipe.bb

RAUC 번들 레시피의 샘플입니다. 실제 프로젝트에서 이 파일을 기반으로 번들 레시피를 작성할 수 있습니다.

### 주요 설정:
- **호환성**: `RAUC_BUNDLE_COMPATIBLE`로 디바이스 호환성 정의
- **버전**: `RAUC_BUNDLE_VERSION`으로 번들 버전 관리
- **슬롯**: `RAUC_BUNDLE_SLOTS`로 업데이트할 컴포넌트 정의
- **포맷**: `RAUC_BUNDLE_FORMAT`으로 번들 포맷 선택 (plain, verity 등)

## 3. create-example-keys.sh

RAUC 번들 서명을 위한 예제 인증서와 키를 생성하는 스크립트입니다.

### 생성되는 파일들:
- **CA 인증서**: `example-ca/ca.cert.pem`
- **CA 개인키**: `example-ca/private/ca.key.pem`
- **개발 인증서**: `example-ca/development-1.cert.pem`
- **개발 개인키**: `example-ca/private/development-1.key.pem`
- **설정 템플릿**: `example-ca/site.conf.template`

### 사용법:
```bash
# 스크립트 실행
./create-example-keys.sh

# 생성된 키 확인
ls -la example-ca/
```

### 주의사항:
⚠️ **이 키들은 개발용 예제입니다. 프로덕션에서는 사용하지 마세요!**

## 4. rauc-bundle-manual.sh

Yocto 빌드 시스템 없이 수동으로 RAUC 번들을 생성하는 스크립트입니다.

### 기능:
- **매니페스트 자동 생성**: 루트파일시스템 파일에서 매니페스트 자동 생성
- **해시 계산**: SHA256 해시 자동 계산
- **번들 생성**: RAUC 번들 파일 생성
- **번들 검증**: 생성된 번들 검증
- **번들 추출**: 번들 내용 확인 (선택사항)

### 사용법:
```bash
# 기본 사용법
./rauc-bundle-manual.sh /path/to/rootfs.ext4

# 번들 추출과 함께 생성
./rauc-bundle-manual.sh /path/to/rootfs.ext4 --extract
```

### 출력:
- **번들 파일**: `output/manual-bundle-1.0.0.raucb`
- **추출 디렉토리**: `output/extracted-manual-bundle/` (--extract 옵션 사용 시)

## Yocto 빌드 시스템과의 통합

### 1. 레이어 설정

`bblayers.conf`에 RAUC 레이어 추가:
```bash
BBLAYERS += " \
  ${TOPDIR}/../meta-rauc \
"
```

### 2. 기능 활성화

`local.conf` 또는 `site.conf`에 RAUC 기능 활성화:
```bash
DISTRO_FEATURES += "rauc"
```

### 3. 키 설정

`site.conf`에 RAUC 키 설정:
```bash
# RAUC 키링 파일 (CA 인증서)
RAUC_KEYRING_FILE = "${TOPDIR}/example-ca/ca.cert.pem"

# RAUC 서명 키와 인증서
RAUC_KEY_FILE = "${TOPDIR}/example-ca/private/development-1.key.pem"
RAUC_CERT_FILE = "${TOPDIR}/example-ca/development-1.cert.pem"
```

### 4. 번들 빌드

```bash
# 번들 레시피 빌드
bitbake my-bundle

# 또는 자동 빌드 스크립트 사용
./build.sh bundle
```

## RAUC 번들 구조

### 매니페스트 파일 (manifest.raucm)
```ini
[update]
compatible=my-device-rauc
version=1.0.0
description=My Device Update Bundle
build.date=2024-08-19 18:30:00

[bundle]
format=plain

[image.rootfs]
filename=rootfs.ext4
size=5368709120
sha256=a1b2c3d4e5f6...
```

### 번들 파일 (.raucb)
RAUC 번들은 SquashFS 파일시스템을 사용하는 압축된 컨테이너입니다:
- **매니페스트**: 번들 메타데이터 및 설정
- **이미지 파일**: 루트파일시스템, 커널, DTB 등
- **서명**: CMS/PKCS#7 형태의 디지털 서명

## 보안 고려사항

### 1. 키 관리
- **개발키와 프로덕션키 분리**: 프로덕션에서는 별도 키 사용
- **하드웨어 보안 모듈**: HSM 사용 고려
- **키 순환**: 정기적인 키 갱신

### 2. 번들 무결성
- **체크섬 검증**: SHA256 해시로 무결성 확인
- **서명 검증**: 디지털 서명으로 신뢰성 확인
- **롤백 방지**: 부팅 실패 시 자동 롤백

## 트러블슈팅

### 일반적인 오류

#### 1. 서명 관련 오류
```
ERROR: 'RAUC_KEY_FILE' not set
```
**해결방법**: site.conf에서 RAUC 키 파일 경로 확인

#### 2. 이미지 누락 오류
```
ERROR: No image set for slot 'rootfs'
```
**해결방법**: RAUC_SLOT_rootfs 변수 설정 확인

#### 3. 호환성 오류
```
ERROR: Bundle is incompatible
```
**해결방법**: 타겟 시스템의 system.conf에서 compatible 설정 확인

### 디버깅 방법

#### 1. 상세 로그 활성화
```bash
bitbake -v my-bundle
```

#### 2. 번들 검증
```bash
rauc info --detailed bundle.raucb
rauc verify bundle.raucb
```

#### 3. 수동 번들 생성
```bash
# 번들 디렉토리 구성
mkdir /tmp/manual-bundle
cp rootfs.ext4 /tmp/manual-bundle/
echo "[update]..." > /tmp/manual-bundle/manifest.raucm

# 수동 번들 생성
rauc bundle --cert=cert.pem --key=key.pem /tmp/manual-bundle bundle.raucb
```

## 결론

이 파일들을 사용하면 Yocto 빌드 시스템의 복잡성 없이도 RAUC 번들을 생성할 수 있습니다. 개발 단계에서는 수동 스크립트를 사용하고, 프로덕션에서는 Yocto 빌드 시스템과 통합하여 사용하는 것을 권장합니다.

핵심 포인트:
- **보안**: PKI 기반 서명과 검증
- **신뢰성**: A/B 파티션과 자동 롤백
- **효율성**: 압축과 증분 업데이트
- **유연성**: 다양한 포맷과 설정 옵션 