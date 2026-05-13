#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "decenlicense_c.h"

char* read_file(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = malloc(len + 1);
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);
    return buf;
}

DL_Client* new_client() {
    DL_Client* client = dl_client_create();
    if (!client) { fprintf(stderr, "Failed to create client\n"); exit(1); }
    DL_ClientConfig config;
    memset(&config, 0, sizeof(config));
    config.udp_port = 13325;
    config.tcp_port = 23325;
    uint32_t rc = dl_client_initialize(client, &config);
    if (rc != DL_ERROR_SUCCESS) { fprintf(stderr, "Init failed: %u\n", rc); exit(1); }
    return client;
}

void test_windsurf_free() {
    const char* dir = "/Volumes/workspace/project/dl-issuer/dl-issuer";
    char tkpath[512], pkpath[512];
    snprintf(tkpath, sizeof(tkpath), "%s/token_windsurf-free_AUTO-GENERATED-LICENSE-2026-018-0B3GNW_20260427090408.json", dir);
    snprintf(pkpath, sizeof(pkpath), "%s/public_windsurf-free_20260427090402.pem", dir);

    char* token_json = read_file(tkpath);
    char* product_key = read_file(pkpath);
    if (!token_json || !product_key) { fprintf(stderr, "windsurf-free files not found\n"); exit(1); }

    printf("windsurf-free token loaded\n");

    // Test 1: import + offline verify
    DL_Client* c1 = new_client();
    dl_client_set_product_public_key(c1, product_key);
    dl_client_import_token(c1, token_json);
    DL_VerificationResult vr1;
    dl_client_offline_verify_current_token(c1, &vr1);
    if (!vr1.valid) { fprintf(stderr, "windsurf-free verify failed: %s\n", vr1.error_message); exit(1); }
    printf("✅ C SDK: windsurf-free offline_verify passed\n");
    dl_client_shutdown(c1);
    dl_client_destroy(c1);

    // Test 2: activate bind device
    DL_Client* c2 = new_client();
    dl_client_set_product_public_key(c2, product_key);
    dl_client_import_token(c2, token_json);
    DL_VerificationResult vr2;
    dl_client_activate_bind_device(c2, &vr2);
    if (!vr2.valid) { fprintf(stderr, "windsurf-free activate failed: %s\n", vr2.error_message); exit(1); }
    printf("✅ C SDK: windsurf-free activate_bind_device passed\n");
    dl_client_shutdown(c2);
    dl_client_destroy(c2);

    free(token_json);
    free(product_key);
}

void test_cursor_free() {
    const char* dir = "/Volumes/workspace/project/dl-issuer/dl-issuer";
    char tkpath[512], pkpath[512];
    snprintf(tkpath, sizeof(tkpath), "%s/token_cursor-free_AUTO-GENERATED-LICENSE-2026-019-I0SKNQ_20260427090411.json", dir);
    snprintf(pkpath, sizeof(pkpath), "%s/public_cursor-free_20260427090405.pem", dir);

    char* token_json = read_file(tkpath);
    char* product_key = read_file(pkpath);
    if (!token_json || !product_key) { fprintf(stderr, "cursor-free files not found\n"); exit(1); }

    printf("cursor-free token loaded\n");

    // Test 1: import + offline verify
    DL_Client* c1 = new_client();
    dl_client_set_product_public_key(c1, product_key);
    dl_client_import_token(c1, token_json);
    DL_VerificationResult vr1;
    dl_client_offline_verify_current_token(c1, &vr1);
    if (!vr1.valid) { fprintf(stderr, "cursor-free verify failed: %s\n", vr1.error_message); exit(1); }
    printf("✅ C SDK: cursor-free offline_verify passed\n");
    dl_client_shutdown(c1);
    dl_client_destroy(c1);

    // Test 2: activate bind device
    DL_Client* c2 = new_client();
    dl_client_set_product_public_key(c2, product_key);
    dl_client_import_token(c2, token_json);
    DL_VerificationResult vr2;
    dl_client_activate_bind_device(c2, &vr2);
    if (!vr2.valid) { fprintf(stderr, "cursor-free activate failed: %s\n", vr2.error_message); exit(1); }
    printf("✅ C SDK: cursor-free activate_bind_device passed\n");
    dl_client_shutdown(c2);
    dl_client_destroy(c2);

    free(token_json);
    free(product_key);
}

int main() {
    test_windsurf_free();
    test_cursor_free();
    printf("\n🎉 All C SDK tests passed!\n");
    return 0;
}
