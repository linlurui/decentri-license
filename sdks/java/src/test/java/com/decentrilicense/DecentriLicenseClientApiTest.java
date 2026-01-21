package com.decentrilicense;

import org.junit.jupiter.api.Test;

import java.lang.reflect.Method;

import static org.junit.jupiter.api.Assertions.assertNotNull;

public class DecentriLicenseClientApiTest {
    @Test
    public void testNewApiMethodsExistWithoutLoadingNativeLib() throws Exception {
        System.setProperty("decenlicense.skipLoad", "true");

        Class<?> cls = Class.forName("com.decentrilicense.DecentriLicenseClient");

        assertNotNull(cls.getDeclaredMethod("setProductPublicKey", String.class));
        assertNotNull(cls.getDeclaredMethod("importToken", String.class));
        assertNotNull(cls.getDeclaredMethod("getCurrentTokenJson"));
        assertNotNull(cls.getDeclaredMethod("exportCurrentTokenEncrypted"));
        assertNotNull(cls.getDeclaredMethod("offlineVerifyCurrentToken"));
        assertNotNull(cls.getDeclaredMethod("getStatus"));
        assertNotNull(cls.getDeclaredMethod("activateBindDevice"));
        assertNotNull(cls.getDeclaredMethod("recordUsage", String.class));

        Method close = cls.getDeclaredMethod("close");
        assertNotNull(close);
    }
}
