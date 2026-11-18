from contextlib import closing
import argparse
import hashlib
import json
from pathlib import Path
import sqlite3
import statistics


MARKER_PROVIDER = "ce339c72-348e-4e51-95ed-8db7f7d8fbc2"
XAML_PROVIDER = "531a35ab-63ce-4bcf-aa98-f88c7a89e455"


class MeasurementError(Exception):
    pass


def extract_iteration(database: Path, app: dict, expected_controls: int) -> dict:
    if app.get("passed") is not True or app.get("controlCount") != expected_controls:
        raise MeasurementError("The app did not confirm the expected control tree.")
    if not database.is_file():
        raise MeasurementError(f"The profiler session does not exist: {database}")
    with closing(sqlite3.connect(database.resolve().as_uri() + "?mode=ro", uri=True)) as db:
        db.row_factory = sqlite3.Row
        if db.execute("PRAGMA user_version").fetchone()[0] != 1:
            raise MeasurementError("Unsupported WinUIProfiler session schema.")
        sessions = db.execute(
            """SELECT ProcessId, QpcFrequency, Source, EndReason, LostByEtw,
                      DroppedByProfiler, CompleteFromSeq, CompleteFromQpc FROM SessionInfo"""
        ).fetchall()
        if len(sessions) != 1:
            raise MeasurementError("Expected exactly one process-local session.")
        session = sessions[0]
        if session["ProcessId"] != app["processId"]:
            raise MeasurementError("The profiler session belongs to another process.")
        if (
            session["Source"] != "Trace"
            or session["EndReason"] != "End of trace"
            or session["LostByEtw"] != 0
            or session["DroppedByProfiler"] != 0
            or session["CompleteFromSeq"] is not None
            or session["CompleteFromQpc"] is not None
        ):
            raise MeasurementError("The trace is incomplete, trimmed, or has lost events.")
        frequency = session["QpcFrequency"]
        if not isinstance(frequency, int) or frequency <= 0:
            raise MeasurementError("The trace has no valid clock frequency.")
        markers = db.execute(
            """SELECT e.Seq, e.Qpc, e.ThreadId, t.EventId, e.Payload
               FROM Events e JOIN EventTypes t ON t.TypeId = e.TypeId
               WHERE lower(t.ProviderId) = ? AND t.TraceLogging = 0 AND t.EventId IN (1, 2)
               ORDER BY e.Seq""",
            (MARKER_PROVIDER,),
        ).fetchall()
        if (
            len(markers) != 2
            or [row["EventId"] for row in markers] != [1, 2]
            or any(row["ThreadId"] != app["uiThreadId"] for row in markers)
            or json.loads(markers[0]["Payload"]) != [expected_controls]
            or json.loads(markers[1]["Payload"]) != [expected_controls, True]
        ):
            raise MeasurementError("Missing, ambiguous, or incorrect scenario markers.")
        start, stop = markers
        if stop["Qpc"] <= start["Qpc"]:
            raise MeasurementError("The scenario marker interval is not positive.")
        events = db.execute(
            """SELECT e.Qpc, t.EventId FROM Events e
               JOIN EventTypes t ON t.TypeId = e.TypeId
               WHERE lower(t.ProviderId) = ? AND t.TraceLogging = 0
                 AND t.EventId IN (41, 42) AND e.ThreadId = ?
                 AND e.Seq > ? AND e.Seq < ?
               ORDER BY e.Seq""",
            (XAML_PROVIDER, app["uiThreadId"], start["Seq"], stop["Seq"]),
        ).fetchall()

    stack = []
    completed = 0
    busy_qpc = 0
    previous_qpc = start["Qpc"]
    for event in events:
        timestamp = event["Qpc"]
        if timestamp < previous_qpc or timestamp > stop["Qpc"]:
            raise MeasurementError("Layout event timestamps are inconsistent.")
        previous_qpc = timestamp
        if event["EventId"] == 41:
            stack.append(timestamp)
        elif not stack:
            raise MeasurementError("A layout stop has no matching start.")
        else:
            began = stack.pop()
            if timestamp < began:
                raise MeasurementError("A layout interval has negative duration.")
            completed += 1
            if not stack:
                busy_qpc += timestamp - began
    if stack or completed == 0:
        raise MeasurementError("The scenario has incomplete or missing layout telemetry.")
    return {
        "processId": app["processId"],
        "uiThreadId": app["uiThreadId"],
        "controlCount": expected_controls,
        "layoutPassCount": completed,
        "scenarioMs": round((stop["Qpc"] - start["Qpc"]) * 1000 / frequency, 4),
        "layoutBusyMs": round(busy_qpc * 1000 / frequency, 4),
        "frameworkPath": app["frameworkPath"],
        "frameworkVersion": app["frameworkVersion"],
        "database": str(database),
        "scenarioStartSeq": start["Seq"],
        "scenarioStopSeq": stop["Seq"],
    }


def summarize(manifest: dict, expected_controls: int, max_layout_passes: int) -> dict:
    runs = manifest["iterations"]
    if (len({Path(run["database"]).resolve() for run in runs}) != len(runs)
            or len({run["iteration"] for run in runs}) != len(runs)):
        raise MeasurementError("The capture contains duplicate iterations.")
    rows = []
    framework_hashes = set()
    for run in manifest["iterations"]:
        app = json.loads(Path(run["appResult"]).read_text(encoding="utf-8-sig"))
        row = extract_iteration(Path(run["database"]), app, expected_controls)
        row.update(
            iteration=run["iteration"],
            trace=run["trace"],
            withinBudget=row["layoutPassCount"] <= max_layout_passes,
        )
        framework_hashes.add(run["frameworkSha256"])
        rows.append(row)
    if len(rows) < 10:
        raise MeasurementError("Acceptance requires at least ten valid iterations.")
    if len(framework_hashes) != 1:
        raise MeasurementError("The iterations loaded different framework binaries.")
    return {
        "schema": "winui-profiler-control-poc/measurement-v1",
        "status": "Passed" if all(row["withinBudget"] for row in rows) else "Failed",
        "measurementKind": "WinUIProfiler-process-scoped-XAML-layout-events",
        "maxLayoutPasses": max_layout_passes,
        "timingIsAcceptanceCriterion": False,
        "iterations": rows,
        "summary": {
            "iterationCount": len(rows),
            "meanLayoutPassCount": statistics.mean(row["layoutPassCount"] for row in rows),
            "meanScenarioMs": round(statistics.mean(row["scenarioMs"] for row in rows), 4),
            "medianScenarioMs": round(statistics.median(row["scenarioMs"] for row in rows), 4),
        },
        "limitations": [
            "The app deliberately requests redundant framework layout work.",
            "This is not evidence of a framework defect or presented-frame latency.",
            "The count gate validates reduced layout work, not a statistical timing claim.",
        ],
    }


def markdown_report(result: dict) -> str:
    lines = [
        "# WinUIProfiler control POC measurement",
        "",
        f"Result: **{result['status']}**",
        "",
        "| Iteration | Layout passes | Scenario ms | Layout busy ms | Within budget |",
        "| --- | ---: | ---: | ---: | --- |",
    ]
    for row in result["iterations"]:
        lines.append(
            f"| {row['iteration']} | {row['layoutPassCount']} | {row['scenarioMs']} "
            f"| {row['layoutBusyMs']} | {row['withinBudget']} |"
        )
    summary = result["summary"]
    lines.extend(
        [
            f"| Mean | {summary['meanLayoutPassCount']} | {summary['meanScenarioMs']} | - | - |",
            "",
            "## Scope and limitations",
            "",
            *[f"- {limitation}" for limitation in result["limitations"]],
            "",
            "Raw ETLs, selected-process SQLite sessions, process identities, runtime hashes,",
            "and source hashes are retained beside this report.",
        ]
    )
    return "\n".join(lines) + "\n"


def compare_manifests(baseline_path: Path, candidate_path: Path) -> str:
    manifests = [json.loads(path.read_text(encoding="utf-8-sig"))
                 for path in (baseline_path, candidate_path)]
    results = []
    for manifest in manifests:
        for path_key, hash_key in (
            ("executable", "executableSha256"),
            ("managedAssembly", "managedAssemblySha256"),
        ):
            if path_key == "managedAssembly" and path_key not in manifest:
                continue
            actual = hashlib.sha256(Path(manifest[path_key]).read_bytes()).hexdigest()
            if actual.lower() != manifest[hash_key].lower():
                raise MeasurementError(f"The retained {path_key} changed.")
        results.append(summarize(manifest, 40, 4))
    frameworks = {run["frameworkSha256"].lower()
                  for manifest in manifests for run in manifest["iterations"]}
    if len(frameworks) != 1:
        raise MeasurementError("Baseline and candidate used different framework binaries.")
    before, after = manifests
    changes = sorted(name for name in before["sourceHashes"].keys() | after["sourceHashes"].keys()
                     if before["sourceHashes"].get(name) != after["sourceHashes"].get(name))
    if changes != ["ScenarioWorkload.cs"]:
        raise MeasurementError(f"Expected only the workload to differ; found {changes}.")
    lines = [
        "# Recounted WinUIProfiler evidence", "",
        "Counts below were recomputed from the retained SQLite events, not copied from an ADE narrative.", "",
        "| Run | Baseline layout passes | Candidate layout passes | Baseline / candidate controls |",
        "| --- | ---: | ---: | --- |",
    ]
    if len(results[0]["iterations"]) != len(results[1]["iterations"]):
        raise MeasurementError("Baseline and candidate have different iteration counts.")
    if ([row["iteration"] for row in results[0]["iterations"]]
            != [row["iteration"] for row in results[1]["iterations"]]):
        raise MeasurementError("Baseline and candidate have different iteration ordering.")
    for left, right in zip(results[0]["iterations"], results[1]["iterations"]):
        lines.append(f"| {left['iteration']} | {left['layoutPassCount']} | "
                     f"{right['layoutPassCount']} | {left['controlCount']} / {right['controlCount']} |")
    lines.extend([
        f"| Mean | {results[0]['summary']['meanLayoutPassCount']} | "
        f"{results[1]['summary']['meanLayoutPassCount']} | 40 / 40 |", "",
        f"Baseline manifest: `{baseline_path}`", "",
        f"Candidate manifest: `{candidate_path}`", "",
        f"Baseline executable: `{before['executable']}`", "",
        f"Candidate executable: `{after['executable']}`", "",
        "Retained executable hashes match their capture manifests. Source manifests differ only in "
        "`ScenarioWorkload.cs`; the loaded framework hash is the same across all runs.", "",
        "Older manifests did not record the managed DLL hash; the executable hash alone does not "
        "authenticate that DLL. New captures retain both hashes and build.log.", "",
        "Every session must have valid process/thread markers, the correct button tree, "
        "balanced layout events, and no reported capture loss. This is a work-count comparison, "
        "not a statistical latency or presented-frame claim.", "",
    ])
    return "\n".join(lines)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Recount a controlled before/after capture pair.")
    parser.add_argument("--baseline", type=Path, required=True)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    arguments = parser.parse_args()
    try:
        report = compare_manifests(arguments.baseline, arguments.candidate)
        with arguments.output.open("x", encoding="utf-8") as output:
            output.write(report)
        print(report)
    except (MeasurementError, OSError, ValueError, KeyError, TypeError, sqlite3.Error) as error:
        parser.exit(2, f"Evidence comparison failed: {error}\n")
