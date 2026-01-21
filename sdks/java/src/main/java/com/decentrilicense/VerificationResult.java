package com.decentrilicense;

/**
 * Verification Result (maps to DL_VerificationResult)
 */
public class VerificationResult {
    private boolean valid;
    private String errorMessage;

    public VerificationResult() {
        this.valid = false;
        this.errorMessage = "";
    }

    public VerificationResult(boolean valid, String errorMessage) {
        this.valid = valid;
        this.errorMessage = errorMessage;
    }

    public boolean isValid() {
        return valid;
    }

    public void setValid(boolean valid) {
        this.valid = valid;
    }

    public String getErrorMessage() {
        return errorMessage;
    }

    public void setErrorMessage(String errorMessage) {
        this.errorMessage = errorMessage;
    }

    @Override
    public String toString() {
        return "VerificationResult{" +
                "valid=" + valid +
                ", errorMessage='" + errorMessage + '\'' +
                '}';
    }
}
