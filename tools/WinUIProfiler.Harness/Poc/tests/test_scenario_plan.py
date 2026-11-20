import copy
import hashlib
import json
from pathlib import Path
import shutil
import tempfile
import unittest

from ControlDemo.scenario_plan import ScenarioError, read_plan, review, stage_plan, validate_plan


class ScenarioPlanTests(unittest.TestCase):
    def setUp(self):
        self.source = Path(__file__).resolve().parents[1] / "ControlDemo"
        self.capabilities = json.loads((self.source / "scenario-capabilities.json").read_text(encoding="utf-8"))
        self.plan = {
            "schema": self.capabilities["planSchema"], "status": "Ready",
            "request": "Populate 60 buttons, resize to 800 by 600 pixels, scroll to the end, then clear.",
            "name": "Populate, resize, scroll and clear",
            "initialButtonCount": 0,
            "steps": [
                {"action": "populate", "count": 60},
                {"action": "resize", "width": 800, "height": 600},
                {"action": "scroll", "position": "end"},
                {"action": "clear"},
            ],
        }

    def test_tracks_final_count_without_moving_population_into_setup(self):
        result = validate_plan(self.plan, self.capabilities, self.plan["request"])
        self.assertEqual({"status": "Ready", "finalButtonCount": 0, "stepCount": 4}, result)
        self.assertEqual(0, self.plan["initialButtonCount"])

    def test_rejects_unsafe_unknown_incomplete_or_out_of_bounds_steps(self):
        cases = [
            [], [{"action": "click", "target": "Button 1"}], [{"action": "populate"}],
            [{"action": "populate", "count": True}], [{"action": "populate", "count": 0}],
            [{"action": "populate", "count": 201}], [{"action": "clear", "code": "anything"}],
            [{"action": "resize", "width": 800}], [{"action": "resize", "width": 399, "height": 600}],
            [{"action": "resize", "width": 800, "height": 1001}],
            [{"action": "scroll", "position": "end"}],
            [{"action": "populate", "count": 60}, {"action": "scroll", "position": "middle"}],
            [{"action": "clear"}] * 13, [None],
        ]
        for steps in cases:
            with self.subTest(steps=steps):
                plan = {**self.plan, "steps": steps}
                with self.assertRaises(ScenarioError):
                    validate_plan(plan, self.capabilities)

    def test_rejects_unknown_top_level_fields_and_changed_request(self):
        with self.assertRaises(ScenarioError):
            validate_plan({**self.plan, "shell": "anything"}, self.capabilities)
        with self.assertRaisesRegex(ScenarioError, "protected scenario request"):
            validate_plan(self.plan, self.capabilities, "A different request")

    def test_preserves_non_ready_outcomes_without_runnable_fallback(self):
        for status in ("Unsupported", "NeedsClarification"):
            plan = {key: self.plan[key] for key in ("schema", "request")}
            plan.update(status=status, reason="The requested action or parameter is not specified.")
            self.assertEqual(status, validate_plan(plan, self.capabilities)["status"])
            with self.assertRaises(ScenarioError):
                validate_plan({**plan, "steps": self.plan["steps"]}, self.capabilities)
            with tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                shutil.copyfile(self.source / "scenario-capabilities.json", root / "scenario-capabilities.json")
                path = root / "generated.json"
                path.write_text(json.dumps(plan), encoding="utf-8")
                with self.assertRaisesRegex(ScenarioError, status):
                    stage_plan(root, path)
                self.assertFalse((root / "scenario-plan.json").exists())

    def test_freezes_plan_and_compiles_matching_measurement_only_contract(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in ("scenario-capabilities.json", "scenario.json"):
                shutil.copyfile(self.source / name, root / name)
            original = root / "generated.json"
            original.write_text(json.dumps(self.plan), encoding="utf-8")
            result = stage_plan(root, original)
            frozen = root / "scenario-plan.json"
            contract = json.loads((root / "scenario.json").read_text(encoding="utf-8"))
            self.assertEqual(self.plan, read_plan(frozen))
            self.assertEqual(original.read_bytes(), frozen.read_bytes())
            self.assertEqual(hashlib.sha256(frozen.read_bytes()).hexdigest(), result["planSha256"])
            self.assertEqual(result["planSha256"], contract["expectedResults"]["scenarioPlanSha256"])
            self.assertEqual(0, contract["expectedResults"]["controlCount"])
            self.assertEqual(4, contract["expectedResults"]["scenarioStepCount"])
            self.assertEqual([0, True], contract["markers"]["stopPayload"])
            self.assertEqual(["--scenario", "{sourceRoot}\\scenario-plan.json"], contract["launchArguments"][-2:])
            self.assertIsNone(contract["maxLayoutPasses"])
            self.assertIs(True, contract["allowZeroLayout"])
            self.assertIn("scenario-plan.json", contract["sourceSnapshots"])
            with self.assertRaises(FileExistsError):
                stage_plan(root, original)
            original.write_text("changed after staging", encoding="utf-8")
            self.assertEqual(self.plan, read_plan(frozen))

    def test_reviews_chat_plan_against_original_request_and_refuses_a_changed_plan(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            shutil.copyfile(self.source / "scenario-capabilities.json", root / "scenario-capabilities.json")
            request = root / "scenario-request.json"
            request.write_text(json.dumps({"text": self.plan["request"]}), encoding="utf-8")
            path = root / "scenario-plan.json"
            path.write_text(json.dumps(self.plan), encoding="utf-8")
            result = review(root, path, request)
            self.assertEqual("Ready", result["status"])
            self.assertEqual(self.plan["steps"], result["steps"])
            self.assertEqual(10, result["iterations"])
            self.assertEqual("MeasurementOnly", result["mode"])
            changed = copy.deepcopy(self.plan)
            changed["steps"][0]["count"] = 20
            path.write_text(json.dumps(changed), encoding="utf-8")
            with self.assertRaisesRegex(ScenarioError, "changed after review"):
                stage_plan(root, path, result["planSha256"])
            plan = copy.deepcopy(self.plan)
            plan["request"] = "Silently substituted request"
            path.write_text(json.dumps(plan), encoding="utf-8")
            with self.assertRaises(ScenarioError):
                review(root, path, request)

    def test_rejects_duplicate_fields_and_oversized_input(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "plan.json"
            path.write_text('{"status":"Ready","status":"Unsupported"}', encoding="utf-8")
            with self.assertRaisesRegex(ScenarioError, "duplicate"):
                read_plan(path)
            path.write_text(" " * 16385, encoding="utf-8")
            with self.assertRaisesRegex(ScenarioError, "16 KiB"):
                read_plan(path)


if __name__ == "__main__":
    unittest.main()
