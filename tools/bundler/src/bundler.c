#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>

#define MAX_PATH 1024
#define MAX_CMD 2048

typedef struct {
    char *manifest_path;
    char *output_path;
    char *cert_path;
    char *key_path;
    int verbose;
    int force;
} bundler_config_t;

void print_usage(const char *prog_name) {
    printf("Usage: %s [OPTIONS] <manifest> <output>\n", prog_name);
    printf("Create a RAUC bundle from a manifest file\n\n");
    printf("Arguments:\n");
    printf("  manifest    Path to the RAUC manifest file\n");
    printf("  output      Path for the output .raucb bundle\n\n");
    printf("Options:\n");
    printf("  -c, --cert PATH    Path to certificate file\n");
    printf("  -k, --key PATH     Path to private key file\n");
    printf("  -v, --verbose      Enable verbose output\n");
    printf("  -f, --force        Overwrite existing output file\n");
    printf("  -h, --help         Show this help message\n\n");
    printf("Example:\n");
    printf("  %s manifest.raucm bundle.raucb\n", prog_name);
    printf("  %s -c cert.pem -k key.pem manifest.raucm bundle.raucb\n", prog_name);
}

int check_file_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

int check_directory_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

int create_bundle(const bundler_config_t *config) {
    char cmd[MAX_CMD];
    int ret;
    
    // Check if manifest exists
    if (!check_file_exists(config->manifest_path)) {
        fprintf(stderr, "Error: Manifest file '%s' not found\n", config->manifest_path);
        return 1;
    }
    
    // Check if output directory exists
    char *output_dir = strdup(config->output_path);
    char *last_slash = strrchr(output_dir, '/');
    if (last_slash) {
        *last_slash = '\0';
        if (strlen(output_dir) > 0 && !check_directory_exists(output_dir)) {
            fprintf(stderr, "Error: Output directory '%s' does not exist\n", output_dir);
            free(output_dir);
            return 1;
        }
    }
    free(output_dir);
    
    // Check if output file already exists
    if (check_file_exists(config->output_path) && !config->force) {
        fprintf(stderr, "Error: Output file '%s' already exists. Use -f to overwrite.\n", config->output_path);
        return 1;
    }
    
    // Build rauc command
    snprintf(cmd, sizeof(cmd), "rauc bundle");
    
    // Add certificate and key if provided
    if (config->cert_path && config->key_path) {
        snprintf(cmd + strlen(cmd), sizeof(cmd) - strlen(cmd), 
                " --cert=%s --key=%s", config->cert_path, config->key_path);
    }
    
    // Add manifest and output
    snprintf(cmd + strlen(cmd), sizeof(cmd) - strlen(cmd), 
            " %s %s", config->manifest_path, config->output_path);
    
    if (config->verbose) {
        printf("Executing: %s\n", cmd);
    }
    
    // Execute rauc bundle command
    ret = system(cmd);
    
    if (ret == 0) {
        printf("Bundle created successfully: %s\n", config->output_path);
    } else {
        fprintf(stderr, "Error: Failed to create bundle (exit code: %d)\n", ret);
    }
    
    return ret;
}

int main(int argc, char *argv[]) {
    bundler_config_t config = {0};
    int opt;
    
    static struct option long_options[] = {
        {"cert", required_argument, 0, 'c'},
        {"key", required_argument, 0, 'k'},
        {"verbose", no_argument, 0, 'v'},
        {"force", no_argument, 0, 'f'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    // Parse command line options
    while ((opt = getopt_long(argc, argv, "c:k:vfh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'c':
                config.cert_path = optarg;
                break;
            case 'k':
                config.key_path = optarg;
                break;
            case 'v':
                config.verbose = 1;
                break;
            case 'f':
                config.force = 1;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // Check for required arguments
    if (optind + 2 != argc) {
        fprintf(stderr, "Error: Missing required arguments\n\n");
        print_usage(argv[0]);
        return 1;
    }
    
    config.manifest_path = argv[optind];
    config.output_path = argv[optind + 1];
    
    // Validate certificate and key are both provided or both omitted
    if ((config.cert_path && !config.key_path) || (!config.cert_path && config.key_path)) {
        fprintf(stderr, "Error: Both certificate (-c) and key (-k) must be provided together\n");
        return 1;
    }
    
    // Check if certificate and key files exist
    if (config.cert_path && !check_file_exists(config.cert_path)) {
        fprintf(stderr, "Error: Certificate file '%s' not found\n", config.cert_path);
        return 1;
    }
    
    if (config.key_path && !check_file_exists(config.key_path)) {
        fprintf(stderr, "Error: Key file '%s' not found\n", config.key_path);
        return 1;
    }
    
    return create_bundle(&config);
} 