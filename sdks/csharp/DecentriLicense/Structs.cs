
using System;
using System.Runtime.InteropServices;

namespace DecentriLicense
{
    internal enum DLErrorCode
    {
        Success = 0,
        InvalidArgument = 1,
        NotInitialized = 2,
        AlreadyInitialized = 3,
        NetworkError = 4,
        CryptoError = 5,
        UnknownError = 6,
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct DLClientConfig
    {
        public IntPtr license_code;
        public int preferred_mode;  // DL_ConnectionMode enum
        public ushort udp_port;
        public ushort tcp_port;
        public IntPtr registry_server_url;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    internal struct DLVerificationResult
    {
        public int valid;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
        public string error_message;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    internal struct DLStatusResult
    {
        public int has_token;
        public int is_activated;
        public long issue_time;
        public long expire_time;
        public ulong state_index;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 64)]
        public string token_id;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 64)]
        public string holder_device_id;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 64)]
        public string app_id;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 64)]
        public string license_code;
    }
}

