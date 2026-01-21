package com.decentrilicense;

/**
 * License Token
 */
public class Token {
    private String tokenId;
    private String holderDeviceId;
    private long issueTime;
    private long expireTime;
    private String signature;
    
    public Token() {}
    
    public Token(String tokenId, String holderDeviceId, long issueTime, long expireTime, String signature) {
        this.tokenId = tokenId;
        this.holderDeviceId = holderDeviceId;
        this.issueTime = issueTime;
        this.expireTime = expireTime;
        this.signature = signature;
    }
    
    // Getters and setters
    public String getTokenId() {
        return tokenId;
    }
    
    public void setTokenId(String tokenId) {
        this.tokenId = tokenId;
    }
    
    public String getHolderDeviceId() {
        return holderDeviceId;
    }
    
    public void setHolderDeviceId(String holderDeviceId) {
        this.holderDeviceId = holderDeviceId;
    }
    
    public long getIssueTime() {
        return issueTime;
    }
    
    public void setIssueTime(long issueTime) {
        this.issueTime = issueTime;
    }
    
    public long getExpireTime() {
        return expireTime;
    }
    
    public void setExpireTime(long expireTime) {
        this.expireTime = expireTime;
    }
    
    public String getSignature() {
        return signature;
    }
    
    public void setSignature(String signature) {
        this.signature = signature;
    }
    
    @Override
    public String toString() {
        return "Token{" +
                "tokenId='" + tokenId + '\'' +
                ", holderDeviceId='" + holderDeviceId + '\'' +
                ", issueTime=" + issueTime +
                ", expireTime=" + expireTime +
                ", signature='" + signature + '\'' +
                '}';
    }
}