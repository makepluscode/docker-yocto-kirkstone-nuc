# Python RAUC Bundler

기존 C 기반 bundler와 동일한 기능을 제공하는 Python 구현체입니다. RAUC 번들 생성의 5단계 프로세스를 완전히 구현하여 `.raucb` 파일을 생성합니다.

## 주요 기능

### RAUC 번들 생성 5단계 구현

1. **준비 단계 (Preparation)**: 입력 파일 및 인증서 검증
2. **매니페스트 생성 (Manifest Creation)**: 번들 메타데이터와 SHA256 해시 자동 생성
3. **번들 디렉토리 구성 (Bundle Directory Setup)**: 임시 디렉토리에 파일 구성
4. **서명 및 번들 생성 (Signing & Bundle Creation)**: RAUC 명령으로 서명된 번들 생성
5. **검증 (Verification)**: 생성된 번들의 무결성과 서명 검증

### C bundler와의 호환성

- **동일한 명령줄 인터페이스**: 기존 스크립트와 완벽 호환
- **동일한 출력 형식**: 같은 `.raucb` 파일 생성
- **동일한 인증서 지원**: 기존 PKI 인프라 사용 가능
- **동일한 에러 처리**: 일관된 에러 메시지와 종료 코드

## 설치 및 요구사항

### 시스템 요구사항

- Python 3.6 이상
- RAUC 도구 (번들 생성 시 필요)
- 리눅스/유닉스 환경

### 의존성 설치

```bash
# Ubuntu/Debian
sudo apt-get install python3 rauc

# CentOS/RHEL
sudo yum install python3 rauc
```

### Python 의존성

표준 라이브러리만 사용하므로 추가 Python 패키지 설치 불필요:

```bash
# requirements.txt 확인
cat requirements.txt
```

## 사용법

### 기본 사용법

```bash
# 서명 없이 번들 생성
./bundler.py rootfs.ext4 output.raucb

# 서명과 함께 번들 생성
./bundler.py -c cert.pem -k key.pem rootfs.ext4 output.raucb

# 상세 출력과 함께 생성
./bundler.py --verbose rootfs.ext4 output.raucb

# 기존 파일 덮어쓰기
./bundler.py --force rootfs.ext4 output.raucb
```

### 고급 사용법

```bash
# 프로젝트의 고정 CA 사용
./bundler.py \
    -c /home/makepluscode/docker-yocto-kirkstone-nuc/kirkstone/meta-nuc/recipes-core/rauc/files/ca-fixed/development-1.cert.pem \
    -k /home/makepluscode/docker-yocto-kirkstone-nuc/kirkstone/meta-nuc/recipes-core/rauc/files/ca-fixed/development-1.key.pem \
    --verbose \
    rootfs.ext4 \
    signed-bundle.raucb
```

### 명령줄 옵션

```
사용법: bundler.py [OPTIONS] <rootfs> <output>

위치 인수:
  rootfs              루트파일시스템 파일 경로 (예: rootfs.ext4)
  output              출력 .raucb 번들 파일 경로

선택적 인수:
  -c, --cert PATH     인증서 파일 경로
  -k, --key PATH      개인키 파일 경로
  -v, --verbose       상세 출력 활성화
  -f, --force         기존 출력 파일 덮어쓰기
  --version           버전 정보 표시
  -h, --help          도움말 메시지 표시
```

## 빌드 및 테스트

### 빌드 스크립트 사용

```bash
# 전체 빌드 및 테스트
./build.sh build

# 의존성 확인
./build.sh check

# 기본 기능 테스트
./build.sh test

# 통합 테스트 (실제 번들 생성)
./build.sh integration

# 시스템에 설치
sudo ./build.sh install

# 빌드 산출물 정리
./build.sh clean
```

### 수동 테스트

```bash
# 구문 검사
python3 -m py_compile bundler.py

# 도움말 테스트
python3 bundler.py --help

# 버전 확인
python3 bundler.py --version

# 실제 번들 생성 테스트 (테스트 파일 생성 후)
dd if=/dev/zero of=test-rootfs.ext4 bs=1M count=1
python3 bundler.py test-rootfs.ext4 test-bundle.raucb
```

## 프로젝트 구조

```
bundler-python/
├── bundler.py          # 메인 Python bundler 구현체
├── build.sh            # 빌드 및 테스트 스크립트
├── requirements.txt    # Python 의존성 (표준 라이브러리만 사용)
├── README.md           # 이 문서
└── build/             # 빌드 산출물 (자동 생성)
    └── test-bundle.raucb
└── test/              # 테스트 파일들 (자동 생성)
    └── test-rootfs.ext4
```

## 기술 구현 상세

### 클래스 구조

```python
class RaucBundler:
    """RAUC Bundle Creator 메인 클래스"""
    
    # 5단계 프로세스 구현
    def create_bundle()           # 메인 진입점
    def _prepare_bundle_directory() # 3단계: 번들 디렉토리 준비
    def _create_manifest()        # 2단계: 매니페스트 생성
    def _execute_rauc_bundle()    # 4단계: RAUC 번들 명령 실행
    def _verify_bundle()          # 5단계: 번들 검증
```

### 보안 기능

- **SHA256 해시 계산**: 파일 무결성 검증을 위한 자동 해시 계산
- **PKI 기반 서명**: X.509 인증서와 개인키를 사용한 디지털 서명
- **인증서 검증**: 번들 생성 전 인증서 파일 존재 확인
- **키링 검증**: CA 인증서를 사용한 번들 서명 검증

### 에러 처리

- **입력 유효성 검사**: 파일 존재, 경로 유효성, 권한 확인
- **RAUC 가용성 검사**: 시스템에 RAUC 도구 설치 확인
- **명확한 에러 메시지**: 문제 상황별 구체적인 에러 메시지 제공
- **정리 작업**: 실패 시 임시 파일 자동 정리

## C bundler와의 차이점

### 개선사항

1. **더 나은 에러 처리**: Python의 예외 처리로 더 명확한 에러 메시지
2. **자동 매니페스트 생성**: 루트파일시스템에서 자동으로 매니페스트 생성
3. **상세한 로깅**: 각 단계별 진행 상황 표시
4. **번들 크기 표시**: 생성된 번들의 크기 정보 제공
5. **자동 정리**: 임시 파일 자동 삭제

### 유지된 호환성

1. **명령줄 인터페이스**: 기존 스크립트와 완전 호환
2. **출력 형식**: 동일한 .raucb 파일 생성
3. **종료 코드**: 동일한 에러 코드 반환
4. **파일 경로**: 동일한 경로 처리 방식

## 성능 고려사항

- **메모리 효율성**: 큰 파일 처리 시 청크 단위로 해시 계산
- **임시 파일 관리**: 자동 정리로 디스크 공간 절약
- **병렬 처리**: 필요시 다중 번들 생성 지원 가능

## 트러블슈팅

### 일반적인 문제

1. **RAUC 미설치**
   ```
   [ERROR] RAUC is not installed or not available in PATH
   ```
   해결: `sudo apt-get install rauc`

2. **Python 버전 부족**
   ```
   [ERROR] Python 3.6 or higher is required
   ```
   해결: Python 3.6+ 설치

3. **인증서 파일 누락**
   ```
   [ERROR] Certificate file 'cert.pem' not found
   ```
   해결: 올바른 인증서 파일 경로 확인

4. **출력 파일 존재**
   ```
   [ERROR] Output file 'bundle.raucb' already exists. Use --force to overwrite
   ```
   해결: `--force` 옵션 사용 또는 기존 파일 삭제

### 디버깅 방법

```bash
# 상세 출력으로 실행
./bundler.py --verbose rootfs.ext4 bundle.raucb

# 의존성 확인
./build.sh check

# 통합 테스트로 환경 확인
./build.sh integration
```

## 기여 및 개발

### 코드 스타일

- PEP 8 준수
- 타입 힌트 사용
- docstring 작성
- 단위 테스트 포함

### 테스트 추가

새로운 기능 추가 시 `build.sh`의 테스트 함수에 테스트 케이스 추가:

```bash
# build.sh의 run_tests() 함수에 추가
run_custom_test() {
    # 새로운 테스트 로직
}
```

## 라이선스

이 프로젝트는 Yocto 프로젝트의 일부로서 동일한 라이선스 조건을 따릅니다.