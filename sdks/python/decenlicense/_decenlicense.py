import ctypes
import os


def _repo_root():
    here = os.path.abspath(os.path.dirname(__file__))
    return os.path.abspath(os.path.join(here, "..", "..", ".."))


def _candidate_library_paths():
    root = _repo_root()
    return [
        os.path.join(root, "dl-core", "build", "libdecentrilicense.dylib"),
        os.path.join(root, "dl-core", "build", "libdecentrilicense.so"),
        os.path.join(root, "build", "libdecentrilicense.so"),
        os.path.join(root, "sdks", "cpp", "lib", "libdecentrilicense.dylib"),
        os.path.join(root, "sdks", "cpp", "lib", "libdecentrilicense.so"),
    ]


def _load_core_lib():
    if os.environ.get("DECENLICENSE_SKIP_LOAD") == "1":
        return None

    override = os.environ.get("DECENLICENSE_CORE_LIB")
    if override:
        if os.path.exists(override):
            return ctypes.CDLL(override)
        raise OSError("DECENLICENSE_CORE_LIB not found: %s" % override)

    for p in _candidate_library_paths():
        if os.path.exists(p):
            return ctypes.CDLL(p)

    raise OSError("Cannot find dl-core library. Set DECENLICENSE_CORE_LIB to a valid path.")


_lib = _load_core_lib()


DL_ERROR_SUCCESS = 0
DL_ERROR_INVALID_ARGUMENT = 1
DL_ERROR_NOT_INITIALIZED = 2
DL_ERROR_ALREADY_INITIALIZED = 3
DL_ERROR_NETWORK_ERROR = 4
DL_ERROR_CRYPTO_ERROR = 5
DL_ERROR_UNKNOWN_ERROR = 6

DL_DEVICE_STATE_IDLE = 0
DL_DEVICE_STATE_DISCOVERING = 1
DL_DEVICE_STATE_ELECTING = 2
DL_DEVICE_STATE_COORDINATOR = 3
DL_DEVICE_STATE_FOLLOWER = 4


class DL_Token(ctypes.Structure):
    _fields_ = [
        ("token_id", ctypes.c_char * 128),
        ("holder_device_id", ctypes.c_char * 256),
        ("issue_time", ctypes.c_int64),
        ("expire_time", ctypes.c_int64),
        ("signature", ctypes.c_char * 512),
        ("license_public_key", ctypes.c_char * 1024),
        ("root_signature", ctypes.c_char * 512),
        ("app_id", ctypes.c_char * 128),
        ("license_code", ctypes.c_char * 128),
    ]


class DL_ActivationResult(ctypes.Structure):
    _fields_ = [
        ("success", ctypes.c_int),
        ("message", ctypes.c_char * 256),
        ("token", ctypes.POINTER(DL_Token)),
    ]


class DL_ClientConfig(ctypes.Structure):
    _fields_ = [
        ("license_code", ctypes.c_char_p),
        ("preferred_mode", ctypes.c_int),  # DL_ConnectionMode enum
        ("udp_port", ctypes.c_uint16),
        ("tcp_port", ctypes.c_uint16),
        ("registry_server_url", ctypes.c_char_p),
    ]


class DL_VerificationResult(ctypes.Structure):
    _fields_ = [
        ("valid", ctypes.c_int),
        ("error_message", ctypes.c_char * 256),
    ]


class DL_StatusResult(ctypes.Structure):
    _fields_ = [
        ("has_token", ctypes.c_int),
        ("is_activated", ctypes.c_int),
        ("issue_time", ctypes.c_int64),
        ("expire_time", ctypes.c_int64),
        ("state_index", ctypes.c_uint64),
        ("token_id", ctypes.c_char * 128),
        ("holder_device_id", ctypes.c_char * 256),
        ("app_id", ctypes.c_char * 128),
        ("license_code", ctypes.c_char * 128),
    ]


def _unavailable(*_args, **_kwargs):
    raise RuntimeError("dl-core library not loaded. Set DECENLICENSE_CORE_LIB or unset DECENLICENSE_SKIP_LOAD.")


if _lib is None:
    dl_client_create = _unavailable
    dl_client_destroy = _unavailable
    dl_client_initialize = _unavailable
    dl_client_set_product_public_key = _unavailable
    dl_client_import_token = _unavailable
    dl_client_get_current_token_json = _unavailable
    dl_client_export_current_token_encrypted = _unavailable
    dl_client_offline_verify_current_token = _unavailable
    dl_client_get_status = _unavailable
    dl_client_activate_bind_device = _unavailable
    dl_client_record_usage = _unavailable
    dl_client_activate = _unavailable
    dl_client_get_current_token = _unavailable
    dl_client_is_activated = _unavailable
    dl_client_get_device_id = _unavailable
    dl_client_get_device_state = _unavailable
    dl_client_verify_token_trust_chain = _unavailable
    dl_client_shutdown = _unavailable
else:
    dl_client_create = _lib.dl_client_create
    dl_client_create.restype = ctypes.c_void_p

    dl_client_destroy = _lib.dl_client_destroy
    dl_client_destroy.argtypes = [ctypes.c_void_p]
    dl_client_destroy.restype = None

    dl_client_initialize = _lib.dl_client_initialize
    dl_client_initialize.argtypes = [ctypes.c_void_p, ctypes.POINTER(DL_ClientConfig)]
    dl_client_initialize.restype = ctypes.c_int

    dl_client_set_product_public_key = _lib.dl_client_set_product_public_key
    dl_client_set_product_public_key.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
    dl_client_set_product_public_key.restype = ctypes.c_int

    dl_client_import_token = _lib.dl_client_import_token
    dl_client_import_token.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
    dl_client_import_token.restype = ctypes.c_int

    dl_client_get_current_token_json = _lib.dl_client_get_current_token_json
    dl_client_get_current_token_json.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t]
    dl_client_get_current_token_json.restype = ctypes.c_int

    dl_client_export_current_token_encrypted = _lib.dl_client_export_current_token_encrypted
    dl_client_export_current_token_encrypted.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t]
    dl_client_export_current_token_encrypted.restype = ctypes.c_int

    dl_client_offline_verify_current_token = _lib.dl_client_offline_verify_current_token
    dl_client_offline_verify_current_token.argtypes = [ctypes.c_void_p, ctypes.POINTER(DL_VerificationResult)]
    dl_client_offline_verify_current_token.restype = ctypes.c_int

    dl_client_get_status = _lib.dl_client_get_status
    dl_client_get_status.argtypes = [ctypes.c_void_p, ctypes.POINTER(DL_StatusResult)]
    dl_client_get_status.restype = ctypes.c_int

    dl_client_activate_bind_device = _lib.dl_client_activate_bind_device
    dl_client_activate_bind_device.argtypes = [ctypes.c_void_p, ctypes.POINTER(DL_VerificationResult)]
    dl_client_activate_bind_device.restype = ctypes.c_int

    dl_client_record_usage = _lib.dl_client_record_usage
    dl_client_record_usage.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(DL_VerificationResult)]
    dl_client_record_usage.restype = ctypes.c_int

    dl_client_export_activated_token_encrypted = _lib.dl_client_export_activated_token_encrypted
    dl_client_export_activated_token_encrypted.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t]
    dl_client_export_activated_token_encrypted.restype = ctypes.c_int

    dl_client_export_state_changed_token_encrypted = _lib.dl_client_export_state_changed_token_encrypted
    dl_client_export_state_changed_token_encrypted.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t]
    dl_client_export_state_changed_token_encrypted.restype = ctypes.c_int

    dl_client_activate = _lib.dl_client_activate
    dl_client_activate.argtypes = [ctypes.c_void_p, ctypes.POINTER(DL_ActivationResult)]
    dl_client_activate.restype = ctypes.c_int

    dl_client_get_current_token = _lib.dl_client_get_current_token
    dl_client_get_current_token.argtypes = [ctypes.c_void_p, ctypes.POINTER(DL_Token)]
    dl_client_get_current_token.restype = ctypes.c_int

    dl_client_is_activated = _lib.dl_client_is_activated
    dl_client_is_activated.argtypes = [ctypes.c_void_p]
    dl_client_is_activated.restype = ctypes.c_int

    dl_client_get_device_id = _lib.dl_client_get_device_id
    dl_client_get_device_id.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t]
    dl_client_get_device_id.restype = ctypes.c_int

    dl_client_get_device_state = _lib.dl_client_get_device_state
    dl_client_get_device_state.argtypes = [ctypes.c_void_p]
    dl_client_get_device_state.restype = ctypes.c_int

    dl_client_verify_token_trust_chain = _lib.dl_client_verify_token_trust_chain
    dl_client_verify_token_trust_chain.argtypes = [ctypes.c_void_p, ctypes.POINTER(DL_Token), ctypes.POINTER(DL_VerificationResult)]
    dl_client_verify_token_trust_chain.restype = ctypes.c_int

    dl_client_shutdown = _lib.dl_client_shutdown
    dl_client_shutdown.argtypes = [ctypes.c_void_p]
    dl_client_shutdown.restype = ctypes.c_int
