#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#include "include/rauc/bundle.h"
#include "include/rauc/install.h"
#include "include/rauc/context.h"
#include "include/rauc/signature.h"

// 진행 상황 콜백 함수
void progress_callback(gint percentage, const gchar* message, gint nesting_depth, gpointer user_data) {
    // 들여쓰기 출력
    for (int i = 0; i < nesting_depth; i++) {
        printf("  ");
    }
    printf("[%d%%] %s\n", percentage, message);
    fflush(stdout);
}

// 완료 콜백 함수
void completion_callback(RInstallResult result, const gchar* message, gpointer user_data) {
    if (result == R_INSTALL_RESULT_SUCCESS) {
        printf("\n✓ 설치 성공: %s\n", message);
    } else {
        printf("\n✗ 설치 실패: %s\n", message);
    }
}

void print_usage(const char* program_name) {
    printf("사용법: %s [옵션] <bundle.raucb>\n", program_name);
    printf("\n");
    printf("RAUC 번들 설치 테스트 도구\n");
    printf("\n");
    printf("옵션:\n");
    printf("  --reboot, -r    설치 완료 후 자동으로 시스템 재부팅\n");
    printf("  --help, -h      이 도움말 출력\n");
    printf("\n");
    printf("예시:\n");
    printf("  %s /path/to/update.raucb\n", program_name);
    printf("  %s --reboot /path/to/update.raucb\n", program_name);
}

int main(int argc, char* argv[]) {
    printf("=== RAUC Bundle Installer Test ===\n");

    gboolean auto_reboot = FALSE;
    const char* bundle_path = NULL;

    // 명령행 인자 파싱
    for (int i = 1; i < argc; i++) {
        if (g_strcmp0(argv[i], "--reboot") == 0 || g_strcmp0(argv[i], "-r") == 0) {
            auto_reboot = TRUE;
        } else if (g_strcmp0(argv[i], "--help") == 0 || g_strcmp0(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (argv[i][0] != '-') {
            if (bundle_path == NULL) {
                bundle_path = argv[i];
            } else {
                fprintf(stderr, "오류: 여러 번들 파일이 지정되었습니다\n\n");
                print_usage(argv[0]);
                return 1;
            }
        } else {
            fprintf(stderr, "오류: 알 수 없는 옵션: %s\n\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    if (bundle_path == NULL) {
        fprintf(stderr, "오류: 번들 파일이 지정되지 않았습니다\n\n");
        print_usage(argv[0]);
        return 1;
    }

    printf("번들 파일: %s\n", bundle_path);
    if (auto_reboot) {
        printf("자동 재부팅: 활성화됨 (RAUC install과 동일)\n");
    }
    printf("\n");

    // RAUC 컨텍스트 초기화
    printf("RAUC 컨텍스트 초기화 중...\n");
    if (!r_context_init()) {
        fprintf(stderr, "오류: RAUC 컨텍스트 초기화 실패\n");
        return 1;
    }

    // 자동 재부팅 옵션 설정 (RAUC install과 동일)
    r_install_auto_reboot = auto_reboot;

    GError* error = NULL;

    // 번들 파일 존재 확인
    if (!g_file_test(bundle_path, G_FILE_TEST_EXISTS)) {
        fprintf(stderr, "오류: 번들 파일을 찾을 수 없습니다: %s\n", bundle_path);
        r_context_cleanup();
        return 1;
    }

    printf("번들 파일 확인됨\n");

    // 번들 로드 및 서명 검증
    printf("\n");
    printf("번들 로드 및 서명 검증을 시작합니다...\n");
    printf("=====================================\n\n");

    RaucBundle *bundle = NULL;
    if (!r_bundle_load(bundle_path, &bundle, &error)) {
        fprintf(stderr, "오류: 번들 로드 실패: %s\n", error->message);
        g_error_free(error);
        r_context_cleanup();
        return 1;
    }

    printf("✓ 번들 로드 성공\n");

    // 서명 검증
    if (!r_bundle_verify_signature(bundle, &error)) {
        fprintf(stderr, "오류: 서명 검증 실패: %s\n", error->message);
        g_error_free(error);
        r_bundle_free(bundle);
        r_context_cleanup();
        return 1;
    }

    printf("✓ 서명 검증 성공\n");

    // 번들 설치 실행 (이미 검증된 번들 객체 사용)
    printf("\n");
    printf("번들 설치를 시작합니다...\n");
    printf("=====================================\n\n");

    // 이미 검증된 번들 객체를 사용하여 설치
    gboolean success = r_install_bundle(bundle, progress_callback, completion_callback, NULL, &error);

    printf("\n");
    printf("=====================================\n");

    if (!success) {
        fprintf(stderr, "설치 실패: ");
        if (error) {
            fprintf(stderr, "%s\n", error->message);
            g_error_free(error);
        } else {
            fprintf(stderr, "알 수 없는 오류\n");
        }
        r_context_cleanup();
        return 1;
    }

    printf("\n");
    printf("설치가 성공적으로 완료되었습니다!\n");
    if (auto_reboot) {
        printf("시스템 재부팅이 자동으로 시작됩니다... (RAUC install과 동일)\n");
    } else {
        printf("시스템을 재부팅하여 업데이트를 적용하세요.\n");
    }

    // 설치 후 상태 정보 출력
    printf("\n");
    printf("=== 설치 후 시스템 상태 ===\n");
    gchar* status_info = r_install_get_status_info();
    if (status_info) {
        printf("%s\n", status_info);
        g_free(status_info);
    }

    // 정리
    if (bundle) {
        r_bundle_free(bundle);
    }
    r_context_cleanup();

    printf("\n");
    printf("프로그램 종료\n");
    return 0;
}
