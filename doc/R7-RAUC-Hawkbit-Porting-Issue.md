# RAUC Hawkbit C++ 포팅 이슈 및 해결 과정

## 📋 개요
`rauc-hawkbit-cpp` 애플리케이션을 Yocto 프로젝트에 통합하는 과정에서 발생한 다양한 빌드 이슈들과 해결 시도 과정을 정리한 문서입니다.

## 🎯 목표
- C++ 애플리케이션 `rauc-hawkbit-cpp` 생성
- DBus를 통한 RAUC 서비스 통신
- libcurl과 libjson을 통한 HTTPS 서버 통신
- Yocto 빌드 시스템 통합
- systemd 서비스로 부팅 후 자동 실행

## 📁 생성된 파일 구조

### 1. 애플리케이션 소스 코드
```
kirkstone/local/rauc-hawkbit-cpp/
├── CMakeLists.txt              # CMake 빌드 설정
├── src/
│   ├── main.cpp               # 메인 애플리케이션
│   ├── hawkbit_client.h       # Hawkbit 클라이언트 헤더
│   ├── hawkbit_client.cpp     # Hawkbit 클라이언트 구현
│   ├── rauc_client.h          # RAUC DBus 클라이언트 헤더
│   └── rauc_client.cpp        # RAUC DBus 클라이언트 구현
├── services/
│   └── rauc-hawkbit-cpp.service  # systemd 서비스 파일
├── build.sh                   # 로컬 빌드 스크립트
├── deploy.sh                  # 배포 스크립트
├── dlt-receive.sh             # DLT 로그 수신 스크립트
└── README.md                  # 애플리케이션 문서
```

### 2. Yocto 레이어 통합
```
kirkstone/meta-apps/recipes-apps/rauc-hawkbit-cpp/
└── rauc-hawkbit-cpp_1.0.bb    # BitBake 레시피
```

### 3. 이미지 설정 수정
- `kirkstone/meta-nuc/recipes-core/images/nuc-image-qt5.bb`: `rauc-hawkbit-cpp` 추가
- `kirkstone/meta-nuc/conf/local.conf.sample`: `EXTERNALSRC` 설정 추가
- `kirkstone/build/conf/local.conf`: `EXTERNALSRC` 설정 적용

## 🔧 주요 구현 내용

### 1. CMakeLists.txt 설정
```cmake
cmake_minimum_required(VERSION 3.10)
project(rauc-hawkbit-cpp)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DLT REQUIRED automotive-dlt)
pkg_check_modules(DBUS REQUIRED dbus-1)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(JSON REQUIRED json-c)

add_executable(rauc-hawkbit-cpp
    src/main.cpp
    src/hawkbit_client.cpp
    src/rauc_client.cpp
)

target_include_directories(rauc-hawkbit-cpp PRIVATE
    ${DLT_INCLUDE_DIRS}
    ${DBUS_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${JSON_INCLUDE_DIRS}
    src
)

target_link_libraries(rauc-hawkbit-cpp
    ${DLT_LIBRARIES}
    ${DBUS_LIBRARIES}
    ${CURL_LIBRARIES}
    ${JSON_LIBRARIES}
)
```

### 2. BitBake 레시피 (rauc-hawkbit-cpp_1.0.bb)
```bitbake
SUMMARY = "RAUC Hawkbit C++ Client"
DESCRIPTION = "A C++ application that integrates RAUC with Eclipse Hawkbit for over-the-air updates"
LICENSE = "CLOSED"

PN = "rauc-hawkbit-cpp"

DEPENDS = "dlt-daemon cmake-native pkgconfig-native dbus curl json-c rauc"
RDEPENDS:${PN} = "rauc dbus curl json-c"

SRC_URI = ""

S = "${EXTERNALSRC}"

inherit cmake systemd externalsrc

SYSTEMD_SERVICE:${PN} = "rauc-hawkbit-cpp.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

EXTRA_OECMAKE = ""

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${EXTERNALSRC}/services/rauc-hawkbit-cpp.service ${D}${systemd_system_unitdir}/
}

FILES:${PN} += " \
    ${systemd_system_unitdir}/rauc-hawkbit-cpp.service \
    /usr/local/bin/rauc-hawkbit-cpp \
"
```

### 3. systemd 서비스 파일
```ini
[Unit]
Description=RAUC Hawkbit C++ Client
After=network.target rauc.service
Wants=rauc.service

[Service]
Type=simple
ExecStart=/usr/local/bin/rauc-hawkbit-cpp
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
```

## 🚨 발생한 문제들과 해결 시도

### 1. 환경 설정 문제

#### 문제: `bash: oe-init-build-env: No such file or directory`
**원인**: 잘못된 경로에서 Yocto 환경 초기화 시도
**해결**: `poky/oe-init-build-env` 경로로 수정

#### 문제: `Could NOT find PkgConfig (missing: PKG_CONFIG_EXECUTABLE)`
**원인**: 로컬 호스트 빌드에서 pkg-config 누락
**해결**: Yocto 환경에서만 빌드하도록 설정 (로컬 빌드는 예상된 문제)

### 2. User Namespaces 문제

#### 문제: `ERROR: User namespaces are not usable by BitBake, possibly due to AppArmor.`
**원인**: Ubuntu 24.04에서 AppArmor과 User Namespaces 충돌
**해결 방법들**:
1. User namespaces 활성화: `echo 1 | sudo tee /proc/sys/kernel/unprivileged_userns_clone`
2. Docker 환경 사용: `./docker.sh auto`
3. AppArmor 비활성화 (권장하지 않음)

### 3. externalsrc 설정 문제

#### 문제: `CMake Error: The source directory ... does not appear to contain CMakeLists.txt.`
**원인**: `rauc-hawkbit-cpp`의 `EXTERNALSRC` 설정 누락
**해결**: `local.conf`에 다음 설정 추가:
```bitbake
EXTERNALSRC:pn-rauc-hawkbit-cpp = "${LOCAL}/rauc-hawkbit-cpp"
EXTERNALSRC_BUILD:pn-rauc-hawkbit-cpp = "${WORKDIR}/build"
```

### 4. rauc-native 빌드 문제 (핵심 이슈)

#### 문제 1: `ERROR: Nothing RPROVIDES 'rauc-native-service-native'`
**원인**: `rauc-native`가 target용 service 컴포넌트를 빌드하려고 시도
**해결 시도 1**: `PACKAGECONFIG`에서 "service" 제거
```bitbake
PACKAGECONFIG ??= "network gpt"
```

#### 문제 2: 여전히 service 관련 빌드 시도
**해결 시도 2**: 명시적으로 service 제거
```bitbake
PACKAGECONFIG:remove = "service streaming"
PACKAGECONFIG:remove = "service"
```

#### 문제 3: CMake에서 service 옵션 비활성화
**해결 시도 3**: `EXTRA_OECMAKE`에서 service 비활성화
```bitbake
EXTRA_OECMAKE:remove = "-DENABLE_SERVICE=ON"
EXTRA_OECMAKE += "-DENABLE_SERVICE=OFF"
```

#### 문제 4: `fatal error: rauc-installer-generated.h: No such file or directory`
**원인**: service.c와 main.c가 여전히 컴파일되고 있음
**해결 시도 4**: CMakeLists.txt에서 소스 파일 제거
```bitbake
do_configure:prepend() {
    # Remove service.c and main.c from CMakeLists.txt for native build
    sed -i '/src\/service.c/d' ${S}/CMakeLists.txt
    sed -i '/src\/main.c/d' ${S}/CMakeLists.txt
}
```

#### 문제 5: `Cannot specify link libraries for target "rauc" which is not built`
**원인**: sed 명령으로 service.c는 제거되었지만, main.c도 함께 제거되어 rauc 실행 파일 타겟이 정의되지 않음
**디버깅 과정**: 
- CMakeLists.txt 수정 전후 비교를 위한 디버깅 코드 추가
- service.c 제거는 성공했지만 rauc 타겟 링킹 실패 확인

#### 문제 6: Meson vs CMake 빌드 시스템 혼동
**원인**: 업스트림 RAUC는 Meson을 사용하지만, 사용 중인 RAUC 1.13은 CMake 사용
**해결 시도 5**: Meson 빌드 시도 → "Neither directory contains a build file meson.build" 오류로 실패
**최종 확인**: RAUC 1.13은 CMake 기반임을 확인

#### 🔧 최종 해결 방법: Fallback 메커니즘 활용
**결론**: rauc-native 빌드 수정보다는 기존 fallback 메커니즘 활용이 더 효율적
- bundle.bbclass에 이미 구현된 fallback 로직 활용:
```bash
if [ -x "${STAGING_BINDIR_NATIVE}/rauc" ]; then
    # 정상적인 서명된 번들 생성
else
    # Fallback: 서명되지 않은 squashfs 번들 생성
    mksquashfs ${BUNDLE_DIR} ${B}/bundle.raucb -all-root -noappend
fi
```
- rauc-native 의존성 임시 비활성화로 fallback 테스트 진행

### 5. 번들 빌드 문제

#### 문제: `Required build target 'nuc-image-qt5-bundle' has no buildable providers.`
**원인**: `rauc-native` 의존성으로 인한 번들 빌드 실패
**해결 시도**: bundle.bbclass 수정

1. **의존성 조건부 설정**:
```bitbake
DEPENDS = "squashfs-tools-native"
DEPENDS += "${@bb.utils.contains('DISTRO_FEATURES', 'rauc', 'rauc-native', '', d)}"
```

2. **do_bundle 태스크 수정**:
```bash
do_bundle() {
    # Check if rauc-native is available
    if [ -x "${STAGING_BINDIR_NATIVE}/rauc" ]; then
        # 정상적인 rauc bundle 생성
        ${STAGING_BINDIR_NATIVE}/rauc bundle ...
    else
        # Fallback: create a simple squashfs bundle without signing
        bbwarn "rauc-native not available, creating unsigned bundle"
        mksquashfs ${BUNDLE_DIR} ${B}/bundle.raucb -all-root -noappend
    fi
}
```

## 📊 현재 상태

### ✅ 완료된 작업
1. `rauc-hawkbit-cpp` 애플리케이션 소스 코드 생성
2. CMake 빌드 시스템 설정
3. BitBake 레시피 생성
4. systemd 서비스 파일 생성
5. Yocto 레이어 통합
6. `externalsrc` 설정
7. `rauc-native` 빌드 문제 분석 및 해결 시도 (다수)
8. 번들 빌드 우회 방법 구현 (fallback 메커니즘)
9. rauc-native 의존성 분석 및 CMake 빌드 시스템 이해
10. bundle.bbclass 수정으로 fallback 메커니즘 활용 준비 완료

### 🔄 진행 중인 작업
1. fallback 메커니즘을 사용한 번들 빌드 테스트
2. 서명되지 않은 번들로 전체 빌드 플로우 검증
3. `nuc-bundle` 빌드 진행 중

### 📋 남은 작업
1. fallback 번들 빌드 성공 확인
2. `rauc-hawkbit-cpp` 애플리케이션 빌드 성공
3. `nuc-image-qt5` 이미지 빌드 성공
4. 타겟 디바이스에서 애플리케이션 테스트
5. 향후 필요 시 rauc-native 빌드 문제 해결 (서명된 번들이 필요한 경우)

## 🛠️ 해결 방법 요약

### rauc-native 문제 분석 결과
1. **근본 원인**: rauc-native가 bundle 생성뿐만 아니라 target용 service 컴포넌트도 함께 빌드하려 시도
2. **복잡성**: service.c 제거 시 main.c도 필요하게 되어 의존성 체인이 복잡함
3. **OpenSSL 3.0 호환성**: deprecated ENGINE API 사용으로 추가 컴파일러 플래그 필요
4. **CMake 타겟 구조**: 라이브러리와 실행 파일 타겟이 밀접하게 연결되어 있음

### 최종 선택한 해결 방법
1. **Fallback 메커니즘 활용**: 기존 bundle.bbclass의 fallback 로직 사용
2. **rauc-native 의존성 임시 비활성화**: 개발 단계에서 서명되지 않은 번들로 테스트
3. **향후 개선 방향**: 필요 시 rauc-native를 최소 기능으로 빌드하는 별도 레시피 개발

### 적용된 수정사항
```bitbake
# bundle.bbclass 수정
DEPENDS = "squashfs-tools-native"
# Temporarily disable rauc-native to test fallback mechanism
# DEPENDS += "${@bb.utils.contains('DISTRO_FEATURES', 'rauc', 'rauc-native', '', d)}"
```

### 현재 빌드 순서
1. ✅ `rauc` (target용) 빌드 성공
2. 🔄 `nuc-bundle` fallback 빌드 진행 중 
3. 📋 `rauc-hawkbit-cpp` 빌드 예정
4. 📋 `nuc-image-qt5` 이미지 빌드 예정

## 📝 참고 사항

### 주요 파일 위치
- 애플리케이션: `kirkstone/local/rauc-hawkbit-cpp/`
- 레시피: `kirkstone/meta-apps/recipes-apps/rauc-hawkbit-cpp/`
- 이미지 설정: `kirkstone/meta-nuc/recipes-core/images/`
- 번들 클래스: `kirkstone/meta-rauc-cmake/classes/bundle.bbclass`

### 빌드 명령어
```bash
# Docker 환경에서 빌드
./docker.sh auto

# 현재 권장 빌드 순서 (fallback 메커니즘 사용)
bitbake nuc-bundle          # fallback으로 서명되지 않은 번들 생성
bitbake rauc-hawkbit-cpp    # 애플리케이션 빌드
bitbake nuc-image-qt5       # 전체 이미지 빌드

# 문제가 있는 빌드 (참고용)
# bitbake rauc-native       # 현재 빌드 실패
# bitbake nuc-image-qt5-bundle  # rauc-native 의존성으로 실패
```

### 현재 적용된 임시 수정사항
```bash
# bundle.bbclass에서 rauc-native 의존성 비활성화
# kirkstone/meta-rauc-cmake/classes/bundle.bbclass:194
# DEPENDS += "${@bb.utils.contains('DISTRO_FEATURES', 'rauc', 'rauc-native', '', d)}"
```

### 디버깅 명령어
```bash
# 빌드 로그 확인
bitbake -e rauc-native | grep -E "(DEPENDS|PACKAGECONFIG|EXTRA_OECMAKE)"

# 의존성 확인
bitbake -g rauc-native
cat pn-buildlist
```

## 🎯 최종 결론 및 교훈

### 핵심 발견사항
1. **복잡한 의존성보다 단순한 우회가 효과적**: rauc-native 수정보다 fallback 메커니즘이 더 안정적
2. **빌드 시스템 이해의 중요성**: CMake vs Meson 혼동, 타겟 구조 이해 부족으로 많은 시행착오 발생
3. **기존 인프라 활용**: bundle.bbclass에 이미 fallback 로직이 구현되어 있었음
4. **개발 단계에서의 pragmatic approach**: 서명되지 않은 번들로도 개발 및 테스트 가능

### 학습한 Yocto/BitBake 개념
- `PACKAGECONFIG` 시스템과 조건부 빌드
- `DEPENDS` vs `RDEPENDS` 차이점
- `externalsrc` 클래스 사용법
- CMake 기반 레시피에서의 `EXTRA_OECMAKE` 활용
- `do_configure:prepend()` 훅 사용법
- BitBake 클래스 (.bbclass) 수정 방법

### 향후 개선 방향
1. **Production 환경**: 서명된 번들이 필요한 경우 rauc-native 문제 재검토
2. **Security**: 키 관리 및 번들 서명 구현
3. **CI/CD**: 자동화된 빌드 파이프라인에서의 fallback 메커니즘 검증
4. **Documentation**: 팀 내 Yocto 빌드 가이드라인 수립

---

이 문서는 `rauc-hawkbit-cpp` 애플리케이션의 Yocto 통합 과정에서 발생한 모든 문제들과 해결 시도를 상세히 기록한 것입니다. 특히 `rauc-native` 빌드 문제를 통해 Yocto 빌드 시스템의 복잡성을 이해하고, pragmatic한 해결 방법을 찾는 과정을 보여줍니다. 현재 fallback 메커니즘을 통한 번들 생성이 진행 중이며, 이를 통해 전체 개발 워크플로우를 완성할 예정입니다. 