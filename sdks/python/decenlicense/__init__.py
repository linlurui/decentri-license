"""
DecentriLicense Python SDK
=========================

A Python wrapper for the DecentriLicense C++ SDK.
"""

from ._decenlicense import *
from .client import DecentriLicenseClient, LicenseError

__version__ = "1.0.0"
__author__ = "Linlurui"
__all__ = ["DecentriLicenseClient", "LicenseError"]