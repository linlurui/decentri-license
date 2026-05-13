import com.decentrilicense.DecentriLicenseClient;
import com.decentrilicense.VerificationResult;
import com.google.gson.Gson;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;

public class Ed25519Test {
    static final String DL_ISSUER_DIR = "/Volumes/workspace/project/dl-issuer/dl-issuer";

    static String readFile(String path) throws IOException {
        return new String(Files.readAllBytes(Paths.get(path)));
    }

    static DecentriLicenseClient newClient() throws Exception {
        DecentriLicenseClient client = new DecentriLicenseClient();
        client.initialize("", 13325, 23325, "");
        return client;
    }

    public static void main(String[] args) throws Exception {
        testWindsurfFree();
        testCursorFree();
        System.out.println("\n🎉 All Java SDK tests passed!");
    }

    static void testWindsurfFree() throws Exception {
        String tokenJSON = readFile(DL_ISSUER_DIR + "/token_windsurf-free_AUTO-GENERATED-LICENSE-2026-018-0B3GNW_20260427090408.json");
        String productKey = readFile(DL_ISSUER_DIR + "/public_windsurf-free_20260427090402.pem");

        JsonObject tokenData = JsonParser.parseString(tokenJSON).getAsJsonObject();
        System.out.println("windsurf-free token alg: " + tokenData.get("alg").getAsString());

        // Test 1: import + offline verify
        DecentriLicenseClient c1 = newClient();
        c1.setProductPublicKey(productKey);
        c1.importToken(tokenJSON);
        VerificationResult r1 = c1.offlineVerifyCurrentToken();
        if (!r1.isValid()) throw new RuntimeException("windsurf-free validate failed: " + r1.getErrorMessage());
        System.out.println("✅ Java SDK: windsurf-free offline_verify passed");
        c1.shutdown();

        // Test 2: activate bind device
        DecentriLicenseClient c2 = newClient();
        c2.setProductPublicKey(productKey);
        c2.importToken(tokenJSON);
        VerificationResult r2 = c2.activateBindDevice();
        if (!r2.isValid()) throw new RuntimeException("windsurf-free activate failed: " + r2.getErrorMessage());
        System.out.println("✅ Java SDK: windsurf-free activate_bind_device passed");
        c2.shutdown();
    }

    static void testCursorFree() throws Exception {
        String tokenJSON = readFile(DL_ISSUER_DIR + "/token_cursor-free_AUTO-GENERATED-LICENSE-2026-019-I0SKNQ_20260427090411.json");
        String productKey = readFile(DL_ISSUER_DIR + "/public_cursor-free_20260427090405.pem");

        JsonObject tokenData = JsonParser.parseString(tokenJSON).getAsJsonObject();
        System.out.println("cursor-free token alg: " + tokenData.get("alg").getAsString());

        // Test 1: import + offline verify
        DecentriLicenseClient c1 = newClient();
        c1.setProductPublicKey(productKey);
        c1.importToken(tokenJSON);
        VerificationResult r1 = c1.offlineVerifyCurrentToken();
        if (!r1.isValid()) throw new RuntimeException("cursor-free validate failed: " + r1.getErrorMessage());
        System.out.println("✅ Java SDK: cursor-free offline_verify passed");
        c1.shutdown();

        // Test 2: activate bind device
        DecentriLicenseClient c2 = newClient();
        c2.setProductPublicKey(productKey);
        c2.importToken(tokenJSON);
        VerificationResult r2 = c2.activateBindDevice();
        if (!r2.isValid()) throw new RuntimeException("cursor-free activate failed: " + r2.getErrorMessage());
        System.out.println("✅ Java SDK: cursor-free activate_bind_device passed");
        c2.shutdown();
    }
}
