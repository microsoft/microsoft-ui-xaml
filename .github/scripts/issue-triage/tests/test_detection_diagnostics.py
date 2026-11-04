import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from detection_diagnostics import diagnose


class DetectionDiagnosticsTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.detector = self.root / "threat-detection"
        self.detector.mkdir()
        self.receipt = self.detector / "execution.json"
        self.receipt.write_text(json.dumps({
            "component": "detection", "run_id": 123, "run_attempt": 2, "state": "started",
        }), encoding="utf-8")
        self.usage = self.root / "sandbox/firewall/logs/api-proxy-logs/token-usage.jsonl"
        self.usage.parent.mkdir(parents=True)

    def write_usage(self, *charges):
        total = 0
        rows = []
        for index, charge in enumerate(charges):
            total += charge
            rows.append({
                "request_id": str(index), "ai_credits_this_response": charge,
                "ai_credits_total": total,
            })
        self.usage.write_text("\n".join(map(json.dumps, rows)), encoding="utf-8")

    def test_cutoff_is_distinguished_from_other_missing_verdicts_without_creating_one(self):
        for charges, reason in (
            ((8.90702, 1.17237), "missing_result"),
            ((19.5, 0.5), "credit_budget_exhausted"),
            ((19.5, 0.6), "credit_budget_exhausted"),
        ):
            with self.subTest(charges=charges):
                self.write_usage(*charges)
                result = diagnose(self.root, 20, 123, 2)
                self.assertEqual(result["reason"], reason)
                self.assertAlmostEqual(result["ai_credits"], sum(charges))
                self.assertFalse((self.detector / "detection_result.json").exists())

    def test_existing_result_is_reported_as_present_not_replaced_or_declared_safe(self):
        self.write_usage(20.1)
        verdict = self.detector / "detection_result.json"
        original = '{"prompt_injection":true}'
        verdict.write_text(original, encoding="utf-8")
        result = diagnose(self.root, 20, 123, 2)
        self.assertEqual(result["reason"], "result_file_present")
        self.assertNotIn("detection_success", result)
        self.assertEqual(verdict.read_text(encoding="utf-8"), original)

    def test_missing_accounting_is_unknown_not_zero_or_an_authentication_claim(self):
        result = diagnose(self.root, 20, 123, 2)
        self.assertEqual(result["reason"], "usage_unavailable")
        self.assertIsNone(result["ai_credits"])
        self.receipt.unlink()
        self.assertEqual(diagnose(self.root, 20, 123, 2)["reason"], "execution_not_recorded")

    def test_duplicate_or_inconsistent_charges_and_wrong_attempt_are_rejected(self):
        self.write_usage(2, 3)
        valid = self.usage.read_text(encoding="utf-8")
        for invalid in (
            valid.replace('"request_id": "1"', '"request_id": "0"'),
            valid.replace('"ai_credits_total": 5', '"ai_credits_total": 4'),
            valid.replace('"ai_credits_this_response": 3', '"ai_credits_this_response": true'),
            valid.replace('"ai_credits_this_response": 3', '"ai_credits_this_response": NaN'),
            valid.replace('"ai_credits_total": 5', '"ai_credits_total": -5'),
            valid + "\n[]",
            valid + "\n{",
        ):
            with self.subTest(invalid=invalid):
                self.usage.write_text(invalid, encoding="utf-8")
                with self.assertRaises(ValueError):
                    diagnose(self.root, 20, 123, 2)
        self.write_usage(2, 3)
        with self.assertRaisesRegex(ValueError, "identity"):
            diagnose(self.root, 20, 123, 3)

    def test_nonexecuted_detector_does_not_count_inherited_agent_usage(self):
        self.write_usage(25)
        self.receipt.write_text(json.dumps({
            "component": "detection", "run_id": 123, "run_attempt": 2, "state": "not_started",
        }), encoding="utf-8")
        result = diagnose(self.root, 20, 123, 2)
        self.assertEqual(result["reason"], "execution_not_started")
        self.assertIsNone(result["ai_credits"])
