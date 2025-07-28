# Dashboard Application

Qt5/QML 기반의 실시간 시스템 모니터링 대시보드 애플리케이션입니다.

## 프로젝트 구조

```
dashboard/
├── src/                    # C++ 소스 파일들
│   ├── main.cpp           # 메인 애플리케이션 진입점
│   ├── systeminfo.cpp     # 시스템 정보 수집 클래스
│   ├── systeminfo.h
│   ├── raucmanager.cpp    # RAUC 업데이트 관리 클래스
│   └── raucmanager.h
├── qml/                   # QML UI 파일들
│   ├── main.qml          # 메인 QML 파일
│   ├── DashboardCard.qml # 대시보드 카드 컴포넌트
│   ├── InfoRow.qml       # 정보 행 컴포넌트
│   └── RaucCard.qml      # RAUC 카드 컴포넌트
├── resources/             # 리소스 파일들
│   └── qml.qrc           # Qt 리소스 파일
├── services/              # SystemD 서비스 파일들
│   ├── dashboard.service
│   └── dashboard-eglfs.service
├── config/                # 설정 파일들
│   └── eglfs_kms_config.json
├── CMakeLists.txt         # CMake 빌드 설정
└── README.md             # 이 파일
```

## 빌드 방법

### Yocto 빌드 (권장)

Docker 컨테이너 내에서 Yocto 빌드 환경을 사용하여 빌드합니다:

```bash
# Docker 컨테이너 실행
docker run -it --rm \
  -v $(pwd):/home/yocto/kirkstone/local/dashboard \
  yocto-kirkstone-nuc

# 컨테이너 내에서 빌드
cd /home/yocto/kirkstone/build
bitbake dashboard
```

### 호스트 시스템 빌드 (개발용)

Qt5 개발 환경이 설치된 호스트 시스템에서 빌드:

```bash
mkdir build
cd build
cmake ..
make
```

## 의존성

- Qt5 (Core, Quick, Network, DBus)
- dlt-daemon
- systemd

## 설치

Yocto 빌드 후 자동으로 설치됩니다:

- 실행 파일: `/usr/bin/dashboard`
- SystemD 서비스: `/etc/systemd/system/dashboard-eglfs.service`
- Qt5 설정: `/etc/qt5/qt5_config.json`
- 데스크톱 파일: `/usr/share/applications/dashboard.desktop`

## 실행

```bash
# SystemD 서비스로 실행 (권장)
systemctl start dashboard-eglfs.service

# 직접 실행
/usr/bin/dashboard
``` 