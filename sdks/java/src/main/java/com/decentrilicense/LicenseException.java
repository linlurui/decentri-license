package com.decentrilicense;

/**
 * License Exception
 */
public class LicenseException extends Exception {
    public LicenseException() {
        super();
    }
    
    public LicenseException(String message) {
        super(message);
    }
    
    public LicenseException(String message, Throwable cause) {
        super(message, cause);
    }
    
    public LicenseException(Throwable cause) {
        super(cause);
    }
}