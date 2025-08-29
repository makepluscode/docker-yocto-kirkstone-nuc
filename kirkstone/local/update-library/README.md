# Update Library

RAUC의 핵심 기능을 포팅한 C++ 라이브러리입니다. D-Bus 의존성 없이 직접 RAUC 번들 설치와 슬롯 상태 조회 기능을 제공합니다.

## 개요

이 라이브러리는 update-service에서 RAUC D-Bus 서비스 대신 사용할 수 있도록 설계되었습니다. RAUC의 핵심 기능인 번들 설치와 슬롯 상태 관리를 직접 구현하여 성능을 향상시키고 의존성을 줄였습니다.

## 주요 특징

- **D-Bus 의존성 제거**: RAUC D-Bus 서비스 없이 동작
- **간소화된 API**: install과 status 기능에 집중
- **콜백 시스템**: 실시간 진행률 및 상태 업데이트
- **C/C++ 혼합**: RAUC C 코드 포팅 + C++ 인터페이스
- **완전한 호환성**: 기존 RAUC 번들 및 설정 파일 지원

## 아키텍처

```
┌─────────────────────────────────────┐
│          update-service             │  ← 기존 D-Bus 브로커 서비스
│      (D-Bus 호출 대신 직접 호출)     │
├─────────────────────────────────────┤
│          UpdateClient               │  ← 메인 공개 API
│        (C++ 인터페이스)             │
├─────────────────────────────────────┤
│          RaucEngine                 │  ← RAUC 기능 래퍼
│       (C++ → C 브리지)              │
├─────────────────────────────────────┤
│      RAUC Core Functions            │  ← 포팅된 RAUC 함수들
│    (포팅된 C 함수들)                │
│  • bundle.c  • install.c           │
│  • slot.c    • manifest.c          │
│  • checksum.c • utils.c             │
└─────────────────────────────────────┘
```

## 빌드 요구사항

### 필수 라이브러리
- **GLib 2.0**: 기본 데이터 구조 및 유틸리티
- **GIO**: 파일 I/O 및 스트림 처리
- **OpenSSL**: 암호화 및 서명 검증
- **CMake 3.16+**: 빌드 시스템

### 선택적 라이브러리
- **DLT**: 로깅 (기존 시스템과의 호환성)

## 빌드 방법

```bash
# 빌드 디렉토리 생성
mkdir build && cd build

# CMake 설정
cmake ..

# 빌드 실행
make -j$(nproc)

# 테스트 실행
./update-library-test
```

### Yocto/OpenEmbedded 환경에서 빌드

```bash
# OE 환경 설정
source /usr/local/oecore-x86_64/environment-setup-corei7-64-oe-linux

# 크로스 컴파일 빌드
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="/usr/local/oecore-x86_64/sysroots/x86_64-oesdk-linux/usr/share/cmake/OEToolchainConfig.cmake"
make -j$(nproc)
```

## API 사용법

### 기본 사용 예제

```cpp
#include <update-library/update_client.h>

using namespace UpdateLibrary;

// 콜백 함수 정의
void onProgress(const ProgressInfo& progress) {
    std::cout << "Progress: " << progress.percentage << "% - "
              << progress.message << std::endl;
}

void onCompleted(InstallResult result, const std::string& message) {
    std::cout << "Installation "
              << (result == InstallResult::SUCCESS ? "completed" : "failed")
              << ": " << message << std::endl;
}

int main() {
    // 1. UpdateClient 생성
    auto client = std::make_unique<UpdateClient>();

    // 2. 콜백 설정
    client->setProgressCallback(onProgress);
    client->setCompletedCallback(onCompleted);

    // 3. 초기화
    if (!client->initialize()) {
        std::cerr << "Failed to initialize: " << client->getLastError() << std::endl;
        return 1;
    }

    // 4. 시스템 정보 조회
    std::cout << "Compatible: " << client->getCompatible() << std::endl;
    std::cout << "Boot Slot: " << client->getBootSlot() << std::endl;

    // 5. 슬롯 상태 조회
    auto slots = client->getSlotStatus();
    for (const auto& slot : slots) {
        std::cout << "Slot: " << slot.slot_name << std::endl;
        for (const auto& prop : slot.properties) {
            std::cout << "  " << prop.first << " = " << prop.second << std::endl;
        }
    }

    // 6. 번들 설치
    if (client->install("/path/to/bundle.raucb")) {
        std::cout << "Installation started" << std::endl;

        // 설치 완료 대기
        while (client->isInstalling()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    return 0;
}
```

### update-service 연동 예제

```cpp
// update_service.cpp에서
#include <update-library/update_client.h>

class UpdateService {
private:
    std::unique_ptr<UpdateLibrary::UpdateClient> update_client_;

public:
    bool initialize() {
        update_client_ = std::make_unique<UpdateLibrary::UpdateClient>();

        // 콜백 설정
        update_client_->setProgressCallback([this](const auto& progress) {
            // D-Bus 신호로 진행률 전송
            sendProgressSignal(progress.percentage, progress.message);
        });

        update_client_->setCompletedCallback([this](auto result, const auto& message) {
            // D-Bus 신호로 완료 알림
            bool success = (result == UpdateLibrary::InstallResult::SUCCESS);
            sendCompletedSignal(success, message);
        });

        return update_client_->initialize();
    }

    DBusMessage* handleInstall(DBusMessage* message) {
        // D-Bus 파라미터 파싱
        const char* bundle_path = ...;

        // 라이브러리 호출 (D-Bus 대신)
        bool success = update_client_->install(bundle_path);

        // D-Bus 응답 생성
        return create_dbus_reply(success);
    }

    DBusMessage* handleGetSlotStatus(DBusMessage* message) {
        // 라이브러리에서 슬롯 상태 조회
        auto slots = update_client_->getSlotStatus();

        // D-Bus 형식으로 변환 후 응답
        return create_slot_status_reply(slots);
    }
};
```

## API 레퍼런스

### UpdateClient 클래스

#### 메서드

- `bool initialize(const std::string& config_file_path = "")`: 클라이언트 초기화
- `bool install(const std::string& bundle_path)`: 번들 설치 시작
- `std::vector<SlotInfo> getSlotStatus()`: 모든 슬롯 상태 조회
- `std::string getBootSlot()`: 현재 부트 슬롯 조회
- `std::string getCompatible()`: 시스템 호환성 문자열 조회
- `bool getBundleInfo(bundle_path, compatible&, version&)`: 번들 정보 조회

#### 콜백 설정

- `setProgressCallback(ProgressCallback)`: 진행률 콜백 설정
- `setCompletedCallback(CompletedCallback)`: 완료 콜백 설정
- `setErrorCallback(ErrorCallback)`: 오류 콜백 설정

#### 상태 조회

- `bool isInitialized()`: 초기화 상태 확인
- `bool isInstalling()`: 설치 진행 상태 확인
- `std::string getLastError()`: 마지막 오류 메시지
- `std::string getOperation()`: 현재 작업 상태

### 데이터 타입

```cpp
// 진행률 정보
struct ProgressInfo {
    int percentage;       // 0-100
    std::string message;
    int nesting_depth;
};

// 슬롯 정보
struct SlotInfo {
    std::string slot_name;
    std::map<std::string, std::string> properties;
};

// 설치 결과
enum class InstallResult {
    SUCCESS = 0,
    FAILURE = 1,
    CANCELLED = 2
};

// 콜백 함수 타입
using ProgressCallback = std::function<void(const ProgressInfo&)>;
using CompletedCallback = std::function<void(InstallResult, const std::string&)>;
using ErrorCallback = std::function<void(const std::string&)>;
```

## 설정 파일

라이브러리는 기존 RAUC 설정 파일(`/etc/rauc/system.conf`)을 사용합니다:

```ini
[system]
compatible=nuc-intel-corei7-64
bootloader=grub
grubenv=/boot/grub/grubenv

[keyring]
path=/etc/rauc/keyring.pem

[slot.rootfs.0]
device=/dev/sda3
type=ext4
bootname=A

[slot.rootfs.1]
device=/dev/sda4
type=ext4
bootname=B
```

## 제약사항

### 지원하는 기능
- ✅ 번들 서명 검증
- ✅ 번들 설치 (A/B 슬롯)
- ✅ 슬롯 상태 조회
- ✅ 호환성 검사
- ✅ GRUB 부트로더 지원

### 지원하지 않는 기능
- ❌ casync 번들
- ❌ 암호화 번들
- ❌ NBD (Network Block Device)
- ❌ HTTP 다운로드
- ❌ 번들 생성/수정

## 문제 해결

### 일반적인 오류

1. **"Failed to initialize RAUC context"**
   - 시스템 권한 확인 (root 필요)
   - 설정 파일 경로 확인

2. **"Bundle check failed"**
   - 번들 서명 확인
   - 키링 파일 경로 확인
   - 번들 파일 손상 여부 확인

3. **"Installation failed"**
   - 대상 슬롯 마운트 상태 확인
   - 충분한 디스크 공간 확인
   - 슬롯 권한 확인

### 디버깅

DLT 로깅 활성화:
```cpp
// 빌드 시 -DWITH_DLT=ON 옵션 사용
// DLT 뷰어에서 "UCLI", "RENG" 컨텍스트 모니터링
```

표준 로깅:
```bash
# G_MESSAGES_DEBUG 환경변수 설정
export G_MESSAGES_DEBUG=all
./your-application
```

## 성능

기존 RAUC D-Bus 대비 예상 성능 향상:
- **설치 속도**: 5-10% 향상 (D-Bus 오버헤드 제거)
- **메모리 사용량**: 20-30% 감소 (별도 프로세스 불필요)
- **응답 시간**: 50% 향상 (IPC 오버헤드 제거)

## 라이센스

이 라이브러리는 RAUC 프로젝트의 라이센스를 따릅니다.

## 기여

버그 리포트나 기능 요청은 프로젝트 이슈 트래커를 사용해 주세요.

## 관련 문서

- [RAUC 공식 문서](https://rauc.readthedocs.io/)
- [구현 계획서](IMPLEMENTATION_PLAN.md)
- [PRD 문서](UPDATE_LIBRARY_PRD.md)
