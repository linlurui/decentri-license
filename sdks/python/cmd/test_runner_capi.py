#!/usr/bin/env python3
import sys, os
from decenlicense import _decenlicense as m

def read_all(p):
    with open(p,'r',encoding='utf-8') as f: return f.read()

if __name__=='__main__':
    if len(sys.argv)<3:
        print('Usage: test_runner_capi.py <product_pub.pem> <token.json>')
        sys.exit(2)
    pub = read_all(sys.argv[1]).encode('utf-8')
    tok = read_all(sys.argv[2]).encode('utf-8')
    c = m.dl_client_create()
    if not c:
        print('dl_client_create failed'); sys.exit(3)
    cfg = m.DL_ClientConfig()
    cfg.license_code = b''
    cfg.udp_port = 0
    cfg.tcp_port = 0
    cfg.registry_server_url = None
    rc = m.dl_client_initialize(c, cfg)
    if rc != m.DL_ERROR_SUCCESS:
        print('init fail', rc); sys.exit(4)
    rc = m.dl_client_set_product_public_key(c, pub)
    if rc != m.DL_ERROR_SUCCESS:
        print('set prod pub fail', rc); sys.exit(5)
    rc = m.dl_client_import_token(c, tok)
    if rc != m.DL_ERROR_SUCCESS:
        print('import token fail', rc); sys.exit(6)
    vr = m.DL_VerificationResult()
    rc = m.dl_client_offline_verify_current_token(c, vr)
    if rc != m.DL_ERROR_SUCCESS:
        print('verify fail', rc); sys.exit(7)
    print('Offline verify: valid=', bool(vr.valid))
    ar = m.DL_VerificationResult()
    rc = m.dl_client_activate_bind_device(c, ar)
    if rc != m.DL_ERROR_SUCCESS:
        print('activate fail', rc); sys.exit(8)
    print('Activate bind: valid=', bool(ar.valid))
    sys.exit(0)
