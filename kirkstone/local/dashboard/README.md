# Dashboard Application

Qt5/QML 기반의 실시간 시스템 모니터링 대시보드 애플리케이션입니다.

## 프로젝트 구조

```
dashboard/
├── src/                    # C++ 소스 파일들
│   ├── main.cpp           # 메인 애플리케이션 진입점
│   ├── system_info.cpp    # 시스템 정보 수집 클래스
│   ├── system_info.h
│   ├── rauc_manager.cpp   # RAUC 업데이트 관리 클래스
│   ├── rauc_manager.h
│   ├── grub_manager.cpp   # GRUB 부팅 관리 클래스
│   └── grub_manager.h
├── qml/                   # QML UI 파일들
│   ├── DashboardMain.qml  # 메인 대시보드 화면
│   ├── DashboardCard.qml  # 대시보드 카드 컴포넌트
│   ├── DashboardCard00.qml # CPU 정보 카드
│   ├── DashboardCard01.qml # 메모리 정보 카드
│   ├── DashboardCard02.qml # 네트워크 정보 카드
│   ├── DashboardCard03.qml # 스토리지 정보 카드
│   ├── DashboardCard10.qml # 시스템 정보 카드
│   ├── DashboardCard40.qml # 부팅 정보 카드 (Boot Info)
│   ├── DashboardCard41.qml # 빈 카드
│   └── DashboardCard42.qml # 빈 카드
├── resources/             # 리소스 파일들
│   └── dashboard_resources.qrc # Qt 리소스 파일
├── services/              # SystemD 서비스 파일들
│   ├── dashboard.service
│   └── dashboard-eglfs.service
├── config/                # 설정 파일들
│   └── qt5_eglfs_config.json
├── CMakeLists.txt         # CMake 빌드 설정
├── build.sh              # 대시보드 빌드 스크립트
├── deploy.sh             # 대시보드 배포 스크립트
└── README.md             # 이 파일
```

## 빌드 방법

### 1. 메인 프로젝트 빌드 스크립트 사용 (권장)

프로젝트 루트에서 대시보드 애플리케이션을 빌드:

```bash
# 프로젝트 루트 디렉토리에서
./build.sh dashboard
```

이 명령은 다음을 수행합니다:
- Yocto SDK 환경 설정 (`/usr/local/oecore-x86_64/environment-setup-corei7-64-oe-linux`)
- `LD_LIBRARY_PATH` 환경변수 해제 (Yocto SDK 호환성)
- 대시보드 디렉토리로 이동
- CMake를 사용한 크로스 컴파일

### 2. 대시보드 디렉토리에서 직접 빌드

```bash
# 대시보드 디렉토리로 이동
cd kirkstone/local/dashboard

# Yocto SDK 환경 설정
unset LD_LIBRARY_PATH
source /usr/local/oecore-x86_64/environment-setup-corei7-64-oe-linux

# 빌드 실행
./build.sh
```

### 3. 수동 빌드 (개발용)

```bash
cd kirkstone/local/dashboard

# 환경 설정
unset LD_LIBRARY_PATH
source /usr/local/oecore-x86_64/environment-setup-corei7-64-oe-linux

# 빌드 디렉토리 생성 및 빌드
mkdir -p build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="/usr/local/oecore-x86_64/sysroots/x86_64-oesdk-linux/usr/share/cmake/OEToolchainConfig.cmake"
make -j$(nproc)
```

## 배포 방법

### 1. 자동 배포 스크립트 사용 (권장)

```bash
# 대시보드 디렉토리에서
cd kirkstone/local/dashboard

# 기본 타겟 (192.168.1.100)으로 배포
./deploy.sh

# 특정 타겟으로 배포
./deploy.sh root@192.168.1.50
```

### 2. 수동 배포

```bash
# 빌드된 바이너리 복사
scp build/dashboard root@192.168.1.100:/usr/bin/dashboard.new

# 서비스 파일 복사
scp services/dashboard-eglfs.service root@192.168.1.100:/lib/systemd/system/

# QML 파일 복사
ssh root@192.168.1.100 "mkdir -p /usr/share/dashboard/qml"
scp qml/*.qml root@192.168.1.100:/usr/share/dashboard/qml/

# 타겟에서 서비스 재시작
ssh root@192.168.1.100 <<'EOF'
systemctl stop dashboard-eglfs.service
mv /usr/bin/dashboard.new /usr/bin/dashboard
chmod +x /usr/bin/dashboard
systemctl daemon-reload
systemctl enable dashboard-eglfs.service
systemctl start dashboard-eglfs.service
EOF
```

## 의존성

- Qt5 (Core, Quick, Network, DBus)
- dlt-daemon
- systemd
- RAUC (업데이트 관리)
- GRUB (부팅 관리)

## 설치

Yocto 빌드 후 자동으로 설치됩니다:

- 실행 파일: `/usr/bin/dashboard`
- SystemD 서비스: `/etc/systemd/system/dashboard-eglfs.service`
- QML 파일: `/usr/share/dashboard/qml/`
- Qt5 설정: `/etc/qt5/qt5_config.json`
- 데스크톱 파일: `/usr/share/applications/dashboard.desktop`

## 실행

```bash
# SystemD 서비스로 실행 (권장)
systemctl start dashboard-eglfs.service

# 직접 실행
/usr/bin/dashboard

# 서비스 상태 확인
systemctl status dashboard-eglfs.service

# 로그 확인
journalctl -u dashboard-eglfs.service -f
```

## 주요 기능

### 대시보드 카드
- **Card00**: CPU 사용률 및 코어별 정보
- **Card01**: 메모리 사용률 및 상세 정보
- **Card02**: 네트워크 연결 상태 및 IP 주소
- **Card03**: 루트 파티션 사용률
- **Card10**: 시스템 정보 (호스트명, 아키텍처, 커널 버전)
- **Card40**: 부팅 정보 (Boot Order, Booted, Status)
- **Card41-42**: 빈 카드 (향후 확장용)

### 키보드 단축키
- **F1**: SW Update - 소프트웨어 업데이트 팝업 표시
- **F2-F3**: 빈 버튼 (향후 확장용)
- **F7**: Exit - 애플리케이션 종료
- **F8**: Reboot - 시스템 재부팅

### 부팅 정보 (Card40)
- **Boot Order**: "A B" 또는 "B A" (GRUB 부팅 순서)
- **Booted**: 현재 부팅된 슬롯 (A: /dev/sda2, B: /dev/sda3)
- **Status**: 슬롯 상태 (Good: 최소 하나의 슬롯이 정상, Bad: 모든 슬롯 비정상)

## 문제 해결

### Yocto SDK 환경 문제
```bash
# LD_LIBRARY_PATH 해제
unset LD_LIBRARY_PATH

# SDK 환경 재설정
source /usr/local/oecore-x86_64/environment-setup-corei7-64-oe-linux
```

### 빌드 실패
```bash
# 빌드 디렉토리 정리
rm -rf build
mkdir build
cd build

# CMake 재설정
cmake .. -DCMAKE_TOOLCHAIN_FILE="/usr/local/oecore-x86_64/sysroots/x86_64-oesdk-linux/usr/share/cmake/OEToolchainConfig.cmake"
```

### 배포 실패
```bash
# SSH 연결 확인
ssh root@192.168.1.100 "echo 'Connection OK'"

# 서비스 로그 확인
ssh root@192.168.1.100 "journalctl -u dashboard-eglfs.service --no-pager -n 20"
```
