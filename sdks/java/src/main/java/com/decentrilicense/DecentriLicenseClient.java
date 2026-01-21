package com.decentrilicense;

/**
 * DecentriLicense Java Client
 * 
 * A Java wrapper for the DecentriLicense C++ SDK through JNI.
 */
public class DecentriLicenseClient implements AutoCloseable {
    // Load the native library
    static {
        if (!Boolean.getBoolean("decenlicense.skipLoad")) {
            System.loadLibrary("decenlicense_jni");
        }
    }
    
    // Native handle to the C++ object
    private long nativeHandle;
    
    /**
     * Create a new DecentriLicense client
     */
    public DecentriLicenseClient() {
        this.nativeHandle = createClient();
    }
    
    /**
     * Initialize the client with configuration
     * 
     * @param licenseCode The license code
     * @param udpPort UDP port for discovery (default: 8888)
     * @param tcpPort TCP port for P2P communication (default: 8889)
     * @param registryServerUrl Optional registry server URL
     * @throws LicenseException if initialization fails
     */
    public void initialize(String licenseCode, int udpPort, int tcpPort, String registryServerUrl) 
            throws LicenseException {
        if (nativeHandle == 0) {
            throw new LicenseException("Client not initialized");
        }
        
        int result = initializeClient(nativeHandle, licenseCode, udpPort, tcpPort, registryServerUrl);
        if (result != 0) {
            throw new LicenseException("Initialization failed with error code: " + result);
        }
    }
    
    /**
     * Activate the license
     * 
     * @return Activation result
     * @throws LicenseException if activation fails
     */
    public ActivationResult activate() throws LicenseException {
        if (nativeHandle == 0) {
            throw new LicenseException("Client not initialized");
        }

        ActivationResult result = activateClient(nativeHandle);
        if (result == null) {
            throw new LicenseException("Activation failed");
        }
        return result;
    }

    public void setProductPublicKey(String productPublicKeyFileContent) throws LicenseException {
        if (nativeHandle == 0) {
            throw new LicenseException("Client not initialized");
        }
        int result = setProductPublicKeyClient(nativeHandle, productPublicKeyFileContent);
        if (result != 0) {
            throw new LicenseException("Set product public key failed with error code: " + result);
        }
    }

    public void importToken(String tokenInput) throws LicenseException {
        if (nativeHandle == 0) {
            throw new LicenseException("Client not initialized");
        }
        int result = importTokenClient(nativeHandle, tokenInput);
        if (result != 0) {
            throw new LicenseException("Import token failed with error code: " + result);
        }
    }

    public String getCurrentTokenJson() throws LicenseException {
        if (nativeHandle == 0) {
            throw new LicenseException("Client not initialized");
        }
        String json = getCurrentTokenJsonClient(nativeHandle);
        if (json == null) {
            throw new LicenseException("Get current token json failed");
        }
        return json;
    }

    public String exportCurrentTokenEncrypted() throws LicenseException {
        if (nativeHandle == 0) {
            throw new LicenseException("Client not initialized");
        }
        String encrypted = exportCurrentTokenEncryptedClient(nativeHandle);
        if (encrypted == null) {
            throw new LicenseException("Export current token encrypted failed");
        }
        return encrypted;
    }

    public VerificationResult offlineVerifyCurrentToken() throws LicenseException {
        if (nativeHandle == 0) {
            throw new LicenseException("Client not initialized");
        }
        VerificationResult result = offlineVerifyCurrentTokenClient(nativeHandle);
        if (result == null) {
            throw new LicenseException("Offline verify failed");
        }
        return result;
    }

    public StatusResult getStatus() throws LicenseException {
        if (nativeHandle == 0) {
            throw new LicenseException("Client not initialized");
        }
        StatusResult status = getStatusClient(nativeHandle);
        if (status == null) {
            throw new LicenseException("Get status failed");
        }
        return status;
    }

    public VerificationResult activateBindDevice() throws LicenseException {
        if (nativeHandle == 0) {
            throw new LicenseException("Client not initialized");
        }
        VerificationResult result = activateBindDeviceClient(nativeHandle);
        if (result == null) {
            throw new LicenseException("Activate bind device failed");
        }
        return result;
    }

    public VerificationResult recordUsage(String newStatePayloadJson) throws LicenseException {
        if (nativeHandle == 0) {
            throw new LicenseException("Client not initialized");
        }
        VerificationResult result = recordUsageClient(nativeHandle, newStatePayloadJson);
        if (result == null) {
            throw new LicenseException("Record usage failed");
        }
        return result;
    }

    /**
     * Export the activated token as an encrypted string
     *
     * @return Encrypted token string
     * @throws LicenseException if the operation fails
     */
    public String exportActivatedTokenEncrypted() throws LicenseException {
        if (nativeHandle == 0) {
            throw new LicenseException("Client not initialized");
        }
        String encrypted = exportActivatedTokenEncryptedClient(nativeHandle);
        if (encrypted == null) {
            throw new LicenseException("Export activated token encrypted failed");
        }
        return encrypted;
    }

    /**
     * Export the state-changed token as an encrypted string
     *
     * @return Encrypted token string
     * @throws LicenseException if the operation fails
     */
    public String exportStateChangedTokenEncrypted() throws LicenseException {
        if (nativeHandle == 0) {
            throw new LicenseException("Client not initialized");
        }
        String encrypted = exportStateChangedTokenEncryptedClient(nativeHandle);
        if (encrypted == null) {
            throw new LicenseException("Export state changed token encrypted failed");
        }
        return encrypted;
    }

    /**
     * Get the current token if activated
     *
     * @return Current token or null if not activated
     * @throws LicenseException if the operation fails
     */
    public Token getCurrentToken() throws LicenseException {
        if (nativeHandle == 0) {
            throw new LicenseException("Client not initialized");
        }
        
        return getCurrentTokenClient(nativeHandle);
    }
    
    /**
     * Check if the license is activated
     * 
     * @return true if activated, false otherwise
     */
    public boolean isActivated() {
        if (nativeHandle == 0) {
            return false;
        }
        
        return isActivatedClient(nativeHandle);
    }
    
    /**
     * Get the device ID
     * 
     * @return Device ID
     * @throws LicenseException if the operation fails
     */
    public String getDeviceId() throws LicenseException {
        if (nativeHandle == 0) {
            throw new LicenseException("Client not initialized");
        }
        
        return getDeviceIdClient(nativeHandle);
    }
    
    /**
     * Get the device state
     * 
     * @return Device state
     */
    public DeviceState getDeviceState() {
        if (nativeHandle == 0) {
            return DeviceState.IDLE;
        }
        
        int state = getDeviceStateClient(nativeHandle);
        return DeviceState.fromInt(state);
    }
    
    /**
     * Shutdown the client
     * 
     * @throws LicenseException if shutdown fails
     */
    public void shutdown() throws LicenseException {
        if (nativeHandle == 0) {
            throw new LicenseException("Client not initialized");
        }

        long handle = nativeHandle;
        int result = shutdownClient(handle);
        if (result != 0) {
            throw new LicenseException("Shutdown failed with error code: " + result);
        }

        destroyClient(handle);
        nativeHandle = 0;
    }
    
    /**
     * Close the client (AutoCloseable interface)
     */
    @Override
    public void close() throws Exception {
        if (nativeHandle != 0) {
            try {
                shutdown();
            } catch (LicenseException e) {
                long handle = nativeHandle;
                nativeHandle = 0;
                destroyClient(handle);
                throw e;
            }
        }
    }
    
    // Native methods
    private native long createClient();
    private native int initializeClient(long handle, String licenseCode, int udpPort, int tcpPort, String registryServerUrl);
    private native ActivationResult activateClient(long handle);
    private native Token getCurrentTokenClient(long handle);
    private native int setProductPublicKeyClient(long handle, String productPublicKeyFileContent);
    private native int importTokenClient(long handle, String tokenInput);
    private native String getCurrentTokenJsonClient(long handle);
    private native String exportCurrentTokenEncryptedClient(long handle);
    private native VerificationResult offlineVerifyCurrentTokenClient(long handle);
    private native StatusResult getStatusClient(long handle);
    private native VerificationResult activateBindDeviceClient(long handle);
    private native VerificationResult recordUsageClient(long handle, String newStatePayloadJson);
    private native String exportActivatedTokenEncryptedClient(long handle);
    private native String exportStateChangedTokenEncryptedClient(long handle);
    private native boolean isActivatedClient(long handle);
    private native String getDeviceIdClient(long handle);
    private native int getDeviceStateClient(long handle);
    private native int shutdownClient(long handle);
    private native void destroyClient(long handle);
}