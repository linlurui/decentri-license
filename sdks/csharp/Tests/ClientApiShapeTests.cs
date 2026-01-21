using System;
using System.Reflection;
using Xunit;

namespace DecentriLicense.Tests
{
    public class ClientApiShapeTests
    {
        [Fact]
        public void DecentriLicenseClient_ShouldExposeFiveStepApi()
        {
            var t = typeof(DecentriLicense.DecentriLicenseClient);

            Assert.NotNull(t.GetMethod("Initialize", new[] { typeof(DecentriLicense.ClientConfig) }));
            Assert.NotNull(t.GetMethod("SetProductPublicKey", new[] { typeof(string) }));
            Assert.NotNull(t.GetMethod("ImportToken", new[] { typeof(string) }));
            Assert.NotNull(t.GetMethod("OfflineVerifyCurrentToken", Type.EmptyTypes));
            Assert.NotNull(t.GetMethod("ActivateBindDevice", Type.EmptyTypes));
            Assert.NotNull(t.GetMethod("GetStatus", Type.EmptyTypes));
            Assert.NotNull(t.GetMethod("RecordUsage", new[] { typeof(string) }));
        }

        [Fact]
        public void NativeMethods_ShouldNotBeTouchedInTests()
        {
            // This test intentionally avoids instantiating DecentriLicenseClient,
            // so no native library load is required for CI/verification.
            var t = typeof(DecentriLicense.DecentriLicenseClient);
            Assert.True(t.GetConstructors(BindingFlags.Public | BindingFlags.Instance).Length > 0);
        }
    }
}
