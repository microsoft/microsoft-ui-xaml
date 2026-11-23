"""Focused synthetic evidence tests. No app, profiler connection or agent is started."""

import copy
import importlib.util
import json
import pathlib
import sqlite3
import tempfile
import unittest


spec = importlib.util.spec_from_file_location("profiler_analysis", pathlib.Path(__file__).parents[1] / "pack" / "tools" / "Analyze-Profiler.py")
profiler = importlib.util.module_from_spec(spec)
spec.loader.exec_module(profiler)


def fixture(directory):
    directory.mkdir()
    database = directory / "recording.sqlite"
    db = sqlite3.connect(database)
    db.executescript("""
        PRAGMA user_version=1;
        CREATE TABLE SessionInfo(SessionId,ProcessId,ProcessCreatedUtc,Source,Coverage,QpcFrequency,StartQpc,
            StartedUtc,EndedUtc,LostByEtw,DroppedByProfiler,CompleteFromSeq,CompleteFromQpc);
        INSERT INTO SessionInfo VALUES('session',101,'2026-11-20T00:00:00.1234567Z','Live','ProcessStart',
            1000,1000,'2026-11-20T00:00:00Z','2026-11-20T00:00:02Z',0,0,NULL,NULL);
        CREATE TABLE Ticks(TickId,ThreadId,FrameNumber,StartMs,EndMs,TickMs,Drew,Slow,Events,Xaml,Composition,Other,FirstSeq,LastSeq);
        INSERT INTO Ticks VALUES(1,7,1,200,240,40,1,1,2,2,0,0,1,2),(2,7,2,500,520,20,NULL,1,0,0,0,0,3,4);
        CREATE TABLE EventTypes(TypeId,Provider,Name,Opcode,Launcher,FieldNames);
        INSERT INTO EventTypes VALUES(1,'Xaml','Layout/Start',1,0,''),(2,'Xaml','Layout/Stop',2,0,'');
        CREATE TABLE Events(Seq,Qpc,ThreadId,TypeId,Payload);
        INSERT INTO Events VALUES(1,1200,7,1,'[]'),(2,1240,7,2,'[]');
        CREATE TABLE InterestingRows(Type,Name,ThreadId,StartMs,StopMs);
        INSERT INTO InterestingRows VALUES('UpdateLayout','UpdateLayout',7,200,240);
        CREATE TABLE CpuSampling(IntervalMs,Samples,Dropped,CompressedStacks,Note);
        INSERT INTO CpuSampling VALUES(1.0,2,2,0,NULL);
        CREATE TABLE CpuFrames(StackId,ParentId,Module,Function,Frame);
        INSERT INTO CpuFrames VALUES(1,NULL,'app','Recursive','app!Recursive'),(2,1,'app','Recursive','app!Recursive');
        CREATE TABLE CodeAddresses(AddressId,ModuleId,Function);
        INSERT INTO CodeAddresses VALUES(1,1,'Recursive'),(2,1,'Recursive');
        CREATE TABLE CpuSamples(Qpc,ProcessId,ThreadId,StackId);
        INSERT INTO CpuSamples VALUES(1250,101,7,2),(1510,101,7,2);
        CREATE TABLE Modules(ModuleId,Name,SymbolStatus);
        INSERT INTO Modules VALUES(1,'app','Resolved');
        CREATE TABLE GraphicsLifetimes(GraphicsObjectId,Kind,KnownFromQpc,StartQpc,StopQpc,EndQpc,EndReason,Trimmed,SizeBytes TEXT);
        CREATE TABLE GraphicsLinks(SourceId,TargetId,Kind,StartSeq,EndSeq);
        CREATE TABLE GraphicsResourceAllocations(ResourceId,DeviceAllocationId,AdapterAllocationId,SizeBytes TEXT);
        CREATE TABLE Objects(ObjectId,Kind,Address,Type,Name);
        INSERT INTO Objects VALUES(1,'Element',1234,'Grid','Root');
        CREATE TABLE ObjectEvents(Seq,ObjectId,Field);
        INSERT INTO ObjectEvents VALUES(1,1,0),(2,1,0);
        CREATE TABLE LaunchSummary(Position,TickId,ThreadId,FrameNumber,EndMs,Ms,MeasuredFrom);
        INSERT INTO LaunchSummary VALUES(1,1,7,1,240,220,'ProcessStart');
    """)
    db.execute("INSERT INTO GraphicsLifetimes VALUES(11,'AdapterAllocation',1000,1000,NULL,NULL,NULL,0,?)", (str(2**64 - 1),))
    db.commit()
    db.close()
    actions = {
        "state": "complete", "qpcFrequency": 1000, "startedQpc": 1100, "endedQpc": 2100,
        "window": {"threadId": 7, "dpi": 96, "width": 1280, "height": 900},
        "steps": [{"id": "expand", "action": "expand", "state": "passed", "from": "Collapsed", "to": "Expanded",
                   "startedQpc": 1150, "endedQpc": 2000}],
    }
    actions_path = directory / "actions.json"
    profiler.write_json(actions_path, actions)
    created = profiler.filetime("2026-11-20T00:00:00.1234567Z")
    readings = []
    for timestamp in (1150, 1500, 2000):
        values = []
        for name, (_, unit) in profiler.NATIVE.items():
            value = None if name == "cpu" and timestamp == 1150 else 2 if unit == "percent" else timestamp / 10
            values.append({"metric": name, "unit": unit, "sample": {
                "timestampQpc": timestamp, "value": value, "source": "synthetic native evidence",
                "unavailableReason": "warmup" if value is None else None}})
        readings.append({"process": {"processId": 101, "creationTimeFileTime": created}, "timestampQpc": timestamp, "values": values})
    measurement = {
        "schema": "ui-profiler/measurement/v1", "state": "complete", "scenarioSha256": "scenario",
        "profilerSha256": "profiler", "frameworkSha256": "framework", "sourceSha256": "source", "executableSha256": "app",
        "database": profiler.reference(database), "actions": profiler.reference(actions_path),
        "metrics": {"scenarioElapsedMs": 1000, "completedTicks": 2, "meanTickMs": 30, "p95TickMs": 40, "maxTickMs": 40, "totalTickMs": 60},
        "sampledMetrics": readings,
    }
    resources = [{"metric": name, "resourceIndex": None, "resourceName": None, "isTracked": True,
                  "isBytes": name.endswith("Size") or name == "AllocatedMemory", "after": None}
                 for name in ("AllocCount", "AllocSize", "AllocatedMemory", "OutstandingAllocCount", "OutstandingAllocSize")]
    snapshot_path = directory / "tree.json"
    profiler.write_json(snapshot_path, {
        "startQpc": 1070, "endQpc": 1090, "qpcFrequency": 1000,
        "xamlNodes": [{"id": 1, "parentId": None, "type": "Grid", "properties": {
            "actualWidth": 64, "visibility": "Visible", "width": {"error": "non-finite native value"}}}],
        "compositionNodes": [], "correlations": [], "resourceSnapshot": {
            "status": "unavailable", "hResult": -2147467262, "values": None},
    })
    shared = {"adapterAllocationId": 11, "sizeBytes": 2**64 - 1}
    capture = {"available": True, "trimmed": False, "knownLinkedBytes": 2**64 - 1, "textures": [
        {"id": index, "kind": "Texture2D", "descriptor": '{"Width":64,"Height":64,"Format":87}',
         "allocations": [shared], "linkedBytes": 2**64 - 1} for index in (31, 32)]}
    engine = {
        "textureCheckpoints": [{"scope": window, "boundary": side, "capture": copy.deepcopy(capture)}
                               for window in ("scenario", "action:expand") for side in ("start", "end")],
        "snapshots": [{"path": str(snapshot_path), "phase": "before-scenario", "resources": resources,
                       "histories": [{"objectId": None}], "diffPaths": []}],
        "launch": {"Milestones": [{"Label": "First frame drawn", "Milliseconds": 220, "Count": 1,
                                  "Definition": {"Label": "First frame drawn", "BuiltIn": "FirstFrameDrawn"}}], "NotReached": []},
    }
    return measurement, actions, engine


def analyze_in_memory(measurement, actions, engine):
    db = sqlite3.connect(measurement["database"]["path"])
    db.row_factory = sqlite3.Row
    try:
        return profiler.Analysis(measurement, actions, db, engine).run()
    finally:
        db.close()


def metric(report, family, name, scope="scenario"):
    return next(row for row in report["metrics"] if (row["family"], row["scope"], row["metric"]) == (family, scope, name))


class AnalysisTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="ui-profiler-analysis-test-")
        self.root = pathlib.Path(self.temp.name)
        self.measurement, self.actions, self.engine = fixture(self.root / "input")

    def tearDown(self):
        self.temp.cleanup()

    def test_qpc_windows_and_tick_consistency(self):
        report = analyze_in_memory(self.measurement, self.actions, self.engine)
        self.assertEqual(metric(report, "scenario", "elapsed")["value"], 1000)
        self.assertEqual(metric(report, "ticks", "duration.mean")["value"], 30)
        self.assertEqual(metric(report, "ticks", "duration.count")["value"], 2)
        self.assertEqual(metric(report, "ticks", "startInterval.mean")["direction"], "context")
        self.actions["qpcFrequency"] = 2000
        with self.assertRaisesRegex(ValueError, "QPC clock"):
            analyze_in_memory(self.measurement, self.actions, self.engine)

    def test_bad_tick_summary_and_action_order_are_rejected(self):
        self.measurement["metrics"]["totalTickMs"] = 999
        with self.assertRaisesRegex(ValueError, "disagrees"):
            analyze_in_memory(self.measurement, self.actions, self.engine)
        self.actions["steps"][0]["startedQpc"] = 1000
        with self.assertRaisesRegex(ValueError, "action intervals"):
            analyze_in_memory(self.measurement, self.actions, self.engine)

    def test_native_missing_is_not_zero_or_integrated_usage(self):
        report = analyze_in_memory(self.measurement, self.actions, self.engine)
        self.assertEqual(metric(report, "cpu", "cpu.mean")["value"], 2)
        self.assertEqual(metric(report, "cpu", "cpu.mean")["status"], "partial")
        self.assertEqual(metric(report, "cpu", "cpu.missing")["value"], 1)
        self.assertFalse(any(row["family"] == "cpu" and row["metric"] == "cpu.sum" for row in report["metrics"]))
        self.assertEqual(metric(report, "memory", "workingSet.change")["value"], 85)

    def test_native_process_units_and_acquisition_completeness(self):
        self.measurement["sampledMetrics"][0]["process"]["creationTimeFileTime"] += 1
        with self.assertRaisesRegex(ValueError, "process instance"):
            analyze_in_memory(self.measurement, self.actions, self.engine)
        self.measurement["sampledMetrics"] = []
        report = analyze_in_memory(self.measurement, self.actions, self.engine)
        self.assertIsNone(metric(report, "memory", "workingSet.mean")["value"])
        self.assertEqual(report["capabilities"]["memory"]["status"], "unavailable")

    def test_sparse_native_samples_and_single_point_trend(self):
        self.measurement["sampledMetrics"] = self.measurement["sampledMetrics"][1:2]
        report = analyze_in_memory(self.measurement, self.actions, self.engine)
        self.assertEqual(metric(report, "memory", "workingSet.mean")["status"], "partial")
        self.assertIsNone(metric(report, "memory", "workingSet.change")["value"])
        self.assertGreater(metric(report, "memory", "workingSet.maxSamplingGap")["value"], 500)

    def test_gpu_carry_in_missing_coverage_and_unsigned_bytes(self):
        large = 2**64 - 1
        rows = [{"qpc": 900, "seq": 1, "value": large}, {"qpc": 1500, "seq": 2, "value": large - 20}]
        result = profiler.gauge(rows, 1100, 2100, "value")
        self.assertEqual(result["start"], large)
        self.assertEqual(result["change"], -20)
        self.assertEqual(result["mean"], large - 12)
        self.assertEqual(result["coveragePercent"], 100)
        result = profiler.gauge(rows[1:], 1100, 2100, "value")
        self.assertIsNone(result["start"])
        self.assertEqual(result["coveragePercent"], 60)
        self.assertIsNone(result["change"])

    def test_gpu_adapter_ambiguity_never_pairs_handles(self):
        report = analyze_in_memory(self.measurement, self.actions, self.engine)
        db = sqlite3.connect(self.measurement["database"]["path"])
        db.row_factory = sqlite3.Row
        try:
            analysis = profiler.Analysis(self.measurement, self.actions, db, self.engine)
            groups = {(0, 0, pointer): [{"qpc": 1000, "seq": pointer, "gauge": "usage", "value": 0, "fields": {}}]
                      for pointer in (12, 13)}
            analysis.gpu_memory(groups)
            report["metrics"] += analysis.metrics
            report["scopeAvailability"].update(analysis.scopes)
            rows = profiler.comparison_rows(report, copy.deepcopy(report))
            usage = [row for row in rows if row["family"] == "gpuMemory" and ".usage.start" in row["metric"]]
            self.assertTrue(usage)
            self.assertTrue(all(row["delta"] is None for row in usage))
        finally:
            db.close()

    def test_cpu_recursion_and_drops_are_separate_from_event_loss(self):
        report = analyze_in_memory(self.measurement, self.actions, self.engine)
        self.assertEqual(metric(report, "cpuStacks", "uiThread.inclusive:app!Recursive")["value"], 2)
        self.assertEqual(metric(report, "cpuStacks", "uiThread.exclusive:app!Recursive")["value"], 2)
        self.assertEqual(metric(report, "cpuStacks", "dropPercent", "recording")["value"], 50)
        self.assertEqual(report["capabilities"]["cpuStacks"]["status"], "partial")

    def test_unresolved_cpu_addresses_are_not_cross_run_function_identities(self):
        db = sqlite3.connect(self.measurement["database"]["path"])
        db.execute("UPDATE CpuFrames SET Function=NULL,Frame='app!0x1234' WHERE StackId=1")
        db.execute("UPDATE CpuFrames SET Function=NULL,Frame='app!0x5678' WHERE StackId=2")
        db.execute("UPDATE CodeAddresses SET Function=NULL")
        db.commit()
        db.close()
        report = analyze_in_memory(self.measurement, self.actions, self.engine)
        self.assertEqual(metric(report, "cpuStacks", "uiThread.inclusive:app!<unresolved>")["value"], 2)
        self.assertFalse(any("0x1234" in row["metric"] or "0x5678" in row["metric"] for row in report["metrics"]))

    def test_symbol_coverage_is_metadata_not_a_performance_win(self):
        db = sqlite3.connect(self.measurement["database"]["path"])
        db.execute("UPDATE CodeAddresses SET Function=NULL WHERE AddressId=2")
        db.execute("UPDATE CpuFrames SET Function=NULL WHERE StackId=2")
        db.commit()
        db.close()
        report = analyze_in_memory(self.measurement, self.actions, self.engine)
        coverage = metric(report, "cpuStacks", "namedAddressPercent", "recording")
        self.assertEqual(coverage["value"], 50)
        self.assertEqual(coverage["direction"], "context")
        self.assertEqual(metric(report, "cpuStacks", "moduleSymbols:Resolved", "recording")["value"], 1)

    def test_shared_texture_bytes_and_unknown_size_are_preserved(self):
        report = analyze_in_memory(self.measurement, self.actions, self.engine)
        self.assertEqual(metric(report, "textures", "start.count")["value"], 2)
        self.assertEqual(metric(report, "textures", "start.knownLinkedBytes")["value"], 2**64 - 1)
        for checkpoint in self.engine["textureCheckpoints"]:
            checkpoint["capture"]["knownLinkedBytes"] = 0
            for texture in checkpoint["capture"]["textures"]:
                texture["linkedBytes"] = None
                texture["allocations"] = []
        report = analyze_in_memory(self.measurement, self.actions, self.engine)
        self.assertEqual(metric(report, "textures", "start.unknownLinkedTextureCount")["value"], 2)
        self.assertEqual(metric(report, "textures", "start.knownLinkedBytes")["direction"], "context")

    def test_unavailable_resources_and_end_snapshot_stay_unknown(self):
        report = analyze_in_memory(self.measurement, self.actions, self.engine)
        self.assertEqual(report["capabilities"]["resources"]["status"], "unavailable")
        self.assertIsNone(metric(report, "resources", "allocator.AllocSize", "before-scenario")["value"])
        self.assertIsNone(metric(report, "trees", "xamlNodes.count", "after-scenario")["value"])
        self.assertEqual(report["capabilities"]["textureOwnership"]["status"], "unsupported")
        self.assertEqual(metric(report, "objects", "snapshotHistoryUnavailable", "before-scenario")["value"], 1)

    def test_cross_run_zero_only_for_observed_activity(self):
        before = analyze_in_memory(self.measurement, self.actions, self.engine)
        after = copy.deepcopy(before)
        absent = metric(after, "objects", "type:Element/Grid.objects")
        after["metrics"].remove(absent)
        missing = metric(after, "memory", "workingSet.mean")
        missing["value"], missing["status"] = None, "unavailable"
        rows = profiler.comparison_rows(before, after)
        activity = next(row for row in rows if row["metric"] == "type:Element/Grid.objects" and row["scope"] == "scenario")
        memory = next(row for row in rows if row["metric"] == "workingSet.mean" and row["scope"] == "scenario")
        self.assertEqual(activity["candidate"], 0)
        self.assertIsNone(memory["delta"])
        after["inputs"]["captureSchema"] = "ui-profiler/measurement/v2"
        with self.assertRaisesRegex(ValueError, "captureSchema"):
            profiler.comparison_rows(before, after)

    def test_end_to_end_manifests_exact_bytes_and_recursive_tamper_detection(self):
        input_directory = pathlib.Path(self.measurement["database"]["path"]).parent
        measurement_path = input_directory / "measurement.json"
        profiler.write_json(measurement_path, self.measurement)
        queries = self.root / "queries"
        queries.mkdir()
        profiler.write_json(queries / "manifest.json", {"files": ["a native payload, not an evidence manifest"]})
        captures = [row["capture"] for row in self.engine["textureCheckpoints"]]
        profiler.write_json(queries / "textures.json", captures)
        profiler.write_json(queries / "launch.json", self.engine["launch"])
        resource_path = queries / "resources.json"
        profiler.write_json(resource_path, self.engine["snapshots"][0]["resources"])
        engine = copy.deepcopy(self.engine)
        engine.update(measurementSha256=profiler.sha256(measurement_path), tools=[],
                      textureCapturesPath=str(queries / "textures.json"), launchPath=str(queries / "launch.json"))
        for index, checkpoint in enumerate(engine["textureCheckpoints"]):
            del checkpoint["capture"]
            checkpoint["captureIndex"] = index
        snapshot = engine["snapshots"][0]
        del snapshot["resources"]
        snapshot.update(sha256=profiler.sha256(snapshot["path"]), resourcesPath=str(resource_path))
        profiler.write_json(queries / "engine.json", engine)
        original_hash = profiler.sha256(self.measurement["database"]["path"])
        before = profiler.analyze(measurement_path, queries / "engine.json", self.root / "before")
        after = profiler.analyze(measurement_path, queries / "engine.json", self.root / "after")
        comparison = profiler.compare(before, after, self.root / "comparison")
        output = profiler.load_json(comparison)
        row = next(row for row in output["metrics"] if row["family"] == "textures" and row["scope"] == "scenario" and row["metric"] == "start.knownLinkedBytes")
        self.assertEqual(row["baseline"], 2**64 - 1)
        self.assertEqual(row["delta"], 0)
        self.assertEqual(profiler.sha256(self.measurement["database"]["path"]), original_hash)
        profiler.verify_manifest(comparison.parent / "manifest.json")
        with resource_path.open("a") as file:
            file.write(" ")
        with self.assertRaisesRegex(ValueError, "Evidence changed"):
            profiler.verify_manifest(comparison.parent / "manifest.json")


if __name__ == "__main__":
    unittest.main(verbosity=2)
