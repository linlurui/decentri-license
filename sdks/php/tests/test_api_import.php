<?php

putenv('DECENLICENSE_SKIP_LOAD=1');

require_once __DIR__ . '/../decentrilicense.php';

if (!class_exists('DecentriLicenseClient')) {
    fwrite(STDERR, "DecentriLicenseClient missing\n");
    exit(1);
}

$required = [
    'initialize',
    'setProductPublicKey',
    'importToken',
    'offlineVerifyCurrentToken',
    'activateBindDevice',
    'getStatus',
    'recordUsage',
    'getDeviceId',
    'shutdown',
];

foreach ($required as $m) {
    if (!method_exists('DecentriLicenseClient', $m)) {
        fwrite(STDERR, "method missing: $m\n");
        exit(1);
    }
}

exit(0);
