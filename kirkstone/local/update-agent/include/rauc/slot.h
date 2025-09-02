#pragma once

#include <glib.h>
#include "checksum.h"

/**
 * @brief 슬롯 상태 열거형
 */
typedef enum {
    ST_UNKNOWN = 0,
    ST_ACTIVE = 1,
    ST_INACTIVE = 2,
    ST_BOOTED = 4 | ST_ACTIVE,
} SlotState;

/**
 * @brief RAUC 슬롯 상태 열거형 (install.c 호환)
 */
typedef enum {
    R_SLOT_STATE_INACTIVE = 0,
    R_SLOT_STATE_BOOTED = 1,
    R_SLOT_STATE_ACTIVE = 2,
    R_SLOT_STATE_GOOD = 3,
    R_SLOT_STATE_BAD = 4,
} RaucSlotState;

/**
 * @brief 슬롯 상태 정보 구조체
 */
typedef struct {
    gchar *bundle_compatible;    // 번들 호환성 문자열
    gchar *bundle_version;       // 번들 버전
    gchar *bundle_description;   // 번들 설명
    gchar *bundle_build;         // 번들 빌드 정보
    gchar *bundle_hash;          // 번들 해시
    gchar *status;               // 슬롯 상태
    RaucChecksum checksum;       // 슬롯 체크섬
    gchar *installed_txn;        // 설치 트랜잭션 ID
    gchar *installed_timestamp;  // 설치 타임스탬프
    guint32 installed_count;     // 설치 카운터
    gchar *activated_timestamp;  // 활성화 타임스탬프
    guint32 activated_count;     // 활성화 카운터
} RaucSlotStatus;

/**
 * @brief 슬롯 구조체
 */
typedef struct _RaucSlot {
    /** 슬롯 이름 (glib intern 문자열) */
    const gchar *name;
    /** 사용자 친화적 설명 */
    gchar *description;
    /** 슬롯 클래스 (glib intern 문자열) */
    const gchar *sclass;
    /** 슬롯 클래스 별칭 (install.c 호환) */
    const gchar *slot_class;
    /** 디바이스 경로 */
    gchar *device;
    /** 파티션 타입 */
    gchar *type;
    /** mkfs 추가 옵션들 */
    gchar **extra_mkfs_opts;
    /** 부트로더가 알고 있는 슬롯 이름 */
    gchar *bootname;
    /** 마운트된 상태에서도 업데이트 허용 여부 */
    gboolean allow_mounted;
    /** 읽기 전용 여부 */
    gboolean readonly;
    /** 동일한 이미지 설치 최적화 스킵 여부 */
    gboolean install_same;
    /** 마운트 추가 옵션들 */
    gchar *extra_mount_opts;
    /** 쓰기 후 리사이즈 여부 (ext4만 해당) */
    gboolean resize;
    /** 첫 번째 부트 파티션 시작 주소 */
    guint64 region_start;
    /** 파티션 크기 */
    guint64 region_size;

    /** 현재 슬롯 상태 (런타임) */
    SlotState state;
    /** RAUC 슬롯 상태 (install.c 호환) */
    RaucSlotState rauc_state;
    gboolean boot_good;
    struct _RaucSlot *parent;
    /** 설정에서 파싱된 부모 이름 (파싱 내부 용도) */
    gchar *parent_name;
    gchar *mount_point;
    gchar *ext_mount_point;
    RaucSlotStatus *status;
    /** 슬롯별 데이터 서브디렉토리 이름 */
    gchar *data_directory;
} RaucSlot;

/**
 * @brief 슬롯 메모리 해제
 * @param value RaucSlot 포인터
 */
void r_slot_free(gpointer value);

/**
 * @brief 슬롯 상태 정보 초기화
 * @param slotstatus RaucSlotStatus 구조체
 */
void r_slot_clear_status(RaucSlotStatus *slotstatus);

/**
 * @brief 슬롯 상태 정보 메모리 해제
 * @param slotstatus RaucSlotStatus 구조체
 */
void r_slot_free_status(RaucSlotStatus *slotstatus);

/**
 * @brief 디바이스 경로로 슬롯 찾기
 * @param slots 슬롯 해시테이블
 * @param device 찾을 디바이스 경로
 * @return 찾은 슬롯, 없으면 NULL
 */
RaucSlot *r_slot_find_by_device(GHashTable *slots, const gchar *device);

/**
 * @brief 부트 슬롯 가져오기
 * @param slots 슬롯 해시테이블
 * @return 부트 슬롯, 없으면 NULL
 */
RaucSlot *r_slot_get_booted(GHashTable *slots);

/**
 * @brief 슬롯 상태를 문자열로 변환
 * @param slotstate 변환할 슬롯 상태
 * @return 상태 문자열
 */
const gchar *r_slot_slotstate_to_str(SlotState slotstate);

/**
 * @brief 문자열을 슬롯 상태로 변환
 * @param str 상태 문자열
 * @return 해당하는 SlotState 값
 */
SlotState r_slot_str_to_slotstate(gchar *str);

/**
 * @brief 슬롯 타입이 유효한지 확인
 * @param type 타입 이름 문자열
 * @return 유효한 타입이면 TRUE, 아니면 FALSE
 */
gboolean r_slot_is_valid_type(const gchar *type);

/**
 * @brief 슬롯이 마운트 가능한지 확인
 * @param slot 확인할 슬롯
 * @return 마운트 가능하면 TRUE, 아니면 FALSE
 */
gboolean r_slot_is_mountable(RaucSlot *slot);

/**
 * @brief 슬롯의 부모 루트 슬롯 가져오기
 * @param slot 슬롯
 * @return 부모 루트 슬롯 (자기 자신일 수도 있음)
 */
RaucSlot *r_slot_get_parent_root(RaucSlot *slot);

/**
 * @brief 체크섬 데이터 디렉토리 이동
 * @param slot 슬롯
 * @param old_digest 이전 다이제스트 (NULL 가능)
 * @param new_digest 새 다이제스트 (NULL 가능)
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_slot_move_checksum_data_directory(const RaucSlot *slot,
                                            const gchar *old_digest,
                                            const gchar *new_digest,
                                            GError **error);

/**
 * @brief 체크섬 데이터 디렉토리 가져오기
 * @param slot 슬롯
 * @param checksum 체크섬 (NULL이면 현재 체크섬 사용)
 * @param error 오류 정보 반환 위치
 * @return 디렉토리 이름 (호출자가 g_free 해야 함)
 */
gchar *r_slot_get_checksum_data_directory(const RaucSlot *slot,
                                         const RaucChecksum *checksum,
                                         GError **error);

/**
 * @brief 슬롯 데이터 디렉토리 정리
 * @param slot 정리할 슬롯
 */
void r_slot_clean_data_directory(const RaucSlot *slot);

/**
 * @brief 부모가 없는 모든 클래스 가져오기
 * @param slots 슬롯 해시테이블
 * @return NULL로 끝나는 intern 문자열 배열 (g_free로 해제)
 */
gchar **r_slot_get_root_classes(GHashTable *slots);

/**
 * @brief 슬롯 리스트에 특정 슬롯이 포함되어 있는지 확인
 * @param slotlist 슬롯 리스트
 * @param testslot 찾을 슬롯 (포인터 비교)
 * @return 포함되어 있으면 TRUE, 아니면 FALSE
 */
gboolean r_slot_list_contains(GList *slotlist, const RaucSlot *testslot);

/**
 * @brief 특정 클래스의 모든 슬롯 가져오기
 * @param slots 슬롯 해시테이블
 * @param class 찾을 클래스 이름
 * @return 슬롯 리스트 (g_list_free로 해제, 슬롯 자체는 해제하지 말 것)
 */
GList *r_slot_get_all_of_class(GHashTable *slots, const gchar *slot_class);

/**
 * @brief 특정 부모 슬롯의 모든 자식 슬롯 가져오기
 * @param slots 슬롯 해시테이블
 * @param parent 부모 슬롯
 * @return 자식 슬롯 리스트 (g_list_free로 해제, 슬롯 자체는 해제하지 말 것)
 */
GList *r_slot_get_all_children(GHashTable *slots, RaucSlot *parent);

/**
 * @brief 새 슬롯 생성
 * @param name 슬롯 이름
 * @return 새 슬롯 (호출자가 해제 필요)
 */
RaucSlot *r_slot_new(const gchar *name);

/**
 * @brief 슬롯 복사
 * @param slot 복사할 슬롯
 * @return 복사된 새 슬롯 (호출자가 해제 필요)
 */
RaucSlot *r_slot_copy(const RaucSlot *slot);

/**
 * @brief 슬롯 상태 정보 로드
 * @param slot 슬롯
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_slot_load_status(RaucSlot *slot, GError **error);

/**
 * @brief 슬롯 상태 정보 저장
 * @param slot 슬롯
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_slot_save_status(RaucSlot *slot, GError **error);

/**
 * @brief 슬롯이 마운트되어 있는지 확인
 * @param slot 확인할 슬롯
 * @return 마운트되어 있으면 TRUE, 아니면 FALSE
 */
gboolean r_slot_is_mounted(RaucSlot *slot);

/**
 * @brief 슬롯 마운트
 * @param slot 마운트할 슬롯
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_slot_mount(RaucSlot *slot, GError **error);

/**
 * @brief 슬롯 언마운트
 * @param slot 언마운트할 슬롯
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_slot_unmount(RaucSlot *slot, GError **error);

/**
 * @brief 슬롯을 문자열로 변환 (디버그용)
 * @param slot 변환할 슬롯
 * @return 슬롯 정보 문자열 (호출자가 g_free 해야 함)
 */
gchar *r_slot_to_string(const RaucSlot *slot);

// GLib 자동 정리 매크로
G_DEFINE_AUTOPTR_CLEANUP_FUNC(RaucSlot, r_slot_free);
G_DEFINE_AUTOPTR_CLEANUP_FUNC(RaucSlotStatus, r_slot_free_status);

// 슬롯 관련 오류 도메인
#define R_SLOT_ERROR r_slot_error_quark()
GQuark r_slot_error_quark(void);

typedef enum {
    R_SLOT_ERROR_INVALID_NAME,
    R_SLOT_ERROR_INVALID_TYPE,
    R_SLOT_ERROR_MOUNT_FAILED,
    R_SLOT_ERROR_UNMOUNT_FAILED,
    R_SLOT_ERROR_STATUS_LOAD_FAILED,
    R_SLOT_ERROR_STATUS_SAVE_FAILED,
    R_SLOT_ERROR_DEVICE_NOT_FOUND,
    R_SLOT_ERROR_NOT_MOUNTABLE,
} RSlotError;
