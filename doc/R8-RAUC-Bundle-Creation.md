# RAUC 번들 생성 완전 가이드

## 1. 개요

RAUC (Robust Auto-Update Client) 번들은 안전한 OTA (Over-The-Air) 업데이트를 위한 패키지 포맷입니다. 이 문서는 Yocto 환경에서 RAUC 번들이 어떻게 생성되는지, 서명과 암호화가 어떻게 적용되는지, 그리고 번들의 내부 구조와 포맷에 대해 상세히 설명합니다.

## 2. RAUC 번들 아키텍처

### 2.1 번들 구조 개요

```
RAUC Bundle (.raucb)
├── 매니페스트 (manifest.raucm)           # 번들 메타데이터 및 설정
├── 루트파일시스템 이미지 (rootfs.ext4)    # 실제 업데이트할 이미지
├── 커널 이미지 (optional)               # 커널 업데이트 (선택사항)
├── 훅 스크립트 (optional)               # 설치 전후 실행 스크립트
└── 디지털 서명                         # CMS/PKCS#7 형태의 서명 데이터
```

### 2.2 번들 포맷

RAUC 번들은 기본적으로 **SquashFS** 파일시스템을 사용하여 압축된 컨테이너입니다:

- **압축**: gzip, lzo, xz, zstd 등 다양한 압축 알고리즘 지원
- **무결성**: SquashFS의 내장 체크섬으로 데이터 무결성 보장
- **효율성**: 읽기 전용 파일시스템으로 빠른 액세스
- **서명**: CMS (Cryptographic Message Syntax) 표준 사용

### 2.3 지원되는 번들 포맷

#### Plain 포맷 (기본)
```
[bundle]
format=plain
```

#### Verity 포맷 (dm-verity 사용)
```
[bundle]
format=verity
```

#### CASYnc 포맷 (증분 업데이트)
```
RAUC_CASYNC_BUNDLE = "1"
```

## 3. 번들 생성 과정

### 3.1 Yocto 빌드 시스템에서의 번들 생성

#### 3.1.1 번들 레시피 정의
```bitbake
# nuc-image-qt5-bundle.bb 예시
inherit bundle

LICENSE = "MIT"

RAUC_BUNDLE_COMPATIBLE ?= "intel-i7-x64-nuc-rauc"
RAUC_BUNDLE_VERSION ?= "1.0"
RAUC_BUNDLE_DESCRIPTION ?= "Intel NUC Qt5 System Update"

RAUC_BUNDLE_SLOTS = "rootfs"
RAUC_SLOT_rootfs[fstype] = "ext4"
RAUC_SLOT_rootfs = "nuc-image-qt5"
```

#### 3.1.2 번들 클래스 (bundle.bbclass) 동작 과정

**단계 1: 의존성 확인**
```python
DEPENDS = "squashfs-tools-native"
DEPENDS += "${@bb.utils.contains('DISTRO_FEATURES', 'rauc', 'rauc-native', '', d)}"
```

**단계 2: 번들 디렉토리 준비**
```python
do_configure[cleandirs] = "${BUNDLE_DIR}"
BUNDLE_DIR = "${S}/bundle"
```

**단계 3: 매니페스트 생성 (do_configure)**
```python
def write_manifest(d):
    manifest = open('%s/manifest.raucm' % bundle_path, 'w')
    
    # 업데이트 정보
    manifest.write('[update]\n')
    manifest.write(d.expand('compatible=${RAUC_BUNDLE_COMPATIBLE}\n'))
    manifest.write(d.expand('version=${RAUC_BUNDLE_VERSION}\n'))
    manifest.write(d.expand('description=${RAUC_BUNDLE_DESCRIPTION}\n'))
    
    # 번들 포맷
    if bundle_format and bundle_format != "plain":
        manifest.write('[bundle]\n')
        manifest.write(d.expand('format=${RAUC_BUNDLE_FORMAT}\n'))
    
    # 슬롯 정보
    for slot in slots:
        manifest.write('[image.%s]\n' % slotname)
        manifest.write("filename=%s\n" % imgname)
```

**단계 4: 이미지 파일 복사**
```python
# DEPLOY_DIR_IMAGE에서 빌드된 이미지를 번들 디렉토리로 복사
searchpath = d.expand("${DEPLOY_DIR_IMAGE}/%s") % imgsource
if os.path.isfile(searchpath):
    shutil.copy(searchpath, bundle_imgpath)
```

**단계 5: 번들 생성 및 서명 (do_bundle)**
```bash
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

### 3.2 매니페스트 파일 상세 구조

#### 3.2.1 기본 매니페스트 구조
```ini
[update]
compatible=intel-i7-x64-nuc-rauc           # 시스템 호환성 식별자
version=2024.08.13-1830                    # 번들 버전
description=Intel NUC Qt5 System Update    # 번들 설명
build.date=2024-08-13 18:30:00             # 빌드 타임스탬프

[bundle]
format=verity                               # 번들 포맷 (plain/verity)

[image.rootfs]
filename=nuc-image-qt5-intel-corei7-64.ext4
size=5368709120                            # 파일 크기 (바이트)
sha256=a1b2c3d4...                         # SHA256 해시
```

#### 3.2.2 고급 매니페스트 옵션
```ini
[hooks]
filename=update-hook.sh                     # 훅 스크립트
hooks=install-check                        # 실행할 훅 유형

[image.rootfs]
filename=rootfs.ext4
hooks=pre-install;post-install             # 슬롯별 훅
adaptive=block-hash-index                  # 적응형 업데이트

[image.kernel]
filename=bzImage-intel-corei7-64.bin
type=kernel

[meta.release]
release-type=stable                        # 메타 섹션
release-notes=Security and feature updates
build-id=20240813-1830
```

### 3.3 서명 시스템

#### 3.3.1 PKI (Public Key Infrastructure) 구조

```
Root CA (ca.cert.pem)
└── Intermediate CA (optional)
    └── Signing Certificate (development-1.cert.pem)
        └── Private Key (development-1.key.pem)
```

#### 3.3.2 인증서 생성 과정

**1단계: CA 루트 인증서 생성**
```bash
# create-example-keys.sh에서 실행
openssl req -newkey rsa -keyout private/ca.key.pem -out ca.csr.pem \
    -subj "/O=Test Org/CN=Test Org rauc CA Development"

openssl ca -batch -selfsign -extensions v3_ca \
    -in ca.csr.pem -out ca.cert.pem -keyfile private/ca.key.pem
```

**2단계: 서명용 인증서 생성**
```bash
openssl req -newkey rsa -keyout private/development-1.key.pem \
    -out development-1.csr.pem -subj "/O=Test Org/CN=Test Org Development-1"

openssl ca -batch -extensions v3_leaf \
    -in development-1.csr.pem -out development-1.cert.pem
```

**3단계: site.conf 설정**
```bash
RAUC_KEYRING_FILE="${TOPDIR}/example-ca/ca.cert.pem"
RAUC_KEY_FILE="${TOPDIR}/example-ca/private/development-1.key.pem"
RAUC_CERT_FILE="${TOPDIR}/example-ca/development-1.cert.pem"
```

#### 3.3.3 서명 과정 상세

**CMS (Cryptographic Message Syntax) 서명**
```bash
# rauc bundle 내부에서 실행되는 서명 과정
openssl cms -sign -in manifest.raucm \
    -signer ${CERT_FILE} -inkey ${KEY_FILE} \
    -outform DER -nosmimecap -binary
```

**서명 검증 과정**
1. 번들을 언마운트하여 매니페스트 추출
2. CMS 서명 데이터에서 인증서 체인 확인
3. 루트 CA와 대조하여 신뢰성 검증
4. 매니페스트 해시와 서명된 해시 비교
5. 각 이미지 파일의 SHA256 해시 검증

## 4. 번들 생성 실습

### 4.1 환경 설정

#### 4.1.1 필수 레이어 설정
```bash
# bblayers.conf
BBLAYERS += " \
  ${TOPDIR}/../meta-rauc \
  ${TOPDIR}/../meta-nuc \
"

# local.conf
DISTRO_FEATURES += " rauc"
```

#### 4.1.2 RAUC 키 생성
```bash
# Docker 컨테이너 내부에서
cd meta-nuc
./create-example-keys.sh
```

### 4.2 번들 레시피 생성

#### 4.2.1 기본 번들 레시피
```bitbake
# recipes-core/bundles/my-bundle.bb
inherit bundle

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

RAUC_BUNDLE_COMPATIBLE = "my-device-rauc"
RAUC_BUNDLE_VERSION = "1.0.0"
RAUC_BUNDLE_DESCRIPTION = "My Device Update Bundle"

RAUC_BUNDLE_SLOTS = "rootfs"
RAUC_SLOT_rootfs = "core-image-minimal"
RAUC_SLOT_rootfs[fstype] = "ext4"
```

#### 4.2.2 다중 슬롯 번들 레시피
```bitbake
RAUC_BUNDLE_SLOTS = "rootfs kernel bootloader"

RAUC_SLOT_rootfs = "core-image-minimal"
RAUC_SLOT_rootfs[fstype] = "ext4"
RAUC_SLOT_rootfs[hooks] = "pre-install;post-install"

RAUC_SLOT_kernel = "linux-yocto"
RAUC_SLOT_kernel[type] = "kernel"
RAUC_SLOT_kernel[file] = "bzImage-${MACHINE}.bin"

RAUC_SLOT_bootloader = "u-boot"
RAUC_SLOT_bootloader[type] = "boot"
RAUC_SLOT_bootloader[file] = "u-boot.imx"
```

### 4.3 빌드 실행

#### 4.3.1 자동 빌드
```bash
./build.sh bundle
```

#### 4.3.2 수동 빌드
```bash
# Docker 컨테이너 진입
./build.sh manual

# 컨테이너 내부에서
bitbake my-bundle
```

### 4.4 번들 검증

#### 4.4.1 번들 정보 확인
```bash
rauc info bundle.raucb
```

출력 예시:
```
Bundle info:
  Compatible:  my-device-rauc
  Version:     1.0.0
  Description: My Device Update Bundle
  Build:       2024-08-13 18:30:00
  Format:      plain
  Signature:   valid

Image info:
  Slot 'rootfs':
    Filename: core-image-minimal-my-device.ext4
    Size:     524288000 bytes
    Checksum: sha256:a1b2c3d4e5f6...
```

#### 4.4.2 번들 내용 확인
```bash
# 번들을 임시 디렉토리에 언마운트
mkdir /tmp/bundle
rauc extract bundle.raucb /tmp/bundle

# 내용 확인
ls -la /tmp/bundle/
cat /tmp/bundle/manifest.raucm
```

## 5. 고급 기능

### 5.1 Verity 포맷 번들

#### 5.1.1 설정
```bitbake
RAUC_BUNDLE_FORMAT = "verity"
```

#### 5.1.2 특징
- **dm-verity 지원**: 커널 레벨에서 무결성 검증
- **해시 트리**: 블록 단위 무결성 확인
- **변조 방지**: 런타임에서 파일 변조 탐지

### 5.2 CASYnc 번들 (증분 업데이트)

#### 5.2.1 설정
```bitbake
RAUC_CASYNC_BUNDLE = "1"
DEPENDS += "casync-native"
```

#### 5.2.2 특징
- **증분 업데이트**: 변경된 블록만 전송
- **대역폭 절약**: 네트워크 사용량 최소화
- **청크 저장소**: 재사용 가능한 데이터 블록 관리

### 5.3 암호화 번들

#### 5.3.1 설정
```bitbake
RAUC_BUNDLE_ENCRYPT = "1"
RAUC_ENCRYPT_KEY = "path/to/encryption.key"
```

#### 5.3.2 암호화 과정
1. AES-256-CBC로 번들 내용 암호화
2. RSA 공개키로 대칭키 암호화
3. 암호화된 헤더를 번들에 첨부

## 6. 번들 최적화

### 6.1 압축 최적화

#### 6.1.1 압축 알고리즘 선택
```bitbake
BUNDLE_ARGS += ' --mksquashfs-args="-comp zstd -Xcompression-level 22" '
```

#### 6.1.2 압축률 비교
| 알고리즘 | 압축률 | 속도 | CPU 사용량 |
|----------|--------|------|------------|
| gzip     | 보통   | 빠름 | 낮음       |
| lzo      | 낮음   | 매우빠름 | 매우낮음 |
| xz       | 높음   | 느림 | 높음       |
| zstd     | 높음   | 빠름 | 보통       |

### 6.2 크기 최적화

#### 6.2.1 불필요한 파일 제거
```bitbake
IMAGE_INSTALL_remove = "debug-tools development-packages"
```

#### 6.2.2 스트립 옵션
```bitbake
INHIBIT_PACKAGE_STRIP = "0"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
```

## 7. 트러블슈팅

### 7.1 일반적인 오류

#### 7.1.1 서명 관련 오류
```
ERROR: 'RAUC_KEY_FILE' not set
```
**해결방법**: site.conf에서 RAUC 키 파일 경로 확인

#### 7.1.2 이미지 누락 오류
```
ERROR: No image set for slot 'rootfs'
```
**해결방법**: RAUC_SLOT_rootfs 변수 설정 확인

#### 7.1.3 호환성 오류
```
ERROR: Bundle is incompatible
```
**해결방법**: 타겟 시스템의 system.conf에서 compatible 설정 확인

### 7.2 디버깅 방법

#### 7.2.1 상세 로그 활성화
```bash
bitbake -v my-bundle
```

#### 7.2.2 번들 검증
```bash
rauc info --detailed bundle.raucb
rauc verify bundle.raucb
```

#### 7.2.3 수동 번들 생성
```bash
# 번들 디렉토리 구성
mkdir /tmp/manual-bundle
cp rootfs.ext4 /tmp/manual-bundle/
echo "[update]..." > /tmp/manual-bundle/manifest.raucm

# 수동 번들 생성
rauc bundle --cert=cert.pem --key=key.pem /tmp/manual-bundle bundle.raucb
```

## 8. 보안 고려사항

### 8.1 키 관리

#### 8.1.1 프로덕션 키
- **개발키와 분리**: 프로덕션에서는 별도 키 사용
- **하드웨어 보안 모듈**: HSM 사용 고려
- **키 순환**: 정기적인 키 갱신

#### 8.1.2 키 저장
```bash
# 키 파일 권한 설정
chmod 600 private/development-1.key.pem
chown root:root private/development-1.key.pem
```

### 8.2 번들 무결성

#### 8.2.1 체크섬 검증
- **SHA256 해시**: 각 이미지 파일의 무결성 확인
- **CRC32**: 전송 오류 탐지

#### 8.2.2 롤백 방지
```ini
# manifest.raucm
[update]
...
rollback-threshold=3    # 3번 부팅 실패 시 롤백
```

## 9. 성능 최적화

### 9.1 빌드 성능

#### 9.1.1 병렬 처리
```bash
# local.conf
BB_NUMBER_THREADS = "8"
PARALLEL_MAKE = "-j 8"
```

#### 9.1.2 sstate 캐시
```bash
SSTATE_DIR = "${TOPDIR}/../sstate-cache"
```

### 9.2 업데이트 성능

#### 9.2.1 증분 업데이트
- CASYnc 사용으로 대역폭 절약
- 델타 압축으로 크기 최소화

#### 9.2.2 백그라운드 다운로드
- 시스템 운영 중 백그라운드에서 다운로드
- 재부팅 시에만 빠른 설치

## 10. 결론

RAUC 번들 시스템은 안전하고 신뢰성 있는 OTA 업데이트를 위한 강력한 솔루션을 제공합니다. 적절한 PKI 설정, 매니페스트 구성, 그리고 빌드 시스템 통합을 통해 프로덕션 환경에서 안정적인 업데이트 시스템을 구축할 수 있습니다.

핵심 포인트:
- **보안**: PKI 기반 서명과 검증
- **신뢰성**: A/B 파티션과 자동 롤백
- **효율성**: 압축과 증분 업데이트
- **유연성**: 다양한 포맷과 설정 옵션

이 가이드를 통해 RAUC 번들 생성의 모든 측면을 이해하고, 프로젝트 요구사항에 맞는 최적의 업데이트 시스템을 구축할 수 있습니다.