from collections import defaultdict
from pathlib import Path
import sys
import unittest
from unittest.mock import patch


SAMPLE_ROOT = Path(__file__).resolve().parents[1] / "sample"
sys.path.insert(0, str(SAMPLE_ROOT))
import verify


class IndexedCatalog:
    def __init__(self, labels):
        self._index = defaultdict(list)
        for position, label in enumerate(labels):
            self._index[label.casefold()].append(position)

    def find(self, query):
        return tuple(self._index.get(query.casefold(), ()))


class IncorrectCatalog:
    def __init__(self, labels):
        pass

    def find(self, query):
        return ()


class VerificationTests(unittest.TestCase):
    def test_repeated_scanning_exceeds_the_work_budget(self):
        result = verify.run_validation()
        self.assertEqual("Failed", result["status"])
        self.assertTrue(
            all(
                row["labelNormalizations"] > result["maxLabelNormalizationsPerIteration"]
                for row in result["iterations"]
            )
        )

    def test_indexed_lookup_meets_the_same_work_budget(self):
        with patch.object(verify, "Catalog", IndexedCatalog):
            result = verify.run_validation()
        self.assertEqual("Passed", result["status"])
        self.assertTrue(all(row["withinBudget"] for row in result["iterations"]))

    def test_incorrect_results_are_not_reported_as_performance_success(self):
        with patch.object(verify, "Catalog", IncorrectCatalog):
            with self.assertRaises(AssertionError):
                verify.run_validation()


if __name__ == "__main__":
    unittest.main()
