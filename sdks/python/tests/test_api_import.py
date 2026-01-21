import os
import unittest


class TestApiImport(unittest.TestCase):
    def test_import_without_native(self):
        os.environ["DECENLICENSE_SKIP_LOAD"] = "1"

        import decenlicense

        self.assertTrue(hasattr(decenlicense, "DecentriLicenseClient"))
        self.assertTrue(hasattr(decenlicense, "LicenseError"))


if __name__ == "__main__":
    unittest.main()
