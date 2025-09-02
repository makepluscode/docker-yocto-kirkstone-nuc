/**
 * @file mockable_server_agent.cpp
 * @brief 모킹 가능한 ServerAgent 래퍼 구현
 */

#include "mockable_server_agent.h"

MockableServerAgent::MockableServerAgent(const std::string& server_url,
                                       const std::string& tenant,
                                       const std::string& device_id,
                                       MockHttpClient* http_client)
    : mock_http_client_(http_client)
    , server_url_(server_url)
    , tenant_(tenant)
    , device_id_(device_id) {

    // 실제 ServerAgent 생성 (파싱 기능은 실제 구현 사용)
    real_agent_ = std::make_unique<ServerAgent>(server_url, tenant, device_id);
}

bool MockableServerAgent::pollForUpdates(std::string& response) {
    if (mock_http_client_) {
        // 모킹된 HTTP 클라이언트 사용
        std::string poll_url = server_url_ + "/" + tenant_ + "/controller/v1/" + device_id_;
        return mock_http_client_->get(poll_url, response);
    } else {
        // 실제 ServerAgent 사용
        return real_agent_->pollForUpdates(response);
    }
}

bool MockableServerAgent::downloadBundle(const std::string& download_url, const std::string& local_path) {
    if (mock_http_client_) {
        // 모킹된 HTTP 클라이언트 사용
        return mock_http_client_->downloadFile(download_url, local_path);
    } else {
        // 실제 ServerAgent 사용
        return real_agent_->downloadBundle(download_url, local_path);
    }
}

bool MockableServerAgent::sendFeedback(const std::string& execution_id, const std::string& status, const std::string& message) {
    if (mock_http_client_) {
        // 모킹된 HTTP 클라이언트 사용
        std::string feedback_url = server_url_ + "/" + tenant_ + "/controller/v1/" + device_id_ + "/deploymentBase/" + execution_id + "/feedback";
        std::string feedback_data = R"({"status":")" + status + R"(","message":")" + message + R"("})";
        std::string response;
        return mock_http_client_->post(feedback_url, feedback_data, response);
    } else {
        // 실제 ServerAgent 사용
        return real_agent_->sendFeedback(execution_id, status, message);
    }
}

bool MockableServerAgent::sendProgressFeedback(const std::string& execution_id, int progress, const std::string& message) {
    if (mock_http_client_) {
        // 모킹된 HTTP 클라이언트 사용
        std::string feedback_url = server_url_ + "/" + tenant_ + "/controller/v1/" + device_id_ + "/deploymentBase/" + execution_id + "/feedback";
        std::string feedback_data = R"({"status":"proceeding","progress":)" + std::to_string(progress) + R"(,"message":")" + message + R"("})";
        std::string response;
        return mock_http_client_->post(feedback_url, feedback_data, response);
    } else {
        // 실제 ServerAgent 사용
        return real_agent_->sendProgressFeedback(execution_id, progress, message);
    }
}

bool MockableServerAgent::sendStartedFeedback(const std::string& execution_id) {
    return sendFeedback(execution_id, "started", "Update started");
}

bool MockableServerAgent::sendFinishedFeedback(const std::string& execution_id, bool success, const std::string& message) {
    std::string status = success ? "finished" : "error";
    return sendFeedback(execution_id, status, message);
}

bool MockableServerAgent::parseUpdateResponse(const std::string& response, UpdateInfo& update_info) {
    // 파싱 기능은 실제 ServerAgent 구현 사용
    return real_agent_->parseUpdateResponse(response, update_info);
}
