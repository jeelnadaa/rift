"""Tests for scripts/next-version.py.   python3 tests/test_next_version.py"""
import importlib.util
import os
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location("nv", os.path.join(HERE, "..", "scripts", "next-version.py"))
nv = importlib.util.module_from_spec(spec)
spec.loader.exec_module(nv)


class NextVersion(unittest.TestCase):
    def test_first_release(self):
        self.assertEqual(nv.next_version([]), "1.0.0")
        self.assertEqual(nv.next_version(["nightly", "v1.2"]), "1.0.0")

    def test_patch_increments(self):
        self.assertEqual(nv.next_version(["v1.0.0"]), "1.0.1")
        self.assertEqual(nv.next_version(["v1.0.9", "v1.0.10", "v1.0.2"]), "1.0.11")  # numeric, not lexical

    def test_highest_wins(self):
        self.assertEqual(nv.next_version(["v2.0.0", "v1.9.9"]), "2.0.1")

    def test_floor_bumps_minor_or_major(self):
        self.assertEqual(nv.next_version(["v1.0.4"], (1, 1)), "1.1.0")
        self.assertEqual(nv.next_version(["v1.3.4"], (2, 0)), "2.0.0")
        self.assertEqual(nv.next_version(["v1.3.4"], (1, 2)), "1.3.5")  # floor below latest is ignored
        self.assertEqual(nv.next_version([], (3, 1)), "3.1.0")

    def test_ignores_junk(self):
        self.assertEqual(nv.next_version(["v1.0.0-rc1", "v1.0.0", "x1.0.5"]), "1.0.1")


if __name__ == "__main__":
    unittest.main(verbosity=1)
