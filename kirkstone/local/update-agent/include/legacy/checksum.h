#pragma once

#include <glib.h>
#include <gio/gio.h>
#include <openssl/evp.h>

/**
 * @brief 체크섬 타입 열거형
 */
typedef enum {
    R_CHECKSUM_NONE = 0,
    R_CHECKSUM_MD5,
    R_CHECKSUM_SHA1,
    R_CHECKSUM_SHA256,
    R_CHECKSUM_SHA512,
} RaucChecksumType;

/**
 * @brief 체크섬 구조체
 */
typedef struct {
    RaucChecksumType type;
    gchar *digest;          // hex로 인코딩된 체크섬 값
    gsize size;             // 원본 데이터 크기
} RaucChecksum;

/**
 * @brief 체크섬 타입을 문자열로 변환
 * @param type 체크섬 타입
 * @return 타입 문자열 ("sha256", "md5" 등)
 */
const gchar *r_checksum_type_to_string(RaucChecksumType type);

/**
 * @brief 문자열을 체크섬 타입으로 변환
 * @param type_str 타입 문자열
 * @return 체크섬 타입, 알 수 없으면 R_CHECKSUM_NONE
 */
RaucChecksumType r_checksum_type_from_string(const gchar *type_str);

/**
 * @brief 새 체크섬 구조체 생성
 * @param type 체크섬 타입
 * @return 새 체크섬 구조체 (호출자가 해제 필요)
 */
RaucChecksum *r_checksum_new(RaucChecksumType type);

/**
 * @brief 체크섬 구조체 해제
 * @param checksum 해제할 체크섬
 */
void r_checksum_free(RaucChecksum *checksum);

/**
 * @brief 체크섬 구조체 복사
 * @param checksum 복사할 체크섬
 * @return 복사된 새 체크섬 구조체
 */
RaucChecksum *r_checksum_copy(const RaucChecksum *checksum);

/**
 * @brief 체크섬 구조체 초기화
 * @param checksum 초기화할 체크섬
 */
void r_checksum_clear(RaucChecksum *checksum);

/**
 * @brief 체크섬이 설정되어 있는지 확인
 * @param checksum 확인할 체크섬
 * @return 설정됨 시 TRUE, 아니면 FALSE
 */
gboolean r_checksum_is_set(const RaucChecksum *checksum);

/**
 * @brief 두 체크섬이 동일한지 비교
 * @param a 첫 번째 체크섬
 * @param b 두 번째 체크섬
 * @return 동일하면 TRUE, 아니면 FALSE
 */
gboolean r_checksum_equal(const RaucChecksum *a, const RaucChecksum *b);

/**
 * @brief 파일의 체크섬 계산
 * @param filename 체크섬을 계산할 파일
 * @param checksum 계산된 체크섬을 저장할 구조체 (type은 미리 설정되어야 함)
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_checksum_file(const gchar *filename, RaucChecksum *checksum, GError **error);

/**
 * @brief 메모리 데이터의 체크섬 계산
 * @param data 데이터 포인터
 * @param length 데이터 길이
 * @param checksum 계산된 체크섬을 저장할 구조체 (type은 미리 설정되어야 함)
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_checksum_memory(const guchar *data, gsize length, RaucChecksum *checksum, GError **error);

/**
 * @brief 파일 디스크립터의 체크섬 계산
 * @param fd 파일 디스크립터
 * @param checksum 계산된 체크섬을 저장할 구조체 (type은 미리 설정되어야 함)
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_checksum_fd(int fd, RaucChecksum *checksum, GError **error);

/**
 * @brief 스트림의 체크섬 계산
 * @param stream GInputStream 스트림
 * @param checksum 계산된 체크섬을 저장할 구조체 (type은 미리 설정되어야 함)
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_checksum_stream(GInputStream *stream, RaucChecksum *checksum, GError **error);

/**
 * @brief 체크섬 값 직접 설정
 * @param checksum 설정할 체크섬
 * @param digest hex로 인코딩된 체크섬 값
 */
void r_checksum_set_digest(RaucChecksum *checksum, const gchar *digest);

/**
 * @brief 체크섬 다이제스트 값 가져오기
 * @param checksum 체크섬
 * @return hex로 인코딩된 다이제스트 값 (읽기 전용)
 */
const gchar *r_checksum_get_digest(const RaucChecksum *checksum);

/**
 * @brief 체크섬을 문자열로 변환
 * @param checksum 변환할 체크섬
 * @return 문자열 표현 (호출자가 g_free 해야 함)
 */
gchar *r_checksum_to_string(const RaucChecksum *checksum);

/**
 * @brief 문자열에서 체크섬 파싱
 * @param checksum_str 체크섬 문자열 (예: "sha256:abc123...")
 * @param checksum 파싱된 체크섬을 저장할 구조체
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_checksum_from_string(const gchar *checksum_str, RaucChecksum *checksum, GError **error);

/**
 * @brief 파일 체크섬 검증
 * @param filename 검증할 파일
 * @param expected_checksum 예상 체크섬
 * @param error 오류 정보 반환 위치
 * @return 체크섬이 일치하면 TRUE, 아니면 FALSE
 */
gboolean r_checksum_verify_file(const gchar *filename, const RaucChecksum *expected_checksum, GError **error);

/**
 * @brief 메모리 데이터 체크섬 검증
 * @param data 데이터 포인터
 * @param length 데이터 길이
 * @param expected_checksum 예상 체크섬
 * @param error 오류 정보 반환 위치
 * @return 체크섬이 일치하면 TRUE, 아니면 FALSE
 */
gboolean r_checksum_verify_memory(const guchar *data, gsize length, const RaucChecksum *expected_checksum, GError **error);

/**
 * @brief 점진적 체크섬 계산을 위한 컨텍스트
 */
typedef struct {
    EVP_MD_CTX *md_ctx;
    RaucChecksumType type;
    gsize bytes_processed;
} RaucChecksumContext;

/**
 * @brief 점진적 체크섬 계산 시작
 * @param type 체크섬 타입
 * @param error 오류 정보 반환 위치
 * @return 체크섬 컨텍스트, 실패 시 NULL
 */
RaucChecksumContext *r_checksum_context_new(RaucChecksumType type, GError **error);

/**
 * @brief 점진적 체크섬 계산에 데이터 추가
 * @param ctx 체크섬 컨텍스트
 * @param data 데이터 포인터
 * @param length 데이터 길이
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_checksum_context_update(RaucChecksumContext *ctx, const guchar *data, gsize length, GError **error);

/**
 * @brief 점진적 체크섬 계산 완료 및 결과 가져오기
 * @param ctx 체크섬 컨텍스트
 * @param checksum 결과를 저장할 체크섬 구조체
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_checksum_context_finalize(RaucChecksumContext *ctx, RaucChecksum *checksum, GError **error);

/**
 * @brief 체크섬 컨텍스트 해제
 * @param ctx 해제할 컨텍스트
 */
void r_checksum_context_free(RaucChecksumContext *ctx);

// GLib 자동 정리 매크로
G_DEFINE_AUTOPTR_CLEANUP_FUNC(RaucChecksum, r_checksum_free);
G_DEFINE_AUTOPTR_CLEANUP_FUNC(RaucChecksumContext, r_checksum_context_free);

// 체크섬 오류 도메인
#define R_CHECKSUM_ERROR r_checksum_error_quark()
GQuark r_checksum_error_quark(void);

typedef enum {
    R_CHECKSUM_ERROR_INVALID_TYPE,
    R_CHECKSUM_ERROR_INVALID_FORMAT,
    R_CHECKSUM_ERROR_MISMATCH,
    R_CHECKSUM_ERROR_FILE_ACCESS,
    R_CHECKSUM_ERROR_CALCULATION_FAILED,
} RChecksumError;
