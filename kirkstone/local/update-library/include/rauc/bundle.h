#pragma once

#include <glib.h>
#include <gio/gio.h>
#include <openssl/cms.h>
#include "manifest.h"
#include "utils.h"

// 번들 관련 오류 도메인
#define R_BUNDLE_ERROR r_bundle_error_quark()
GQuark r_bundle_error_quark(void);

/**
 * @brief 번들 오류 타입
 */
typedef enum {
    R_BUNDLE_ERROR_SIGNATURE,
    R_BUNDLE_ERROR_KEYRING,
    R_BUNDLE_ERROR_IDENTIFIER,
    R_BUNDLE_ERROR_UNSAFE,
    R_BUNDLE_ERROR_PAYLOAD,
    R_BUNDLE_ERROR_FORMAT,
    R_BUNDLE_ERROR_VERITY,
    R_BUNDLE_ERROR_CRYPT,
    R_BUNDLE_ERROR_UNSUPPORTED,
} RBundleError;

/**
 * @brief 번들 액세스 인자 (간소화된 버전)
 */
typedef struct {
    gchar *tls_cert;            // TLS 클라이언트 인증서 (사용 안함)
    gchar *tls_key;             // TLS 개인키 (사용 안함)
    gchar *tls_ca;              // TLS CA 인증서 (사용 안함)
    gboolean tls_no_verify;     // TLS 검증 비활성화 (사용 안함)
    GStrv http_headers;         // HTTP 헤더들 (사용 안함)
    GStrv http_info_headers;    // HTTP 정보 헤더들 (사용 안함)
} RaucBundleAccessArgs;

/**
 * @brief RAUC 번들 구조체
 */
typedef struct {
    gchar *path;                        // 번들 파일 경로
    gchar *origpath;                    // 원본 경로
    gchar *storepath;                   // 저장 경로

    // NBD 관련 (사용하지 않음, NULL로 설정)
    gpointer nbd_dev;                   // RaucNBDDevice*
    gpointer nbd_srv;                   // RaucNBDServer*

    GInputStream *stream;               // 번들 입력 스트림

    goffset size;                       // 번들 크기
    GBytes *enveloped_data;             // CMS 봉투 데이터
    GBytes *sigdata;                    // 서명 데이터
    gchar *mount_point;                 // 마운트 포인트
    RaucManifest *manifest;             // 번들 매니페스트

    // 검증 상태
    gboolean verification_disabled;     // 검증 비활성화 여부
    gboolean signature_verified;        // 서명 검증 완료 여부
    gboolean payload_verified;          // 페이로드 검증 완료 여부
    gboolean exclusive_verified;        // 배타 검증 완료 여부
    gboolean was_encrypted;             // 암호화 여부
    gchar *exclusive_check_error;       // 배타 검사 오류

    STACK_OF(X509) *verified_chain;     // 검증된 인증서 체인

} RaucBundle;

/**
 * @brief 번들 검사 매개변수
 */
typedef enum {
    CHECK_BUNDLE_DEFAULT       = 0,
    CHECK_BUNDLE_NO_VERIFY     = BIT(1),      // 번들 서명 검증 건너뛰기
    CHECK_BUNDLE_NO_CHECK_TIME = BIT(2),      // 인증서 유효기간 검사 건너뛰기
    CHECK_BUNDLE_TRUST_ENV     = BIT(3),      // 런타임 환경 완전 신뢰
} CheckBundleParams;

/**
 * @brief 번들 생성 (사용하지 않음, 스텁만 제공)
 * @param bundlename 생성할 번들 파일명
 * @param contentdir 번들 내용 디렉토리
 * @param error 오류 정보 반환 위치
 * @return 항상 FALSE (구현되지 않음)
 */
gboolean create_bundle(const gchar *bundlename, const gchar *contentdir, GError **error);

/**
 * @brief 번들 검사
 * @param bundlename 검사할 번들 파일명
 * @param bundle 번들 구조체 반환 위치
 * @param params 검사 매개변수
 * @param access_args 액세스 인자 (간소화, 사용 안함)
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean check_bundle(const gchar *bundlename,
                     RaucBundle **bundle,
                     CheckBundleParams params,
                     RaucBundleAccessArgs *access_args,
                     GError **error);

/**
 * @brief 번들 페이로드 검사
 * @param bundle 검사할 번들
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean check_bundle_payload(RaucBundle *bundle, GError **error);

/**
 * @brief 번들 재서명 (사용하지 않음, 스텁만 제공)
 */
gboolean resign_bundle(RaucBundle *bundle, const gchar *outpath, GError **error);

/**
 * @brief 번들 서명 교체 (사용하지 않음, 스텁만 제공)
 */
gboolean replace_signature(RaucBundle *bundle, const gchar *insig, const gchar *outpath, CheckBundleParams params, GError **error);

/**
 * @brief 번들 서명 추출 (사용하지 않음, 스텁만 제공)
 */
gboolean extract_signature(RaucBundle *bundle, const gchar *outputsig, GError **error);

/**
 * @brief 번들 추출 (사용하지 않음, 스텁만 제공)
 */
gboolean extract_bundle(RaucBundle *bundle, const gchar *outputdir, GError **error);

/**
 * @brief 번들에서 매니페스트 추출 및 로드
 * @param bundle 번들
 * @param manifest 매니페스트 반환 위치
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean load_manifest_from_bundle(RaucBundle *bundle, RaucManifest **manifest, GError **error);

/**
 * @brief casync 번들 생성 (사용하지 않음, 스텁만 제공)
 */
gboolean create_casync_bundle(RaucBundle *bundle, const gchar *outbundle, const gchar **ignore_images, GError **error);

/**
 * @brief 번들 암호화 (사용하지 않음, 스텁만 제공)
 */
gboolean encrypt_bundle(RaucBundle *bundle, const gchar *outbundle, GError **error);

/**
 * @brief 번들 마운트
 * @param bundle 마운트할 번들
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean mount_bundle(RaucBundle *bundle, GError **error);

/**
 * @brief 번들 언마운트
 * @param bundle 언마운트할 번들
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean umount_bundle(RaucBundle *bundle, GError **error);

/**
 * @brief 번들 메모리 해제
 * @param bundle 해제할 번들
 */
void free_bundle(RaucBundle *bundle);

/**
 * @brief 번들 액세스 인자 정리
 * @param access_args 정리할 액세스 인자
 */
void clear_bundle_access_args(RaucBundleAccessArgs *access_args);

/**
 * @brief 새 번들 구조체 생성
 * @param path 번들 파일 경로
 * @return 새 번들 구조체 (호출자가 해제 필요)
 */
RaucBundle *r_bundle_new(const gchar *path);

/**
 * @brief 번들이 마운트되어 있는지 확인
 * @param bundle 확인할 번들
 * @return 마운트되어 있으면 TRUE, 아니면 FALSE
 */
gboolean r_bundle_is_mounted(RaucBundle *bundle);

/**
 * @brief 번들 정보 가져오기 (간소화된 버전)
 * @param bundle_path 번들 파일 경로
 * @param compatible 호환성 문자열 반환 (NULL 가능)
 * @param version 버전 문자열 반환 (NULL 가능)
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_bundle_get_info(const gchar *bundle_path,
                          gchar **compatible,
                          gchar **version,
                          GError **error);

/**
 * @brief 번들 서명 검증
 * @param bundle 검증할 번들
 * @param error 오류 정보 반환 위치
 * @return 검증 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_bundle_verify_signature(RaucBundle *bundle, GError **error);

/**
 * @brief 번들 열기
 * @param bundlename 번들 파일 경로
 * @param bundle 번들 구조체 반환 위치
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_bundle_open(const gchar *bundlename, RaucBundle **bundle, GError **error);

/**
 * @brief 번들 메모리 해제
 * @param bundle 해제할 번들
 */
void r_bundle_free(RaucBundle *bundle);

/**
 * @brief 번들 매니페스트 로드
 * @param bundle 번들
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_bundle_load_manifest(RaucBundle *bundle, GError **error);

/**
 * @brief 번들 호환성 확인
 * @param bundle 번들
 * @param error 오류 정보 반환 위치
 * @return 호환되면 TRUE, 아니면 FALSE
 */
gboolean r_bundle_check_compatible(RaucBundle *bundle, GError **error);

/**
 * @brief 번들 내용 검증
 * @param bundle 번들
 * @param error 오류 정보 반환 위치
 * @return 검증 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_bundle_verify_content(RaucBundle *bundle, GError **error);

/**
 * @brief 번들에서 이미지 파일 경로 가져오기
 * @param bundle 번들
 * @param slotclass 슬롯 클래스
 * @param error 오류 정보 반환 위치
 * @return 이미지 파일 경로 (호출자가 g_free 해야 함)
 */
gchar* r_bundle_get_image_path(RaucBundle *bundle, const gchar *slotclass, GError **error);

/**
 * @brief 번들을 문자열로 변환 (디버그용)
 * @param bundle 변환할 번들
 * @return 번들 정보 문자열 (호출자가 g_free 해야 함)
 */
gchar *r_bundle_to_string(const RaucBundle *bundle);

// GLib 자동 정리 매크로
G_DEFINE_AUTOPTR_CLEANUP_FUNC(RaucBundle, free_bundle);
