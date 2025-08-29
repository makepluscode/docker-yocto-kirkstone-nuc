#pragma once

#include <glib.h>
#include "bundle.h"
#include "manifest.h"
#include "slot.h"

// 설치 관련 오류 도메인
#define R_INSTALL_ERROR r_install_error_quark()
GQuark r_install_error_quark(void);

/**
 * @brief 설치 오류 타입
 */
typedef enum {
    R_INSTALL_ERROR_FAILED,
    R_INSTALL_ERROR_COMPAT_MISMATCH,
    R_INSTALL_ERROR_VERSION_MISMATCH,
    R_INSTALL_ERROR_REJECTED,
    R_INSTALL_ERROR_MARK_BOOTABLE,
    R_INSTALL_ERROR_MARK_NONBOOTABLE,
    R_INSTALL_ERROR_TARGET_GROUP,
    R_INSTALL_ERROR_MOUNTED,
} RInstallError;

/**
 * @brief 설치 결과 열거형
 */
typedef enum {
    R_INSTALL_RESULT_SUCCESS = 0,
    R_INSTALL_RESULT_FAILURE = 1,
    R_INSTALL_RESULT_CANCELLED = 2
} RInstallResult;

/**
 * @brief 설치 진행률 콜백 함수 타입
 * @param percentage 진행률 (0-100)
 * @param message 진행률 메시지
 * @param nesting_depth 중첩 깊이
 * @param user_data 사용자 데이터
 */
typedef void (*RaucProgressCallback)(gint percentage,
                                    const gchar *message,
                                    gint nesting_depth,
                                    gpointer user_data);

/**
 * @brief 설치 완료 콜백 함수 타입
 * @param result 설치 결과
 * @param message 결과 메시지
 * @param user_data 사용자 데이터
 */
typedef void (*RaucCompletionCallback)(RInstallResult result,
                                      const gchar *message,
                                      gpointer user_data);

/**
 * @brief 설치 인자 구조체
 */
typedef struct {
    gchar *name;                        // 설치 작업 이름
    gchar *bundle_path;                 // 번들 파일 경로

    // 콜백 함수들
    RaucProgressCallback progress_callback;
    RaucCompletionCallback completed_callback;
    gpointer user_data;

    // 설치 옵션들
    gboolean ignore_compatible;         // 호환성 검사 무시
    gboolean ignore_version_limit;      // 버전 제한 무시
    gchar *transaction;                 // 트랜잭션 ID

    // 상태 관리
    GMutex status_mutex;
    GQueue status_messages;
    gint status_result;
    gboolean completed;

    // 번들 액세스 인자들 (간소화)
    RaucBundleAccessArgs access_args;

} RaucInstallArgs;

/**
 * @brief 외부 마운트 포인트 업데이트
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean update_external_mount_points(GError **error);

/**
 * @brief 슬롯 상태 결정 (ACTIVE | INACTIVE | BOOTED)
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean determine_slot_states(GError **error);

/**
 * @brief 부트 상태 정보 획득
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean determine_boot_states(GError **error);

/**
 * @brief 설치 대상 그룹 결정
 * @return 슬롯클래스를 슬롯 인스턴스로 매핑하는 해시테이블
 */
GHashTable *determine_target_install_group(void);

/**
 * @brief 기본 번들 설치 절차
 * @param args RaucInstallArgs 인스턴스
 * @param error 오류 정보 반환 위치
 * @return 설치 성공 시 TRUE, 치명적 오류 발생 시 FALSE
 */
gboolean do_install_bundle(RaucInstallArgs *args, GError **error);

/**
 * @brief 새 RaucInstallArgs 구조체 초기화
 * @return 새로 할당된 RaucInstallArgs (install_args_free로 해제)
 */
RaucInstallArgs *install_args_new(void);

/**
 * @brief RaucInstallArgs 구조체 해제
 * @param args 해제할 인스턴스
 */
void install_args_free(RaucInstallArgs *args);

/**
 * @brief 새 설치 스레드 시작 (단순화된 버전)
 * @param bundle_path 번들 파일 경로
 * @param progress_callback 진행률 콜백
 * @param completed_callback 완료 콜백
 * @param user_data 사용자 데이터
 * @param error 오류 정보 반환 위치
 * @return 시작 성공 시 TRUE, 아니면 FALSE
 */
gboolean install_run_simple(const gchar *bundle_path,
                            RaucProgressCallback progress_callback,
                            RaucCompletionCallback completed_callback,
                            gpointer user_data,
                            GError **error);

/**
 * @brief 이미지 설치 계획 구조체
 */
typedef struct {
    RaucImage *image;           // 설치할 이미지
    RaucSlot *target_slot;      // 대상 슬롯
    gpointer slot_handler;      // 슬롯 핸들러 (img_to_slot_handler)
    gpointer target_repo;       // 대상 리포지토리 (RArtifactRepo*)
} RImageInstallPlan;

/**
 * @brief 이미지 설치 계획 해제
 * @param value RImageInstallPlan 포인터
 */
void r_image_install_plan_free(gpointer value);

/**
 * @brief 설치 계획 생성
 * @param manifest 매니페스트
 * @param target_group 대상 그룹
 * @param error 오류 정보 반환 위치
 * @return RImageInstallPlan 배열, 실패 시 NULL
 */
GPtrArray *r_install_make_plans(const RaucManifest *manifest,
                               GHashTable *target_group,
                               GError **error);

/**
 * @brief HTTP 헤더 지원 여부 확인
 * @param header 헤더 설정 이름
 * @return 지원하면 TRUE, 아니면 FALSE
 */
gboolean r_install_is_supported_http_header(const gchar *header);

/**
 * @brief 간단한 번들 설치 (동기 버전)
 * @param bundle_path 번들 파일 경로
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_install_bundle_simple(const gchar *bundle_path, GError **error);

/**
 * @brief 번들 설치 (콜백 포함)
 * @param bundle_path 번들 파일 경로
 * @param progress_callback 진행률 콜백 (NULL 가능)
 * @param completed_callback 완료 콜백 (NULL 가능)
 * @param user_data 콜백용 사용자 데이터
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_install_bundle_with_callbacks(const gchar *bundle_path,
                                         RaucProgressCallback progress_callback,
                                         RaucCompletionCallback completed_callback,
                                         gpointer user_data,
                                         GError **error);

/**
 * @brief 설치 진행률 업데이트 (내부용)
 * @param args 설치 인자
 * @param percentage 진행률
 * @param message 메시지
 * @param nesting_depth 중첩 깊이
 */
void r_install_update_progress(RaucInstallArgs *args,
                              gint percentage,
                              const gchar *message,
                              gint nesting_depth);

/**
 * @brief 설치 완료 알림 (내부용)
 * @param args 설치 인자
 * @param success 성공 여부
 * @param error_message 오류 메시지
 */
void r_install_notify_completed(RaucInstallArgs *args,
                               gboolean success,
                               const gchar *error_message);

/**
 * @brief 현재 설치 상태 확인
 * @return 설치 중이면 TRUE, 아니면 FALSE
 */
gboolean r_install_is_active(void);

/**
 * @brief 현재 설치 진행률 가져오기
 * @param percentage 진행률 반환 (NULL 가능)
 * @param message 메시지 반환 (NULL 가능, 호출자가 g_free)
 * @param nesting_depth 중첩 깊이 반환 (NULL 가능)
 * @return 진행률 정보가 있으면 TRUE, 아니면 FALSE
 */
gboolean r_install_get_progress(gint *percentage,
                               gchar **message,
                               gint *nesting_depth);

/**
 * @brief 파일에서 번들 설치 (CLI 용)
 * @param bundle_path 번들 파일 경로
 * @param progress_callback 진행률 콜백
 * @param completed_callback 완료 콜백
 * @param user_data 사용자 데이터
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_install_bundle_from_file(const gchar *bundle_path,
                                   RaucProgressCallback progress_callback,
                                   RaucCompletionCallback completed_callback,
                                   gpointer user_data,
                                   GError **error);

/**
 * @brief 설치 상태 정보 문자열 가져오기
 * @return 상태 정보 문자열 (호출자가 g_free 해야 함)
 */
gchar* r_install_get_status_info(void);

// GLib 자동 정리 매크로
G_DEFINE_AUTOPTR_CLEANUP_FUNC(RaucInstallArgs, install_args_free);
G_DEFINE_AUTOPTR_CLEANUP_FUNC(RImageInstallPlan, r_image_install_plan_free);
