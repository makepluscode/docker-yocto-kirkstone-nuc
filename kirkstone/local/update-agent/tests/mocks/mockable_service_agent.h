/**
 * @file mockable_package_installer.h
 * @brief 모킹 가능한 PackageInstaller 래퍼
 *
 * PackageInstaller의 D-Bus 통신을 모킹할 수 있도록
 * 의존성 주입을 지원하는 래퍼 클래스입니다.
 */

#ifndef MOCKABLE_PACKAGE_INSTALLER_H
#define MOCKABLE_PACKAGE_INSTALLER_H

#include <memory>
#include <string>
#include <functional>
#include "package_installer.h"
#include "mock_dbus_client.h"

/**
 * @class MockablePackageInstaller
 * @brief 모킹 가능한 PackageInstaller 래퍼 클래스
 *
 * PackageInstaller의 D-Bus 통신을 모킹할 수 있도록
 * 의존성 주입을 지원합니다.
 */
class MockablePackageInstaller {
public:
    /**
     * @brief 생성자
     * @param dbus_client D-Bus 클라이언트 (선택적, nullptr이면 실제 클라이언트 사용)
     */
    MockablePackageInstaller(MockDbusClient* dbus_client = nullptr);

    /**
     * @brief 소멸자
     */
    ~MockablePackageInstaller() = default;

    /**
     * @brief D-Bus 연결 (모킹 가능)
     * @return 연결 성공 여부
     */
    bool connect();

    /**
     * @brief D-Bus 연결 해제 (모킹 가능)
     */
    void disconnect();

    /**
     * @brief 연결 상태 확인 (모킹 가능)
     * @return 연결 상태
     */
    bool isConnected() const;

    /**
     * @brief 서비스 확인 (모킹 가능)
     * @return 서비스 사용 가능 여부
     */
    bool checkService();

    /**
     * @brief 번들 설치 (모킹 가능)
     * @param package_path 패키지 파일 경로
     * @return 설치 시작 성공 여부
     */
    bool installPackage(const std::string& package_path);

    /**
     * @brief 상태 조회 (모킹 가능)
     * @param status 상태 정보를 받을 문자열 참조
     * @return 조회 성공 여부
     */
    bool getStatus(std::string& status);

    /**
     * @brief 부트 슬롯 조회 (모킹 가능)
     * @param boot_slot 부트 슬롯 정보를 받을 문자열 참조
     * @return 조회 성공 여부
     */
    bool getBootSlot(std::string& boot_slot);

    /**
     * @brief 좋음 마킹 (모킹 가능)
     * @return 마킹 성공 여부
     */
    bool markGood();

    /**
     * @brief 나쁨 마킹 (모킹 가능)
     * @return 마킹 성공 여부
     */
    bool markBad();

    /**
     * @brief 번들 정보 조회 (모킹 가능)
     * @param package_path 패키지 파일 경로
     * @param info 패키지 정보를 받을 문자열 참조
     * @return 조회 성공 여부
     */
    bool getPackageInfo(const std::string& package_path, std::string& info);

    /**
     * @brief 진행률 콜백 설정 (모킹 가능)
     * @param callback 콜백 함수
     */
    void setProgressCallback(std::function<void(int)> callback);

    /**
     * @brief 완료 콜백 설정 (모킹 가능)
     * @param callback 콜백 함수
     */
    void setCompletedCallback(std::function<void(bool, const std::string&)> callback);

    /**
     * @brief 메시지 처리 (모킹 가능)
     */
    void processMessages();

private:
    std::unique_ptr<PackageInstaller> real_agent_;
    MockDbusClient* mock_dbus_client_;
};

#endif // MOCKABLE_PACKAGE_INSTALLER_H
