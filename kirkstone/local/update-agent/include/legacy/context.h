#pragma once

#include <glib.h>
#include <glib/gstdio.h>

// Forward declarations
typedef struct _RaucSlot RaucSlot;

/**
 * @brief RAUC 설정 구조체 (간소화됨)
 */
typedef struct {
    gchar *system_compatible;
    GHashTable *slots;  // 슬롯 설정들
} RaucConfig;

/**
 * @brief RAUC 전역 컨텍스트 구조체
 *
 * RAUC의 전역 상태와 설정을 관리하는 구조체입니다.
 * 라이브러리 초기화 시 한 번 설정되고, 모든 RAUC 함수에서 참조됩니다.
 */
typedef struct {
	/* 설정 파일 경로 */
	gchar *configpath;
	gchar *keyringpath;
	gchar *certpath;

	/* 설정 구조체 */
	RaucConfig *config;

	/* 시스템 정보 */
	gchar *systeminfo_path;
	gchar *compatible;
	gchar *variant;
	gchar *bootslot;

	/* 슬롯 정보 */
	GHashTable *config_slots;  // 설정에서 로드된 슬롯들
	GHashTable *system_slots;  // 런타임 슬롯 상태

	/* 부트로더 설정 */
	gchar *bootloader;
	gchar *grubenv_path;

	/* 데이터 디렉토리 */
	gchar *data_directory;

	/* 로깅 설정 */
	gboolean debug;
	gchar *logfile_path;

	/* 설치 옵션들 */
	gboolean ignore_checksum;
	gboolean force_install_same;

	/* 런타임 상태 */
	gboolean initialized;
	gchar *install_info_dir;
	gchar *mount_prefix;

} RaucContext;

/**
 * @brief 전역 RAUC 컨텍스트 인스턴스
 */
extern RaucContext *r_context;

/**
 * @brief RAUC 컨텍스트 가져오기
 * @return 글로벌 RAUC 컨텍스트
 */
RaucContext* r_context_get(void);

/**
 * @brief RAUC 컨텍스트 초기화
 *
 * 전역 컨텍스트를 생성하고 기본값으로 설정합니다.
 * 실제 설정 파일 로드는 별도로 수행해야 합니다.
 *
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_context_init(void);

/**
 * @brief RAUC 컨텍스트 정리
 *
 * 할당된 메모리를 해제하고 컨텍스트를 정리합니다.
 */
void r_context_cleanup(void);

/**
 * @brief 컨텍스트가 초기화되었는지 확인
 *
 * @return 초기화됨 시 TRUE, 아니면 FALSE
 */
gboolean r_context_is_initialized(void);

/**
 * @brief 설정 파일 경로 설정
 *
 * @param config_path 설정 파일 경로
 */
void r_context_set_config_path(const gchar *config_path);

/**
 * @brief 키링 경로 설정
 *
 * @param keyring_path 키링 파일 경로
 */
void r_context_set_keyring_path(const gchar *keyring_path);

/**
 * @brief 인증서 경로 설정
 *
 * @param cert_path 인증서 파일 경로
 */
void r_context_set_cert_path(const gchar *cert_path);

/**
 * @brief 호환성 문자열 설정
 *
 * @param compatible 호환성 문자열
 */
void r_context_set_compatible(const gchar *compatible);

/**
 * @brief 부트 슬롯 설정
 *
 * @param bootslot 부트 슬롯 이름
 */
void r_context_set_bootslot(const gchar *bootslot);

/**
 * @brief 부트로더 타입 설정
 *
 * @param bootloader 부트로더 타입 ("grub", "uboot" 등)
 */
void r_context_set_bootloader(const gchar *bootloader);

/**
 * @brief GRUB 환경 파일 경로 설정
 *
 * @param grubenv_path grubenv 파일 경로
 */
void r_context_set_grubenv_path(const gchar *grubenv_path);

/**
 * @brief 데이터 디렉토리 설정
 *
 * @param data_directory RAUC 데이터 디렉토리 경로
 */
void r_context_set_data_directory(const gchar *data_directory);

/**
 * @brief 디버그 모드 설정
 *
 * @param debug 디버그 모드 활성화 여부
 */
void r_context_set_debug(gboolean debug);

/**
 * @brief 마운트 접두사 설정
 *
 * @param mount_prefix 마운트 접두사 경로
 */
void r_context_set_mount_prefix(const gchar *mount_prefix);

/**
 * @brief 슬롯을 컨텍스트에 추가
 *
 * @param slot 추가할 슬롯
 */
void r_context_add_slot(RaucSlot *slot);

/**
 * @brief 슬롯 이름으로 슬롯 찾기
 *
 * @param slot_name 찾을 슬롯 이름
 * @return 찾은 슬롯, 없으면 NULL
 */
RaucSlot *r_context_get_slot_by_name(const gchar *slot_name);

/**
 * @brief 모든 설정 슬롯 가져오기
 *
 * @return 슬롯 해시 테이블 (읽기 전용)
 */
GHashTable *r_context_get_config_slots(void);

/**
 * @brief 모든 시스템 슬롯 가져오기
 *
 * @return 슬롯 해시 테이블 (읽기 전용)
 */
GHashTable *r_context_get_system_slots(void);

/**
 * @brief 부트 슬롯 이름 가져오기
 *
 * @return 부트 슬롯 이름 (NULL일 수 있음)
 */
const gchar *r_context_get_bootslot(void);

/**
 * @brief 호환성 문자열 가져오기
 *
 * @return 호환성 문자열 (NULL일 수 있음)
 */
const gchar *r_context_get_compatible(void);

/**
 * @brief 부트로더 타입 가져오기
 *
 * @return 부트로더 타입 문자열 (NULL일 수 있음)
 */
const gchar *r_context_get_bootloader(void);

/**
 * @brief GRUB 환경 파일 경로 가져오기
 *
 * @return grubenv 파일 경로 (NULL일 수 있음)
 */
const gchar *r_context_get_grubenv_path(void);

/**
 * @brief 데이터 디렉토리 가져오기
 *
 * @return RAUC 데이터 디렉토리 경로 (NULL일 수 있음)
 */
const gchar *r_context_get_data_directory(void);

/**
 * @brief 마운트 접두사 가져오기
 *
 * @return 마운트 접두사 경로
 */
const gchar *r_context_get_mount_prefix(void);

/**
 * @brief 디버그 모드 여부 확인
 *
 * @return 디버그 모드면 TRUE, 아니면 FALSE
 */
gboolean r_context_is_debug_enabled(void);

/**
 * @brief 슬롯 클래스로 슬롯 찾기
 *
 * @param context 컨텍스트
 * @param slotclass 찾을 슬롯 클래스 이름
 * @return 찾은 슬롯, 없으면 NULL
 */
RaucSlot *r_context_find_slot_by_class(RaucContext *context, const gchar *slotclass);

/**
 * @brief 컨텍스트 정보를 문자열로 출력 (디버그용)
 *
 * @return 컨텍스트 정보 문자열 (호출자가 g_free 해야 함)
 */
gchar *r_context_to_string(void);
