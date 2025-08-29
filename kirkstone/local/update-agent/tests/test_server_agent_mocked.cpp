/**
 * @file test_server_agent_mocked.cpp
 * @brief ServerAgent 모킹 테스트
 * 
 * MockHttpClient를 사용하여 ServerAgent의 HTTP 통신을 모킹하고
 * 실제 네트워크 호출 없이 테스트합니다.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <memory>
#include "mocks/mock_http_client.h"
#include "mocks/mockable_server_agent.h"

namespace {

/**
 * @class ServerAgentMockedTest
 * @brief ServerAgent 모킹 테스트 클래스
 * 
 * MockHttpClient를 사용하여 HTTP 통신을 모킹하고
 * ServerAgent의 로직을 테스트합니다.
 */
class ServerAgentMockedTest : public ::testing::Test {
protected:
    /**
     * @brief 각 테스트 시작 전 초기화
     */
    void SetUp() override {
        // Mock HTTP 클라이언트 생성
        mock_http_client_ = std::make_unique<MockHttpClient>();
        
        // 테스트용 서버 정보 설정
        test_server_url_ = "https://hawkbit.example.com";
        test_tenant_ = "default";
        test_device_id_ = "test-device-001";
        
        // MockableServerAgent 생성 (Mock HTTP 클라이언트 주입)
        server_agent_ = std::make_unique<MockableServerAgent>(
            test_server_url_, test_tenant_, test_device_id_, mock_http_client_.get());
    }
    
    /**
     * @brief 각 테스트 종료 후 정리
     */
    void TearDown() override {
        server_agent_.reset();
        mock_http_client_.reset();
    }
    
    // 테스트용 멤버 변수들
    std::string test_server_url_;
    std::string test_tenant_;
    std::string test_device_id_;
    std::unique_ptr<MockHttpClient> mock_http_client_;
    std::unique_ptr<MockableServerAgent> server_agent_;
    
    /**
     * @brief 테스트용 유효한 JSON 응답 생성
     */
    std::string createValidJsonResponse() const {
        return R"({
            "config": {
                "polling": {
                    "sleep": "00:01:00"
                }
            }
        })";
    }
    
    /**
     * @brief 테스트용 배포 정보가 포함된 JSON 응답 생성
     */
    std::string createDeploymentJsonResponse() const {
        return R"({
            "id": "deployment-123",
            "deployment": {
                "download": "forced",
                "update": "forced",
                "chunks": [{
                    "part": "os",
                    "version": "2.1.0",
                    "name": "Update Package v2.1.0",
                    "artifacts": [{
                        "filename": "update-v2.1.0.tar.gz",
                        "hashes": {
                            "md5": "d41d8cd98f00b204e9800998ecf8427e",
                            "sha1": "da39a3ee5e6b4b0d3255bfef95601890afd80709",
                            "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
                        },
                        "size": 1024000,
                        "_links": {
                            "download": {
                                "href": "https://hawkbit.example.com/default/controller/v1/test-device-001/softwaremodules/456/artifacts/update-v2.1.0.tar.gz"
                            }
                        }
                    }]
                }]
            }
        })";
    }
};

/**
 * @brief 성공적인 업데이트 폴링 테스트
 * 
 * MockHttpClient가 성공 응답을 반환할 때
 * ServerAgent가 올바르게 처리하는지 검증합니다.
 */
TEST_F(ServerAgentMockedTest, PollForUpdatesSuccess) {
    // Given: Mock HTTP 클라이언트가 성공 응답을 반환하도록 설정
    const std::string expected_response = createValidJsonResponse();
    EXPECT_CALL(*mock_http_client_, get(testing::_, testing::_))
        .WillOnce(testing::DoAll(
            testing::SetArgReferee<1>(expected_response),
            testing::Return(true)
        ));
    
    // When: 업데이트 폴링 실행
    std::string response;
    const bool result = server_agent_->pollForUpdates(response);
    
    // Then: 성공적으로 처리되어야 함
    EXPECT_TRUE(result) << "업데이트 폴링이 성공해야 합니다";
    EXPECT_EQ(response, expected_response) << "응답 데이터가 올바르게 반환되어야 합니다";
}

/**
 * @brief 업데이트 폴링 실패 테스트
 * 
 * MockHttpClient가 실패를 반환할 때
 * ServerAgent가 올바르게 처리하는지 검증합니다.
 */
TEST_F(ServerAgentMockedTest, PollForUpdatesFailure) {
    // Given: Mock HTTP 클라이언트가 실패를 반환하도록 설정
    EXPECT_CALL(*mock_http_client_, get(testing::_, testing::_))
        .WillOnce(testing::Return(false));
    
    // When: 업데이트 폴링 실행
    std::string response;
    const bool result = server_agent_->pollForUpdates(response);
    
    // Then: 실패가 올바르게 처리되어야 함
    EXPECT_FALSE(result) << "업데이트 폴링이 실패해야 합니다";
}

/**
 * @brief 성공적인 번들 다운로드 테스트
 * 
 * MockHttpClient가 성공적으로 파일을 다운로드할 때
 * ServerAgent가 올바르게 처리하는지 검증합니다.
 */
TEST_F(ServerAgentMockedTest, DownloadBundleSuccess) {
    // Given: Mock HTTP 클라이언트가 성공적으로 다운로드하도록 설정
    const std::string download_url = "https://example.com/update.raucb";
    const std::string local_path = "/tmp/update.raucb";
    
    EXPECT_CALL(*mock_http_client_, downloadFile(download_url, local_path))
        .WillOnce(testing::Return(true));
    
    // When: 번들 다운로드 실행
    const bool result = server_agent_->downloadBundle(download_url, local_path);
    
    // Then: 성공적으로 처리되어야 함
    EXPECT_TRUE(result) << "번들 다운로드가 성공해야 합니다";
}

/**
 * @brief 번들 다운로드 실패 테스트
 * 
 * MockHttpClient가 다운로드 실패를 반환할 때
 * ServerAgent가 올바르게 처리하는지 검증합니다.
 */
TEST_F(ServerAgentMockedTest, DownloadBundleFailure) {
    // Given: Mock HTTP 클라이언트가 다운로드 실패를 반환하도록 설정
    const std::string download_url = "https://example.com/update.raucb";
    const std::string local_path = "/tmp/update.raucb";
    
    EXPECT_CALL(*mock_http_client_, downloadFile(download_url, local_path))
        .WillOnce(testing::Return(false));
    
    // When: 번들 다운로드 실행
    const bool result = server_agent_->downloadBundle(download_url, local_path);
    
    // Then: 실패가 올바르게 처리되어야 함
    EXPECT_FALSE(result) << "번들 다운로드가 실패해야 합니다";
}

/**
 * @brief 성공적인 피드백 전송 테스트
 * 
 * MockHttpClient가 성공적으로 피드백을 전송할 때
 * ServerAgent가 올바르게 처리하는지 검증합니다.
 */
TEST_F(ServerAgentMockedTest, SendFeedbackSuccess) {
    // Given: Mock HTTP 클라이언트가 성공적으로 POST 요청을 처리하도록 설정
    const std::string execution_id = "test-execution-123";
    const std::string status = "finished";
    const std::string message = "Update completed successfully";
    
    EXPECT_CALL(*mock_http_client_, post(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(true));
    
    // When: 피드백 전송 실행
    const bool result = server_agent_->sendFeedback(execution_id, status, message);
    
    // Then: 성공적으로 처리되어야 함
    EXPECT_TRUE(result) << "피드백 전송이 성공해야 합니다";
}

/**
 * @brief 피드백 전송 실패 테스트
 * 
 * MockHttpClient가 피드백 전송 실패를 반환할 때
 * ServerAgent가 올바르게 처리하는지 검증합니다.
 */
TEST_F(ServerAgentMockedTest, SendFeedbackFailure) {
    // Given: Mock HTTP 클라이언트가 POST 요청 실패를 반환하도록 설정
    const std::string execution_id = "test-execution-123";
    const std::string status = "error";
    const std::string message = "Update failed";
    
    EXPECT_CALL(*mock_http_client_, post(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(false));
    
    // When: 피드백 전송 실행
    const bool result = server_agent_->sendFeedback(execution_id, status, message);
    
    // Then: 실패가 올바르게 처리되어야 함
    EXPECT_FALSE(result) << "피드백 전송이 실패해야 합니다";
}

/**
 * @brief 진행률 피드백 전송 테스트
 * 
 * MockHttpClient를 사용하여 진행률 피드백 전송을 테스트합니다.
 */
TEST_F(ServerAgentMockedTest, SendProgressFeedback) {
    // Given: Mock HTTP 클라이언트가 성공적으로 POST 요청을 처리하도록 설정
    const std::string execution_id = "test-execution-456";
    const int progress = 50;
    const std::string message = "Half way done";
    
    EXPECT_CALL(*mock_http_client_, post(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(true));
    
    // When: 진행률 피드백 전송 실행
    const bool result = server_agent_->sendProgressFeedback(execution_id, progress, message);
    
    // Then: 성공적으로 처리되어야 함
    EXPECT_TRUE(result) << "진행률 피드백 전송이 성공해야 합니다";
}

/**
 * @brief JSON 파싱 테스트 (실제 구현 사용)
 * 
 * MockableServerAgent의 parseUpdateResponse는 실제 ServerAgent 구현을 사용하므로
 * JSON 파싱 로직을 테스트할 수 있습니다.
 */
TEST_F(ServerAgentMockedTest, ParseUpdateResponseWithDeploymentInfo) {
    // Given: 배포 정보가 포함된 JSON 응답
    const std::string deployment_json = createDeploymentJsonResponse();
    UpdateInfo update_info;
    
    // When: JSON 파싱 실행 (실제 구현 사용)
    const bool parse_result = server_agent_->parseUpdateResponse(deployment_json, update_info);
    
    // Then: 파싱이 성공하고 배포 정보가 올바르게 설정되어야 함
    EXPECT_TRUE(parse_result) << "배포 정보가 포함된 JSON은 성공적으로 파싱되어야 합니다";
    
    EXPECT_TRUE(update_info.is_available) << "배포 정보가 있는 경우 is_available은 true여야 합니다";
    EXPECT_EQ(update_info.execution_id, "deployment-123") << "실행 ID가 올바르게 설정되어야 합니다";
    EXPECT_EQ(update_info.version, "2.1.0") << "버전 정보가 올바르게 설정되어야 합니다";
    EXPECT_EQ(update_info.filename, "update-v2.1.0.tar.gz") << "파일명이 올바르게 설정되어야 합니다";
    EXPECT_EQ(update_info.expected_size, 1024000L) << "예상 파일 크기가 올바르게 설정되어야 합니다";
}

/**
 * @brief 완전한 업데이트 플로우 테스트
 * 
 * 폴링, 파싱, 다운로드, 피드백 전송의 전체 플로우를 모킹으로 테스트합니다.
 */
TEST_F(ServerAgentMockedTest, CompleteUpdateFlow) {
    // Given: Mock HTTP 클라이언트 설정
    const std::string deployment_json = createDeploymentJsonResponse();
    const std::string download_url = "https://example.com/update-v2.1.0.tar.gz";
    const std::string local_path = "/tmp/update.raucb";
    const std::string execution_id = "deployment-123";
    
    // 폴링 성공 설정
    EXPECT_CALL(*mock_http_client_, get(testing::_, testing::_))
        .WillOnce(testing::DoAll(
            testing::SetArgReferee<1>(deployment_json),
            testing::Return(true)
        ));
    
    // 다운로드 성공 설정
    EXPECT_CALL(*mock_http_client_, downloadFile(download_url, local_path))
        .WillOnce(testing::Return(true));
    
    // 피드백 전송 성공 설정 (시작, 진행률, 완료)
    EXPECT_CALL(*mock_http_client_, post(testing::_, testing::_, testing::_))
        .Times(3)  // started, progress, finished
        .WillRepeatedly(testing::Return(true));
    
    // When: 전체 업데이트 플로우 실행
    std::string response;
    bool poll_result = server_agent_->pollForUpdates(response);
    
    UpdateInfo update_info;
    bool parse_result = server_agent_->parseUpdateResponse(response, update_info);
    
    bool download_result = false;
    bool feedback_result = false;
    
    if (parse_result && update_info.is_available) {
        download_result = server_agent_->downloadBundle(update_info.download_url, local_path);
        
        if (download_result) {
            server_agent_->sendStartedFeedback(update_info.execution_id);
            server_agent_->sendProgressFeedback(update_info.execution_id, 50, "Half way done");
            feedback_result = server_agent_->sendFinishedFeedback(update_info.execution_id, true, "Update completed");
        }
    }
    
    // Then: 모든 단계가 성공해야 함
    EXPECT_TRUE(poll_result) << "폴링이 성공해야 합니다";
    EXPECT_TRUE(parse_result) << "파싱이 성공해야 합니다";
    EXPECT_TRUE(update_info.is_available) << "업데이트가 사용 가능해야 합니다";
    EXPECT_TRUE(download_result) << "다운로드가 성공해야 합니다";
    EXPECT_TRUE(feedback_result) << "피드백 전송이 성공해야 합니다";
}

} // namespace