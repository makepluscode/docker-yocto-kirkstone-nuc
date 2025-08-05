# RAUC Bundle 암호화 및 서명 가이드

## 1. 개요

RAUC (Robust Auto-Update Client) 번들의 암호화 및 서명 시스템은 안전한 OTA 업데이트를 위한 핵심 보안 메커니즘입니다. 이 문서는 Yocto 빌드 과정에서 RAUC 번들이 어떻게 생성, 서명, 암호화되는지를 상세히 설명합니다.

## 2. PKI (Public Key Infrastructure) 구조

### 2.1 인증서 체인 구조

```
Root CA (ca.cert.pem)
└── Development Certificate (development-1.cert.pem)  
    └── Private Key (development-1.key.pem)
```

### 2.2 키 생성 과정

#### 키 생성 스크립트 실행
`kirkstone/meta-nuc/create-example-keys.sh` 스크립트가 자동으로 PKI 인증서를 생성합니다:

```bash
#!/bin/bash
set -e

if [ -z $BBPATH ]; then
  printf "Please call from within a set-up bitbake environment!\nRun 'source oe-init-build-env <builddir>' first\n"
  exit 1
fi

ORG="Test Org"
CA="rauc CA"
CRL="-crldays 5000"
BASE="$BBPATH/example-ca"
```

#### OpenSSL 설정 파일 생성
스크립트가 자동으로 `openssl.cnf`를 생성합니다:

```bash
cat > $BASE/openssl.cnf <<EOF
[ ca ]
default_ca      = CA_default

[ CA_default ]
dir            = .
database       = \$dir/index.txt
new_certs_dir  = \$dir/certs
certificate    = \$dir/ca.cert.pem
serial         = \$dir/serial
private_key    = \$dir/private/ca.key.pem
RANDFILE       = \$dir/private/.rand

default_startdate = 19700101000000Z
default_enddate = 99991231235959Z
default_crl_days= 30
default_md     = sha256

policy         = policy_any
email_in_dn    = no
name_opt       = ca_default
cert_opt       = ca_default
copy_extensions = none

[ policy_any ]
organizationName       = match
commonName             = supplied

[ req ]
default_bits           = 2048
distinguished_name     = req_distinguished_name
x509_extensions        = v3_leaf
encrypt_key = no
default_md = sha256

[ v3_ca ]
subjectKeyIdentifier=hash
authorityKeyIdentifier=keyid:always,issuer:always
basicConstraints = CA:TRUE

[ v3_leaf ]
subjectKeyIdentifier=hash
authorityKeyIdentifier=keyid:always,issuer:always
basicConstraints = CA:FALSE
EOF
```

#### CA 인증서 생성
Root CA 키와 인증서를 생성합니다:

```bash
export OPENSSL_CONF=$BASE/openssl.cnf
cd $BASE

echo "Development CA"
openssl req -newkey rsa -keyout private/ca.key.pem -out ca.csr.pem -subj "/O=$ORG/CN=$ORG $CA Development"
openssl ca -batch -selfsign -extensions v3_ca -in ca.csr.pem -out ca.cert.pem -keyfile private/ca.key.pem
```

#### 서명용 키 쌍 생성
번들 서명에 사용될 키 쌍을 생성합니다:

```bash
echo "Development Signing Keys 1"
openssl req -newkey rsa -keyout private/development-1.key.pem -out development-1.csr.pem -subj "/O=$ORG/CN=$ORG Development-1"
openssl ca -batch -extensions v3_leaf -in development-1.csr.pem -out development-1.cert.pem
```

#### site.conf 설정 자동 추가
생성된 키들을 Yocto 빌드에서 사용하도록 설정을 추가합니다:

```bash
CONFFILE=${BUILDDIR}/conf/site.conf

echo "RAUC_KEYRING_FILE=\"${BUILDDIR}/example-ca/ca.cert.pem\"" >> $CONFFILE
echo "RAUC_KEY_FILE=\"${BUILDDIR}/example-ca/private/development-1.key.pem\"" >> $CONFFILE
echo "RAUC_CERT_FILE=\"${BUILDDIR}/example-ca/development-1.cert.pem\"" >> $CONFFILE
```

## 3. Bundle Class 구조 분석

### 3.1 Bundle 빌드 과정
`kirkstone/meta-rauc/classes/bundle.bbclass`에서 번들 생성 과정을 관리합니다:

#### 서명 키 검증
```python
do_bundle() {
    if [ -z "${RAUC_KEY_FILE}" ]; then
        bbfatal "'RAUC_KEY_FILE' not set. Please set to a valid key file location."
    fi

    if [ -z "${RAUC_CERT_FILE}" ]; then
        bbfatal "'RAUC_CERT_FILE' not set. Please set to a valid certificate file location."
    fi
```

#### RAUC 번들 생성 명령
```bash
${STAGING_BINDIR_NATIVE}/rauc bundle \
    --debug \
    --cert="${RAUC_CERT_FILE}" \
    --key="${RAUC_KEY_FILE}" \
    ${BUNDLE_ARGS} \
    ${BUNDLE_DIR} \
    ${B}/bundle.raucb
```

### 3.2 Manifest 파일 구조
Python 함수 `write_manifest(d)`가 `manifest.raucm` 파일을 생성합니다:

```python
def write_manifest(d):
    manifest = open('%s/manifest.raucm' % bundle_path, 'w')
    
    manifest.write('[update]\n')
    manifest.write(d.expand('compatible=${RAUC_BUNDLE_COMPATIBLE}\n'))
    manifest.write(d.expand('version=${RAUC_BUNDLE_VERSION}\n'))
    manifest.write(d.expand('description=${RAUC_BUNDLE_DESCRIPTION}\n'))
    manifest.write(d.expand('build=${RAUC_BUNDLE_BUILD}\n'))
    manifest.write('\n')

    # Bundle format 설정 (plain/verity)
    bundle_format = d.getVar('RAUC_BUNDLE_FORMAT')
    if bundle_format and bundle_format != "plain":
        manifest.write('[bundle]\n')
        manifest.write(d.expand('format=${RAUC_BUNDLE_FORMAT}\n'))
        manifest.write('\n')
```

#### 슬롯 정보 추가
각 이미지 슬롯에 대한 정보를 manifest에 추가합니다:

```python
for slot in (d.getVar('RAUC_BUNDLE_SLOTS') or "").split():
    slotflags = d.getVarFlags('RAUC_SLOT_%s' % slot, expand=slot_varflags) or {}
    
    slotname = slotflags.get('name', slot)
    manifest.write('[image.%s]\n' % slotname)
    
    imgtype = slotflags.get('type', 'image')
    img_fstype = slotflags.get('fstype', d.getVar('RAUC_IMAGE_FSTYPE'))
    
    # 이미지 파일명 결정
    if imgtype == 'image':
        fallback = "%s-%s.%s" % (d.getVar('RAUC_SLOT_%s' % slot), machine, img_fstype)
        imgname = imgsource = slotflags.get('file', fallback)
    
    manifest.write("filename=%s\n" % imgname)
    if 'hooks' in slotflags:
        manifest.write("hooks=%s\n" % slotflags.get('hooks'))
    if 'adaptive' in slotflags:
        manifest.write("adaptive=%s\n" % slotflags.get('adaptive'))
```

## 4. 빌드 과정에서의 서명 프로세스

### 4.1 Docker 환경에서의 키 설정
`docker/entrypoint.sh`에서 RAUC 키를 자동으로 설정합니다:

```bash
# Setup RAUC keys if they don't exist
echo "🔑 Setting up RAUC keys..."
if [ ! -f "$BUILDDIR/example-ca/private/development-1.key.pem" ]; then
  echo "🛠 Generating RAUC keys..."
  cd "$BUILDDIR/../meta-nuc"
  ./create-example-keys.sh
  cd "$BUILDDIR"
else
  echo "ℹ️  RAUC keys already exist"
fi
```

#### site.conf 자동 생성
```bash
# Ensure site.conf has RAUC key configuration
SITECONF="$BUILDDIR/conf/site.conf"
if [ ! -f "$SITECONF" ] || ! grep -q "RAUC_KEY_FILE" "$SITECONF"; then
  echo "🛠 Creating site.conf with RAUC key configuration..."
  cat > "$SITECONF" <<EOF
# RAUC Key Configuration
RAUC_KEYRING_FILE="\${TOPDIR}/example-ca/ca.cert.pem"
RAUC_KEY_FILE="\${TOPDIR}/example-ca/private/development-1.key.pem"
RAUC_CERT_FILE="\${TOPDIR}/example-ca/development-1.cert.pem"
EOF
fi
```

### 4.2 Bundle 빌드 과정
`complete_bundle_build()` 함수에서 번들 빌드를 실행합니다:

```bash
complete_bundle_build() {
  echo "🧹 Cleaning sstate for dashboard, rauc, and bundles ..."
  for r in dashboard rauc nuc-image-qt5-bundle; do
    if bitbake-layers show-recipes "$r" | grep -q "^$r"; then
      bitbake -c cleansstate "$r" || true
    fi
  done

  echo "📦 Building nuc-image-qt5-bundle ..."
  if ! bitbake nuc-image-qt5-bundle; then
    echo "❌ Bundle build failed"; exec bash; fi
  echo "✅ Bundle build completed successfully"
  
  # 생성된 번들 위치 표시
  BUNDLE_PATH="$(find "$BUILDDIR/tmp-glibc/deploy/images/intel-corei7-64/" -name "*nuc-image-qt5-bundle*.raucb" 2>/dev/null | head -1)"
  if [ -n "$BUNDLE_PATH" ]; then
    echo "📍 Bundle created at: $BUNDLE_PATH"
    echo "📏 Bundle size: $(du -h "$BUNDLE_PATH" | cut -f1)"
  fi
}
```

## 5. 시스템 설정 파일

### 5.1 RAUC 시스템 설정
`kirkstone/meta-nuc/recipes-core/rauc/files/intel-corei7-64/system.conf`:

```ini
[system]
compatible=@NUC_BUNDLE_COMPATIBLE@
bootloader=grub
grubenv=/grubenv/grubenv
statusfile=/data/rauc.status

[keyring]
path=ca.cert.pem

[slot.rootfs.0]
device=/dev/sda2
type=ext4
bootname=A

[slot.rootfs.1]
device=/dev/sda3
type=ext4
bootname=B
```

### 5.2 NUC 특화 설정
`kirkstone/meta-nuc/recipes-core/rauc/rauc_rauc.inc`:

```bash
# additional dependencies required to run RAUC on the target  
RDEPENDS:${PN} += "grub-editenv e2fsprogs-mke2fs"

# Define compatible bundles (by default, same as RAUC)
NUC_BUNDLE_COMPATIBLE ?= "${RAUC_BUNDLE_COMPATIBLE}"

# Change to SATA device instead of NVMe
ROOT_BLOCK_DEVICE_NAME ?= "sda"

do_install:prepend () {
  # Replace root block device name parameters
  sed -i -e 's:@ROOT_BLOCK_DEVICE_NAME@:${ROOT_BLOCK_DEVICE_NAME}:g' ${WORKDIR}/system.conf
  
  # Replace compatible bundle parameter  
  sed -i -e 's:@NUC_BUNDLE_COMPATIBLE@:${NUC_BUNDLE_COMPATIBLE}:g' ${WORKDIR}/system.conf
}
```

## 6. 번들 내용물 구조

### 6.1 생성된 번들 파일 구조
```
bundle.raucb (SquashFS 압축 파일시스템)
├── manifest.raucm           # 번들 메타데이터
├── nuc-image-qt5-intel-corei7-64.ext4  # 루트 파일시스템 이미지
└── signature                # 디지털 서명
```

### 6.2 Manifest 파일 예시
```ini
[update]
compatible=intel-corei7-64-oe
version=1.0
description=NUC Qt5 Image Bundle
build=20250805123456

[image.rootfs]
filename=nuc-image-qt5-intel-corei7-64.ext4
```

## 7. 서명 검증 과정

### 7.1 번들 생성 시 서명
RAUC 네이티브 도구가 다음 과정으로 서명을 생성합니다:

1. **Manifest 해시 계산**: SHA-256으로 manifest.raucm 파일의 해시 계산
2. **이미지 해시 계산**: 포함된 모든 이미지 파일의 해시 계산  
3. **종합 해시 계산**: Manifest와 이미지 해시들을 결합한 최종 해시 계산
4. **서명 생성**: RSA 개인키로 최종 해시에 대한 디지털 서명 생성
5. **번들 패킹**: 모든 파일과 서명을 SquashFS로 압축

### 7.2 타겟에서의 검증 과정

#### RAUC 설치 명령
```bash
sudo rauc install /data/nuc-image-qt5-bundle-intel-corei7-64.raucb
```

#### 검증 단계
1. **번들 압축 해제**: SquashFS 번들을 임시 디렉토리로 추출
2. **서명 검증**: 
   ```bash
   # ca.cert.pem으로 서명 검증
   openssl dgst -sha256 -verify ca_public_key.pem -signature signature manifest_and_images
   ```
3. **Compatible 검사**: 시스템 설정과 번들의 compatible 문자열 비교
4. **무결성 검증**: 각 이미지 파일의 해시와 manifest의 해시 비교
5. **설치 수행**: 검증 완료 후 실제 파티션에 이미지 기록

## 8. 보안 특징

### 8.1 암호화 알고리즘
- **해시 알고리즘**: SHA-256
- **서명 알고리즘**: RSA-2048 with PKCS#1 v1.5 padding
- **인증서 표준**: X.509 v3

### 8.2 보안 정책
- **키 길이**: 2048비트 RSA 키 사용
- **인증서 유효기간**: 99991231235959Z (거의 무제한)
- **CRL 유효기간**: 5000일

### 8.3 공격 방어
- **무결성 보장**: 디지털 서명으로 번들 변조 방지
- **인증 보장**: CA 체인을 통한 번들 출처 인증  
- **재생 공격 방지**: 버전 정보를 통한 다운그레이드 방지

## 9. 문제 해결

### 9.1 일반적인 오류

#### 키 파일 없음 오류
```
FATAL: 'RAUC_KEY_FILE' not set. Please set to a valid key file location.
```
**해결방법**: `./create-example-keys.sh` 실행 후 `site.conf` 확인

#### 인증서 검증 실패
```
ERROR: Failed to verify bundle signature
```
**해결방법**: 타겟의 `/etc/rauc/system.conf`에서 keyring path 확인

#### Compatible 불일치
```  
ERROR: Bundle is not compatible with this system
```
**해결방법**: 번들과 시스템의 compatible 문자열 일치 확인

### 9.2 디버깅 명령

#### 번들 정보 확인
```bash
rauc info bundle.raucb
```

#### 시스템 상태 확인  
```bash
rauc status
```

#### 로그 확인
```bash
journalctl -u rauc
```

## 10. 고급 설정

### 10.1 Verity 포맷 번들
무결성 보장을 위한 dm-verity 지원:

```bash
RAUC_BUNDLE_FORMAT = "verity"
```

### 10.2 Casync 번들
차등 업데이트를 위한 casync 지원:

```bash  
RAUC_CASYNC_BUNDLE = "1"
```

### 10.3 커스텀 후크
설치 과정에서 실행할 스크립트 추가:

```bash
RAUC_BUNDLE_HOOKS[file] = "install-hook.sh"
RAUC_BUNDLE_HOOKS[hooks] = "pre-install;post-install"
```

이 문서는 RAUC 번들의 암호화 및 서명 과정에 대한 완전한 가이드를 제공합니다. 실제 프로덕션 환경에서는 보안 요구사항에 따라 키 길이와 알고리즘을 조정하고, 안전한 키 관리 시스템을 구축해야 합니다.