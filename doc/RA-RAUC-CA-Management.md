# RAUC CA Management System

이 문서는 Yocto 빌드 시스템과 bundler 간 RAUC CA(Certificate Authority) 인증서 관리 시스템의 구조와 동작 과정을 설명합니다.

## System Overview

RAUC 업데이트 시스템에서 번들 서명과 검증을 위해 bundler와 Yocto 빌드 시스템이 동일한 CA 인증서를 사용하도록 통합 관리합니다.

## Directory Structure

```
project-root/
├── tools/bundler/files/example-ca/          # bundler 원본 CA 파일들
│   ├── ca.cert.pem                          # CA 인증서 (공개키)
│   ├── development-1.cert.pem               # 개발 서명 인증서
│   └── private/
│       ├── ca.key.pem                       # CA 개인키
│       └── development-1.key.pem            # 개발 서명 개인키
│
├── kirkstone/meta-nuc/recipes-core/rauc/files/
│   └── ca-fixed/                            # Yocto용 고정 CA 파일들 (bundler 복사본)
│       ├── ca.cert.pem                      # 동일한 CA 인증서
│       ├── ca.key.pem                       # 동일한 CA 개인키
│       ├── development-1.cert.pem           # 동일한 개발 서명 인증서
│       └── development-1.key.pem            # 동일한 개발 서명 개인키
│
└── kirkstone/build/example-ca/              # 빌드 시 생성되는 작업 디렉토리
    ├── ca.cert.pem                          # ca-fixed/에서 복사됨
    ├── development-1.cert.pem               # ca-fixed/에서 복사됨
    └── private/
        ├── ca.key.pem                       # ca-fixed/에서 복사됨
        └── development-1.key.pem            # ca-fixed/에서 복사됨
```

## Configuration Files

### 1. RAUC Recipe Configuration
`kirkstone/meta-nuc/recipes-core/rauc/rauc_rauc.inc`:
```bash
# Use fixed CA from meta-nuc layer (compatible with bundler)
RAUC_KEYRING_FILE = "ca-fixed/ca.cert.pem"
```

### 2. CA Generation Script
`kirkstone/meta-nuc/create-example-keys.sh`:
- 새 CA 생성 대신 고정 CA 파일들을 복사
- `ca-fixed/` 디렉토리에서 `build/example-ca/`로 복사

## Build Flow

### Step 1: Build Initiation
```bash
./build.sh auto     # 또는 ./build.sh bundle
```

### Step 2: Docker Container Setup
- `docker.sh` 실행으로 Yocto 빌드 컨테이너 시작
- `entrypoint.sh` 스크립트 실행

### Step 3: Build Environment Setup
`entrypoint.sh`:
1. Yocto 빌드 환경 초기화 (`source poky/oe-init-build-env build`)
2. `bblayers.conf` 및 `local.conf` 설정
3. RAUC 키 설정 단계로 진입

### Step 4: RAUC CA Setup
```bash
echo "🔑 Setting up RAUC keys..."
if [ ! -f "$BUILDDIR/example-ca/private/development-1.key.pem" ]; then
    echo "🛠 Generating RAUC keys..."
    cd "$BUILDDIR/../meta-nuc"
    ./create-example-keys.sh    # 고정 CA 복사 실행
    cd "$BUILDDIR"
else
    echo "ℹ️  RAUC keys already exist"
fi
```

### Step 5: Fixed CA Copy Process
`create-example-keys.sh`:
1. 고정 CA 디렉토리 확인: `$BBPATH/../meta-nuc/recipes-core/rauc/files/ca-fixed`
2. 작업 디렉토리 생성: `$BBPATH/example-ca`
3. 고정 CA 파일들 복사:
   ```bash
   cp "$FIXED_CA_DIR/ca.cert.pem" "$BASE/"
   cp "$FIXED_CA_DIR/development-1.cert.pem" "$BASE/"
   cp "$FIXED_CA_DIR/ca.key.pem" "$BASE/private/"
   cp "$FIXED_CA_DIR/development-1.key.pem" "$BASE/private/"
   ```
4. OpenSSL CA 데이터베이스 파일 생성 (`index.txt`, `serial`)

### Step 6: Site Configuration
`site.conf` 파일에 RAUC 키 경로 설정:
```bash
RAUC_KEYRING_FILE="${BUILDDIR}/example-ca/ca.cert.pem"
RAUC_KEY_FILE="${BUILDDIR}/example-ca/private/development-1.key.pem"
RAUC_CERT_FILE="${BUILDDIR}/example-ca/development-1.cert.pem"
```

### Step 7: RAUC Recipe Build
1. `rauc-target.inc`의 `do_install()` 함수 실행
2. `RAUC_KEYRING_FILE` (ca-fixed/ca.cert.pem) 사용
3. CA 인증서를 `/etc/rauc/ca.cert.pem`으로 이미지에 설치

### Step 8: Bundle Creation (bundle mode)
`bitbake nuc-image-qt5-bundle`:
1. `development-1.key.pem`으로 RAUC 번들 서명
2. 서명된 `.raucb` 파일 생성

## Verification Process

### CA Consistency Check
```bash
# bundler와 meta-nuc CA가 동일한지 확인
md5sum tools/bundler/files/example-ca/ca.cert.pem
md5sum kirkstone/meta-nuc/recipes-core/rauc/files/ca-fixed/ca.cert.pem
# 결과: 97cd48de2c2bff9022ebd6597878cc78 (동일)
```

### Bundle Installation Test
```bash
# NUC 타겟에서 번들 설치
sudo rauc install /path/to/bundle.raucb
# 성공: CA 인증서 검증 통과
```

## Key Benefits

1. **일관된 서명**: bundler와 Yocto 빌드가 동일한 CA 사용
2. **자동화**: 빌드 프로세스에서 자동으로 고정 CA 적용
3. **영구 적용**: Git으로 관리되어 팀원 간 일관성 보장
4. **호환성**: 모든 RAUC 번들이 NUC 타겟에서 설치 가능

## Certificate Details

### CA Certificate (ca.cert.pem)
- **Issuer**: `C=US, ST=State, L=City, O=Example Org, CN=Example Org RAUC CA Development`
- **Validity**: 2025-08-19 ~ 2035-08-17
- **Serial Number**: 582671644467...
- **Public Key**: RSA 2048 bit

### Development Signing Certificate (development-1.cert.pem)
- **Issuer**: Example Org RAUC CA Development
- **Subject**: `C=US, ST=State, O=Example Org, CN=Example Org Development-1`
- **Validity**: 2025-08-19 ~ 2026-08-19
- **Usage**: Digital Signature, TLS Web Client/Server Authentication

## Troubleshooting

### CA Files Not Found
```bash
❌ Error: Fixed CA files not found in $FIXED_CA_DIR
```
**Solution**: bundler CA 파일들이 `meta-nuc/recipes-core/rauc/files/ca-fixed/`에 복사되었는지 확인

### Bundle Installation Failure
```bash
rauc: signature verification failed
```
**Solution**: NUC 타겟의 `/etc/rauc/ca.cert.pem`과 bundler CA가 동일한지 확인

### Build Failure
```bash
RAUC_KEYRING_FILE not found
```
**Solution**: `create-example-keys.sh` 실행 여부 및 `site.conf` 설정 확인