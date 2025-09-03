/**
 * @file mockable_package_installer.cpp
 * @brief 모킹 가능한 PackageInstaller 래퍼 구현
 */

#include "mockable_package_installer.h"

MockablePackageInstaller::MockablePackageInstaller(MockDbusClient* dbus_client)
    : mock_dbus_client_(dbus_client) {

    // 실제 PackageInstaller 생성
    real_agent_ = std::make_unique<PackageInstaller>();
}

bool MockablePackageInstaller::connect() {
    if (mock_dbus_client_) {
        return mock_dbus_client_->connect();
    } else {
        return real_agent_->connect();
    }
}

void MockablePackageInstaller::disconnect() {
    if (mock_dbus_client_) {
        mock_dbus_client_->disconnect();
    } else {
        real_agent_->disconnect();
    }
}

bool MockablePackageInstaller::isConnected() const {
    if (mock_dbus_client_) {
        return mock_dbus_client_->isConnected();
    } else {
        return real_agent_->isConnected();
    }
}

bool MockablePackageInstaller::checkService() {
    if (mock_dbus_client_) {
        return mock_dbus_client_->checkService();
    } else {
        return real_agent_->checkService();
    }
}

bool MockablePackageInstaller::installPackage(const std::string& package_path) {
    if (mock_dbus_client_) {
        return mock_dbus_client_->installPackage(package_path);
    } else {
        return real_agent_->installPackage(package_path);
    }
}

bool MockablePackageInstaller::getStatus(std::string& status) {
    if (mock_dbus_client_) {
        return mock_dbus_client_->getStatus(status);
    } else {
        return real_agent_->getStatus(status);
    }
}

bool MockablePackageInstaller::getBootSlot(std::string& boot_slot) {
    if (mock_dbus_client_) {
        return mock_dbus_client_->getBootSlot(boot_slot);
    } else {
        return real_agent_->getBootSlot(boot_slot);
    }
}

bool MockablePackageInstaller::markGood() {
    if (mock_dbus_client_) {
        return mock_dbus_client_->markGood();
    } else {
        return real_agent_->markGood();
    }
}

bool MockablePackageInstaller::markBad() {
    if (mock_dbus_client_) {
        return mock_dbus_client_->markBad();
    } else {
        return real_agent_->markBad();
    }
}

bool MockablePackageInstaller::getPackageInfo(const std::string& package_path, std::string& info) {
    if (mock_dbus_client_) {
        return mock_dbus_client_->getPackageInfo(package_path, info);
    } else {
        return real_agent_->getPackageInfo(package_path, info);
    }
}

void MockablePackageInstaller::setProgressCallback(std::function<void(int)> callback) {
    if (mock_dbus_client_) {
        mock_dbus_client_->setProgressCallback(callback);
    } else {
        real_agent_->setProgressCallback(callback);
    }
}

void MockablePackageInstaller::setCompletedCallback(std::function<void(bool, const std::string&)> callback) {
    if (mock_dbus_client_) {
        mock_dbus_client_->setCompletedCallback(callback);
    } else {
        real_agent_->setCompletedCallback(callback);
    }
}

void MockablePackageInstaller::processMessages() {
    if (mock_dbus_client_) {
        mock_dbus_client_->processMessages();
    } else {
        real_agent_->processMessages();
    }
}
