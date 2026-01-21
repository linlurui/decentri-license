<?php
require __DIR__ . '/../decentrilicense.php';
if ($argc < 3) {
    fwrite(STDERR, "Usage: php test_runner.php <product_pub.pem> <token.json>\n");
    exit(2);
}
$pub = file_get_contents($argv[1]);
$tok = file_get_contents($argv[2]);
try {
    $c = new DecentriLicenseClient();
    $cfg = new DecentriLicenseClientConfig();
    $cfg->license_code = '';
    $cfg->udp_port = 0;
    $cfg->tcp_port = 0;
    $cfg->registry_server_url = '';
    $c->initialize($cfg);
    $c->setProductPublicKey($pub);
    $c->importToken($tok);
    $v = $c->offlineVerifyCurrentToken();
    echo "Offline verify: valid=" . ($v['valid'] ? '1' : '0') . " msg=" . ($v['error_message'] ?? '') . "\n";
    $a = $c->activateBindDevice();
    echo "Activate bind: valid=" . ($a['valid'] ? '1' : '0') . " msg=" . ($a['error_message'] ?? '') . "\n";
    exit(0);
} catch (Exception $e) {
    fwrite(STDERR, "Exception: " . $e->getMessage() . "\n");
    exit(3);
}
