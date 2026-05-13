package comprehensive;

import java.io.File;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Locale;
import java.util.Scanner;

import com.decentrilicense.DecentriLicenseClient;
import com.decentrilicense.VerificationResult;
import com.decentrilicense.StatusResult;
import com.decentrilicense.LicenseException;

/**
 * Comprehensive validator for DecentriLicense tokens using dl-core native library.
 */
public class ComprehensiveValidator {

    /**
     * Validate a token using dl-core native library (trust chain + signature verification).
     *
     * @param tokenFile Path to the token JSON file
     * @param productPublicKeyFile Path to the product public key file
     * @return true if validation succeeds, false otherwise
     */
    public boolean validateTokenWithDlCore(String tokenFile, String productPublicKeyFile) {
        DecentriLicenseClient client = new DecentriLicenseClient();
        try {
            // Initialize client
            client.initialize("", 13325, 23325, "");

            // Set product public key
            String productKeyContent = new String(Files.readAllBytes(Paths.get(productPublicKeyFile)));
            client.setProductPublicKey(productKeyContent);

            // Import token
            String tokenContent = new String(Files.readAllBytes(Paths.get(tokenFile)));
            client.importToken(tokenContent);

            // Verify using dl-core
            VerificationResult result = client.offlineVerifyCurrentToken();

            if (result.isValid()) {
                System.out.println("✅ Token validation successful!");
                try {
                    StatusResult status = client.getStatus();
                    if (status.hasToken()) {
                        System.out.println("   Token ID: " + status.getTokenId());
                        System.out.println("   License Code: " + status.getLicenseCode());
                        System.out.println("   App ID: " + status.getAppId());
                        System.out.println("   Holder Device: " + status.getHolderDeviceId());
                    }
                } catch (Exception ignored) {}
            } else {
                System.out.println("❌ Token validation failed: " + result.getErrorMessage());
            }

            return result.isValid();
        } catch (Exception e) {
            System.err.println("Error validating token: " + e.getMessage());
            return false;
        } finally {
            try { client.shutdown(); } catch (Exception ignored) {}
        }
    }

    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);

        String tokenFile = args.length > 0 ? args[0] : "";
        String productPublicKeyFile = args.length > 1 ? args[1] : "";

        if (tokenFile == null || tokenFile.trim().isEmpty()) {
            tokenFile = pickFileFromCwd(scanner, "请选择 token 文件:", new String[]{".json", ".txt"});
        }
        if (productPublicKeyFile == null || productPublicKeyFile.trim().isEmpty()) {
            productPublicKeyFile = pickFileFromCwd(scanner, "请选择产品公钥文件:", new String[]{".pem"});
        }

        if (tokenFile == null || tokenFile.trim().isEmpty() || productPublicKeyFile == null || productPublicKeyFile.trim().isEmpty()) {
            System.out.println("Usage: ComprehensiveValidator <token_file> <product_public_key_file>");
            System.exit(1);
        }

        ComprehensiveValidator validator = new ComprehensiveValidator();
        boolean valid = validator.validateTokenWithDlCore(tokenFile, productPublicKeyFile);
        System.exit(valid ? 0 : 1);
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