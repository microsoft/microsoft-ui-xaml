from contextlib import closing
import hashlib
import json
from pathlib import Path
import sqlite3
import shutil
import tempfile
import unittest
from unittest.mock import patch

from helpers.metrics import XAML_PROVIDER, MeasurementError, compare_manifests, extract_iteration, markdown_report, summarize
from helpers.profiler_query import query, scenario_events, validate_inputs, verify_queries
from helpers.verify import verify as verify_capture


class MetricsTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.database = Path(self.directory.name) / "fixture.sqlite"
        self.scenario = {
            "name": "Synthetic fixture",
            "markers": {"provider": "fixture-marker-provider", "startEventId": 1, "stopEventId": 2,
                        "startPayload": [40], "stopPayload": [40, True]},
            "expectedResults": {"controlCount": 40},
            "minimumIterations": 10, "maxLayoutPasses": 4,
            "editableFiles": ["ScenarioWorkload.cs"],
            "sourceSnapshots": ["App.xaml.cs", "ScenarioWorkload.cs"],
            "limitations": ["Synthetic fixture, not a capture."],
        }
        self.app = {
            "passed": True,
            "processId": 123,
            "uiThreadId": 456,
            "controlCount": 40,
            "frameworkPath": "fixture-only.dll",
            "frameworkVersion": "fixture-only",
        }
        with closing(sqlite3.connect(self.database)) as db, db:
            db.executescript(
                """
                PRAGMA user_version=1;
                CREATE TABLE SessionInfo(
                    ProcessId INTEGER, QpcFrequency INTEGER, Source TEXT, EndReason TEXT,
                    LostByEtw INTEGER, DroppedByProfiler INTEGER,
                    CompleteFromSeq INTEGER, CompleteFromQpc INTEGER
                );
                INSERT INTO SessionInfo VALUES(123, 1000000, 'Trace', 'End of trace', 0, 0, NULL, NULL);
                CREATE TABLE EventTypes(TypeId INTEGER, ProviderId TEXT, EventId INTEGER, TraceLogging INTEGER);
                CREATE TABLE Events(Seq INTEGER, Qpc INTEGER, ThreadId INTEGER, TypeId INTEGER, Payload TEXT);
                """
            )
            db.executemany(
                "INSERT INTO EventTypes VALUES(?,?,?,0)",
                [(1, self.scenario["markers"]["provider"], 1), (2, self.scenario["markers"]["provider"], 2),
                 (3, XAML_PROVIDER, 41), (4, XAML_PROVIDER, 42),
                 (5, "unrelated-provider", 41), (6, "unrelated-provider", 42)],
            )
            db.executemany(
                "INSERT INTO Events VALUES(?,?,?,?,?)",
                [
                    (1, 10, 456, 3, "[]"), (2, 20, 456, 4, "[]"),
                    (10, 100, 456, 1, json.dumps([40])),
                    (20, 200, 456, 3, "[]"),
                    (21, 210, 999, 3, "[]"), (22, 220, 456, 5, "[]"),
                    (28, 480, 456, 6, "[]"), (29, 490, 999, 4, "[]"),
                    (30, 500, 456, 4, "[]"),
                    (40, 900, 456, 2, json.dumps([40, True])),
                ],
            )

    def change(self, statement):
        with closing(sqlite3.connect(self.database)) as db, db:
            db.execute(statement)

    def test_scopes_to_process_ui_thread_provider_and_markers(self):
        result = extract_iteration(self.database, self.app, self.scenario)
        self.assertEqual(result["layoutPassCount"], 1)
        self.assertEqual(result["scenarioMs"], 0.8)
        self.assertEqual(result["layoutBusyMs"], 0.3)

    def test_counts_nested_passes_without_double_counting_busy_time(self):
        self.change("INSERT INTO Events VALUES(24,240,456,3,'[]')")
        self.change("INSERT INTO Events VALUES(26,260,456,4,'[]')")
        result = extract_iteration(self.database, self.app, self.scenario)
        self.assertEqual(result["layoutPassCount"], 2)
        self.assertEqual(result["layoutBusyMs"], 0.3)

    def test_uses_the_supplied_marker_and_correctness_contract(self):
        self.scenario["markers"] = {
            "provider": "another-app", "startEventId": 101, "stopEventId": 102,
            "startPayload": ["open"], "stopPayload": ["complete", True],
        }
        self.scenario["expectedResults"] = {"itemCount": 3}
        self.app["itemCount"] = 3
        del self.app["controlCount"]
        self.change("UPDATE EventTypes SET ProviderId='another-app',EventId=101 WHERE TypeId=1")
        self.change("UPDATE EventTypes SET ProviderId='another-app',EventId=102 WHERE TypeId=2")
        self.change("""UPDATE Events SET Payload='["open"]' WHERE Seq=10""")
        self.change("""UPDATE Events SET Payload='["complete",true]' WHERE Seq=40""")
        result = extract_iteration(self.database, self.app, self.scenario)
        self.assertEqual(3, result["itemCount"])
        self.assertNotIn("controlCount", result)

    def test_rejects_missing_or_ambiguous_markers(self):
        for statement in (
            "DELETE FROM Events WHERE Seq=40",
            "INSERT INTO Events VALUES(50,1000,456,2,'[40,true]')",
            "UPDATE Events SET ThreadId=999 WHERE Seq=40",
        ):
            with self.subTest(statement=statement):
                with closing(sqlite3.connect(self.database)) as db, db:
                    db.execute(statement)
                try:
                    with self.assertRaises(MeasurementError):
                        extract_iteration(self.database, self.app, self.scenario)
                finally:
                    with closing(sqlite3.connect(self.database)) as db, db:
                        db.execute("DELETE FROM Events WHERE Seq IN (40,50)")
                        db.execute("INSERT INTO Events VALUES(40,900,456,2,'[40,true]')")

    def test_rejects_lost_trimmed_or_wrong_process_sessions(self):
        for statement in (
            "UPDATE SessionInfo SET LostByEtw=1",
            "UPDATE SessionInfo SET DroppedByProfiler=1",
            "UPDATE SessionInfo SET CompleteFromSeq=1",
            "UPDATE SessionInfo SET ProcessId=999",
        ):
            with self.subTest(statement=statement):
                self.change(statement)
                with self.assertRaises(MeasurementError):
                    extract_iteration(self.database, self.app, self.scenario)
                self.change(
                    "UPDATE SessionInfo SET ProcessId=123,LostByEtw=0,"
                    "DroppedByProfiler=0,CompleteFromSeq=NULL"
                )

    def test_rejects_missing_or_unbalanced_layout_instead_of_reporting_zero(self):
        self.change("DELETE FROM Events WHERE Seq=30")
        with self.assertRaises(MeasurementError):
            extract_iteration(self.database, self.app, self.scenario)
        self.change("DELETE FROM Events WHERE Seq=20")
        with self.assertRaises(MeasurementError):
            extract_iteration(self.database, self.app, self.scenario)

    def test_rejects_incorrect_control_tree(self):
        self.app["controlCount"] = 39
        with self.assertRaises(MeasurementError):
            extract_iteration(self.database, self.app, self.scenario)

    def test_zero_layout_requires_opt_in_and_positive_process_thread_provider_evidence(self):
        self.change("DELETE FROM Events WHERE Seq IN (20,30)")
        with self.assertRaises(MeasurementError):
            extract_iteration(self.database, self.app, self.scenario)
        self.scenario["allowZeroLayout"] = True
        self.assertEqual(0, extract_iteration(self.database, self.app, self.scenario)["layoutPassCount"])
        self.change("UPDATE Events SET ThreadId=999 WHERE Seq IN (1,2)")
        with self.assertRaises(MeasurementError):
            extract_iteration(self.database, self.app, self.scenario)

    def test_zero_layout_opt_in_does_not_accept_missing_or_unbalanced_telemetry(self):
        self.scenario["allowZeroLayout"] = True
        self.change("DELETE FROM Events WHERE Seq=30")
        with self.assertRaises(MeasurementError):
            extract_iteration(self.database, self.app, self.scenario)
        self.change("DELETE FROM Events WHERE TypeId IN (3,4)")
        with self.assertRaises(MeasurementError):
            extract_iteration(self.database, self.app, self.scenario)

    def test_rejects_changed_plan_identity_or_incomplete_action_execution(self):
        self.scenario["expectedResults"].update(scenarioPlanSha256="frozen-plan", scenarioStepCount=4)
        self.app.update(scenarioPlanSha256="frozen-plan", scenarioStepCount=4)
        self.assertEqual("frozen-plan", extract_iteration(self.database, self.app, self.scenario)["scenarioPlanSha256"])
        for name, value in (("scenarioPlanSha256", "different"), ("scenarioStepCount", 3)):
            with self.subTest(name=name):
                expected = self.app[name]
                self.app[name] = value
                with self.assertRaises(MeasurementError):
                    extract_iteration(self.database, self.app, self.scenario)
                self.app[name] = expected

    def test_scenario_drilldown_excludes_other_threads_and_outside_markers(self):
        self.change(
            "CREATE TABLE EventDetails(Seq INTEGER, ThreadId INTEGER, Ms REAL, Provider TEXT, "
            "Name TEXT, Opcode TEXT, Fields TEXT, Mentions TEXT)"
        )
        self.change(
            "INSERT INTO EventDetails VALUES"
            "(5,456,0.1,'fixture','Work','Start','{}','[]'),"
            "(20,456,0.2,'fixture','Work','Start','{}','[]'),"
            "(21,999,0.3,'fixture','Work','Start','{}','[]'),"
            "(30,456,0.5,'fixture','Work','Stop','{}','[]'),"
            "(50,456,1.0,'fixture','Work','Stop','{}','[]')"
        )
        scope = extract_iteration(self.database, self.app, self.scenario)
        result = json.loads(scenario_events(scope, "work"))
        self.assertEqual([20, 30], [row["Seq"] for row in result["events"]])
        self.assertEqual(2, sum(row["Count"] for row in result["counts"]))

    def test_query_failure_is_not_reported_as_empty_success(self):
        scope = extract_iteration(self.database, self.app, self.scenario)
        scope["iteration"] = 1
        root = Path(self.directory.name)
        with patch("helpers.profiler_query.validate_inputs", return_value=(
            {"profilerCli": "fixture-only.exe"}, {"iterations": [scope]}
        )), patch("helpers.profiler_query.subprocess.run") as run:
            run.return_value.returncode = 3
            run.return_value.stderr = "fixture read failure"
            with self.assertRaisesRegex(MeasurementError, "fixture read failure"):
                query(root, "overview", 1, None, None)

    def test_investigation_requires_drilldown_and_detects_changed_receipts(self):
        scope = extract_iteration(self.database, self.app, self.scenario)
        scope["iteration"] = 1
        root = Path(self.directory.name)
        with patch("helpers.profiler_query.validate_inputs", return_value=(
            {"profilerCli": "fixture-only.exe"}, {"iterations": [scope]}
        )), patch("helpers.profiler_query.subprocess.run") as run:
            run.return_value.returncode = 0
            run.return_value.stderr = ""
            run.return_value.stdout = "explicitly synthetic profiler response"
            overview = query(root, "overview", 1, None, None)
            (root / "ade-output" / "investigation.md").write_text("Fixture diagnosis", encoding="utf-8")
            (root / "ade-output" / "decision.json").write_text(json.dumps({
                "status": "RecommendationReady", "reason": "Fixture-only recommendation"
            }), encoding="utf-8")
            with self.assertRaisesRegex(MeasurementError, "drilldown"):
                verify_queries(root)
            query(root, "events", 1, None, "Work")
            self.assertEqual("EvidenceVerified", verify_queries(root)["status"])
            Path(overview["receipt"]).write_text("changed", encoding="utf-8")
            with self.assertRaisesRegex(MeasurementError, "receipt changed"):
                verify_queries(root)

    def make_capture(self, name, passes):
        root = Path(self.directory.name) / name
        root.mkdir()
        executable = root / "fixture-only.exe"
        executable.write_bytes(b"synthetic fixture, not an executable")
        runs = []
        for iteration in range(1, 11):
            database = root / f"{iteration}.sqlite"
            shutil.copyfile(self.database, database)
            with closing(sqlite3.connect(database)) as db, db:
                db.execute("DELETE FROM Events WHERE Seq IN (20,30)")
                db.execute("UPDATE Events SET Seq=1000,Qpc=9000 WHERE Seq=40")
                db.executemany("INSERT INTO Events VALUES(?,?,?,?,?)", [
                    (100 + 2 * index + offset, 1000 + 20 * index + offset, 456, 3 + offset, "[]")
                    for index in range(passes) for offset in range(2)
                ])
            app_path = root / f"{iteration}.json"
            app_path.write_text(json.dumps(self.app), encoding="utf-8")
            runs.append({"iteration": iteration, "database": str(database), "appResult": str(app_path),
                         "frameworkSha256": "fixture-framework", "trace": "fixture-only.etl"})
        manifest = {
            "executable": str(executable), "executableSha256": hashlib.sha256(executable.read_bytes()).hexdigest(),
            "iterations": runs, "sourceHashes": {"App.xaml.cs": "same", "ScenarioWorkload.cs": name},
        }
        path = root / "capture-manifest.json"
        path.write_text(json.dumps(manifest), encoding="utf-8")
        return path, manifest

    def test_comparison_recounts_each_database_and_rejects_different_framework(self):
        baseline_path, _ = self.make_capture("baseline", 40)
        candidate_path, candidate = self.make_capture("candidate", 1)
        report = compare_manifests(baseline_path, candidate_path, self.scenario)
        self.assertIn("| 1 | 40 | 1 | 40 / 40 |", report)
        candidate["iterations"][0]["frameworkSha256"] = "different-framework"
        candidate_path.write_text(json.dumps(candidate), encoding="utf-8")
        with self.assertRaisesRegex(MeasurementError, "different framework"):
            compare_manifests(baseline_path, candidate_path, self.scenario)

    def test_measurement_only_has_no_performance_pass_or_budget_claim(self):
        _, manifest = self.make_capture("custom", 0)
        self.scenario.update(maxLayoutPasses=None, allowZeroLayout=True)
        result = summarize(manifest, self.scenario)
        self.assertEqual("Measured", result["status"])
        self.assertEqual(10, result["summary"]["iterationCount"])
        self.assertTrue(all(row["withinBudget"] is None for row in result["iterations"]))
        self.assertIn("Not evaluated", markdown_report(result))

    def test_comparison_rejects_repeated_database_or_changed_binary(self):
        baseline_path, _ = self.make_capture("baseline", 40)
        candidate_path, candidate = self.make_capture("candidate", 1)
        candidate["iterations"][1]["database"] = candidate["iterations"][0]["database"]
        candidate_path.write_text(json.dumps(candidate), encoding="utf-8")
        with self.assertRaisesRegex(MeasurementError, "duplicate iterations"):
            compare_manifests(baseline_path, candidate_path, self.scenario)
        Path(candidate["executable"]).write_bytes(b"changed")
        with self.assertRaisesRegex(MeasurementError, "executable changed"):
            compare_manifests(baseline_path, candidate_path, self.scenario)

    def test_verifier_passes_the_source_and_scenario_to_shared_capture(self):
        _, manifest = self.make_capture("capture", 1)
        source = Path(self.directory.name) / "app source"
        source.mkdir()
        (source / "scenario.json").write_text(json.dumps(self.scenario), encoding="utf-8")
        profiler = Path(manifest["executable"])
        (source / "measurement-contract.json").write_text(json.dumps({
            "profilerCli": str(profiler),
            "profilerSha256": hashlib.sha256(profiler.read_bytes()).hexdigest(),
            "initRun": "fixture-init.ps1", "dotnet": "fixture-dotnet.exe",
            "artifactsRoot": str(source / "measurements"),
        }), encoding="utf-8")
        with patch("helpers.verify.subprocess.run") as run:
            def capture(arguments, **kwargs):
                output = Path(arguments[arguments.index("-RunDirectory") + 1])
                output.mkdir(parents=True)
                (output / "capture-manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
                return run.return_value
            run.side_effect = capture
            run.return_value.returncode = 0
            self.assertEqual("Passed", verify_capture(source)["status"])
            arguments = run.call_args.args[0]
            self.assertEqual(str(source.resolve()), arguments[arguments.index("-SourceRoot") + 1])
            self.assertEqual(str((source / "scenario.json").resolve()),
                             arguments[arguments.index("-ScenarioFile") + 1])
            self.assertEqual("helpers", Path(arguments[arguments.index("-File") + 1]).parent.name)

    def test_investigation_validates_configured_source_snapshots(self):
        manifest_path, manifest = self.make_capture("capture", 1)
        root = Path(self.directory.name) / "investigation"
        root.mkdir()
        self.scenario["sourceSnapshots"] = ["OtherApp.cs"]
        (root / "scenario.json").write_text(json.dumps(self.scenario), encoding="utf-8")
        (root / "OtherApp.cs").write_text("synthetic fixture source", encoding="utf-8")
        manifest["sourceHashes"] = {
            name: hashlib.sha256((root / name).read_bytes()).hexdigest()
            for name in ("scenario.json", "OtherApp.cs")
        }
        paths = [manifest_path, Path(manifest["executable"])]
        manifest["profilerSha256"] = hashlib.sha256(paths[1].read_bytes()).hexdigest()
        manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
        (root / "investigation.json").write_text(json.dumps({
            "schema": "winui-profiler-harness/investigation-v1",
            "captureManifest": str(manifest_path), "profilerCli": manifest["executable"],
            "inputs": [{"path": str(path), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}
                       for path in paths],
        }), encoding="utf-8")
        self.assertEqual("Passed", validate_inputs(root)[1]["status"])
        (root / "OtherApp.cs").write_text("changed source", encoding="utf-8")
        with self.assertRaisesRegex(MeasurementError, "not the measured OtherApp.cs"):
            validate_inputs(root)


if __name__ == "__main__":
    unittest.main()
