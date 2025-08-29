#pragma once

#include "update_types.h"
#include <string>
#include <memory>

// Forward declarations for RAUC structures
typedef struct _RaucBundle RaucBundle;
typedef struct _RaucSlot RaucSlot;
typedef struct _RaucInstallArgs RaucInstallArgs;
typedef void* GHashTable;
typedef void* GError;

namespace UpdateLibrary {

/**
 * @brief RAUC 엔진 - RAUC 기능을 직접 구현한 C++ 래퍼 클래스
 *
 * 이 클래스는 RAUC의 C 코드를 포팅하여 직접 번들 설치와 상태 조회 기능을 제공합니다.
 * D-Bus 없이 동작하며 update-service에서 직접 사용할 수 있습니다.
 */
class RaucEngine {
public:
    RaucEngine();
    ~RaucEngine();

    /**
     * @brief 엔진 초기화
     * @param config_file_path RAUC 설정 파일 경로 (기본: /etc/rauc/system.conf)
     * @return 성공 시 true
     */
    bool initialize(const std::string& config_file_path = "/etc/rauc/system.conf");

    /**
     * @brief 번들 설치 시작
     * @param bundle_path 설치할 번들 파일 경로
     * @param progress_callback 진행률 콜백
     * @param completed_callback 완료 콜백
     * @return 성공 시 true
     */
    bool installBundle(const std::string& bundle_path,
                      ProgressCallback progress_callback = nullptr,
                      CompletedCallback completed_callback = nullptr);

    /**
     * @brief 모든 슬롯 상태 정보 가져오기
     * @return 슬롯 정보 벡터
     */
    std::vector<SlotInfo> getSlotStatus();

    /**
     * @brief 현재 부트 슬롯 가져오기
     * @return 부트 슬롯 이름
     */
    std::string getBootSlot();

    /**
     * @brief 시스템 호환성 문자열 가져오기
     * @return 호환성 문자열
     */
    std::string getCompatible();

    /**
     * @brief 현재 진행률 정보 가져오기
     * @return 진행률 정보
     */
    ProgressInfo getCurrentProgress();

    /**
     * @brief 마지막 오류 메시지 가져오기
     * @return 오류 메시지
     */
    std::string getLastError();

    /**
     * @brief 현재 작업 상태 가져오기
     * @return 작업 상태 문자열
     */
    std::string getOperation();

    /**
     * @brief 번들 정보 확인
     * @param bundle_path 번들 파일 경로
     * @param compatible 호환성 문자열 반환
     * @param version 버전 문자열 반환
     * @return 성공 시 true
     */
    bool getBundleInfo(const std::string& bundle_path,
                      std::string& compatible,
                      std::string& version);

    /**
     * @brief 설치 진행 중인지 확인
     * @return 설치 중이면 true
     */
    bool isInstalling() const;

    /**
     * @brief 초기화 상태 확인
     * @return 초기화됨 시 true
     */
    bool isInitialized() const;

private:
    bool initialized_;
    bool installing_;
    std::string last_error_;
    std::string current_operation_;
    ProgressInfo current_progress_;

    // 콜백 저장
    ProgressCallback progress_callback_;
    CompletedCallback completed_callback_;

    // RAUC 내부 데이터
    GHashTable* system_slots_;  // 시스템 슬롯 테이블
    std::string config_file_path_;
    std::string system_compatible_;

    /**
     * @brief RAUC 설정 파일 로드
     * @return 성공 시 true
     */
    bool loadSystemConfig();

    /**
     * @brief 슬롯 상태 결정 (determine_slot_states 포팅)
     * @return 성공 시 true
     */
    bool determineSlotStates();

    /**
     * @brief 부트 상태 결정 (determine_boot_states 포팅)
     * @return 성공 시 true
     */
    bool determineBootStates();

    /**
     * @brief 번들 검증 (check_bundle 포팅)
     * @param bundle_path 번들 경로
     * @param bundle 번들 구조체 반환
     * @return 성공 시 true
     */
    bool checkBundle(const std::string& bundle_path, RaucBundle** bundle);

    /**
     * @brief 설치 실행 (do_install_bundle 포팅)
     * @param bundle_path 번들 경로
     * @return 성공 시 true
     */
    bool doInstallBundle(const std::string& bundle_path);

    /**
     * @brief 진행률 업데이트 콜백 (RAUC에서 호출)
     */
    static void onProgressUpdate(int percentage, const char* message, void* user_data);

    /**
     * @brief 설치 완료 콜백 (RAUC에서 호출)
     */
    static void onInstallCompleted(int result, const char* message, void* user_data);

    /**
     * @brief GHashTable을 SlotInfo 벡터로 변환
     */
    std::vector<SlotInfo> convertSlotsToVector(GHashTable* slots);

    /**
     * @brief 로그 함수들
     */
    void logInfo(const std::string& message);
    void logError(const std::string& message);
    void logDebug(const std::string& message);
};
