
namespace DecentriLicense
{
    public sealed class VerificationResult
    {
        public bool Valid { get; set; }
        public string ErrorMessage { get; set; } = string.Empty;
    }

    public sealed class StatusResult
    {
        public bool HasToken { get; set; }
        public bool IsActivated { get; set; }
        public long IssueTime { get; set; }
        public long ExpireTime { get; set; }
        public ulong StateIndex { get; set; }
        public string TokenId { get; set; } = string.Empty;
        public string HolderDeviceId { get; set; } = string.Empty;
        public string AppId { get; set; } = string.Empty;
        public string LicenseCode { get; set; } = string.Empty;
    }
}

