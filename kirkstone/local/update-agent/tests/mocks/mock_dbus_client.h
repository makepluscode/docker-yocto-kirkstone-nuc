/**
 * @file mock_dbus_client.h
 * @brief D-Bus 클라이언트 모킹 인터페이스
 * 
 * ServiceAgent의 D-Bus 통신을 모킹하기 위한 인터페이스입니다.
 * 실제 D-Bus 서비스 없이 테스트할 수 있도록 합니다.
 */

#ifndef MOCK_DBUS_CLIENT_H
#define MOCK_DBUS_CLIENT_H

#include <gmock/gmock.h>
#include <string>
#include <functional>

/**
 * @class MockDbusClient
 * @brief D-Bus 클라이언트 모킹 클래스
 * 
 * ServiceAgent의 D-Bus 통신을 모킹하여
 * 실제 RAUC 서비스 없이 테스트할 수 있도록 합니다.
 */
class MockDbusClient {
public:
    virtual ~MockDbusClient() = default;
    
    /**
     * @brief D-Bus 연결 모킹
     * @return 연결 성공 여부
     */
    MOCK_METHOD(bool, connect, (), ());
    
    /**
     * @brief D-Bus 연결 해제 모킹
     */
    MOCK_METHOD(void, disconnect, (), ());
    
    /**
     * @brief 연결 상태 확인 모킹
     * @return 연결 상태
     */
    MOCK_METHOD(bool, isConnected, (), (const));
    
    /**
     * @brief 서비스 확인 모킹
     * @return 서비스 사용 가능 여부
     */
    MOCK_METHOD(bool, checkService, (), ());
    
    /**
     * @brief 번들 설치 모킹
     * @param bundle_path 번들 파일 경로
     * @return 설치 시작 성공 여부
     */
    MOCK_METHOD(bool, installBundle, (const std::string& bundle_path), ());
    
    /**
     * @brief 상태 조회 모킹
     * @param status 상태 정보를 받을 문자열 참조
     * @return 조회 성공 여부
     */
    MOCK_METHOD(bool, getStatus, (std::string& status), ());
    
    /**
     * @brief 부트 슬롯 조회 모킹
     * @param boot_slot 부트 슬롯 정보를 받을 문자열 참조
     * @return 조회 성공 여부
     */
    MOCK_METHOD(bool, getBootSlot, (std::string& boot_slot), ());
    
    /**
     * @brief 좋음 마킹 모킹
     * @return 마킹 성공 여부
     */
    MOCK_METHOD(bool, markGood, (), ());
    
    /**
     * @brief 나쁨 마킹 모킹
     * @return 마킹 성공 여부
     */
    MOCK_METHOD(bool, markBad, (), ());
    
    /**
     * @brief 번들 정보 조회 모킹
     * @param bundle_path 번들 파일 경로
     * @param info 번들 정보를 받을 문자열 참조
     * @return 조회 성공 여부
     */
    MOCK_METHOD(bool, getBundleInfo, (const std::string& bundle_path, std::string& info), ());
    
    /**
     * @brief 진행률 콜백 설정 모킹
     * @param callback 콜백 함수
     */
    MOCK_METHOD(void, setProgressCallback, (std::function<void(int)> callback), ());
    
    /**
     * @brief 완료 콜백 설정 모킹
     * @param callback 콜백 함수
     */
    MOCK_METHOD(void, setCompletedCallback, (std::function<void(bool, const std::string&)> callback), ());
    
    /**
     * @brief 메시지 처리 모킹
     */
    MOCK_METHOD(void, processMessages, (), ());
};

#endif // MOCK_DBUS_CLIENT_H