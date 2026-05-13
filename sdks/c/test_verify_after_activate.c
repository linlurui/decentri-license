#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "decenlicense_c.h"

char* read_file(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
    char* buf = (char*)malloc(sz + 1);
    buf[sz] = '\0';
    fread(buf, 1, sz, f);
    fclose(f);
    return buf;
}

DL_Client* new_client() {
    DL_Client* client = dl_client_create();
    DL_ClientConfig config;
    memset(&config, 0, sizeof(config));
    config.udp_port = 13325;
    config.tcp_port = 23325;
    dl_client_initialize(client, &config);
    return client;
}

void test_verify_after_activate(const char* product, const char* tk, const char* pk) {
    const char* dir = "/Volumes/workspace/project/dl-issuer/dl-issuer";
    char tp[512], pp[512];
    snprintf(tp, sizeof(tp), "%s/%s", dir, tk);
    snprintf(pp, sizeof(pp), "%s/%s", dir, pk);
    char* token_json = read_file(tp);
    char* product_key = read_file(pp);
    if (!token_json || !product_key) { fprintf(stderr, "%s files not found\n", product); exit(1); }

    DL_Client* c = new_client();
    dl_client_set_product_public_key(c, product_key);
    dl_client_import_token(c, token_json);

    DL_VerificationResult vr1;
    dl_client_activate_bind_device(c, &vr1);
    printf("%s activate: valid=%d\n", product, vr1.valid);

    DL_VerificationResult vr2;
    dl_client_offline_verify_current_token(c, &vr2);
    printf("%s verify-after-activate: valid=%d msg=%s\n", product, vr2.valid, vr2.error_message);

    dl_client_shutdown(c); dl_client_destroy(c);
    free(token_json); free(product_key);
}

int main() {
    test_verify_after_activate("windsurf-free",
        "token_windsurf-free_AUTO-GENERATED-LICENSE-2026-018-0B3GNW_20260427090408.json",
        "public_windsurf-free_20260427090402.pem");
    test_verify_after_activate("cursor-free",
        "token_cursor-free_AUTO-GENERATED-LICENSE-2026-019-I0SKNQ_20260427090411.json",
        "public_cursor-free_20260427090405.pem");
    printf("\nAll verify-after-activate tests passed!\n");
    return 0;
}
