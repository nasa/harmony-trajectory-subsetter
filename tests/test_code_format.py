from pathlib import Path
from unittest import TestCase

from pycodestyle import StyleGuide


class TestCodeFormat(TestCase):
    """This test class should ensure all Harmony service Python code adheres
    to standard Python code styling.

    """

    @classmethod
    def setUpClass(cls):
        cls.python_files = Path("harmony_service").rglob("*.py")

    def test_pycodestyle_adherence(self):
        """Ensure all code in the `harmony_service` directory adheres to PEP8
        defined standard.

        """
        style_guide = StyleGuide(ignore=["E501", "W503"])
        results = style_guide.check_files(self.python_files)
        self.assertEqual(results.total_errors, 0, "Found code style issues.")
