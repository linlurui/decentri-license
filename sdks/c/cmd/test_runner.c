#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "decenlicense_c.h"

static char* read_file_alloc(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return NULL; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }
    char* buf = malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t n = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[n] = '\0';
    return buf;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <product_public.pem> <token_file>\n", argv[0]);
        return 2;
    }
    const char* pub_path = argv[1];
    const char* token_path = argv[2];

    char* pub = read_file_alloc(pub_path);
    if (!pub) { fprintf(stderr, "Failed to read product pub %s\n", pub_path); return 3; }
    char* tok = read_file_alloc(token_path);
    if (!tok) { fprintf(stderr, "Failed to read token %s\n", token_path); free(pub); return 4; }

    DL_Client* c = dl_client_create();
    if (!c) { fprintf(stderr, "dl_client_create failed\n"); free(pub); free(tok); return 5; }
    DL_ClientConfig cfg = {0};
    cfg.license_code = "";
    cfg.udp_port = 0;
    cfg.tcp_port = 0;
    cfg.registry_server_url = "";
    if (dl_client_initialize(c, &cfg) != DL_ERROR_SUCCESS) { fprintf(stderr, "dl_client_initialize failed\n"); dl_client_destroy(c); free(pub); free(tok); return 6; }

    if (dl_client_set_product_public_key(c, pub) != DL_ERROR_SUCCESS) { fprintf(stderr, "dl_client_set_product_public_key failed\n"); dl_client_destroy(c); free(pub); free(tok); return 7; }

    if (dl_client_import_token(c, tok) != DL_ERROR_SUCCESS) { fprintf(stderr, "dl_client_import_token failed\n"); dl_client_destroy(c); free(pub); free(tok); return 8; }

    DL_VerificationResult vr = {0};
    if (dl_client_offline_verify_current_token(c, &vr) != DL_ERROR_SUCCESS) { fprintf(stderr, "dl_client_offline_verify_current_token failed\n"); dl_client_destroy(c); free(pub); free(tok); return 9; }
    printf("Offline verify: valid=%d msg=%s\n", vr.valid, vr.error_message);

    DL_VerificationResult ar = {0};
    if (dl_client_activate_bind_device(c, &ar) != DL_ERROR_SUCCESS) { fprintf(stderr, "dl_client_activate_bind_device failed\n"); dl_client_destroy(c); free(pub); free(tok); return 10; }
    printf("Activate bind device: valid=%d msg=%s\n", ar.valid, ar.error_message);

    dl_client_destroy(c);
    free(pub); free(tok);
    return 0;
}
