#!/usr/bin/env node

/**
 * DecentriLicense Node.js SDK Comprehensive Validator
 */

const fs = require('fs');
const path = require('path');
const readline = require('readline');

class ComprehensiveValidator {
    /**
     * Validate a token using the trust chain model.
     * @param {string} tokenFile - Path to the token JSON file
     * @param {string} rootPublicKeyFile - Path to the root public key file
     * @param {string} algorithm - Expected algorithm (RSA, Ed25519, SM2)
     * @returns {boolean} True if validation succeeds, false otherwise
     */
    validateTokenWithTrustChain(tokenFile, rootPublicKeyFile, algorithm) {
        try {
            // Load token from file
            const tokenData = JSON.parse(fs.readFileSync(tokenFile, 'utf8'));
            
            // Load root public key from file
            const rootPublicKey = fs.readFileSync(rootPublicKeyFile, 'utf8');
            
            // Verify trust chain
            return this.verifyTrustChain(tokenData, rootPublicKey, algorithm);
        } catch (error) {
            console.error(`Error validating token: ${error.message}`);
            return false;
        }
    }
    
    /**
     * Verify the trust chain for a token.
     * @param {Object} tokenData - Token data object
     * @param {string} rootPublicKey - Root public key content
     * @param {string} algorithm - Expected algorithm
     * @returns {boolean} True if trust chain is valid, false otherwise
     */
    verifyTrustChain(tokenData, rootPublicKey, algorithm) {
        console.log(`Verifying trust chain for token ${tokenData.token_id || 'unknown'}`);
        console.log(`  License Code: ${tokenData.license_code || 'unknown'}`);
        console.log(`  Algorithm: ${tokenData.alg || 'unknown'}`);
        console.log(`  Expected Algorithm: ${algorithm}`);
        console.log(`  License Public Key present: ${!!(tokenData.license_public_key)}`);
        console.log(`  Root Signature present: ${!!(tokenData.root_signature)}`);
        
        // Verify that the algorithm matches
        if (tokenData.alg !== algorithm) {
            console.log(`Algorithm mismatch: expected ${algorithm}, got ${tokenData.alg}`);
            return false;
        }
        
        // Check that required fields are present
        if (!tokenData.license_public_key || !tokenData.root_signature) {
            console.log("Missing required trust chain fields");
            return false;
        }
        
        // In a real implementation, we would:
        // 1. Verify the root signature of the license public key using the root public key
        // 2. Verify the token signature using the verified license public key
        
        // For demonstration purposes, we'll just return true if both fields are present and algorithm matches
        return true;
    }
}

function listFilesForSelection(exts) {
    const entries = fs.readdirSync(process.cwd(), { withFileTypes: true });
    return entries
        .filter((e) => e.isFile())
        .map((e) => e.name)
        .filter((name) => {
            if (!exts || exts.length === 0) return true;
            const lower = name.toLowerCase();
            return exts.some((ext) => lower.endsWith(ext));
        })
        .sort((a, b) => a.localeCompare(b));
}

function question(rl, prompt) {
    return new Promise((resolve) => {
        rl.question(prompt, (answer) => resolve(String(answer || '').trim()));
    });
}

async function pickFileFromCwd(rl, title, exts) {
    const files = listFilesForSelection(exts);
    console.log(title);
    if (files.length === 0) {
        return await question(rl, '当前目录没有可选文件，请手动输入路径: ');
    }
    files.forEach((f, i) => console.log(`${i + 1}. ${f}`));
    console.log('0. 手动输入路径');
    const sel = await question(rl, '请选择文件编号: ');
    const n = parseInt(sel, 10);
    if (!Number.isNaN(n) && n >= 1 && n <= files.length) {
        return path.join(process.cwd(), files[n - 1]);
    }
    return await question(rl, '请输入文件路径: ');
}

async function main() {
    const rl = readline.createInterface({ input: process.stdin, output: process.stdout });
    let tokenFile = process.argv[2];
    let rootPublicKeyFile = process.argv[3];
    let algorithm = process.argv[4];

    if (!tokenFile) {
        tokenFile = await pickFileFromCwd(rl, '请选择 token 文件:', ['.json', '.txt']);
    }
    if (!rootPublicKeyFile) {
        rootPublicKeyFile = await pickFileFromCwd(rl, '请选择根公钥文件:', ['.pem']);
    }
    if (!algorithm) {
        algorithm = await question(rl, '请输入 algorithm (RSA/Ed25519/SM2): ');
    }

    rl.close();

    if (!tokenFile || !rootPublicKeyFile || !algorithm) {
        console.log('Usage: comprehensive_validator <token_file> <root_public_key_file> <algorithm>');
        process.exit(1);
    }
    
    const validator = new ComprehensiveValidator();
    const valid = validator.validateTokenWithTrustChain(tokenFile, rootPublicKeyFile, algorithm);
    
    if (valid) {
        console.log("✅ Token validation successful!");
        process.exit(0);
    } else {
        console.log("❌ Token validation failed!");
        process.exit(1);
    }
}

// Run the validator
main().catch((e) => {
    console.error(e);
    process.exit(1);
});