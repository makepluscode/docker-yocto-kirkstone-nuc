#pragma once

#include "update_types.h"
#include <memory>

class RaucEngine;

/**
 * @brief Update 클라이언트 메인 인터페이스
 *
 * 이 클래스는 update-service에서 사용할 수 있는 간단한 인터페이스를 제공합니다.
 * RAUC 기능을 직접 포팅하여 D-Bus 없이 동작합니다.
 */
class UpdateClient {
public:
    UpdateClient();
    ~UpdateClient();

    /**
     * @brief 클라이언트 초기화
     * @param config_file_path RAUC 설정 파일 경로 (선택사항)
     * @return 성공 시 true, 실패 시 false
     */
    bool initialize(const std::string& config_file_path = "");

    /**
     * @brief 번들 설치 시작
     * @param bundle_path 설치할 번들 파일 경로
     * @return 성공 시 true, 실패 시 false
     */
    bool install(const std::string& bundle_path);

    /**
     * @brief 모든 슬롯의 상태 정보 가져오기
     * @return 슬롯 정보 벡터
     */
    std::vector<SlotInfo> getSlotStatus();

    /**
     * @brief 현재 부트 슬롯 정보 가져오기
     * @return 부트 슬롯 이름
     */
    std::string getBootSlot();

    /**
     * @brief 시스템 호환성 문자열 가져오기
     * @return 호환성 문자열
     */
    std::string getCompatible();

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
     * @brief 설치 완료 콜백 설정
     * @param callback 완료 시 호출될 콜백 함수
     */
    void setCompletedCallback(CompletedCallback callback);

    /**
     * @brief 진행률 콜백 설정
     * @param callback 진행률 업데이트 시 호출될 콜백 함수
     */
    void setProgressCallback(ProgressCallback callback);

    /**
     * @brief 오류 콜백 설정
     * @param callback 오류 발생 시 호출될 콜백 함수
     */
    void setErrorCallback(ErrorCallback callback);

    /**
     * @brief 초기화 상태 확인
     * @return 초기화됨 시 true
     */
    bool isInitialized() const;

    /**
     * @brief 설치가 진행 중인지 확인
     * @return 설치 중이면 true
     */
    bool isInstalling() const;

private:
    std::unique_ptr<RaucEngine> rauc_engine_;
    CompletedCallback completed_callback_;
    ProgressCallback progress_callback_;
    ErrorCallback error_callback_;

    bool initialized_;
    bool installing_;
};
