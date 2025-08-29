/**
 * @file mockable_service_agent.cpp
 * @brief 모킹 가능한 ServiceAgent 래퍼 구현
 */

#include "mockable_service_agent.h"

MockableServiceAgent::MockableServiceAgent(MockDbusClient* dbus_client)
    : mock_dbus_client_(dbus_client) {

    // 실제 ServiceAgent 생성
    real_agent_ = std::make_unique<ServiceAgent>();
}

bool MockableServiceAgent::connect() {
    if (mock_dbus_client_) {
        return mock_dbus_client_->connect();
    } else {
        return real_agent_->connect();
    }
}

void MockableServiceAgent::disconnect() {
    if (mock_dbus_client_) {
        mock_dbus_client_->disconnect();
    } else {
        real_agent_->disconnect();
    }
}

bool MockableServiceAgent::isConnected() const {
    if (mock_dbus_client_) {
        return mock_dbus_client_->isConnected();
    } else {
        return real_agent_->isConnected();
    }
}

bool MockableServiceAgent::checkService() {
    if (mock_dbus_client_) {
        return mock_dbus_client_->checkService();
    } else {
        return real_agent_->checkService();
    }
}

bool MockableServiceAgent::installBundle(const std::string& bundle_path) {
    if (mock_dbus_client_) {
        return mock_dbus_client_->installBundle(bundle_path);
    } else {
        return real_agent_->installBundle(bundle_path);
    }
}

bool MockableServiceAgent::getStatus(std::string& status) {
    if (mock_dbus_client_) {
        return mock_dbus_client_->getStatus(status);
    } else {
        return real_agent_->getStatus(status);
    }
}

bool MockableServiceAgent::getBootSlot(std::string& boot_slot) {
    if (mock_dbus_client_) {
        return mock_dbus_client_->getBootSlot(boot_slot);
    } else {
        return real_agent_->getBootSlot(boot_slot);
    }
}

bool MockableServiceAgent::markGood() {
    if (mock_dbus_client_) {
        return mock_dbus_client_->markGood();
    } else {
        return real_agent_->markGood();
    }
}

bool MockableServiceAgent::markBad() {
    if (mock_dbus_client_) {
        return mock_dbus_client_->markBad();
    } else {
        return real_agent_->markBad();
    }
}

bool MockableServiceAgent::getBundleInfo(const std::string& bundle_path, std::string& info) {
    if (mock_dbus_client_) {
        return mock_dbus_client_->getBundleInfo(bundle_path, info);
    } else {
        return real_agent_->getBundleInfo(bundle_path, info);
    }
}

void MockableServiceAgent::setProgressCallback(std::function<void(int)> callback) {
    if (mock_dbus_client_) {
        mock_dbus_client_->setProgressCallback(callback);
    } else {
        real_agent_->setProgressCallback(callback);
    }
}

void MockableServiceAgent::setCompletedCallback(std::function<void(bool, const std::string&)> callback) {
    if (mock_dbus_client_) {
        mock_dbus_client_->setCompletedCallback(callback);
    } else {
        real_agent_->setCompletedCallback(callback);
    }
}

void MockableServiceAgent::processMessages() {
    if (mock_dbus_client_) {
        mock_dbus_client_->processMessages();
    } else {
        real_agent_->processMessages();
    }
}
