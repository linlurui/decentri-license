
using System;
using System.Runtime.InteropServices;
using System.Text;

namespace DecentriLicense
{
    internal static class NativeMethods
    {
        private const string LibraryName = "decentrilicense";

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr dl_client_create();

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void dl_client_destroy(IntPtr client);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern DLErrorCode dl_client_initialize(IntPtr client, ref DLClientConfig config);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal static extern DLErrorCode dl_client_set_product_public_key(IntPtr client, string product_public_key_file_content);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal static extern DLErrorCode dl_client_import_token(IntPtr client, string token_input);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern DLErrorCode dl_client_offline_verify_current_token(IntPtr client, out DLVerificationResult result);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern DLErrorCode dl_client_get_status(IntPtr client, out DLStatusResult status);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern DLErrorCode dl_client_activate_bind_device(IntPtr client, out DLVerificationResult result);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal static extern DLErrorCode dl_client_record_usage(IntPtr client, string new_state_payload_json, out DLVerificationResult result);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal static extern DLErrorCode dl_client_export_activated_token_encrypted(IntPtr client, IntPtr out_encrypted, UIntPtr out_encrypted_size);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal static extern DLErrorCode dl_client_export_state_changed_token_encrypted(IntPtr client, IntPtr out_encrypted, UIntPtr out_encrypted_size);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern DLErrorCode dl_client_shutdown(IntPtr client);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal static extern DLErrorCode dl_client_get_device_id(IntPtr client, StringBuilder device_id, UIntPtr device_id_size);
    }
}

