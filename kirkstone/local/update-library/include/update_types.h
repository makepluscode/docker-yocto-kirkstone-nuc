#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>

namespace UpdateLibrary {

/**
 * @brief 설치 진행률 정보 구조체
 */
struct ProgressInfo {
    int percentage;       // 진행률 백분율 (0-100)
    std::string message;  // 진행 메시지
    int nesting_depth;    // 중첩 깊이

    ProgressInfo() : percentage(0), nesting_depth(0) {}
    ProgressInfo(int p, const std::string& m, int d)
        : percentage(p), message(m), nesting_depth(d) {}
};

/**
 * @brief 슬롯 상태 정보 구조체
 */
struct SlotInfo {
    std::string slot_name;                    // 슬롯 이름 (예: "rootfs.0")
    std::map<std::string, std::string> properties;  // 슬롯 속성들

    SlotInfo() = default;
    SlotInfo(const std::string& name) : slot_name(name) {}
};

/**
 * @brief 설치 결과 열거형
 */
enum class InstallResult {
    SUCCESS = 0,      // 성공
    FAILURE = 1,      // 실패
    CANCELLED = 2     // 취소됨
};

/**
 * @brief 설치 완료 콜백 함수 타입
 * @param result 설치 결과
 * @param message 결과 메시지
 */
using CompletedCallback = std::function<void(InstallResult result, const std::string& message)>;

/**
 * @brief 진행률 업데이트 콜백 함수 타입
 * @param progress 진행률 정보
 */
using ProgressCallback = std::function<void(const ProgressInfo& progress)>;

/**
 * @brief 오류 콜백 함수 타입
 * @param error_message 오류 메시지
 */
using ErrorCallback = std::function<void(const std::string& error_message)>;

} // namespace UpdateLibrary
