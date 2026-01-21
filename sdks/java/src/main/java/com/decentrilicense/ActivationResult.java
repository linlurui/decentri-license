package com.decentrilicense;

/**
 * Activation Result
 */
public class ActivationResult {
    private boolean success;
    private String message;
    private Token token;
    
    public ActivationResult() {}
    
    public ActivationResult(boolean success, String message, Token token) {
        this.success = success;
        this.message = message;
        this.token = token;
    }
    
    // Getters and setters
    public boolean isSuccess() {
        return success;
    }
    
    public void setSuccess(boolean success) {
        this.success = success;
    }
    
    public String getMessage() {
        return message;
    }
    
    public void setMessage(String message) {
        this.message = message;
    }
    
    public Token getToken() {
        return token;
    }
    
    public void setToken(Token token) {
        this.token = token;
    }
    
    @Override
    public String toString() {
        return "ActivationResult{" +
                "success=" + success +
                ", message='" + message + '\'' +
                ", token=" + token +
                '}';
    }
}