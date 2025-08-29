/**
 * @file mockable_server_agent.h
 * @brief 모킹 가능한 ServerAgent 래퍼
 * 
 * ServerAgent의 HTTP 통신을 모킹할 수 있도록
 * 의존성 주입을 지원하는 래퍼 클래스입니다.
 */

#ifndef MOCKABLE_SERVER_AGENT_H
#define MOCKABLE_SERVER_AGENT_H

#include <memory>
#include <string>
#include "server_agent.h"
#include "mock_http_client.h"

/**
 * @class MockableServerAgent
 * @brief 모킹 가능한 ServerAgent 래퍼 클래스
 * 
 * ServerAgent의 HTTP 통신을 모킹할 수 있도록
 * 의존성 주입을 지원합니다.
 */
class MockableServerAgent {
public:
    /**
     * @brief 생성자
     * @param server_url 서버 URL
     * @param tenant 테넌트 이름
     * @param device_id 디바이스 ID
     * @param http_client HTTP 클라이언트 (선택적, nullptr이면 실제 클라이언트 사용)
     */
    MockableServerAgent(const std::string& server_url, 
                       const std::string& tenant, 
                       const std::string& device_id,
                       MockHttpClient* http_client = nullptr);
    
    /**
     * @brief 소멸자
     */
    ~MockableServerAgent() = default;
    
    /**
     * @brief 업데이트 폴링 (모킹 가능)
     * @param response 응답 데이터를 받을 문자열 참조
     * @return 폴링 성공 여부
     */
    bool pollForUpdates(std::string& response);
    
    /**
     * @brief 번들 다운로드 (모킹 가능)
     * @param download_url 다운로드 URL
     * @param local_path 로컬 저장 경로
     * @return 다운로드 성공 여부
     */
    bool downloadBundle(const std::string& download_url, const std::string& local_path);
    
    /**
     * @brief 피드백 전송 (모킹 가능)
     * @param execution_id 실행 ID
     * @param status 상태
     * @param message 메시지
     * @return 전송 성공 여부
     */
    bool sendFeedback(const std::string& execution_id, const std::string& status, const std::string& message = "");
    
    /**
     * @brief 진행률 피드백 전송 (모킹 가능)
     * @param execution_id 실행 ID
     * @param progress 진행률
     * @param message 메시지
     * @return 전송 성공 여부
     */
    bool sendProgressFeedback(const std::string& execution_id, int progress, const std::string& message = "");
    
    /**
     * @brief 시작 피드백 전송 (모킹 가능)
     * @param execution_id 실행 ID
     * @return 전송 성공 여부
     */
    bool sendStartedFeedback(const std::string& execution_id);
    
    /**
     * @brief 완료 피드백 전송 (모킹 가능)
     * @param execution_id 실행 ID
     * @param success 성공 여부
     * @param message 메시지
     * @return 전송 성공 여부
     */
    bool sendFinishedFeedback(const std::string& execution_id, bool success, const std::string& message = "");
    
    /**
     * @brief 업데이트 응답 파싱 (실제 구현 사용)
     * @param response 응답 문자열
     * @param update_info 업데이트 정보를 받을 구조체 참조
     * @return 파싱 성공 여부
     */
    bool parseUpdateResponse(const std::string& response, UpdateInfo& update_info);

private:
    std::unique_ptr<ServerAgent> real_agent_;
    MockHttpClient* mock_http_client_;
    std::string server_url_;
    std::string tenant_;
    std::string device_id_;
};

#endif // MOCKABLE_SERVER_AGENT_H