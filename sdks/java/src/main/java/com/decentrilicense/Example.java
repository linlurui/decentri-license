package com.decentrilicense;

/**
 * DecentriLicense Java SDK Example
 */
public class Example {
    public static void main(String[] args) {
        String licenseCode = "EXAMPLE-LICENSE-12345";
        if (args.length > 0) {
            licenseCode = args[0];
        }
        
        System.out.println("==========================================");
        System.out.println("DecentriLicense Java SDK Example");
        System.out.println("==========================================");
        System.out.println("Using license code: " + licenseCode);
        System.out.println();
        
        try (DecentriLicenseClient client = new DecentriLicenseClient()) {
            // Initialize the client
            System.out.println("[1] Initializing client...");
            client.initialize(licenseCode, 8888, 8889, "");
            System.out.println("    Device ID: " + client.getDeviceId());
            System.out.println("    Initialization successful!");
            System.out.println();
            
            // Activate the license
            System.out.println("[2] Activating license...");
            System.out.println("    Broadcasting discovery on LAN...");
            System.out.println("    Waiting for peers and election...");
            System.out.println();
            
            try {
                ActivationResult result = client.activate();
                System.out.println("    Success: " + result.isSuccess());
                System.out.println("    Message: " + result.getMessage());
            } catch (LicenseException e) {
                System.out.println("    Activation failed: " + e.getMessage());
            }
            System.out.println();
            
            // Check activation status
            System.out.println("[3] Current status:");
            System.out.println("    Activated: " + client.isActivated());
            System.out.println("    Device State: " + client.getDeviceState());
            System.out.println();
            
            // Get current token if available
            System.out.println("[4] Current token:");
            try {
                Token token = client.getCurrentToken();
                if (token != null) {
                    System.out.println("    Token ID: " + token.getTokenId());
                    System.out.println("    Holder: " + token.getHolderDeviceId());
                    System.out.println("    License: " + licenseCode);
                } else {
                    System.out.println("    No token available");
                }
            } catch (LicenseException e) {
                System.out.println("    Failed to get token: " + e.getMessage());
            }
            System.out.println();
            
        } catch (Exception e) {
            System.err.println("Error: " + e.getMessage());
            e.printStackTrace();
        }
        
        System.out.println("[5] Example completed successfully!");
    }
}