#!/usr/bin/env node

/**
 * DecentriLicense Node.js SDK Comprehensive Validator
 * Uses dl-core native addon for all verification (trust chain + signature)
 */

const fs = require('fs');
const path = require('path');
const readline = require('readline');
const { DecentriLicenseClient } = require('./index.js');

function listFiles(exts) {
    return fs.readdirSync(process.cwd())
        .filter(f => exts.some(ext => f.toLowerCase().endsWith(ext)))
        .sort();
}

function question(rl, prompt) {
    return new Promise(resolve => rl.question(prompt, a => resolve(String(a || '').trim())));
}

async function pickFile(rl, title, exts) {
    const files = listFiles(exts);
    console.log(title);
    if (!files.length) return await question(rl, '当前目录没有可选文件，请手动输入路径: ');
    files.forEach((f, i) => console.log(`${i + 1}. ${f}`));
    console.log('0. 手动输入路径');
    const sel = await question(rl, '请选择文件编号: ');
    const n = parseInt(sel, 10);
    if (!Number.isNaN(n) && n >= 1 && n <= files.length) return path.join(process.cwd(), files[n - 1]);
    return await question(rl, '请输入文件路径: ');
}

async function main() {
    const rl = readline.createInterface({ input: process.stdin, output: process.stdout });
    let tokenFile = process.argv[2];
    let productKeyFile = process.argv[3];

    if (!tokenFile) tokenFile = await pickFile(rl, '请选择 token 文件:', ['.json', '.txt']);
    if (!productKeyFile) productKeyFile = await pickFile(rl, '请选择产品公钥文件:', ['.pem']);
    rl.close();

    if (!tokenFile || !productKeyFile) {
        console.log('Usage: comprehensive_validator <token_file> <product_public_key_file>');
        process.exit(1);
    }

    const tokenContent = fs.readFileSync(tokenFile, 'utf8');
    const productKeyContent = fs.readFileSync(productKeyFile, 'utf8');

    // Use dl-core native addon for verification
    const client = new DecentriLicenseClient();
    client.initialize({ udpPort: 13325, tcpPort: 23325 });
    client.setProductPublicKey(productKeyContent);
    client.importToken(tokenContent);

    const result = client.offlineVerify();

    if (result.valid) {
        console.log('✅ Token validation successful!');
        const status = client.getStatus();
        if (status.hasToken) {
            console.log(`   Token ID: ${status.tokenId}`);
            console.log(`   License Code: ${status.licenseCode}`);
            console.log(`   App ID: ${status.appId}`);
            console.log(`   Holder Device: ${status.holderDeviceId}`);
        }
    } else {
        console.log(`❌ Token validation failed: ${result.error}`);
    }

    client.shutdown();
    process.exit(result.valid ? 0 : 1);
}

main().catch(e => { console.error(e); process.exit(1); });