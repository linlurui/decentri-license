const fs = require('fs');
const path = require('path');
const DecentriLicenseClient = require('./index');

const DL_ISSUER_DIR = '/Volumes/workspace/project/dl-issuer/dl-issuer';

function readFile(p) { return fs.readFileSync(p, 'utf-8'); }

function newClient() {
  const client = new DecentriLicenseClient();
  client.initialize({
    licenseCode: '',
    udpPort: 13325,
    tcpPort: 23325,
    registryServerUrl: ''
  });
  return client;
}

function testWindsurfFree() {
  const tokenJSON = readFile(`${DL_ISSUER_DIR}/token_windsurf-free_AUTO-GENERATED-LICENSE-2026-018-0B3GNW_20260427090408.json`);
  const productKey = readFile(`${DL_ISSUER_DIR}/public_windsurf-free_20260427090402.pem`);
  const tokenData = JSON.parse(tokenJSON);
  console.log(`windsurf-free token alg: ${tokenData.alg}`);

  // Test 1: import + offline verify
  const c1 = newClient();
  c1.setProductPublicKey(productKey);
  c1.importToken(tokenJSON);
  const r1 = c1.offlineVerify();
  if (!r1.valid) throw new Error(`windsurf-free validate failed: ${r1.errorMessage}`);
  console.log('✅ Node.js SDK: windsurf-free offline_verify passed');
  c1.shutdown();

  // Test 2: activate bind device
  const c2 = newClient();
  c2.setProductPublicKey(productKey);
  c2.importToken(tokenJSON);
  const r2 = c2.activateBindDevice();
  if (!r2.valid) throw new Error(`windsurf-free activate failed: ${r2.errorMessage}`);
  console.log('✅ Node.js SDK: windsurf-free activate_bind_device passed');
  c2.shutdown();
}

function testCursorFree() {
  const tokenJSON = readFile(`${DL_ISSUER_DIR}/token_cursor-free_AUTO-GENERATED-LICENSE-2026-019-I0SKNQ_20260427090411.json`);
  const productKey = readFile(`${DL_ISSUER_DIR}/public_cursor-free_20260427090405.pem`);
  const tokenData = JSON.parse(tokenJSON);
  console.log(`cursor-free token alg: ${tokenData.alg}`);

  // Test 1: import + offline verify
  const c1 = newClient();
  c1.setProductPublicKey(productKey);
  c1.importToken(tokenJSON);
  const r1 = c1.offlineVerify();
  if (!r1.valid) throw new Error(`cursor-free validate failed: ${r1.errorMessage}`);
  console.log('✅ Node.js SDK: cursor-free offline_verify passed');
  c1.shutdown();

  // Test 2: activate bind device
  const c2 = newClient();
  c2.setProductPublicKey(productKey);
  c2.importToken(tokenJSON);
  const r2 = c2.activateBindDevice();
  if (!r2.valid) throw new Error(`cursor-free activate failed: ${r2.errorMessage}`);
  console.log('✅ Node.js SDK: cursor-free activate_bind_device passed');
  c2.shutdown();
}

testWindsurfFree();
testCursorFree();
console.log('\n🎉 All Node.js SDK tests passed!');
