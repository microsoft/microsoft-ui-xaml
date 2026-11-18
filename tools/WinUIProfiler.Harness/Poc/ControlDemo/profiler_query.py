import argparse
from contextlib import closing
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import sqlite3
import subprocess
import sys
import uuid

from metrics import MeasurementError, summarize


ROOT = Path(__file__).resolve().parent


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest().lower()


def validate_inputs(root: Path) -> tuple[dict, dict]:
    contract = json.loads((root / "investigation.json").read_text(encoding="utf-8-sig"))
    if contract["schema"] != "winui-profiler-control-poc/investigation-v1":
        raise MeasurementError("Unsupported investigation contract.")
    for item in contract["inputs"]:
        if sha256(Path(item["path"])) != item["sha256"].lower():
            raise MeasurementError(f"Investigation input changed: {item['path']}")
    manifest = json.loads(Path(contract["captureManifest"]).read_text(encoding="utf-8-sig"))
    if sha256(Path(contract["profilerCli"])) != manifest["profilerSha256"].lower():
        raise MeasurementError("The profiler executable differs from the measured tool.")
    for name in ("App.xaml.cs", "ScenarioWorkload.cs"):
        if sha256(root / name) != manifest["sourceHashes"][name].lower():
            raise MeasurementError(f"The source snapshot was not the measured {name}.")
    result = summarize(manifest, 40, 4)
    return contract, result


def command_arguments(view: str, database: str, identifier: int | None,
                      name: str | None) -> list[list[str]]:
    if view == "overview":
        return [["summary", database], ["frames", database, "--json"],
                ["interesting", database, "--json"]]
    if view == "events":
        if not name:
            raise MeasurementError("events requires --name from the profiler's event names.")
        return [["dump", database, "--name", name, "--max", "300"]]
    if view == "scenario":
        return [["sqlite", database, "EventDetails", name or ""]]
    if view == "objects":
        arguments = ["objects", database, "--kind", "element"]
        return [arguments + (["--type", name] if name else [])]
    if view not in ("tick", "region", "timeline") or identifier is None or identifier < 1:
        raise MeasurementError("Select tick, region, or timeline with a positive --id.")
    if view == "tick":
        # --json returns a tick summary, not its events, in the existing CLI.
        arguments = ["frames", database, "--frame", str(identifier)]
        return [arguments + (["--filter", name] if name else [])]
    if view == "region":
        return [["interesting", database, "--region", str(identifier), "--json"]]
    return [["timeline", database, "--object", str(identifier)]]


def scenario_events(row: dict, name: str | None) -> str:
    with closing(sqlite3.connect(Path(row["database"]).resolve().as_uri() + "?mode=ro", uri=True)) as db:
        db.row_factory = sqlite3.Row
        bounds = (row["uiThreadId"], row["scenarioStartSeq"], row["scenarioStopSeq"])
        where = "ThreadId = ? AND Seq > ? AND Seq < ?"
        counts = db.execute(
            f"SELECT Provider, Name, Opcode, count(*) AS Count FROM EventDetails WHERE {where} "
            "GROUP BY Provider, Name, Opcode ORDER BY Count DESC, Name", bounds,
        ).fetchall()
        if name:
            where += " AND instr(lower(Name), lower(?)) > 0"
        parameters = (*bounds, name) if name else bounds
        events = db.execute(
            f"SELECT Seq, Ms, Provider, Name, Opcode, Fields, Mentions FROM EventDetails WHERE {where} "
            "ORDER BY Seq LIMIT 301", parameters,
        ).fetchall()
    return json.dumps({
        "counts": [dict(value) for value in counts],
        "events": [dict(value) for value in events[:300]],
        "eventsTruncated": len(events) > 300,
    }, indent=2)


def query(root: Path, view: str, iteration: int, identifier: int | None,
          name: str | None) -> dict:
    contract, measurement = validate_inputs(root)
    rows = [row for row in measurement["iterations"] if row["iteration"] == iteration]
    if len(rows) != 1:
        raise MeasurementError("Select one recorded iteration.")
    row = rows[0]
    commands = command_arguments(view, row["database"], identifier, name)
    output = root / "ade-output"
    output.mkdir(exist_ok=True)
    receipt = {
        "recordedUtc": datetime.now(timezone.utc).isoformat(),
        "view": view, "iteration": iteration, "id": identifier, "name": name,
        "processId": row["processId"], "uiThreadId": row["uiThreadId"],
        "scenarioStartSeq": row["scenarioStartSeq"], "scenarioStopSeq": row["scenarioStopSeq"],
        "database": row["database"], "commands": [],
    }
    for arguments in commands:
        if arguments[0] == "sqlite":
            receipt["commands"].append({
                "arguments": arguments, "exitCode": 0,
                "stdout": scenario_events(row, name), "stderr": "",
            })
            continue
        completed = subprocess.run(
            [contract["profilerCli"], *arguments], capture_output=True, text=True,
            encoding="utf-8", errors="strict", timeout=120, check=False,
        )
        receipt["commands"].append({
            "arguments": arguments, "exitCode": completed.returncode,
            "stdout": completed.stdout, "stderr": completed.stderr,
        })
        if completed.returncode != 0:
            raise MeasurementError(
                f"WinUIProfiler {arguments[0]} failed ({completed.returncode}): {completed.stderr}"
            )
    path = output / f"query-{uuid.uuid4().hex}.json"
    path.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    with (output / "queries.jsonl").open("a", encoding="utf-8") as log:
        log.write(json.dumps({"file": path.name, "sha256": sha256(path)}) + "\n")
    return {
        "receipt": str(path), **receipt,
        "commands": [{**command, "stdout": command["stdout"][:32768],
                      "stdoutTruncated": len(command["stdout"]) > 32768}
                     for command in receipt["commands"]],
    }


def verify_queries(root: Path) -> dict:
    _, measurement = validate_inputs(root)
    output = root / "ade-output"
    receipts = []
    for line in (output / "queries.jsonl").read_text(encoding="utf-8").splitlines():
        record = json.loads(line)
        name = record["file"]
        if Path(name).name != name or not name.startswith("query-") or not name.endswith(".json"):
            raise MeasurementError("Invalid query receipt path.")
        path = output / name
        if sha256(path) != record["sha256"]:
            raise MeasurementError(f"Query receipt changed: {name}")
        receipt = json.loads(path.read_text(encoding="utf-8"))
        row = next((row for row in measurement["iterations"]
                    if row["iteration"] == receipt["iteration"]), None)
        if row is None or any(receipt[key] != row[key] for key in
                              ("database", "processId", "uiThreadId", "scenarioStartSeq", "scenarioStopSeq")):
            raise MeasurementError("Query receipt belongs to a different measurement scope.")
        commands = command_arguments(receipt["view"], row["database"], receipt["id"], receipt["name"])
        if [command["arguments"] for command in receipt["commands"]] != commands:
            raise MeasurementError("Receipt does not describe the selected profiler query.")
        if any(command["exitCode"] != 0 for command in receipt["commands"]):
            raise MeasurementError("A profiler query failed.")
        receipts.append(receipt)
    views = {receipt["view"] for receipt in receipts}
    if "overview" not in views or not views.intersection({"events", "scenario", "tick", "region", "objects", "timeline"}):
        raise MeasurementError("Investigation requires an overview and a model-selected drilldown.")
    diagnosis = output / "investigation.md"
    if not diagnosis.is_file() or diagnosis.stat().st_size > 32768:
        raise MeasurementError("Expected an investigation.md deliverable of at most 32 KiB.")
    decision_path = output / "decision.json"
    decision = json.loads(decision_path.read_text(encoding="utf-8"))
    if decision.get("status") not in ("RecommendationReady", "NeedsMeasurement"):
        raise MeasurementError("The investigation must recommend a change or request measurements.")
    if not isinstance(decision.get("reason"), str) or not decision["reason"].strip():
        raise MeasurementError("The investigation decision requires a reason.")
    if (decision["status"] == "NeedsMeasurement"
            and (not isinstance(decision.get("additionalMeasurements"), list)
                 or not decision["additionalMeasurements"])):
        raise MeasurementError("An inconclusive investigation must specify additional measurements.")
    return {
        "status": "EvidenceVerified", "queryCount": len(receipts),
        "views": sorted(views), "diagnosis": str(diagnosis),
        "diagnosisSha256": sha256(diagnosis),
        "decision": decision,
        "limitation": "Receipts establish queried evidence, not the correctness of the diagnosis.",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Read-only, scoped WinUIProfiler investigation queries.")
    parser.add_argument("view", choices=["validate-inputs", "verify", "overview", "events", "scenario",
                                        "tick", "region", "objects", "timeline"])
    parser.add_argument("--iteration", type=int, default=1)
    parser.add_argument("--id", type=int)
    parser.add_argument("--name")
    arguments = parser.parse_args()
    try:
        if arguments.view == "validate-inputs":
            _, result = validate_inputs(ROOT)
            result = {"status": "InputsVerified", "iterations": result["iterations"]}
        elif arguments.view == "verify":
            result = verify_queries(ROOT)
        else:
            result = query(ROOT, arguments.view, arguments.iteration, arguments.id, arguments.name)
    except (MeasurementError, OSError, ValueError, KeyError, TypeError, sqlite3.Error,
            subprocess.SubprocessError) as error:
        print(json.dumps({"status": "InvestigationFailure", "error": str(error)}), file=sys.stderr)
        return 2
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
