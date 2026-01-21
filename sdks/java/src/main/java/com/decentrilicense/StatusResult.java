package com.decentrilicense;

/**
 * Status Result (maps to DL_StatusResult)
 */
public class StatusResult {
    private boolean hasToken;
    private boolean activated;
    private long issueTime;
    private long expireTime;
    private long stateIndex;
    private String tokenId;
    private String holderDeviceId;
    private String appId;
    private String licenseCode;

    public StatusResult() {
        this.hasToken = false;
        this.activated = false;
        this.issueTime = 0;
        this.expireTime = 0;
        this.stateIndex = 0;
        this.tokenId = "";
        this.holderDeviceId = "";
        this.appId = "";
        this.licenseCode = "";
    }

    public boolean hasToken() {
        return hasToken;
    }

    public void setHasToken(boolean hasToken) {
        this.hasToken = hasToken;
    }

    public boolean isActivated() {
        return activated;
    }

    public void setActivated(boolean activated) {
        this.activated = activated;
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

    public long getStateIndex() {
        return stateIndex;
    }

    public void setStateIndex(long stateIndex) {
        this.stateIndex = stateIndex;
    }

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

    public String getAppId() {
        return appId;
    }

    public void setAppId(String appId) {
        this.appId = appId;
    }

    public String getLicenseCode() {
        return licenseCode;
    }

    public void setLicenseCode(String licenseCode) {
        this.licenseCode = licenseCode;
    }

    @Override
    public String toString() {
        return "StatusResult{" +
                "hasToken=" + hasToken +
                ", activated=" + activated +
                ", issueTime=" + issueTime +
                ", expireTime=" + expireTime +
                ", stateIndex=" + stateIndex +
                ", tokenId='" + tokenId + '\'' +
                ", holderDeviceId='" + holderDeviceId + '\'' +
                ", appId='" + appId + '\'' +
                ", licenseCode='" + licenseCode + '\'' +
                '}';
    }
}
