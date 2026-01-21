
using System;

namespace DecentriLicense
{
    public class LicenseException : Exception
    {
        public LicenseException(string message) : base(message)
        {
        }

        public LicenseException(string message, Exception innerException) : base(message, innerException)
        {
        }
    }
}

