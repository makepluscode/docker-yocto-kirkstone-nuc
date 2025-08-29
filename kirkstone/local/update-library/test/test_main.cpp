#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include "update_client.h"

using namespace UpdateLibrary;

class TestCallbacks {
public:
    static void onProgress(const ProgressInfo& progress) {
        std::cout << "Progress: " << progress.percentage << "% - " << progress.message << std::endl;
    }

    static void onCompleted(InstallResult result, const std::string& message) {
        std::cout << "Installation " << (result == InstallResult::SUCCESS ? "completed" : "failed")
                  << ": " << message << std::endl;
    }

    static void onError(const std::string& error_message) {
        std::cerr << "Error: " << error_message << std::endl;
    }
};

void printSlotStatus(const std::vector<SlotInfo>& slots) {
    std::cout << "\n=== Slot Status ===" << std::endl;
    for (const auto& slot : slots) {
        std::cout << "Slot: " << slot.slot_name << std::endl;
        for (const auto& prop : slot.properties) {
            std::cout << "  " << prop.first << " = " << prop.second << std::endl;
        }
        std::cout << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::cout << "Update Library Test Application" << std::endl;
    std::cout << "===============================" << std::endl;

    // UpdateClient 생성 및 초기화
    auto client = std::make_unique<UpdateClient>();

    // 콜백 설정
    client->setProgressCallback(TestCallbacks::onProgress);
    client->setCompletedCallback(TestCallbacks::onCompleted);
    client->setErrorCallback(TestCallbacks::onError);

    // 초기화
    std::cout << "\n1. Initializing UpdateClient..." << std::endl;
    if (!client->initialize()) {
        std::cerr << "Failed to initialize UpdateClient: " << client->getLastError() << std::endl;
        return 1;
    }
    std::cout << "UpdateClient initialized successfully" << std::endl;

    // 시스템 정보 출력
    std::cout << "\n2. System Information:" << std::endl;
    std::cout << "Compatible: " << client->getCompatible() << std::endl;
    std::cout << "Boot Slot: " << client->getBootSlot() << std::endl;
    std::cout << "Operation: " << client->getOperation() << std::endl;

    // 슬롯 상태 조회
    std::cout << "\n3. Querying slot status..." << std::endl;
    auto slots = client->getSlotStatus();
    printSlotStatus(slots);

    // 번들 설치 테스트 (명령줄 인자로 번들 경로 제공된 경우)
    if (argc > 1) {
        std::string bundle_path = argv[1];
        std::cout << "\n4. Bundle Information:" << std::endl;

        std::string compatible, version;
        if (client->getBundleInfo(bundle_path, compatible, version)) {
            std::cout << "Bundle Compatible: " << compatible << std::endl;
            std::cout << "Bundle Version: " << version << std::endl;

            // 호환성 확인
            if (compatible == client->getCompatible()) {
                std::cout << "Bundle is compatible with system" << std::endl;

                std::cout << "\n5. Starting installation..." << std::endl;
                if (client->install(bundle_path)) {
                    std::cout << "Installation started successfully" << std::endl;

                    // 설치 완료까지 대기
                    while (client->isInstalling()) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }

                    std::cout << "\nInstallation process finished" << std::endl;

                    // 설치 후 슬롯 상태 재조회
                    std::cout << "\n6. Post-installation slot status:" << std::endl;
                    auto post_slots = client->getSlotStatus();
                    printSlotStatus(post_slots);

                } else {
                    std::cerr << "Failed to start installation: " << client->getLastError() << std::endl;
                }
            } else {
                std::cout << "Bundle is not compatible with system" << std::endl;
                std::cout << "System requires: " << client->getCompatible() << std::endl;
                std::cout << "Bundle provides: " << compatible << std::endl;
            }
        } else {
            std::cerr << "Failed to get bundle info: " << client->getLastError() << std::endl;
        }
    } else {
        std::cout << "\n4. Skipping installation test (no bundle path provided)" << std::endl;
        std::cout << "Usage: " << argv[0] << " [bundle_path.raucb]" << std::endl;
    }

    std::cout << "\nTest completed successfully" << std::endl;
    return 0;
}
