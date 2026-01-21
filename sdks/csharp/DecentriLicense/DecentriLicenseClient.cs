
using System;
using System.Runtime.InteropServices;
using System.Text;

namespace DecentriLicense
{
    public sealed class ClientConfig
    {
        public string LicenseCode { get; set; } = string.Empty;
        public ushort UdpPort { get; set; } = 8888;
        public ushort TcpPort { get; set; } = 8889;
        public string RegistryServerUrl { get; set; } = string.Empty;
    }

    public sealed class DecentriLicenseClient : IDisposable
    {
        private IntPtr _client;
        private bool _initialized;
        private bool _disposed;

        public DecentriLicenseClient()
        {
            _client = NativeMethods.dl_client_create();
            if (_client == IntPtr.Zero)
            {
                throw new LicenseException("failed to create client");
            }
        }

        public void Initialize(ClientConfig config)
        {
            if (_disposed)
            {
                throw new ObjectDisposedException(nameof(DecentriLicenseClient));
            }
            if (_initialized)
            {
                throw new LicenseException("client already initialized");
            }

            var cConfig = new DLClientConfig();
            cConfig.license_code = Marshal.StringToHGlobalAnsi(config?.LicenseCode ?? string.Empty);
            cConfig.preferred_mode = 1; // DL_CONNECTION_MODE_LAN_P2P
            cConfig.udp_port = config?.UdpPort ?? 0;
            cConfig.tcp_port = config?.TcpPort ?? 0;
            cConfig.registry_server_url = Marshal.StringToHGlobalAnsi(config?.RegistryServerUrl ?? string.Empty);

            try
            {
                var rc = NativeMethods.dl_client_initialize(_client, ref cConfig);
                ThrowIfError(rc, "initialize failed");
                _initialized = true;
            }
            finally
            {
                Marshal.FreeHGlobal(cConfig.license_code);
                Marshal.FreeHGlobal(cConfig.registry_server_url);
            }
        }

        public void SetProductPublicKey(string productPublicKeyFileContent)
        {
            EnsureInitialized();
            var rc = NativeMethods.dl_client_set_product_public_key(_client, productPublicKeyFileContent ?? string.Empty);
            ThrowIfError(rc, "set product public key failed");
        }

        public void ImportToken(string tokenInput)
        {
            EnsureInitialized();
            var rc = NativeMethods.dl_client_import_token(_client, tokenInput ?? string.Empty);
            ThrowIfError(rc, "import token failed");
        }

        public VerificationResult OfflineVerifyCurrentToken()
        {
            EnsureInitialized();
            var rc = NativeMethods.dl_client_offline_verify_current_token(_client, out var vr);
            ThrowIfError(rc, "offline verify failed");
            return new VerificationResult
            {
                Valid = vr.valid != 0,
                ErrorMessage = vr.error_message ?? string.Empty,
            };
        }

        public StatusResult GetStatus()
        {
            EnsureInitialized();
            var rc = NativeMethods.dl_client_get_status(_client, out var st);
            ThrowIfError(rc, "get status failed");
            return new StatusResult
            {
                HasToken = st.has_token != 0,
                IsActivated = st.is_activated != 0,
                IssueTime = st.issue_time,
                ExpireTime = st.expire_time,
                StateIndex = st.state_index,
                TokenId = st.token_id ?? string.Empty,
                HolderDeviceId = st.holder_device_id ?? string.Empty,
                AppId = st.app_id ?? string.Empty,
                LicenseCode = st.license_code ?? string.Empty,
            };
        }

        public VerificationResult ActivateBindDevice()
        {
            EnsureInitialized();
            var rc = NativeMethods.dl_client_activate_bind_device(_client, out var vr);
            ThrowIfError(rc, "activate bind device failed");
            return new VerificationResult
            {
                Valid = vr.valid != 0,
                ErrorMessage = vr.error_message ?? string.Empty,
            };
        }

        public VerificationResult RecordUsage(string newStatePayloadJson)
        {
            EnsureInitialized();
            var rc = NativeMethods.dl_client_record_usage(_client, newStatePayloadJson ?? string.Empty, out var vr);
            ThrowIfError(rc, "record usage failed");
            return new VerificationResult
            {
                Valid = vr.valid != 0,
                ErrorMessage = vr.error_message ?? string.Empty,
            };
        }

        public string ExportActivatedTokenEncrypted()
        {
            EnsureInitialized();
            IntPtr buffer = Marshal.AllocHGlobal(65536);
            try
            {
                var rc = NativeMethods.dl_client_export_activated_token_encrypted(_client, buffer, (UIntPtr)65536);
                ThrowIfError(rc, "export activated token failed");
                return Marshal.PtrToStringAnsi(buffer) ?? string.Empty;
            }
            finally
            {
                Marshal.FreeHGlobal(buffer);
            }
        }

        public string ExportStateChangedTokenEncrypted()
        {
            EnsureInitialized();
            IntPtr buffer = Marshal.AllocHGlobal(65536);
            try
            {
                var rc = NativeMethods.dl_client_export_state_changed_token_encrypted(_client, buffer, (UIntPtr)65536);
                ThrowIfError(rc, "export state changed token failed");
                return Marshal.PtrToStringAnsi(buffer) ?? string.Empty;
            }
            finally
            {
                Marshal.FreeHGlobal(buffer);
            }
        }

        public string GetDeviceId()
        {
            EnsureInitialized();
            var sb = new StringBuilder(256);
            var rc = NativeMethods.dl_client_get_device_id(_client, sb, (UIntPtr)sb.Capacity);
            ThrowIfError(rc, "get device id failed");
            return sb.ToString();
        }

        public void Shutdown()
        {
            if (_disposed || _client == IntPtr.Zero)
            {
                return;
            }
            var rc = NativeMethods.dl_client_shutdown(_client);
            ThrowIfError(rc, "shutdown failed");
        }

        private void EnsureInitialized()
        {
            if (_disposed)
            {
                throw new ObjectDisposedException(nameof(DecentriLicenseClient));
            }
            if (!_initialized)
            {
                throw new LicenseException("client not initialized");
            }
        }

        private static void ThrowIfError(DLErrorCode code, string message)
        {
            if (code == DLErrorCode.Success)
            {
                return;
            }
            throw new LicenseException(message + ": " + code);
        }

        public void Dispose()
        {
            if (_disposed)
            {
                return;
            }
            _disposed = true;
            if (_client != IntPtr.Zero)
            {
                try
                {
                    NativeMethods.dl_client_destroy(_client);
                }
                finally
                {
                    _client = IntPtr.Zero;
                }
            }
        }
    }
}

