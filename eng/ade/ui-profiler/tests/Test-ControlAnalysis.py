"""Synthetic control evidence tests: no capture, SQL, app launch, or native process."""

import copy
import importlib.util
import pathlib
import shutil
import unittest
import uuid


spec = importlib.util.spec_from_file_location(
    "control_analysis", pathlib.Path(__file__).parents[1] / "pack" / "tools" / "Analyze-Control.py")
control = importlib.util.module_from_spec(spec)
spec.loader.exec_module(control)
p = control.profiler


def fixture():
    session = {"ProcessId": 10, "ProcessCreatedUtc": "2026-11-20T00:00:00Z",
               "StartQpc": 1000, "QpcFrequency": 1000}
    row = {"ObjectId": 7, "Kind": "Element", "Type": "Example.ControlX", "Name": "outer",
           "Address": 900, "FirstQpc": 1100, "LastQpc": 1500, "EventCount": 2, "Mentions": 2}
    actions = {"qpcFrequency": 1000, "startedQpc": 1000, "endedQpc": 2000, "steps": [
        {"id": "activate", "state": "passed", "startedQpc": 1050, "endedQpc": 1800}]}
    analysis = {
        "schema": "ui-profiler/analysis/v1", "session": session, "window": {"threadId": 3},
        "windows": [{"id": "scenario", "startQpc": 1000, "endQpc": 2000},
                    {"id": "action:activate", "startQpc": 1050, "endQpc": 1800}],
        "capabilities": {"heap": {"status": "unsupported"}},
        "metrics": [{"family": "memory", "scope": "scenario", "metric": "workingSet.change",
                     "value": 20, "unit": "MiB", "status": "partial"}],
        "details": {
            "objectActivity": {"recording": [row], "action:activate": [copy.deepcopy(row)]},
            "ticks": [{"TickId": 5, "StartMs": 50, "EndMs": 600, "ThreadId": 3, "TickMs": 550}],
            "cpuSampling": {"Samples": 6, "Dropped": 4},
            "cpuModules": [{"SymbolStatus": "NotAttempted"}],
            "eventSpans": [{"startQpc": 1080, "endQpc": 1550, "threadId": 3,
                            "provider": "Xaml", "name": "Layout"}],
        },
    }

    def node(identity, parent, kind="Example.ControlX", name=""):
        return {"id": identity, "parentId": parent, "rootId": 1, "type": kind, "name": name, "properties": {}}

    document = {
        "captureId": "capture", "processId": 10, "creationTimeFileTime": p.filetime(session["ProcessCreatedUtc"]),
        "qpcFrequency": 1000, "startQpc": 1700, "endQpc": 1750,
        "roots": [{"id": 1, "startQpc": 1701, "endQpc": 1749}],
        "xamlNodes": [node(1, None, name="outer"), node(2, 1, "Example.Panel"), node(3, 2, name="inner")],
        "compositionNodes": [node(11, None, "ContainerVisual"), node(12, 11, "SpriteVisual")],
        "limitations": ["public realized nodes only"],
        "resourceSnapshot": {"status": "unavailable", "hResult": -1, "values": None},
    }
    snapshot = {"path": "snapshot.json", "phase": "after-scenario", "histories": [
        {"tree": "Xaml", "nodeId": 1, "matchingIds": 11, "objectId": 7, "status": "matched"},
        {"tree": "Xaml", "nodeId": 2, "matchingIds": [], "objectId": None, "status": "unavailable"},
        {"tree": "Xaml", "nodeId": 3, "matchingIds": [12], "objectId": None, "status": "unavailable"}]}

    def event(qpc, starts, pointer=900, field="ObjectPointer"):
        return {"Event": {"TimestampQpc": qpc, "RelativeMilliseconds": qpc - 1000, "ProcessId": 10,
                          "ThreadId": 3, "Provider": "Xaml", "Name": "FrameworkElement_ApplyTemplate",
                          "Fields": [{"Name": field, "Value": pointer}, {"Name": "IsStart", "Value": starts}]},
                "Fields": [field], "FrameNumber": 1}

    timeline = [{"Object": {"ObjectId": 7, "Address": 900, "Kind": "Element"},
                 "Offset": 0, "Total": 2, "Events": [event(1100, True), event(1300, False)]}]
    return analysis, [(snapshot, document)], {7: timeline}, actions, {"health": {"ended": True}}


def report(data):
    analysis, documents, timelines, actions, measurement = data
    return control.build_report(analysis, "ControlX", documents, timelines, actions, measurement)


class ControlAnalysisTests(unittest.TestCase):
    def test_scoped_template_attribution_and_nonexclusive_tick_context(self):
        result = report(fixture())
        timing = result["time"]["perInstanceCapabilities"]["templating"]
        self.assertEqual(timing["observedElapsed"]["sum"], 200)
        instance = result["memoryObjectsGraphics"]["indexedInstances"][0]
        self.assertEqual(instance["history"]["spans"][0]["actionOverlap"], ["action:activate"])
        self.assertEqual(instance["firstMentionTickSpans"][0]["spans"][0]["elapsedMs"]["sum"], 470)
        self.assertFalse(instance["history"]["actionOverlap"][0]["provesActionTarget"])
        self.assertIsNone(result["cpu"]["controlOwned"]["value"])
        self.assertIsNone(result["memoryObjectsGraphics"]["ownedHeapBytes"]["value"])
        self.assertEqual(result["cpu"]["quality"]["droppedPercent"], 40)

    def test_snapshot_subtrees_use_canonical_correlations_and_do_not_guess_identity(self):
        result = report(fixture())
        outer, inner = result["memoryObjectsGraphics"]["snapshots"][0]["instances"]
        self.assertEqual(outer["canonicalObjectId"], 7)
        self.assertIsNone(inner["canonicalObjectId"])
        self.assertEqual(inner["placement"], "nested-same-type")
        self.assertEqual(inner["sameTypeAncestorNodeIds"], [1])
        self.assertEqual(outer["subtree"]["xamlElements"], 3)
        self.assertEqual(outer["subtree"]["correlatedVisualDescendants"], 2)
        self.assertEqual(inner["subtree"]["correlatedVisualDescendants"], 1)
        self.assertEqual(outer["subtree"]["status"], "partial")

    def test_missing_data_never_means_zero_or_owned_bytes(self):
        data = fixture()
        data[0]["details"]["objectActivity"] = {}
        data[0]["details"]["cpuSampling"] = None
        data[1].clear()
        data[2].clear()
        result = report(data)
        for name in ("templating", "measure", "arrange"):
            self.assertIsNone(result["time"]["perInstanceCapabilities"][name]["value"])
        self.assertEqual(result["memoryObjectsGraphics"]["indexedInstances"], [])
        self.assertIsNone(result["cpu"]["quality"]["droppedPercent"])
        self.assertIsNone(result["beforeAfter"]["percentChange"])
        self.assertEqual(result["measured"]["contextMetrics"], data[0]["metrics"])

    def test_parent_mentions_wrong_pointer_and_incomplete_pairs_are_not_timed(self):
        for mutation in ("parent", "wrong", "missing", "partial"):
            data = fixture()
            timeline = data[2][7][0]
            if mutation == "parent":
                for row in timeline["Events"]:
                    row["Fields"] = ["ParentId"]
                    row["Event"]["Fields"][0]["Name"] = "ParentId"
            elif mutation == "wrong":
                timeline["Events"][1]["Event"]["Fields"][0]["Value"] = 901
            elif mutation == "missing":
                timeline["Events"] = timeline["Events"][:1]
            else:
                timeline["Events"] = timeline["Events"][1:]
                timeline["Offset"] = 1
            with self.subTest(mutation=mutation):
                result = report(data)
                self.assertIsNone(result["time"]["perInstanceCapabilities"]["templating"]["value"])

    def test_truncated_history_keeps_observed_pairs_but_marks_partial(self):
        data = fixture()
        data[2][7][0]["Total"] = 1000
        result = report(data)
        self.assertEqual(result["time"]["perInstanceCapabilities"]["templating"]["status"], "partial")
        self.assertEqual(result["memoryObjectsGraphics"]["indexedInstances"][0]["history"]["status"], "partial")

    def test_complete_history_with_unpaired_boundary_is_partial_timing(self):
        data = fixture()
        timeline = data[2][7][0]
        extra = copy.deepcopy(timeline["Events"][0])
        extra["Event"]["TimestampQpc"] = 1400
        timeline["Events"].append(extra)
        timeline["Total"] = 3
        result = report(data)
        self.assertEqual(result["memoryObjectsGraphics"]["indexedInstances"][0]["history"]["status"], "complete")
        self.assertEqual(result["time"]["perInstanceCapabilities"]["templating"]["status"], "partial")

    def test_snapshot_process_reuse_missing_correlation_and_cycle(self):
        data = fixture()
        data[1][0][1]["creationTimeFileTime"] += 1
        with self.assertRaisesRegex(ValueError, "process instance"):
            report(data)
        data = fixture()
        data[1][0][0]["histories"].pop()
        result = report(data)
        outer = result["memoryObjectsGraphics"]["snapshots"][0]["instances"][0]
        self.assertIsNone(outer["subtree"]["correlatedVisualDescendants"])
        data = fixture()
        data[1][0][1]["xamlNodes"][0]["parentId"] = 3
        with self.assertRaisesRegex(ValueError, "Cyclic"):
            report(data)

    def test_generic_exact_type_matching_and_required_input(self):
        self.assertTrue(control.is_type("Example.ControlX", "controlx"))
        self.assertTrue(control.is_type("Example.ControlX", "Example.ControlX"))
        self.assertFalse(control.is_type("Example.ControlXItem", "ControlX"))
        data = fixture()
        with self.assertRaisesRegex(ValueError, "nonempty"):
            control.build_report(data[0], " ", *data[1:])

    def test_full_saved_evidence_run_is_sealed_no_cli_and_detects_tampering(self):
        root = pathlib.Path.cwd() / (".control-analysis-tests-" + uuid.uuid4().hex)
        root.mkdir()
        try:
            analysis, documents, _, actions, measurement = fixture()
            database = root / "recording.sqlite"
            database.write_bytes(b"synthetic immutable recording; analyzer does not execute SQL")
            snapshot, document = documents[0]
            snapshot_path = root / "snapshot.json"
            p.write_json(snapshot_path, document)
            snapshot["path"] = str(snapshot_path)
            actions_path, measurement_path = root / "actions.json", root / "measurement.json"
            p.write_json(actions_path, actions)
            measurement["database"] = p.reference(database)
            p.write_json(measurement_path, measurement)
            analysis.update(database=p.reference(database), actions=p.reference(actions_path),
                            measurement=p.reference(measurement_path))
            analysis["details"]["snapshots"] = [snapshot]
            analysis_path = root / "analysis.json"
            p.write_json(analysis_path, analysis)
            p.seal(root, [p.reference(path) for path in (database, snapshot_path, actions_path, measurement_path)],
                   [analysis_path])
            output = root / "result"
            path = control.analyze(analysis_path, "ControlX", output)
            self.assertEqual(path.name, "control-analysis.json")
            manifest = p.verify_manifest(output / "manifest.json")
            self.assertTrue((output / "control-analysis.md").is_file())
            self.assertTrue(any(item["origin"] == "control-analyzer" for item in manifest["inputs"]))
            self.assertEqual(p.load_json(path)["queryBudget"]["queriedInstances"], 0)
            with self.assertRaisesRegex(ValueError, "fresh"):
                control.analyze(analysis_path, "ControlX", output)
            database.write_bytes(b"tampered")
            with self.assertRaisesRegex(ValueError, "Evidence changed"):
                p.verify_manifest(output / "manifest.json")
        finally:
            shutil.rmtree(root)

    def test_pinned_cli_closure_rejects_replaced_or_added_payload(self):
        root = pathlib.Path.cwd() / (".control-analysis-tests-" + uuid.uuid4().hex)
        root.mkdir()
        try:
            cli = root / "WinUIProfiler.Cli.exe"
            cli.write_bytes(b"synthetic native payload")
            engine = {"measurementSha256": "measurement", "tools": [p.reference(cli)]}
            control.pinned_closure(cli, engine, {"sha256": "measurement"}, lambda path: path)
            extra = root / "injected.dll"
            extra.write_bytes(b"unexpected")
            with self.assertRaisesRegex(ValueError, "closure differs"):
                control.pinned_closure(cli, engine, {"sha256": "measurement"}, lambda path: path)
            extra.unlink()
            cli.write_bytes(b"replaced")
            with self.assertRaisesRegex(ValueError, "Evidence changed"):
                control.pinned_closure(cli, engine, {"sha256": "measurement"}, lambda path: path)
        finally:
            shutil.rmtree(root)


if __name__ == "__main__":
    unittest.main()
