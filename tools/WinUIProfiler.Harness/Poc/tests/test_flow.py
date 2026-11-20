from datetime import timedelta
import json
from pathlib import Path
import shutil
import subprocess
import sys
import unittest
from unittest.mock import patch

from helpers import flow
from helpers.metrics import MeasurementError
from helpers.profiler_query import validate_decision
from helpers.validate_fix import compare, digest, load_json, verify as validate_fix
from tests import test_metrics as fixtures


class FlowTests(unittest.TestCase):
    def setUp(self):
        self.fixture = fixtures.MetricsTests()
        self.fixture.setUp()
        self.addCleanup(self.fixture.doCleanups)
        self.root = Path(self.fixture.directory.name)
        self.source = self.root / "source"
        self.source.mkdir()
        self.scenario = {**self.fixture.scenario,
                         "sourceFiles": ["App.xaml.cs", "ScenarioWorkload.cs", "scenario.json"]}
        flow.write_json(self.source / "scenario.json", self.scenario)
        (self.source / "App.xaml.cs").write_text("protected synthetic caller", encoding="utf-8")
        (self.source / "ScenarioWorkload.cs").write_text("synthetic original workload", encoding="utf-8")
        for name in (".gitignore", "README.md"):
            (self.source / name).write_text("fixture", encoding="utf-8")
        flow.write_json(self.source / "measurement-contract.json", {"artifactsRoot": str(self.root / "measurements")})
        self.baseline = self.capture("baseline", 40, self.source)
        self.measured = {"captureManifest": str(self.baseline), "report": "synthetic baseline report"}
        flow.write_json(self.root / "baseline.json", self.measured)
        self.state_path = self.root / "state.json"
        flow.write_json(self.state_path, {
            "Schema": "winui-profiler-ade-poc/state-v1", "Scenario": "Control",
            "SampleRoot": str(self.source), "BaselineMeasurement": str(self.root / "baseline.json"),
            "ScenarioPlanSha256": "fixture-plan-hash", "Investigation": None,
        })
        self.adapter = Path(__file__).resolve().parents[1] / "Invoke-AdePoc.ps1"
        self.path = flow.stage(self.state_path, "Is this synthetic work avoidable?", self.adapter, 3, 3600)
        self.data = load_json(self.path)
        self.recommendation = {
            "status": "RecommendationReady", "reason": "Synthetic evidence only",
            "recommendation": {
                "change": "Remove redundant synthetic work", "expectedImpact": "39 fewer passes",
                "editableFiles": ["ScenarioWorkload.cs"], "validationScenario": "Unchanged frozen scenario",
                "markers": "Unchanged fixture markers",
                "validation": {"metric": "layoutPassCount", "minimumReduction": 39},
            },
        }

    def capture(self, name, passes, source):
        path, manifest = self.fixture.make_capture(name, passes)
        trace = path.parent / "fixture-only.etl"
        trace.write_bytes(b"synthetic trace placeholder; SQLite is a fixture")
        for run in manifest["iterations"]:
            run["trace"] = str(trace)
        manifest["profilerSha256"] = "fixture-profiler"
        manifest["sourceHashes"] = {name: digest(source / name) for name in self.scenario["sourceFiles"]}
        flow.write_json(path, manifest)
        return path

    def fake_ade(self, decisions):
        self.ade_calls = []
        self.evidence = {}

        def invoke(path, data, action, state_path, phase, **kwargs):
            self.ade_calls.append((action, phase))
            state = load_json(state_path)
            if action == "Investigate":
                index = data["analysisCycles"]
                output = state_path.parent / "workcopy" / "ade-output"
                output.mkdir(parents=True)
                decision = decisions[index - 1]
                flow.write_json(output / "decision.json", decision)
                (output / "investigation.md").write_text("Synthetic diagnosis, not live profiling", encoding="utf-8")
                (output / "queries.jsonl").write_text("", encoding="utf-8")
                evidence = {
                    "diagnosis": str(output / "investigation.md"),
                    "diagnosisSha256": digest(output / "investigation.md"),
                    "decisionPath": str(output / "decision.json"),
                    "decisionSha256": digest(output / "decision.json"), "decision": decision,
                }
                self.evidence[output.parent] = evidence
                state["Investigation"] = {"RunId": f"analysis-{index}", "ProposalSha256": f"hash-{index}",
                                          "Report": "fixture analysis proposal", "Evidence": evidence}
            elif action == "Propose":
                state.update(RunId="fix", ProposalSha256="fix-hash", Report="fixture fix proposal")
            elif action == "Run" and phase == "Fix":
                candidate_run = self.root / "candidate-run"
                candidate = candidate_run / "workcopy"
                shutil.copytree(Path(data["sourceRoot"]), candidate)
                (candidate / "ScenarioWorkload.cs").write_text("synthetic optimized workload", encoding="utf-8")
                self.candidate_manifest = self.capture("candidate", 1, candidate)
                state["FixRunDir"] = str(candidate_run)
            flow.write_json(state_path, state)
            return {"stopReason": "Passed"}
        return invoke

    def test_complete_loop_requires_three_separate_approvals_and_recounts_real_fixtures(self):
        invoke = self.fake_ade([self.recommendation, {"status": "Accepted", "reason": "Controlled fixture supports cause"}])
        with patch("helpers.flow.invoke", side_effect=invoke), patch(
                "helpers.flow.verify_queries", side_effect=lambda root: self.evidence[root]), patch(
                "helpers.validate_fix.measure", side_effect=lambda root: {
                    "captureManifest": str(self.candidate_manifest),
                    "report": str(self.candidate_manifest.parent / "report.md")}):
            flow.advance(self.path, self.data)
            self.assertEqual([("Investigate", "Investigation")], self.ade_calls)
            flow.advance(self.path, self.data)  # No approval: no model execution.
            self.assertEqual(1, len(self.ade_calls))
            with self.assertRaisesRegex(MeasurementError, "exact ADE proposal"):
                flow.advance(self.path, self.data, "wrong")
            flow.advance(self.path, self.data, "hash-1")
            self.assertEqual("Fix", self.data["pending"]["phase"])
            self.assertNotIn(("Run", "Fix"), self.ade_calls)
            flow.advance(self.path, self.data, "fix-hash")
            self.assertEqual("validation", self.data["phase"])
            self.assertEqual("Investigation", self.data["pending"]["phase"])
            self.assertEqual("Passed", self.data["comparison"]["status"])
            self.assertEqual(39, self.data["comparison"]["iterations"][0]["reduction"])
            self.assertNotEqual(self.source, Path(self.data["sourceRoot"]))
            flow.advance(self.path, self.data, "hash-2")
            self.assertEqual("Accepted", self.data["status"])
            self.assertEqual(3, sum(action == "Approve" for action, _ in self.ade_calls))
            self.assertEqual("synthetic original workload", (self.source / "ScenarioWorkload.cs").read_text())

    def test_absent_problem_stops_without_a_fix(self):
        invoke = self.fake_ade([{"status": "NotDemonstrated", "reason": "No avoidable work observed"}])
        with patch("helpers.flow.invoke", side_effect=invoke), patch(
                "helpers.flow.verify_queries", side_effect=lambda root: self.evidence[root]):
            flow.advance(self.path, self.data)
            flow.advance(self.path, self.data, "hash-1")
        self.assertEqual("NotDemonstrated", self.data["status"])
        self.assertNotIn(("Propose", "Fix"), self.ade_calls)

    def test_refined_measurement_repeats_identical_scenario_then_respects_cycle_limit(self):
        decision = {"status": "NeedsMeasurement", "reason": "Need repeatability",
                    "additionalMeasurements": [{"kind": "repeat", "question": "Does the count recur?"}]}
        self.data["maxAnalysisCycles"] = 2
        invoke = self.fake_ade([decision, decision])
        repeat = self.capture("repeat", 40, Path(self.data["sourceRoot"]))
        with patch("helpers.flow.invoke", side_effect=invoke), patch(
                "helpers.flow.verify_queries", side_effect=lambda root: self.evidence[root]), patch(
                "helpers.flow.measure", return_value={"captureManifest": str(repeat)}) as measure:
            flow.advance(self.path, self.data)
            flow.advance(self.path, self.data, "hash-1")
            self.assertEqual("Does the count recur?", self.data["question"])
            measure.assert_called_once_with(Path(self.data["sourceRoot"]))
            flow.advance(self.path, self.data, "hash-2")
            self.assertEqual("Inconclusive", self.data["status"])
            self.assertEqual(1, measure.call_count)

    def test_unsupported_instrumentation_is_not_substituted_with_a_repeat(self):
        invoke = self.fake_ade([{"status": "NeedsMeasurement", "reason": "Need CPU stacks",
                                "additionalMeasurements": [{"kind": "unsupported", "question": "Capture CPU stacks"}]}])
        with patch("helpers.flow.invoke", side_effect=invoke), patch(
                "helpers.flow.verify_queries", side_effect=lambda root: self.evidence[root]), patch(
                "helpers.flow.measure") as measure:
            flow.advance(self.path, self.data)
            flow.advance(self.path, self.data, "hash-1")
            self.assertEqual("NeedsInput", self.data["status"])
            measure.assert_not_called()

    def test_deadline_and_interrupted_operation_never_reexecute(self):
        self.data["deadlineUtc"] = (flow.now() - timedelta(seconds=1)).isoformat()
        with patch("helpers.flow.invoke") as invoke:
            self.assertEqual("TimedOut", flow.advance(self.path, self.data)["status"])
            invoke.assert_not_called()
        self.data.update(status="Ready", runningOperation="interrupted native run")
        self.assertEqual("RecoveryRequired", flow.advance(self.path, self.data)["status"])

    def test_consumed_approval_is_not_replayed_after_a_crash_between_steps(self):
        self.data.update(status="AwaitingApproval", pending={"approvalConsumed": True})
        with patch("helpers.flow.invoke") as invoke:
            self.assertEqual("RecoveryRequired", flow.advance(self.path, self.data)["status"])
            invoke.assert_not_called()
        self.data.update(status="Running", pending=None, runningOperation=None)
        with patch("helpers.flow.invoke") as invoke:
            self.assertEqual("RecoveryRequired", flow.advance(self.path, self.data)["status"])
            invoke.assert_not_called()

    def test_changed_evidence_is_not_consumed_and_lock_excludes_another_advancer(self):
        retained = Path(load_json(self.baseline)["iterations"][0]["appResult"])
        retained.write_text("changed", encoding="utf-8")
        with patch("helpers.flow.invoke") as invoke:
            with self.assertRaisesRegex(MeasurementError, "evidence changed"):
                flow.advance(self.path, self.data)
            invoke.assert_not_called()
        with flow.locked(self.path):
            with self.assertRaisesRegex(MeasurementError, "already advancing"):
                with flow.locked(self.path):
                    self.fail("Concurrent lock unexpectedly acquired")

    def test_relative_gate_rejects_regressions_changed_contract_and_reused_baseline(self):
        candidate_source = self.root / "candidate-source"
        shutil.copytree(self.source, candidate_source)
        (candidate_source / "ScenarioWorkload.cs").write_text("candidate", encoding="utf-8")
        candidate = self.capture("regression", 41, candidate_source)
        rule = {"metric": "layoutPassCount", "minimumReduction": 1}
        self.assertEqual("Failed", compare(self.baseline, candidate, self.scenario, rule)["status"])
        manifest = load_json(candidate)
        manifest["sourceHashes"]["App.xaml.cs"] = "changed"
        flow.write_json(candidate, manifest)
        with self.assertRaisesRegex(MeasurementError, "only allowed app source"):
            compare(self.baseline, candidate, self.scenario, rule)
        with self.assertRaisesRegex(MeasurementError, "only allowed app source"):
            compare(self.baseline, self.baseline, self.scenario, rule)
        with self.assertRaisesRegex(MeasurementError, "integer minimumReduction"):
            compare(self.baseline, candidate, self.scenario, {"metric": "layoutPassCount", "minimumReduction": True})

    def test_baseline_check_does_not_capture_and_unexpected_compile_source_is_rejected(self):
        source = Path(self.data["sourceRoot"])
        flow.write_json(source / "fix-validation.json", {
            "schema": "winui-profiler-harness/fix-validation-v1", "baselineManifest": str(self.baseline),
            "inputs": self.data["inputs"], "allowedFiles": self.data["allowedFiles"],
            "protectedHashes": {name: digest(source / name) for name in self.data["allowedFiles"]
                                if name not in self.scenario["editableFiles"]},
            "rule": {"metric": "layoutPassCount", "minimumReduction": 1},
        })
        with patch("helpers.validate_fix.measure") as measure:
            self.assertEqual("Failed", validate_fix(source)["status"])
            measure.assert_not_called()
            (source / "Bypass.cs").write_text("unexpected compile item", encoding="utf-8")
            with self.assertRaisesRegex(MeasurementError, "Unexpected file"):
                validate_fix(source)
            measure.assert_not_called()

    def test_capture_failure_is_persisted_as_environment_failure_not_a_rejected_hypothesis(self):
        self.data["status"] = "RepeatMeasurement"
        flow.persist(self.path, self.data)
        with patch.object(sys, "argv", ["flow", "ContinueFlow", "--adapter", str(self.adapter),
                                       "--flow", str(self.path)]), patch(
                "helpers.flow.measure", side_effect=MeasurementError("Lost ETW data")):
            self.assertEqual(2, flow.main())
        result = load_json(self.path)
        self.assertEqual("EnvironmentFailure", result["status"])
        self.assertIn("Lost ETW data", result["reason"])

    def test_native_failed_check_is_a_named_non_success_not_an_unparseable_command_failure(self):
        def native(command, stdout, **kwargs):
            stdout.write('ADE-RESULT: {"stopReason":"ChecksFailed"}\n')
            stdout.flush()
            return subprocess.CompletedProcess(command, 1)
        with patch("helpers.flow.subprocess.run", side_effect=native):
            result = flow.invoke(self.path, self.data, "Run", self.state_path, "Fix")
        flow.finish_fix(self.path, self.data, self.state_path, result)
        self.assertEqual("FixNotValidated", self.data["status"])
        self.data["status"] = "Running"
        flow.finish_fix(self.path, self.data, self.state_path, {"stopReason": "EnvironmentFailure"})
        self.assertEqual("EnvironmentFailure", self.data["status"])

    def test_decisions_require_phase_appropriate_evidence_and_executable_contracts(self):
        context = {"phase": "diagnosis", "editableFiles": ["ScenarioWorkload.cs"]}
        validate_decision(self.recommendation, context)
        for decision in (
            {"status": "RecommendationReady", "reason": "No rule"},
            {"status": "NeedsMeasurement", "reason": "Unstructured", "additionalMeasurements": ["repeat"]},
            {"status": "Accepted", "reason": "Premature"},
        ):
            with self.assertRaises(MeasurementError):
                validate_decision(decision, context)
        with self.assertRaisesRegex(MeasurementError, "independently passing"):
            validate_decision({"status": "Accepted", "reason": "No passing gate"},
                              {"phase": "validation", "comparisonPassed": False})


if __name__ == "__main__":
    unittest.main()
