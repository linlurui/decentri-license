#!/usr/bin/env python3
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(__file__)), '..'))
from decenlicense.client import DecentriLicenseClient, LicenseError

def read_all(p):
    with open(p, 'r', encoding='utf-8') as f:
        return f.read()

if __name__ == '__main__':
    pub = read_all(sys.argv[1])
    tok = read_all(sys.argv[2])
    c = DecentriLicenseClient()
    print('type', type(c))
    print('hasattr set_product_public_key', hasattr(c, 'set_product_public_key'))
    print('dir', [n for n in dir(c) if not n.startswith('_')])
    try:
        c.set_product_public_key(pub)
        print('set_product_public_key OK')
    except Exception as e:
        print('call error:', e)
        raise
