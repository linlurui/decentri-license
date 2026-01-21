#!/usr/bin/env python3
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(__file__)), '..'))
from decenlicense.client import DecentriLicenseClient, LicenseError

def read_all(p):
    with open(p, 'r', encoding='utf-8') as f:
        return f.read()

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print('Usage: test_runner.py <product_pub.pem> <token.json>')
        sys.exit(2)
    pub = read_all(sys.argv[1])
    tok = read_all(sys.argv[2])
    try:
        c = DecentriLicenseClient()
        c.initialize('', udp_port=0, tcp_port=0, registry_server_url='')
        c.set_product_public_key(pub)
        c.import_token(tok)
        v = c.offline_verify_current_token()
        print('Offline verify: valid=%s msg=%s' % (v.get('valid'), v.get('error_message','')))
        a = c.activate_bind_device()
        print('Activate bind device: valid=%s msg=%s' % (a.get('valid'), a.get('error_message','')))
        sys.exit(0)
    except LicenseError as e:
        print('LicenseError:', e)
        sys.exit(3)
    except Exception as e:
        print('Exception:', e)
        sys.exit(4)
