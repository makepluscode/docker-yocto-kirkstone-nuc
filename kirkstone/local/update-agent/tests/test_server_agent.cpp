/**
 * @file test_server_agent.cpp
 * @brief UpdateClient 클래스 테스트
 *
 * Hawkbit 서버와의 통신을 담당하는 UpdateClient 클래스의
 * 주요 기능들을 테스트합니다.
 *
 * 테스트 범위:
 * - 생성자 및 소멸자 동작
 * - URL 빌더 함수들
 * - JSON 파싱 기능
 * - HTTP 요청 시뮬레이션
 * - 에러 처리 로직
 *
 * MISRA 2023 C++ 규칙 준수:
 * - Rule 15.1.1: 예외 처리 명시적 관리
 * - Rule 8.4.1: 사용 전 선언
 * - Rule 12.1.1: 명시적 초기화
 */

#include <gtest/gtest.h>
#include <string>
#include <memory>
#include <vector>
#include <json-c/json.h>
#include "update_client.h"
#include "config.h"

namespace {

/**
 * @class UpdateClientTest
 * @brief UpdateClient 테스트 클래스
 *
 * Google Test의 Test Fixture를 상속받아
 * UpdateClient 관련 테스트들을 그룹화합니다.
 */
class UpdateClientTest : public ::testing::Test {
protected:
    /**
     * @brief 각 테스트 시작 전 초기화
     */
    void SetUp() override {
        // 테스트용 서버 정보 설정
        test_server_url_ = "https://hawkbit.example.com";
        test_tenant_ = "default";
        test_device_id_ = "test-device-001";

        // UpdateClient 객체 생성
        server_agent_ = std::make_unique<UpdateClient>(
            test_server_url_, test_tenant_, test_device_id_);
    }

    /**
     * @brief 각 테스트 종료 후 정리
     */
    void TearDown() override {
        // unique_ptr이 자동으로 정리해줌
        server_agent_.reset();
    }

    // 테스트용 멤버 변수들
    std::string test_server_url_;
    std::string test_tenant_;
    std::string test_device_id_;
    std::unique_ptr<UpdateClient> server_agent_;

    /**
     * @brief 테스트용 JSON 문자열 생성
     * @return 유효한 Hawkbit 응답 JSON 문자열
     */
    std::string createValidJsonResponse() const {
        return R"({
            "config": {
                "polling": {
                    "sleep": "00:01:00"
                }
            },
            "_links": {
                "deploymentBase": {
                    "href": "https://hawkbit.example.com/default/controller/v1/test-device-001/deploymentBase/123"
                }
            }
        })";
    }

    /**
     * @brief 배포 정보가 포함된 테스트용 JSON 문자열 생성
     * @return 배포 정보가 있는 Hawkbit 응답 JSON 문자열
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
 * @brief 생성자 테스트
 *
 * UpdateClient의 생성자가 올바른 매개변수로
 * 객체를 초기화하는지 검증합니다.
 */
TEST_F(UpdateClientTest, ConstructorInitializesCorrectly) {
    // Given & When: SetUp에서 이미 생성됨

    // Then: 객체가 성공적으로 생성되었는지 확인
    ASSERT_NE(server_agent_.get(), nullptr)
        << "UpdateClient 객체가 성공적으로 생성되어야 합니다";

    // 추가 검증: 내부 상태가 올바르게 초기화되었는지 간접 확인
    // (private 멤버에 직접 접근할 수 없으므로 동작을 통해 확인)

    // 빈 응답으로 업데이트 정보 파싱 시도
    UpdateInfo test_update;
    const bool parse_result = server_agent_->parseUpdateResponse("{}", test_update);

    // 빈 JSON이므로 파싱은 실패해야 하지만, 예외가 발생하지 않아야 함
    EXPECT_FALSE(parse_result)
        << "빈 JSON 파싱은 실패해야 하지만 예외는 발생하지 않아야 합니다";
}

/**
 * @brief 소멸자 테스트
 *
 * UpdateClient가 안전하게 소멸되는지 확인합니다.
 * 메모리 누수나 이중 해제가 없어야 합니다.
 */
TEST_F(UpdateClientTest, DestructorCleansUpSafely) {
            // Given: 추가 UpdateClient 객체 생성
            auto additional_agent = std::make_unique<UpdateClient>(
        "https://test.com", "tenant", "device");

    // When: 명시적 소멸
    additional_agent.reset();

    // Then: 예외나 크래시 없이 완료되어야 함
    // (이 테스트는 주로 메모리 검사 도구로 확인)
            SUCCEED() << "UpdateClient가 안전하게 소멸되었습니다";
}

/**
 * @brief UpdateInfo 구조체 초기화 테스트
 *
 * UpdateInfo 구조체가 올바르게 초기화되는지 검증합니다.
 */
TEST_F(UpdateClientTest, UpdateInfoStructureInitialization) {
    // Given: UpdateInfo 객체 생성
    UpdateInfo info;

    // Then: 모든 필드가 기본값으로 초기화되어야 함
    EXPECT_TRUE(info.execution_id.empty())
        << "execution_id는 빈 문자열로 초기화되어야 합니다";

    EXPECT_TRUE(info.download_url.empty())
        << "download_url은 빈 문자열로 초기화되어야 합니다";

    EXPECT_TRUE(info.version.empty())
        << "version은 빈 문자열로 초기화되어야 합니다";

    EXPECT_TRUE(info.description.empty())
        << "description은 빈 문자열로 초기화되어야 합니다";

    EXPECT_TRUE(info.filename.empty())
        << "filename은 빈 문자열로 초기화되어야 합니다";

    EXPECT_EQ(info.expected_size, 0)
        << "expected_size는 0으로 초기화되어야 합니다";

    EXPECT_TRUE(info.md5_hash.empty())
        << "md5_hash는 빈 문자열로 초기화되어야 합니다";

    EXPECT_TRUE(info.sha1_hash.empty())
        << "sha1_hash는 빈 문자열로 초기화되어야 합니다";

    EXPECT_TRUE(info.sha256_hash.empty())
        << "sha256_hash는 빈 문자열로 초기화되어야 합니다";

    EXPECT_FALSE(info.is_available)
        << "is_available은 false로 초기화되어야 합니다";
}

/**
 * @brief JSON 파싱 테스트 - 유효한 응답
 *
 * 올바른 JSON 형식의 Hawkbit 응답을
 * 정확하게 파싱하는지 검증합니다.
 */
TEST_F(UpdateClientTest, ParseValidJsonResponse) {
    // Given: 유효한 JSON 응답
    const std::string valid_json = createValidJsonResponse();
    UpdateInfo update_info;

    // When: JSON 파싱 시도
    const bool parse_result = server_agent_->parseUpdateResponse(valid_json, update_info);

    // Then: 파싱이 성공해야 함
    EXPECT_TRUE(parse_result)
        << "유효한 JSON 응답은 성공적으로 파싱되어야 합니다";

    // 기본적으로는 배포 정보가 없으므로 is_available은 false여야 함
    EXPECT_FALSE(update_info.is_available)
        << "배포 정보가 없는 경우 is_available은 false여야 합니다";
}

/**
 * @brief JSON 파싱 테스트 - 배포 정보 포함
 *
 * 배포 정보가 포함된 JSON 응답을
 * 올바르게 파싱하고 UpdateInfo에 설정하는지 검증합니다.
 */
TEST_F(UpdateClientTest, ParseJsonWithDeploymentInfo) {
    // Given: 배포 정보가 포함된 JSON 응답
    const std::string deployment_json = createDeploymentJsonResponse();
    UpdateInfo update_info;

    // When: JSON 파싱 시도
    const bool parse_result = server_agent_->parseUpdateResponse(deployment_json, update_info);

    // Then: 파싱이 성공하고 배포 정보가 올바르게 설정되어야 함
    EXPECT_TRUE(parse_result)
        << "배포 정보가 포함된 JSON은 성공적으로 파싱되어야 합니다";

    EXPECT_TRUE(update_info.is_available)
        << "배포 정보가 있는 경우 is_available은 true여야 합니다";

    EXPECT_EQ(update_info.execution_id, "deployment-123")
        << "실행 ID가 올바르게 설정되어야 합니다";

    EXPECT_EQ(update_info.version, "2.1.0")
        << "버전 정보가 올바르게 설정되어야 합니다";

    EXPECT_EQ(update_info.filename, "update-v2.1.0.tar.gz")
        << "파일명이 올바르게 설정되어야 합니다";

    EXPECT_EQ(update_info.expected_size, 1024000L)
        << "예상 파일 크기가 올바르게 설정되어야 합니다";

    // 해시값들 검증
    EXPECT_EQ(update_info.md5_hash, "d41d8cd98f00b204e9800998ecf8427e")
        << "MD5 해시가 올바르게 설정되어야 합니다";

    EXPECT_EQ(update_info.sha1_hash, "da39a3ee5e6b4b0d3255bfef95601890afd80709")
        << "SHA1 해시가 올바르게 설정되어야 합니다";

    EXPECT_EQ(update_info.sha256_hash, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")
        << "SHA256 해시가 올바르게 설정되어야 합니다";

    // 다운로드 URL 검증
    EXPECT_FALSE(update_info.download_url.empty())
        << "다운로드 URL이 설정되어야 합니다";
}

/**
 * @brief JSON 파싱 테스트 - 잘못된 형식
 *
 * 잘못된 형식의 JSON에 대해
 * 적절히 에러를 처리하는지 검증합니다.
 */
TEST_F(UpdateClientTest, ParseInvalidJsonHandlesError) {
    // Given: 잘못된 JSON 문자열들
    const std::vector<std::string> invalid_jsons = {
        "",                    // 빈 문자열
        "{",                   // 불완전한 JSON
        "not json at all",     // JSON이 아닌 문자열
        "{\"invalid\": }",     // 문법 오류
        "null",               // null 값
        "[]"                  // 배열 (객체가 아님)
    };

    for (const auto& invalid_json : invalid_jsons) {
        // When: 잘못된 JSON 파싱 시도
        UpdateInfo update_info;
        const bool parse_result = server_agent_->parseUpdateResponse(invalid_json, update_info);

        // Then: 파싱이 실패해야 하고 예외가 발생하지 않아야 함
        EXPECT_FALSE(parse_result)
            << "잘못된 JSON '" << invalid_json << "'의 파싱은 실패해야 합니다";

        // 실패 시 UpdateInfo가 초기 상태를 유지해야 함
        EXPECT_FALSE(update_info.is_available)
            << "파싱 실패 시 is_available은 false를 유지해야 합니다";
    }
}

/**
 * @brief 피드백 전송 테스트
 *
 * 피드백 메시지 생성 로직을 테스트합니다.
 * (실제 네트워크 전송은 하지 않고 로직만 검증)
 */
TEST_F(UpdateClientTest, FeedbackMessageConstruction) {
    // Given: 테스트 데이터
    const std::string execution_id = "test-execution-123";
    const std::string status = "proceeding";
    const std::string message = "업데이트 진행 중";

    // When & Then: 피드백 전송 메서드 호출
    // (실제 네트워크 전송 없이 내부 로직만 테스트)
    // 이 테스트에서는 메서드가 예외 없이 실행되는지만 확인
    EXPECT_NO_THROW({
        // 실제 전송은 실패할 수 있지만 예외는 발생하지 않아야 함
        server_agent_->sendFeedback(execution_id, status, message);
    }) << "피드백 전송 메서드는 예외를 발생시키지 않아야 합니다";
}

/**
 * @brief 진행률 피드백 테스트
 *
 * 진행률 피드백 메서드의 매개변수 검증을 테스트합니다.
 */
TEST_F(UpdateClientTest, ProgressFeedbackParameterValidation) {
    // Given: 테스트 데이터
    const std::string execution_id = "progress-test-456";

    // When & Then: 유효한 진행률 범위 테스트
    const std::vector<int> valid_progress_values = {0, 25, 50, 75, 100};

    for (int progress : valid_progress_values) {
        EXPECT_NO_THROW({
            server_agent_->sendProgressFeedback(execution_id, progress);
        }) << "유효한 진행률 " << progress << "%에 대해 예외가 발생하지 않아야 합니다";
    }

    // 경계값 및 잘못된 값들도 테스트 (방어적 프로그래밍)
    const std::vector<int> boundary_values = {-1, 101, -100, 200};

    for (int progress : boundary_values) {
        EXPECT_NO_THROW({
            server_agent_->sendProgressFeedback(execution_id, progress);
        }) << "경계값 " << progress << "%에 대해서도 예외가 발생하지 않아야 합니다 (내부에서 처리)";
    }
}

/**
 * @brief 빈 문자열 매개변수 테스트
 *
 * 빈 문자열이나 null 값에 대한 방어적 처리를 검증합니다.
 */
TEST_F(UpdateClientTest, EmptyParameterHandling) {
    // Given: 빈 문자열들
    const std::string empty_string = "";
    UpdateInfo update_info;

    // When & Then: 빈 문자열 파싱 테스트
    const bool parse_result = server_agent_->parseUpdateResponse(empty_string, update_info);

    EXPECT_FALSE(parse_result)
        << "빈 문자열 파싱은 실패해야 합니다";

    // When & Then: 빈 execution_id로 피드백 전송 테스트
    EXPECT_NO_THROW({
        server_agent_->sendFeedback(empty_string, "closed", "완료");
    }) << "빈 execution_id에 대해서도 예외가 발생하지 않아야 합니다";
}

} // namespace
