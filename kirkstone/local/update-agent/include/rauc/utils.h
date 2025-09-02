#pragma once

#include <glib.h>
#include <gio/gio.h>
#include <sys/types.h>

// RAUC 유틸리티 함수들 (포팅된 버전)

/**
 * @brief 파일이 존재하는지 확인
 * @param filename 확인할 파일 경로
 * @return 존재하면 TRUE, 아니면 FALSE
 */
gboolean r_file_exists(const gchar *filename);

/**
 * @brief 디렉토리가 존재하는지 확인
 * @param dirname 확인할 디렉토리 경로
 * @return 존재하면 TRUE, 아니면 FALSE
 */
gboolean r_directory_exists(const gchar *dirname);

/**
 * @brief 파일 내용을 문자열로 읽기
 * @param filename 읽을 파일 경로
 * @param error 오류 정보 반환 위치
 * @return 파일 내용 문자열, 실패 시 NULL (호출자가 g_free 해야 함)
 */
gchar *read_file_str(const gchar *filename, GError **error);

/**
 * @brief 문자열을 파일에 쓰기
 * @param filename 쓸 파일 경로
 * @param content 쓸 내용
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean write_file_str(const gchar *filename, const gchar *content, GError **error);

/**
 * @brief 파일 복사
 * @param src_path 원본 파일 경로
 * @param dest_path 대상 파일 경로
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_copy_file(const gchar *src_path, const gchar *dest_path, GError **error);

/**
 * @brief 디렉토리 생성 (필요시 부모 디렉토리도 생성)
 * @param dirname 생성할 디렉토리 경로
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_mkdir_parents(const gchar *dirname, GError **error);

/**
 * @brief 파일 권한 변경
 * @param filename 파일 경로
 * @param mode 새로운 권한
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_chmod(const gchar *filename, mode_t mode, GError **error);

/**
 * @brief 임시 디렉토리 생성
 * @param template_name 템플릿 이름 (예: "rauc-XXXXXX")
 * @param error 오류 정보 반환 위치
 * @return 생성된 임시 디렉토리 경로, 실패 시 NULL (호출자가 g_free 해야 함)
 */
gchar *r_create_temp_dir(const gchar *template_name, GError **error);

/**
 * @brief 디렉토리 재귀적 삭제
 * @param dirname 삭제할 디렉토리 경로
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_remove_tree(const gchar *dirname, GError **error);

/**
 * @brief 심볼릭 링크 생성
 * @param target 링크 대상
 * @param linkpath 링크 파일 경로
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_symlink(const gchar *target, const gchar *linkpath, GError **error);

/**
 * @brief 블록 디바이스인지 확인
 * @param path 확인할 경로
 * @return 블록 디바이스면 TRUE, 아니면 FALSE
 */
gboolean r_is_block_device(const gchar *path);

/**
 * @brief 파일 시스템 동기화
 * @param path 동기화할 경로
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_fsync(const gchar *path, GError **error);

/**
 * @brief 바이트 단위를 사람이 읽기 쉬운 형태로 변환
 * @param bytes 바이트 수
 * @return 변환된 문자열 (호출자가 g_free 해야 함)
 */
gchar *r_format_size(guint64 bytes);

/**
 * @brief 문자열이 특정 접두사로 시작하는지 확인
 * @param str 확인할 문자열
 * @param prefix 접두사
 * @return 시작하면 TRUE, 아니면 FALSE
 */
gboolean r_str_has_prefix(const gchar *str, const gchar *prefix);

/**
 * @brief 문자열이 특정 접미사로 끝나는지 확인
 * @param str 확인할 문자열
 * @param suffix 접미사
 * @return 끝나면 TRUE, 아니면 FALSE
 */
gboolean r_str_has_suffix(const gchar *str, const gchar *suffix);

/**
 * @brief 경로에서 파일명 부분만 추출
 * @param path 전체 경로
 * @return 파일명 (호출자가 g_free 해야 함)
 */
gchar *r_path_get_basename(const gchar *path);

/**
 * @brief 경로에서 디렉토리 부분만 추출
 * @param path 전체 경로
 * @return 디렉토리 경로 (호출자가 g_free 해야 함)
 */
gchar *r_path_get_dirname(const gchar *path);

/**
 * @brief 경로 결합
 * @param first_element 첫 번째 경로 요소
 * @param ... NULL로 끝나는 추가 경로 요소들
 * @return 결합된 경로 (호출자가 g_free 해야 함)
 */
gchar *r_build_path(const gchar *first_element, ...) G_GNUC_NULL_TERMINATED;

/**
 * @brief 절대 경로로 변환
 * @param path 상대 경로
 * @return 절대 경로 (호출자가 g_free 해야 함)
 */
gchar *r_realpath(const gchar *path);

/**
 * @brief 현재 시간을 ISO 8601 형식으로 반환
 * @return 시간 문자열 (호출자가 g_free 해야 함)
 */
gchar *r_get_current_time_iso8601(void);

/**
 * @brief 프로세스 실행 및 출력 캡처
 * @param argv NULL로 끝나는 명령어 배열
 * @param stdout_output 표준 출력 반환 (선택사항)
 * @param stderr_output 표준 오류 반환 (선택사항)
 * @param exit_status 종료 코드 반환 (선택사항)
 * @param error 오류 정보 반환 위치
 * @return 성공 시 TRUE, 실패 시 FALSE
 */
gboolean r_subprocess_run(gchar * const *argv,
                         gchar **stdout_output,
                         gchar **stderr_output,
                         gint *exit_status,
                         GError **error);

// 매크로 정의
#define BIT(x) (1UL << (x))

// 로깅을 위한 매크로들
#ifdef WITH_DLT
#include <dlt/dlt.h>
extern DltContext dlt_context;
#define r_debug(fmt, ...) DLT_LOG(dlt_context, DLT_LOG_DEBUG, DLT_CSTRING(fmt), ##__VA_ARGS__)
#define r_info(fmt, ...)  DLT_LOG(dlt_context, DLT_LOG_INFO, DLT_CSTRING(fmt), ##__VA_ARGS__)
#define r_warn(fmt, ...)  DLT_LOG(dlt_context, DLT_LOG_WARN, DLT_CSTRING(fmt), ##__VA_ARGS__)
#define r_error(fmt, ...) DLT_LOG(dlt_context, DLT_LOG_ERROR, DLT_CSTRING(fmt), ##__VA_ARGS__)
#else
#define r_debug(fmt, ...) g_debug(fmt, ##__VA_ARGS__)
#define r_info(fmt, ...)  g_info(fmt, ##__VA_ARGS__)
#define r_warn(fmt, ...)  g_warning(fmt, ##__VA_ARGS__)
#define r_error(fmt, ...) g_critical(fmt, ##__VA_ARGS__)
#endif

// 추가 유틸리티 함수들 (stubs.c에 구현됨)
gboolean r_subprocess_new(const gchar *command, gchar **out, GError **error);
gboolean r_verify_signature(const gchar *manifest_path, const gchar *signature_path, GError **error);
