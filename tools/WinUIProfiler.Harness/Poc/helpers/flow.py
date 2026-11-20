import argparse
from contextlib import contextmanager
from datetime import datetime, timedelta, timezone
import json
from pathlib import Path
import shutil
import sqlite3
import subprocess
import sys
import uuid

from .metrics import MeasurementError, read_capture
from .profiler_query import verify_queries
from .validate_fix import check_seals, compare, digest, load_json, seal_capture, source_file
from .verify import verify as measure


TERMINAL = {"Accepted", "Rejected", "NotDemonstrated", "NeedsInput", "Inconclusive",
            "TimedOut", "FixNotValidated", "EnvironmentFailure", "RecoveryRequired"}
HELPERS = ("__init__.py", "capture.ps1", "metrics.py", "verify.py", "profiler_query.py", "validate_fix.py")


def now() -> datetime:
    return datetime.now(timezone.utc)


def write_json(path: Path, value: dict) -> None:
    temporary = path.with_suffix(f".{uuid.uuid4().hex}.tmp")
    temporary.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
    temporary.replace(path)


def persist(path: Path, flow: dict) -> None:
    write_json(path, flow)
    lines = ["# WinUI profiler investigation flow", "", f"Status: **{flow['status']}**", "",
             f"Problem: {flow['problem']}", "", f"State: `{path}`", "",
             f"Scenario plan SHA-256: `{flow.get('scenarioPlanSha256') or 'built-in scenario'}`", "",
             "## Chronological orchestration log", ""]
    for event in flow["events"]:
        details = json.dumps({key: value for key, value in event.items() if key != "time"}, indent=2)
        lines.extend([f"### {event['time']} - {event['event']}", "", "```json", details, "```", ""])
    if flow.get("comparison"):
        lines.extend(["## Independent identical-scenario comparison", "",
                      "| Run | Baseline layouts | Candidate layouts | Reduction |",
                      "| --- | ---: | ---: | ---: |"])
        for row in flow["comparison"].get("iterations", []):
            lines.append(f"| {row['iteration']} | {row['baseline']} | {row['candidate']} | {row['reduction']} |")
    lines.extend(["", "## Scope", "",
                  "App-level layout-work evidence only. Timing is informational; this is not a statistical latency claim.",
                  "The framework binary, action plan, markers and correctness contract must remain unchanged.",
                  "ADE owns approvals, isolated code execution, repair rounds and its per-step timeouts.",
                  "The flow deadline is checked at safe phase boundaries, including after approval waits;",
                  "an in-flight ADE/build/capture step is allowed to finish and clean up before stopping.",
                  "Code remains in the ADE working copy. Nothing is committed, pushed or published by this flow.", ""])
    (path.parent / "report.md").write_text("\n".join(lines), encoding="utf-8")


def record(path: Path, flow: dict, event: str, **details) -> None:
    flow["events"].append({"time": now().isoformat(), "event": event, **details})
    persist(path, flow)


@contextmanager
def locked(path: Path):
    # Windows releases the file lock if the owning process exits unexpectedly.
    import msvcrt
    lock_path = path.with_suffix(".lock")
    with lock_path.open("a+b") as handle:
        handle.seek(0, 2)
        if handle.tell() == 0:
            handle.write(b"\0")
            handle.flush()
        handle.seek(0)
        try:
            msvcrt.locking(handle.fileno(), msvcrt.LK_NBLCK, 1)
        except OSError as error:
            raise MeasurementError("Another process is already advancing this flow.") from error
        try:
            yield
        finally:
            handle.seek(0)
            msvcrt.locking(handle.fileno(), msvcrt.LK_UNLCK, 1)


def stop(path: Path, flow: dict, status: str, reason: str) -> dict:
    flow.update(status=status, reason=reason, pending=None)
    record(path, flow, "Stopped", status=status, reason=reason)
    return flow


def checkpoint(path: Path, flow: dict) -> bool:
    check_seals(flow["inputs"])
    if now() >= datetime.fromisoformat(flow["deadlineUtc"]):
        stop(path, flow, "TimedOut", "The flow deadline was reached; no further phase was started.")
        return False
    return True


def invoke(path: Path, flow: dict, action: str, state_path: Path, phase: str, **arguments) -> dict:
    operation = f"{len(flow['events']):03}-{action}-{uuid.uuid4().hex[:8]}"
    log = path.parent / f"{operation}.log"
    command = ["pwsh", "-NoProfile", "-File", flow["adapter"], "-Action", action,
               "-StatePath", str(state_path), "-Phase", phase]
    for key, value in arguments.items():
        command.extend([f"-{key}", str(value)])
    flow["runningOperation"] = operation
    record(path, flow, "Calling ADE", action=action, phase=phase, inputs=command, log=str(log),
           reason=flow["question"])
    with log.open("w", encoding="utf-8") as output:
        completed = subprocess.run(command, stdout=output, stderr=subprocess.STDOUT, check=False)
    text = log.read_text(encoding="utf-8-sig")
    records = [line[len("ADE-RESULT: "):] for line in text.splitlines() if line.startswith("ADE-RESULT: ")]
    result = json.loads(records[-1]) if records else {}
    flow["runningOperation"] = None
    record(path, flow, "ADE returned", action=action, phase=phase, exitCode=completed.returncode,
           result=result, log=str(log))
    if completed.returncode != 0 and not (
            action == "Run" and result.get("stopReason") not in {None, "Passed"}):
        raise MeasurementError(f"ADE {action} failed ({completed.returncode}); see {log}")
    return result


def stage(state_path: Path, problem: str, adapter: Path, max_cycles: int, timeout: int) -> Path:
    state = load_json(state_path)
    if state.get("Schema") != "winui-profiler-ade-poc/state-v1" or state.get("Scenario") != "Control":
        raise MeasurementError("The integrated flow currently requires a prepared Control app state.")
    if not problem.strip():
        raise MeasurementError("An investigation requires a problem or hypothesis.")
    original = Path(state["SampleRoot"])
    scenario = load_json(original / "scenario.json")
    measured = load_json(Path(state["BaselineMeasurement"]))
    manifest_path = Path(measured["captureManifest"])
    manifest, _ = read_capture(manifest_path, scenario)
    for name in scenario["sourceFiles"]:
        if digest(source_file(original, name)).lower() != manifest["sourceHashes"][name].lower():
            raise MeasurementError(f"The measured source changed: {name}")
    root = state_path.parent / f"flow-{uuid.uuid4().hex}"
    source = root / "source"
    source.mkdir(parents=True)
    files = [*scenario["sourceFiles"], "measurement-contract.json", ".gitignore", "README.md"]
    for name in files:
        target = source_file(source, name)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source_file(original, name), target)
    contract = load_json(source / "measurement-contract.json")
    contract["artifactsRoot"] = str(root / "measurements")
    write_json(source / "measurement-contract.json", contract)
    for folder, names in (("helpers", HELPERS), ("tests", ("__init__.py", "test_metrics.py"))):
        (source / folder).mkdir()
        for name in names:
            shutil.copyfile(Path(__file__).parent.parent / folder / name, source / folder / name)
            files.append(str(Path(folder) / name))
    flow = {
        "schema": "winui-profiler-harness/flow-v1", "adapter": str(adapter.resolve()),
        "problem": problem, "question": problem, "status": "Ready", "phase": "diagnosis",
        "deadlineUtc": (now() + timedelta(seconds=timeout)).isoformat(),
        "maxAnalysisCycles": max_cycles, "analysisCycles": 0, "sourceRoot": str(source),
        "measurement": measured, "scenarioPlanSha256": state.get("ScenarioPlanSha256"),
        "pocState": state, "allowedFiles": files, "pending": None, "runningOperation": None,
        "history": [], "events": [],
        "inputs": seal_capture(manifest_path) + [
            {"path": str(source_file(source, name)), "sha256": digest(source_file(source, name))}
            for name in files
        ] + [{"path": str(item.resolve()), "sha256": digest(item)}
             for item in (adapter, Path(__file__), *(Path(__file__).parent / name for name in HELPERS))],
    }
    path = root / "flow.json"
    record(path, flow, "Measured scenario attached", state=str(state_path), manifest=str(manifest_path),
           reason=problem, iterations=len(manifest["iterations"]))
    return path


def propose_analysis(path: Path, flow: dict) -> None:
    flow["status"] = "ProposingAnalysis"
    flow["analysisCycles"] += 1
    directory = path.parent / f"analysis-{flow['analysisCycles']:02}"
    directory.mkdir()
    baseline = directory / "measurement.json"
    write_json(baseline, flow["measurement"])
    state = dict(flow["pocState"])
    state.update(SampleRoot=flow["sourceRoot"], BaselineMeasurement=str(baseline),
                 Investigation=None, RunId=None, ProposalSha256=None, Report=None, LastResult=None,
                 FlowContext={"phase": flow["phase"], "history": flow["history"],
                              "editableFiles": load_json(Path(flow["sourceRoot"]) / "scenario.json")["editableFiles"],
                              "comparisonPassed": flow.get("comparison", {}).get("status") == "Passed",
                              "comparison": flow.get("comparison"), "inputs": flow["inputs"]})
    state_path = directory / "state.json"
    write_json(state_path, state)
    invoke(path, flow, "Investigate", state_path, "Investigation", Problem=flow["question"])
    if not checkpoint(path, flow):
        return
    current = load_json(state_path)["Investigation"]
    flow.update(status="AwaitingApproval", pending={
        "phase": "Investigation", "statePath": str(state_path),
        "runId": current["RunId"], "proposalSha256": current["ProposalSha256"], "report": current["Report"],
    })
    record(path, flow, "Approval required", **flow["pending"])


def prepare_fix(path: Path, flow: dict, state_path: Path, evidence: dict) -> None:
    flow["status"] = "ProposingFix"
    source = Path(flow["sourceRoot"])
    editable = load_json(source / "scenario.json")["editableFiles"]
    reference = {
        "schema": "winui-profiler-harness/fix-validation-v1",
        "baselineManifest": flow["measurement"]["captureManifest"],
        "inputs": flow["inputs"], "allowedFiles": flow["allowedFiles"],
        "protectedHashes": {name: digest(source_file(source, name))
                            for name in flow["allowedFiles"] if name not in editable},
        "rule": evidence["decision"]["recommendation"]["validation"],
    }
    reference_path = source / "fix-validation.json"
    write_json(reference_path, reference)
    flow["inputs"].append({"path": str(reference_path), "sha256": digest(reference_path)})
    state = load_json(state_path)
    state["FlowValidation"] = True
    write_json(state_path, state)
    invoke(path, flow, "Propose", state_path, "Fix")
    if not checkpoint(path, flow):
        return
    current = load_json(state_path)
    flow["baselineManifest"] = reference["baselineManifest"]
    flow.update(status="AwaitingApproval", pending={
        "phase": "Fix", "statePath": str(state_path),
        "runId": current["RunId"], "proposalSha256": current["ProposalSha256"], "report": current["Report"],
    })
    record(path, flow, "Approval required", **flow["pending"])


def repeat_measurement(path: Path, flow: dict) -> None:
    flow["runningOperation"] = "Repeat measurement"
    record(path, flow, "Calling Measure", source=flow["sourceRoot"], reason=flow["question"])
    measurement = measure(Path(flow["sourceRoot"]))
    scenario = load_json(Path(flow["sourceRoot"]) / "scenario.json")
    read_capture(Path(measurement["captureManifest"]), scenario)
    flow["measurement"] = measurement
    flow["inputs"].extend(seal_capture(Path(measurement["captureManifest"])))
    if flow["phase"] == "validation":
        flow["comparison"] = compare(Path(flow["baselineManifest"]), Path(measurement["captureManifest"]),
                                     scenario, flow["comparison"]["rule"])
    flow["runningOperation"] = None
    flow["status"] = "Ready"
    record(path, flow, "Measure returned", measurement=measurement, comparison=flow.get("comparison"))


def finish_fix(path: Path, flow: dict, state_path: Path, result: dict) -> None:
    if result.get("stopReason") != "Passed":
        status = "EnvironmentFailure" if result.get("stopReason") == "EnvironmentFailure" else "FixNotValidated"
        stop(path, flow, status, f"ADE fix stopped with {result.get('stopReason')}; no acceptance.")
        return
    # Independently build/capture/recount using the harness, not a model's report.
    from .validate_fix import verify as validate_fix
    candidate = Path(load_json(state_path)["FixRunDir"]) / "workcopy"
    if digest(candidate / "fix-validation.json") != digest(Path(flow["sourceRoot"]) / "fix-validation.json"):
        raise MeasurementError("The candidate validation contract differs from the approved contract.")
    flow["runningOperation"] = "Independent fix validation"
    record(path, flow, "Calling Measure for independent fix validation", source=str(candidate),
           baseline=flow["baselineManifest"])
    comparison = validate_fix(candidate)
    flow["runningOperation"] = None
    flow["comparison"] = comparison
    if comparison["status"] != "Passed":
        stop(path, flow, "FixNotValidated", "The independent same-scenario comparison did not pass.")
        return
    measurement = comparison["measurement"]
    flow.update(sourceRoot=str(candidate), measurement=measurement, phase="validation", status="Ready",
                question=flow["problem"] + " Assess whether the controlled fix supports the proposed cause. "
                         "Read the sealed diagnosis history and independent comparison. Do not apply another change.")
    flow["inputs"].extend(seal_capture(Path(measurement["captureManifest"])))
    flow["inputs"].extend({"path": str(source_file(candidate, name)), "sha256": digest(source_file(candidate, name))}
                          for name in [*flow["allowedFiles"], "fix-validation.json"])
    flow["inputs"].append({"path": comparison["report"], "sha256": digest(Path(comparison["report"]))})
    record(path, flow, "Independent fix validation returned", comparison=comparison,
           candidateSource=str(candidate), reason="A passing count gate still requires causal analysis.")


def finish_analysis(path: Path, flow: dict, state_path: Path) -> None:
    state = load_json(state_path)
    evidence = state["Investigation"]["Evidence"]
    if not evidence:
        raise MeasurementError("ADE returned without independently verified profiler evidence.")
    current = verify_queries(Path(evidence["diagnosis"]).parent.parent)
    if (current["diagnosisSha256"] != evidence["diagnosisSha256"]
            or current["decisionSha256"] != evidence["decisionSha256"]):
        raise MeasurementError("The approved analysis deliverables changed.")
    decision = current["decision"]
    flow["history"].append({"phase": flow["phase"], "question": flow["question"],
                            "decision": decision, "diagnosis": current["diagnosis"]})
    output = Path(current["diagnosis"]).parent
    receipt_log = output / "queries.jsonl"
    evidence_paths = [Path(current["diagnosis"]), Path(current["decisionPath"]), receipt_log]
    evidence_paths.extend(output / json.loads(line)["file"]
                          for line in receipt_log.read_text(encoding="utf-8").splitlines())
    flow["inputs"].extend({"path": str(item), "sha256": digest(item)} for item in evidence_paths)
    record(path, flow, "Analysis decision", evidence=current)
    if decision["status"] in {"Accepted", "Rejected", "NotDemonstrated"}:
        stop(path, flow, decision["status"], decision["reason"])
    elif decision["status"] == "RecommendationReady":
        if flow["analysisCycles"] >= flow["maxAnalysisCycles"]:
            stop(path, flow, "Inconclusive", "No analysis cycle remains for causal validation; no fix proposed.")
        else:
            flow["status"] = "ProposeFix"
            flow["fixAnalysisState"] = str(state_path)
    else:
        requests = decision["additionalMeasurements"]
        if any(request["kind"] == "unsupported" for request in requests):
            stop(path, flow, "NeedsInput", "Requested instrumentation/scenario is not supported: " + json.dumps(requests))
        elif flow["analysisCycles"] >= flow["maxAnalysisCycles"]:
            stop(path, flow, "Inconclusive", "The analysis-cycle limit was reached.")
        else:
            flow["question"] = "\n".join(request["question"] for request in requests)
            flow["status"] = "RepeatMeasurement"


def advance(path: Path, flow: dict, approved_hash: str | None = None) -> dict:
    if flow["status"] in TERMINAL:
        if approved_hash:
            raise MeasurementError("A stopped flow cannot consume an approval.")
        return flow
    if (flow["runningOperation"] or flow["status"] not in
            {"Ready", "ProposeFix", "RepeatMeasurement", "AwaitingApproval"}
            or (flow["pending"] and flow["pending"].get("approvalConsumed"))):
        return stop(path, flow, "RecoveryRequired",
                    "A prior operation was interrupted. Inspect its log and ADE run before starting a new flow; it will not be rerun automatically.")
    if not checkpoint(path, flow):
        return flow
    pending = flow["pending"]
    if pending:
        if not approved_hash:
            return flow
        if approved_hash != pending["proposalSha256"]:
            raise MeasurementError("Approval does not match the currently pending exact ADE proposal.")
        state_path = Path(pending["statePath"])
        phase = pending["phase"]
        pending["approvalConsumed"] = True
        record(path, flow, "Consuming exact proposal approval", phase=phase, runId=pending["runId"],
               proposalSha256=approved_hash)
        invoke(path, flow, "Approve", state_path, phase, ProposalSha256=approved_hash)
        if not checkpoint(path, flow):
            return flow
        flow["status"] = "Running"
        result = invoke(path, flow, "Run", state_path, phase)
        flow["pending"] = None
        if not checkpoint(path, flow):
            return flow
        if phase == "Fix":
            finish_fix(path, flow, state_path, result)
        else:
            if result.get("stopReason") != "Passed":
                return stop(path, flow, "EnvironmentFailure",
                            f"ADE analysis stopped with {result.get('stopReason')}; no diagnosis accepted.")
            finish_analysis(path, flow, state_path)
    elif approved_hash:
        raise MeasurementError("There is no pending proposal to approve.")
    while flow["status"] not in TERMINAL and not flow["pending"]:
        if not checkpoint(path, flow):
            break
        if flow["status"] == "RepeatMeasurement":
            repeat_measurement(path, flow)
        elif flow["status"] == "ProposeFix":
            state_path = Path(flow["fixAnalysisState"])
            prepare_fix(path, flow, state_path, load_json(state_path)["Investigation"]["Evidence"])
        elif flow["analysisCycles"] >= flow["maxAnalysisCycles"]:
            stop(path, flow, "Inconclusive", "The analysis-cycle limit was reached.")
        else:
            propose_analysis(path, flow)
    return flow


def main() -> int:
    parser = argparse.ArgumentParser(description="Resume the local profiler loop up to its next exact ADE approval or stop.")
    parser.add_argument("action", choices=["StartFlow", "ContinueFlow", "FlowStatus", "FlowReport"])
    parser.add_argument("--adapter", type=Path, required=True)
    parser.add_argument("--state", type=Path)
    parser.add_argument("--flow", type=Path)
    parser.add_argument("--problem")
    parser.add_argument("--max-cycles", type=int, default=3, choices=range(1, 11))
    parser.add_argument("--timeout-seconds", type=int, default=3600)
    parser.add_argument("--approved-hash")
    arguments = parser.parse_args()
    path = arguments.flow.resolve() if arguments.flow else None
    try:
        if arguments.action == "StartFlow":
            if not arguments.state or not arguments.problem or not 60 <= arguments.timeout_seconds <= 86400:
                raise MeasurementError("StartFlow requires a state, problem and timeout from 60 to 86400 seconds.")
            path = stage(arguments.state.resolve(), arguments.problem, arguments.adapter,
                         arguments.max_cycles, arguments.timeout_seconds)
        if path is None:
            raise MeasurementError("An explicit flow path is required.")
        with locked(path):
            flow = load_json(path)
            if flow.get("schema") != "winui-profiler-harness/flow-v1":
                raise MeasurementError("Unsupported flow state.")
            if arguments.action in {"StartFlow", "ContinueFlow"}:
                if arguments.approved_hash and (
                        not flow["pending"] or arguments.approved_hash != flow["pending"]["proposalSha256"]):
                    raise MeasurementError("Approval does not match the currently pending exact ADE proposal.")
                try:
                    advance(path, flow, arguments.approved_hash)
                except (MeasurementError, OSError, ValueError, KeyError, TypeError, sqlite3.Error) as error:
                    stop(path, flow, "EnvironmentFailure", str(error))
                persist(path, flow)
            if arguments.action == "FlowReport":
                print((path.parent / "report.md").read_text(encoding="utf-8"))
            else:
                print(json.dumps({"flow": str(path), "status": flow["status"], "reason": flow.get("reason"),
                                  "pending": flow["pending"], "report": str(path.parent / "report.md")}, indent=2))
                print(f"POC-FLOW: {path}")
            return 2 if flow["status"] in {"EnvironmentFailure", "RecoveryRequired"} else 0
    except (MeasurementError, OSError, ValueError, KeyError, TypeError, sqlite3.Error) as error:
        print(f"Flow failed: {error}", file=sys.stderr)
        if path:
            print(f"POC-FLOW: {path}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
