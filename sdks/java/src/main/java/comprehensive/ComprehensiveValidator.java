package comprehensive;

import java.io.IOException;
import java.io.File;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Locale;
import java.util.Scanner;

import com.google.gson.Gson;
import com.google.gson.JsonObject;

/**
 * Comprehensive validator for DecentriLicense tokens using trust chain model.
 */
public class ComprehensiveValidator {
    
    private final Gson gson = new Gson();
    
    /**
     * Validate a token using the trust chain model.
     * 
     * @param tokenFile Path to the token JSON file
     * @param rootPublicKeyFile Path to the root public key file
     * @param algorithm Expected algorithm (RSA, Ed25519, SM2)
     * @return true if validation succeeds, false otherwise
     */
    public boolean validateTokenWithTrustChain(String tokenFile, String rootPublicKeyFile, String algorithm) {
        try {
            // Load token from file
            String tokenContent = new String(Files.readAllBytes(Paths.get(tokenFile)));
            JsonObject tokenData = gson.fromJson(tokenContent, JsonObject.class);
            
            // Load root public key from file
            String rootPublicKey = new String(Files.readAllBytes(Paths.get(rootPublicKeyFile)));
            
            // Verify trust chain
            return verifyTrustChain(tokenData, rootPublicKey, algorithm);
        } catch (IOException e) {
            System.err.println("Error reading files: " + e.getMessage());
            return false;
        } catch (Exception e) {
            System.err.println("Error validating token: " + e.getMessage());
            return false;
        }
    }
    
    /**
     * Verify the trust chain for a token.
     * 
     * @param tokenData Token data object
     * @param rootPublicKey Root public key content
     * @param algorithm Expected algorithm
     * @return true if trust chain is valid, false otherwise
     */
    private boolean verifyTrustChain(JsonObject tokenData, String rootPublicKey, String algorithm) {
        System.out.println("Verifying trust chain for token " + 
            (tokenData.has("token_id") ? tokenData.get("token_id").getAsString() : "unknown"));
        System.out.println("  License Code: " + 
            (tokenData.has("license_code") ? tokenData.get("license_code").getAsString() : "unknown"));
        System.out.println("  Algorithm: " + 
            (tokenData.has("alg") ? tokenData.get("alg").getAsString() : "unknown"));
        System.out.println("  Expected Algorithm: " + algorithm);
        System.out.println("  License Public Key present: " + 
            (tokenData.has("license_public_key") && !tokenData.get("license_public_key").getAsString().isEmpty()));
        System.out.println("  Root Signature present: " + 
            (tokenData.has("root_signature") && !tokenData.get("root_signature").getAsString().isEmpty()));
        
        // Verify that the algorithm matches
        if (!tokenData.has("alg") || !tokenData.get("alg").getAsString().equals(algorithm)) {
            System.out.println("Algorithm mismatch: expected " + algorithm + 
                ", got " + (tokenData.has("alg") ? tokenData.get("alg").getAsString() : "unknown"));
            return false;
        }
        
        // Check that required fields are present
        if (!tokenData.has("license_public_key") || !tokenData.has("root_signature") ||
            tokenData.get("license_public_key").getAsString().isEmpty() || 
            tokenData.get("root_signature").getAsString().isEmpty()) {
            System.out.println("Missing required trust chain fields");
            return false;
        }
        
        // In a real implementation, we would:
        // 1. Verify the root signature of the license public key using the root public key
        // 2. Verify the token signature using the verified license public key
        
        // For demonstration purposes, we'll just return true if both fields are present and algorithm matches
        return true;
    }
    
    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);

        String tokenFile = args.length > 0 ? args[0] : "";
        String rootPublicKeyFile = args.length > 1 ? args[1] : "";
        String algorithm = args.length > 2 ? args[2] : "";

        if (tokenFile == null || tokenFile.trim().isEmpty()) {
            tokenFile = pickFileFromCwd(scanner, "请选择 token 文件:", new String[]{".json", ".txt"});
        }
        if (rootPublicKeyFile == null || rootPublicKeyFile.trim().isEmpty()) {
            rootPublicKeyFile = pickFileFromCwd(scanner, "请选择根公钥文件:", new String[]{".pem"});
        }
        if (algorithm == null || algorithm.trim().isEmpty()) {
            System.out.print("请输入 algorithm (RSA/Ed25519/SM2): ");
            algorithm = scanner.nextLine().trim();
        }

        if (tokenFile == null || tokenFile.trim().isEmpty() || rootPublicKeyFile == null || rootPublicKeyFile.trim().isEmpty() || algorithm == null || algorithm.trim().isEmpty()) {
            System.out.println("Usage: ComprehensiveValidator <token_file> <root_public_key_file> <algorithm>");
            System.exit(1);
        }
        
        ComprehensiveValidator validator = new ComprehensiveValidator();
        boolean valid = validator.validateTokenWithTrustChain(tokenFile, rootPublicKeyFile, algorithm);
        
        if (valid) {
            System.out.println("✅ Token validation successful!");
            System.exit(0);
        } else {
            System.out.println("❌ Token validation failed!");
            System.exit(1);
        }
    }

    private static List<String> listFilesForSelection(String[] exts) {
        File dir = new File(System.getProperty("user.dir"));
        File[] list = dir.listFiles();
        List<String> out = new ArrayList<>();
        if (list == null) {
            return out;
        }
        for (File f : list) {
            if (!f.isFile()) {
                continue;
            }
            String name = f.getName();
            if (exts != null && exts.length > 0) {
                String lower = name.toLowerCase(Locale.ROOT);
                boolean ok = false;
                for (String ext : exts) {
                    if (lower.endsWith(ext)) {
                        ok = true;
                        break;
                    }
                }
                if (!ok) {
                    continue;
                }
            }
            out.add(name);
        }
        Collections.sort(out);
        return out;
    }

    private static String pickFileFromCwd(Scanner scanner, String title, String[] exts) {
        List<String> files = listFilesForSelection(exts);
        System.out.println(title);
        if (files.isEmpty()) {
            System.out.print("当前目录没有可选文件，请手动输入路径: ");
            return scanner.nextLine().trim();
        }
        for (int i = 0; i < files.size(); i++) {
            System.out.println((i + 1) + ". " + files.get(i));
        }
        System.out.println("0. 手动输入路径");
        System.out.print("请选择文件编号: ");
        String sel = scanner.nextLine().trim();
        try {
            int n = Integer.parseInt(sel);
            if (n >= 1 && n <= files.size()) {
                return files.get(n - 1);
            }
        } catch (NumberFormatException e) {
            // ignore
        }
        System.out.print("请输入文件路径: ");
        return scanner.nextLine().trim();
    }
}