/**
 * @file test_service_agent_mocked.cpp
 * @brief ServiceAgent 모킹 테스트
 *
 * MockDbusClient를 사용하여 ServiceAgent의 D-Bus 통신을 모킹하고
 * 실제 D-Bus 서비스 없이 테스트합니다.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <memory>
#include <functional>
#include "mocks/mock_dbus_client.h"
#include "mocks/mockable_service_agent.h"

namespace {

/**
 * @class ServiceAgentMockedTest
 * @brief ServiceAgent 모킹 테스트 클래스
 *
 * MockDbusClient를 사용하여 D-Bus 통신을 모킹하고
 * ServiceAgent의 로직을 테스트합니다.
 */
class ServiceAgentMockedTest : public ::testing::Test {
protected:
    /**
     * @brief 각 테스트 시작 전 초기화
     */
    void SetUp() override {
        // Mock D-Bus 클라이언트 생성
        mock_dbus_client_ = std::make_unique<MockDbusClient>();

        // MockableServiceAgent 생성 (Mock D-Bus 클라이언트 주입)
        service_agent_ = std::make_unique<MockableServiceAgent>(mock_dbus_client_.get());

        // 테스트용 콜백 호출 기록을 위한 변수 초기화
        progress_callback_called_ = false;
        completed_callback_called_ = false;
        last_progress_ = -1;
        last_success_ = false;
        last_message_.clear();
    }

    /**
     * @brief 각 테스트 종료 후 정리
     */
    void TearDown() override {
        service_agent_.reset();
        mock_dbus_client_.reset();
    }

    // 테스트용 멤버 변수들
    std::unique_ptr<MockDbusClient> mock_dbus_client_;
    std::unique_ptr<MockableServiceAgent> service_agent_;

    // 콜백 테스트를 위한 상태 변수들
    bool progress_callback_called_;
    bool completed_callback_called_;
    int last_progress_;
    bool last_success_;
    std::string last_message_;

    /**
     * @brief 테스트용 진행률 콜백 함수
     * @param progress 진행률 (0-100)
     */
    void testProgressCallback(int progress) {
        progress_callback_called_ = true;
        last_progress_ = progress;
    }

    /**
     * @brief 테스트용 완료 콜백 함수
     * @param success 성공 여부
     * @param message 상태 메시지
     */
    void testCompletedCallback(bool success, const std::string& message) {
        completed_callback_called_ = true;
        last_success_ = success;
        last_message_ = message;
    }
};

/**
 * @brief 성공적인 D-Bus 연결 테스트
 *
 * MockDbusClient가 성공적으로 연결할 때
 * ServiceAgent가 올바르게 처리하는지 검증합니다.
 */
TEST_F(ServiceAgentMockedTest, ConnectSuccess) {
    // Given: Mock D-Bus 클라이언트가 성공적으로 연결하도록 설정
    EXPECT_CALL(*mock_dbus_client_, connect())
        .WillOnce(testing::Return(true));

    EXPECT_CALL(*mock_dbus_client_, isConnected())
        .WillOnce(testing::Return(true));

    // When: D-Bus 연결 실행
    const bool result = service_agent_->connect();

    // Then: 성공적으로 처리되어야 함
    EXPECT_TRUE(result) << "D-Bus 연결이 성공해야 합니다";
    EXPECT_TRUE(service_agent_->isConnected()) << "연결 상태가 true여야 합니다";
}

/**
 * @brief D-Bus 연결 실패 테스트
 *
 * MockDbusClient가 연결 실패를 반환할 때
 * ServiceAgent가 올바르게 처리하는지 검증합니다.
 */
TEST_F(ServiceAgentMockedTest, ConnectFailure) {
    // Given: Mock D-Bus 클라이언트가 연결 실패를 반환하도록 설정
    EXPECT_CALL(*mock_dbus_client_, connect())
        .WillOnce(testing::Return(false));

    EXPECT_CALL(*mock_dbus_client_, isConnected())
        .WillOnce(testing::Return(false));

    // When: D-Bus 연결 실행
    const bool result = service_agent_->connect();

    // Then: 실패가 올바르게 처리되어야 함
    EXPECT_FALSE(result) << "D-Bus 연결이 실패해야 합니다";
    EXPECT_FALSE(service_agent_->isConnected()) << "연결 상태가 false여야 합니다";
}

/**
 * @brief 연결 해제 테스트
 *
 * MockDbusClient를 사용하여 연결 해제를 테스트합니다.
 */
TEST_F(ServiceAgentMockedTest, Disconnect) {
    // Given: Mock D-Bus 클라이언트가 연결 해제를 처리하도록 설정
    EXPECT_CALL(*mock_dbus_client_, disconnect())
        .Times(1);

    EXPECT_CALL(*mock_dbus_client_, isConnected())
        .WillOnce(testing::Return(false));

    // When: 연결 해제 실행
    service_agent_->disconnect();

    // Then: 연결 상태가 false여야 함
    EXPECT_FALSE(service_agent_->isConnected()) << "연결 해제 후 상태가 false여야 합니다";
}

/**
 * @brief 서비스 확인 성공 테스트
 *
 * MockDbusClient가 서비스를 사용 가능하다고 반환할 때
 * ServiceAgent가 올바르게 처리하는지 검증합니다.
 */
TEST_F(ServiceAgentMockedTest, CheckServiceSuccess) {
    // Given: Mock D-Bus 클라이언트가 서비스 확인 성공을 반환하도록 설정
    EXPECT_CALL(*mock_dbus_client_, checkService())
        .WillOnce(testing::Return(true));

    // When: 서비스 확인 실행
    const bool result = service_agent_->checkService();

    // Then: 성공적으로 처리되어야 함
    EXPECT_TRUE(result) << "서비스 확인이 성공해야 합니다";
}

/**
 * @brief 서비스 확인 실패 테스트
 *
 * MockDbusClient가 서비스를 사용 불가능하다고 반환할 때
 * ServiceAgent가 올바르게 처리하는지 검증합니다.
 */
TEST_F(ServiceAgentMockedTest, CheckServiceFailure) {
    // Given: Mock D-Bus 클라이언트가 서비스 확인 실패를 반환하도록 설정
    EXPECT_CALL(*mock_dbus_client_, checkService())
        .WillOnce(testing::Return(false));

    // When: 서비스 확인 실행
    const bool result = service_agent_->checkService();

    // Then: 실패가 올바르게 처리되어야 함
    EXPECT_FALSE(result) << "서비스 확인이 실패해야 합니다";
}

/**
 * @brief 성공적인 번들 설치 테스트
 *
 * MockDbusClient가 성공적으로 번들을 설치할 때
 * ServiceAgent가 올바르게 처리하는지 검증합니다.
 */
TEST_F(ServiceAgentMockedTest, InstallBundleSuccess) {
    // Given: Mock D-Bus 클라이언트가 성공적으로 번들을 설치하도록 설정
    const std::string bundle_path = "/tmp/test-bundle.raucb";

    EXPECT_CALL(*mock_dbus_client_, installBundle(bundle_path))
        .WillOnce(testing::Return(true));

    // When: 번들 설치 실행
    const bool result = service_agent_->installBundle(bundle_path);

    // Then: 성공적으로 처리되어야 함
    EXPECT_TRUE(result) << "번들 설치가 성공해야 합니다";
}

/**
 * @brief 번들 설치 실패 테스트
 *
 * MockDbusClient가 번들 설치 실패를 반환할 때
 * ServiceAgent가 올바르게 처리하는지 검증합니다.
 */
TEST_F(ServiceAgentMockedTest, InstallBundleFailure) {
    // Given: Mock D-Bus 클라이언트가 번들 설치 실패를 반환하도록 설정
    const std::string bundle_path = "/tmp/invalid-bundle.raucb";

    EXPECT_CALL(*mock_dbus_client_, installBundle(bundle_path))
        .WillOnce(testing::Return(false));

    // When: 번들 설치 실행
    const bool result = service_agent_->installBundle(bundle_path);

    // Then: 실패가 올바르게 처리되어야 함
    EXPECT_FALSE(result) << "번들 설치가 실패해야 합니다";
}

/**
 * @brief 상태 조회 성공 테스트
 *
 * MockDbusClient가 상태 정보를 성공적으로 조회할 때
 * ServiceAgent가 올바르게 처리하는지 검증합니다.
 */
TEST_F(ServiceAgentMockedTest, GetStatusSuccess) {
    // Given: Mock D-Bus 클라이언트가 상태 조회 성공을 반환하도록 설정
    const std::string expected_status = "idle";

    EXPECT_CALL(*mock_dbus_client_, getStatus(testing::_))
        .WillOnce(testing::DoAll(
            testing::SetArgReferee<0>(expected_status),
            testing::Return(true)
        ));

    // When: 상태 조회 실행
    std::string status;
    const bool result = service_agent_->getStatus(status);

    // Then: 성공적으로 처리되어야 함
    EXPECT_TRUE(result) << "상태 조회가 성공해야 합니다";
    EXPECT_EQ(status, expected_status) << "상태 정보가 올바르게 반환되어야 합니다";
}

/**
 * @brief 부트 슬롯 조회 성공 테스트
 *
 * MockDbusClient가 부트 슬롯 정보를 성공적으로 조회할 때
 * ServiceAgent가 올바르게 처리하는지 검증합니다.
 */
TEST_F(ServiceAgentMockedTest, GetBootSlotSuccess) {
    // Given: Mock D-Bus 클라이언트가 부트 슬롯 조회 성공을 반환하도록 설정
    const std::string expected_boot_slot = "A";

    EXPECT_CALL(*mock_dbus_client_, getBootSlot(testing::_))
        .WillOnce(testing::DoAll(
            testing::SetArgReferee<0>(expected_boot_slot),
            testing::Return(true)
        ));

    // When: 부트 슬롯 조회 실행
    std::string boot_slot;
    const bool result = service_agent_->getBootSlot(boot_slot);

    // Then: 성공적으로 처리되어야 함
    EXPECT_TRUE(result) << "부트 슬롯 조회가 성공해야 합니다";
    EXPECT_EQ(boot_slot, expected_boot_slot) << "부트 슬롯 정보가 올바르게 반환되어야 합니다";
}

/**
 * @brief 마킹 메서드 테스트
 *
 * MockDbusClient를 사용하여 부트 슬롯 마킹을 테스트합니다.
 */
TEST_F(ServiceAgentMockedTest, SlotMarkingMethods) {
    // Given: Mock D-Bus 클라이언트가 마킹 성공을 반환하도록 설정
    EXPECT_CALL(*mock_dbus_client_, markGood())
        .WillOnce(testing::Return(true));

    EXPECT_CALL(*mock_dbus_client_, markBad())
        .WillOnce(testing::Return(true));

    // When & Then: 마킹 메서드들 실행
    const bool mark_good_result = service_agent_->markGood();
    const bool mark_bad_result = service_agent_->markBad();

    EXPECT_TRUE(mark_good_result) << "markGood이 성공해야 합니다";
    EXPECT_TRUE(mark_bad_result) << "markBad이 성공해야 합니다";
}

/**
 * @brief 콜백 설정 테스트
 *
 * MockDbusClient를 사용하여 콜백 설정을 테스트합니다.
 */
TEST_F(ServiceAgentMockedTest, CallbackSetup) {
    // Given: Mock D-Bus 클라이언트가 콜백 설정을 처리하도록 설정
    EXPECT_CALL(*mock_dbus_client_, setProgressCallback(testing::_))
        .Times(1);

    EXPECT_CALL(*mock_dbus_client_, setCompletedCallback(testing::_))
        .Times(1);

    // When: 콜백 설정 실행
    service_agent_->setProgressCallback([this](int progress) {
        testProgressCallback(progress);
    });

    service_agent_->setCompletedCallback([this](bool success, const std::string& message) {
        testCompletedCallback(success, message);
    });

    // Then: 예외 없이 설정되어야 함
    SUCCEED() << "콜백 함수들이 성공적으로 설정되었습니다";
}

/**
 * @brief 메시지 처리 테스트
 *
 * MockDbusClient를 사용하여 메시지 처리를 테스트합니다.
 */
TEST_F(ServiceAgentMockedTest, MessageProcessing) {
    // Given: Mock D-Bus 클라이언트가 메시지 처리를 하도록 설정
    EXPECT_CALL(*mock_dbus_client_, processMessages())
        .Times(1);

    // When: 메시지 처리 실행
    service_agent_->processMessages();

    // Then: 예외 없이 처리되어야 함
    SUCCEED() << "메시지 처리가 성공적으로 완료되었습니다";
}

/**
 * @brief 완전한 업데이트 플로우 테스트
 *
 * 연결, 서비스 확인, 번들 설치, 상태 조회의 전체 플로우를 모킹으로 테스트합니다.
 */
TEST_F(ServiceAgentMockedTest, CompleteUpdateFlow) {
    // Given: Mock D-Bus 클라이언트 설정
    const std::string bundle_path = "/tmp/update-bundle.raucb";
    const std::string expected_status = "installing";
    const std::string expected_boot_slot = "B";

    // 연결 성공 설정
    EXPECT_CALL(*mock_dbus_client_, connect())
        .WillOnce(testing::Return(true));

    EXPECT_CALL(*mock_dbus_client_, isConnected())
        .WillRepeatedly(testing::Return(true));

    // 서비스 확인 성공 설정
    EXPECT_CALL(*mock_dbus_client_, checkService())
        .WillOnce(testing::Return(true));

    // 번들 설치 성공 설정
    EXPECT_CALL(*mock_dbus_client_, installBundle(bundle_path))
        .WillOnce(testing::Return(true));

    // 상태 조회 성공 설정
    EXPECT_CALL(*mock_dbus_client_, getStatus(testing::_))
        .WillOnce(testing::DoAll(
            testing::SetArgReferee<0>(expected_status),
            testing::Return(true)
        ));

    // 부트 슬롯 조회 성공 설정
    EXPECT_CALL(*mock_dbus_client_, getBootSlot(testing::_))
        .WillOnce(testing::DoAll(
            testing::SetArgReferee<0>(expected_boot_slot),
            testing::Return(true)
        ));

    // 마킹 성공 설정
    EXPECT_CALL(*mock_dbus_client_, markGood())
        .WillOnce(testing::Return(true));

    // When: 전체 업데이트 플로우 실행
    bool connect_result = service_agent_->connect();
    bool service_result = service_agent_->checkService();
    bool install_result = service_agent_->installBundle(bundle_path);

    std::string status;
    bool status_result = service_agent_->getStatus(status);

    std::string boot_slot;
    bool boot_slot_result = service_agent_->getBootSlot(boot_slot);

    bool mark_result = service_agent_->markGood();

    // Then: 모든 단계가 성공해야 함
    EXPECT_TRUE(connect_result) << "연결이 성공해야 합니다";
    EXPECT_TRUE(service_result) << "서비스 확인이 성공해야 합니다";
    EXPECT_TRUE(install_result) << "번들 설치가 성공해야 합니다";
    EXPECT_TRUE(status_result) << "상태 조회가 성공해야 합니다";
    EXPECT_EQ(status, expected_status) << "상태 정보가 올바르게 반환되어야 합니다";
    EXPECT_TRUE(boot_slot_result) << "부트 슬롯 조회가 성공해야 합니다";
    EXPECT_EQ(boot_slot, expected_boot_slot) << "부트 슬롯 정보가 올바르게 반환되어야 합니다";
    EXPECT_TRUE(mark_result) << "마킹이 성공해야 합니다";
}

} // namespace
