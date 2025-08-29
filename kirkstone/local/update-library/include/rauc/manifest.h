#pragma once

#include <glib.h>
#include "checksum.h"

// 매니페스트 관련 오류 도메인
#define R_MANIFEST_ERROR r_manifest_error_quark()
GQuark r_manifest_error_quark(void);

/**
 * @brief 매니페스트 오류 타입
 */
typedef enum {
    R_MANIFEST_ERROR_INVALID_FORMAT,
    R_MANIFEST_ERROR_MISSING_FIELD,
    R_MANIFEST_ERROR_INVALID_VALUE,
    R_MANIFEST_ERROR_CHECKSUM_MISMATCH,
    R_MANIFEST_ERROR_COMPAT_MISMATCH,
    R_MANIFEST_ERROR_VERSION_INVALID,
} RManifestError;

/**
 * @brief 번들 이미지 구조체
 */
typedef struct {
    gchar *slotclass;               // 슬롯 클래스
    gchar *variant;                 // 변형 (선택사항)
    gchar *filename;                // 이미지 파일명
    RaucChecksum checksum;          // 이미지 체크섬
    gsize size;                     // 이미지 크기
    gchar *hooks;                   // 훅 스크립트 이름 (단순화)
    GHashTable *adaptive;           // 적응형 정보 (사용하지 않음)
} RaucImage;

/**
 * @brief 번들 훅 구조체 (간소화)
 */
typedef struct {
    gchar *name;                    // 훅 이름
    gchar *content;                 // 훅 스크립트 내용
} RaucHook;

/**
 * @brief RAUC 매니페스트 구조체
 */
typedef struct {
    // 필수 필드들
    gchar *compatible;              // 호환성 문자열
    gchar *version;                 // 버전 문자열
    gchar *description;             // 설명 (선택사항)
    gchar *build;                   // 빌드 정보 (선택사항)

    // 이미지 목록
    GHashTable *images;             // RaucImage 해시 테이블 (slotclass -> RaucImage)

    // 훅 스크립트들
    GHashTable *hooks;              // 전역 훅들 (이름 -> RaucHook)

    // 번들 포맷 정보
    gchar *bundle_format;           // 번들 포맷 ("plain", "verity" 등)
    gchar *bundle_version;          // 번들 버전

    // 메타데이터
    gchar *handler_name;            // 핸들러 이름 (선택사항)
    GHashTable *handler_args;       // 핸들러 인자들 (선택사항)

    // 키/값 쌍들
    GHashTable *keyring;            // 키링 정보

} RaucManifest;

/**
 * @brief 매니페스트 메모리 해제
 * @param manifest 해제할 매니페스트
 */
void free_manifest(RaucManifest *manifest);

/**
 * @brief 매니페스트 메모리 해제 (호환성 함수)
 * @param manifest 해제할 매니페스트
 */
void r_manifest_free(RaucManifest *manifest);

/**
 * @brief 이미지 메모리 해제
 * @param image 해제할 이미지
 */
void r_manifest_free_image(RaucImage *image);

/**
 * @brief 훅 메모리 해제
 * @param hook 해제할 훅
 */
void r_manifest_free_hook(RaucHook *hook);

/**
 * @brief 파일에서 매니페스트 로드
 * @param filename 매니페스트 파일 경로
 * @param manifest 매니페스트 반환 위치
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean load_manifest_file(const gchar *filename, RaucManifest **manifest, GError **error);

/**
 * @brief 파일에서 매니페스트 로드 (호환성 함수)
 * @param filename 매니페스트 파일 경로
 * @param manifest 매니페스트 반환 위치
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_manifest_read_file(const gchar *filename, RaucManifest **manifest, GError **error);

/**
 * @brief 메모리에서 매니페스트 파싱
 * @param mem 매니페스트 데이터
 * @param manifest 매니페스트 반환 위치
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean load_manifest_mem(GBytes *mem, RaucManifest **manifest, GError **error);

/**
 * @brief 매니페스트를 파일에 저장 (사용하지 않음, 스텁만 제공)
 * @param filename 저장할 파일 경로
 * @param manifest 저장할 매니페스트
 * @param error 오류 정보 반환 위치
 * @return 항상 FALSE (구현되지 않음)
 */
gboolean save_manifest_file(const gchar *filename, RaucManifest *manifest, GError **error);

/**
 * @brief 새 매니페스트 생성
 * @return 새 매니페스트 (호출자가 해제 필요)
 */
RaucManifest *r_manifest_new(void);

/**
 * @brief 새 이미지 생성
 * @param slotclass 슬롯 클래스
 * @param filename 파일명
 * @return 새 이미지 (호출자가 해제 필요)
 */
RaucImage *r_manifest_image_new(const gchar *slotclass, const gchar *filename);

/**
 * @brief 새 훅 생성
 * @param name 훅 이름
 * @param content 훅 내용
 * @return 새 훅 (호출자가 해제 필요)
 */
RaucHook *r_manifest_hook_new(const gchar *name, const gchar *content);

/**
 * @brief 매니페스트에 이미지 추가
 * @param manifest 매니페스트
 * @param image 추가할 이미지
 */
void r_manifest_add_image(RaucManifest *manifest, RaucImage *image);

/**
 * @brief 매니페스트에 훅 추가
 * @param manifest 매니페스트
 * @param hook 추가할 훅
 */
void r_manifest_add_hook(RaucManifest *manifest, RaucHook *hook);

/**
 * @brief 슬롯 클래스로 이미지 찾기
 * @param manifest 매니페스트
 * @param slotclass 찾을 슬롯 클래스
 * @return 찾은 이미지, 없으면 NULL
 */
RaucImage *r_manifest_find_image_by_slotclass(RaucManifest *manifest, const gchar *slotclass);

/**
 * @brief 매니페스트 검증
 * @param manifest 검증할 매니페스트
 * @param error 오류 정보 반환 위치
 * @return 유효하면 TRUE, 아니면 FALSE
 */
gboolean r_manifest_validate(RaucManifest *manifest, GError **error);

/**
 * @brief 매니페스트 호환성 확인
 * @param manifest 확인할 매니페스트
 * @param system_compatible 시스템 호환성 문자열
 * @param error 오류 정보 반환 위치
 * @return 호환되면 TRUE, 아니면 FALSE
 */
gboolean r_manifest_check_compatible(RaucManifest *manifest, const gchar *system_compatible, GError **error);

/**
 * @brief 매니페스트를 문자열로 변환 (디버그용)
 * @param manifest 변환할 매니페스트
 * @return 매니페스트 정보 문자열 (호출자가 g_free 해야 함)
 */
gchar *r_manifest_to_string(const RaucManifest *manifest);

/**
 * @brief 이미지를 문자열로 변환 (디버그용)
 * @param image 변환할 이미지
 * @return 이미지 정보 문자열 (호출자가 g_free 해야 함)
 */
gchar *r_manifest_image_to_string(const RaucImage *image);

/**
 * @brief 이미지 체크섬 설정
 * @param image 이미지
 * @param checksum_type 체크섬 타입
 * @param digest 체크섬 다이제스트
 */
void r_manifest_image_set_checksum(RaucImage *image, RaucChecksumType checksum_type, const gchar *digest);

/**
 * @brief 이미지 체크섬 가져오기
 * @param image 이미지
 * @return 체크섬 (읽기 전용)
 */
const RaucChecksum *r_manifest_image_get_checksum(const RaucImage *image);

/**
 * @brief 매니페스트에서 모든 슬롯 클래스 가져오기
 * @param manifest 매니페스트
 * @return NULL로 끝나는 슬롯 클래스 배열 (호출자가 g_strfreev로 해제)
 */
gchar **r_manifest_get_slotclasses(RaucManifest *manifest);

/**
 * @brief 매니페스트의 이미지 개수 가져오기
 * @param manifest 매니페스트
 * @return 이미지 개수
 */
guint r_manifest_get_image_count(RaucManifest *manifest);

// GLib 자동 정리 매크로
G_DEFINE_AUTOPTR_CLEANUP_FUNC(RaucManifest, free_manifest);
G_DEFINE_AUTOPTR_CLEANUP_FUNC(RaucImage, r_manifest_free_image);
G_DEFINE_AUTOPTR_CLEANUP_FUNC(RaucHook, r_manifest_free_hook);
