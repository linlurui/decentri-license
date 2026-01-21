package com.decentrilicense.tests;

public class ApiImportSmokeTest {
    public static void main(String[] args) throws Exception {
        System.setProperty("decenlicense.skipLoad", "true");

        Class<?> cls = Class.forName("com.decentrilicense.DecentriLicenseClient");

        requireMethod(cls, "initialize", String.class, int.class, int.class, String.class);
        requireMethod(cls, "setProductPublicKey", String.class);
        requireMethod(cls, "importToken", String.class);
        requireMethod(cls, "getCurrentTokenJson");
        requireMethod(cls, "exportCurrentTokenEncrypted");
        requireMethod(cls, "offlineVerifyCurrentToken");
        requireMethod(cls, "getStatus");
        requireMethod(cls, "activateBindDevice");
        requireMethod(cls, "recordUsage", String.class);
        requireMethod(cls, "getDeviceId");
        requireMethod(cls, "shutdown");
        requireMethod(cls, "close");

        System.out.println("ApiImportSmokeTest: PASS");
    }

    private static void requireMethod(Class<?> cls, String name, Class<?>... paramTypes) throws Exception {
        try {
            cls.getDeclaredMethod(name, paramTypes);
        } catch (NoSuchMethodException e) {
            throw new Exception("missing method: " + name, e);
        }
    }
}
