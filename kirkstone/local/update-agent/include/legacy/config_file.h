#pragma once

#include <glib.h>
#include "slot.h"

// 설정 파일 관련 오류 도메인
#define R_CONFIG_ERROR r_config_error_quark()
GQuark r_config_error_quark(void);

/**
 * @brief 설정 파일 오류 타입
 */
typedef enum {
    R_CONFIG_ERROR_INVALID_FORMAT,
    R_CONFIG_ERROR_MISSING_SECTION,
    R_CONFIG_ERROR_MISSING_KEY,
    R_CONFIG_ERROR_INVALID_VALUE,
    R_CONFIG_ERROR_SLOT_INVALID,
    R_CONFIG_ERROR_BOOTLOADER_INVALID,
} RConfigError;

/**
 * @brief 설정 파일 로드
 * @param filename 설정 파일 경로 (NULL이면 기본 경로 사용)
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean load_config_file(const gchar *filename, GError **error);

/**
 * @brief 설정 파일에서 슬롯들 로드
 * @param filename 설정 파일 경로
 * @param slots 슬롯들을 저장할 해시테이블 반환 위치
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_config_load_slots(const gchar *filename, GHashTable **slots, GError **error);

/**
 * @brief 시스템 정보 로드 (compatible, bootloader 등)
 * @param filename 설정 파일 경로
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_config_load_system_info(const gchar *filename, GError **error);

/**
 * @brief 키파일에서 슬롯 파싱
 * @param key_file GKeyFile 인스턴스
 * @param group_name 그룹 이름
 * @param slot_name 슬롯 이름
 * @param error 오류 정보 반환 위치
 * @return 파싱된 슬롯, 실패 시 NULL
 */
RaucSlot *r_config_parse_slot(GKeyFile *key_file, const gchar *group_name, const gchar *slot_name, GError **error);

/**
 * @brief 슬롯 부모 관계 설정
 * @param slots 슬롯 해시테이블
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_config_setup_slot_parents(GHashTable *slots, GError **error);

/**
 * @brief 설정의 유효성 검증
 * @param error 오류 정보 반환 위치
 * @return 유효하면 TRUE, 아니면 FALSE
 */
gboolean r_config_validate(GError **error);

/**
 * @brief 기본 설정 파일 경로 가져오기
 * @return 기본 설정 파일 경로 ("/etc/rauc/system.conf")
 */
const gchar *r_config_get_default_path(void);

/**
 * @brief 키링 경로 가져오기 (설정에서)
 * @return 키링 경로, 설정되지 않으면 NULL
 */
const gchar *r_config_get_keyring_path(void);

/**
 * @brief 인증서 경로 가져오기 (설정에서)
 * @return 인증서 경로, 설정되지 않으면 NULL
 */
const gchar *r_config_get_cert_path(void);

/**
 * @brief 부트로더 타입 가져오기 (설정에서)
 * @return 부트로더 타입, 설정되지 않으면 NULL
 */
const gchar *r_config_get_bootloader(void);

/**
 * @brief GRUB 환경 파일 경로 가져오기 (설정에서)
 * @return grubenv 파일 경로, 설정되지 않으면 NULL
 */
const gchar *r_config_get_grubenv_path(void);

/**
 * @brief 데이터 디렉토리 가져오기 (설정에서)
 * @return 데이터 디렉토리 경로, 설정되지 않으면 기본값
 */
const gchar *r_config_get_data_directory(void);

/**
 * @brief 마운트 접두사 가져오기 (설정에서)
 * @return 마운트 접두사, 설정되지 않으면 기본값
 */
const gchar *r_config_get_mount_prefix(void);

/**
 * @brief 설정이 로드되었는지 확인
 * @return 로드됨 시 TRUE, 아니면 FALSE
 */
gboolean r_config_is_loaded(void);

/**
 * @brief 설정 파일 경로 설정 (테스트용)
 * @param path 설정 파일 경로
 */
void r_config_set_path_for_test(const gchar *path);

/**
 * @brief 설정 정보를 문자열로 변환 (디버그용)
 * @return 설정 정보 문자열 (호출자가 g_free 해야 함)
 */
gchar *r_config_to_string(void);
