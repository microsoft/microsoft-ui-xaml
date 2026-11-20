import argparse
import hashlib
import json
import os
from pathlib import Path
import sqlite3
from .metrics import MeasurementError, read_capture, validation_rule
from .verify import verify as measure


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def source_file(root: Path, name: str) -> Path:
    path = (root / name).resolve()
    if not path.is_relative_to(root.resolve()) or path == root.resolve():
        raise MeasurementError(f"Source path escapes the working copy: {name}")
    return path


def seal_capture(manifest_path: Path) -> list[dict]:
    manifest = load_json(manifest_path)
    paths = {manifest_path.resolve(), Path(manifest["executable"]).resolve()}
    if "managedAssembly" in manifest:
        paths.add(Path(manifest["managedAssembly"]).resolve())
    for run in manifest["iterations"]:
        paths.update(Path(run[key]).resolve() for key in ("database", "appResult", "trace"))
    return [{"path": str(path), "sha256": digest(path)} for path in sorted(paths)]


def check_seals(items: list[dict]) -> None:
    for item in items:
        if digest(Path(item["path"])) != item["sha256"]:
            raise MeasurementError(f"Retained evidence changed: {item['path']}")


def compare(baseline_path: Path, candidate_path: Path, scenario: dict, rule: dict) -> dict:
    validation_rule(rule)
    before_manifest, before = read_capture(baseline_path, scenario)
    after_manifest, after = read_capture(candidate_path, scenario)
    if before_manifest["profilerSha256"].lower() != after_manifest["profilerSha256"].lower():
        raise MeasurementError("Baseline and candidate used different profilers.")
    frameworks = {run["frameworkSha256"].lower()
                  for manifest in (before_manifest, after_manifest) for run in manifest["iterations"]}
    if len(frameworks) != 1:
        raise MeasurementError("Baseline and candidate used different framework binaries.")
    changed = sorted(name for name in before_manifest["sourceHashes"].keys() | after_manifest["sourceHashes"].keys()
                     if before_manifest["sourceHashes"].get(name, "").lower()
                     != after_manifest["sourceHashes"].get(name, "").lower())
    if not changed or not set(changed).issubset(scenario["editableFiles"]):
        raise MeasurementError(f"A fix must change only allowed app source files; found {changed}.")
    if [row["iteration"] for row in before["iterations"]] != [row["iteration"] for row in after["iterations"]]:
        raise MeasurementError("Baseline and candidate must have the same iteration ordering and count.")
    if {Path(row["database"]).resolve() for row in before["iterations"]}.intersection(
            Path(row["database"]).resolve() for row in after["iterations"]):
        raise MeasurementError("Validation must use fresh captures, not baseline databases.")
    rows = [{"iteration": left["iteration"], "baseline": left["layoutPassCount"],
             "candidate": right["layoutPassCount"],
             "reduction": left["layoutPassCount"] - right["layoutPassCount"]}
            for left, right in zip(before["iterations"], after["iterations"])]
    passed = all(row["reduction"] >= rule["minimumReduction"] for row in rows)
    return {
        "status": "Passed" if passed else "Failed", "rule": rule, "iterations": rows,
        "changedFiles": changed, "baselineManifest": str(baseline_path),
        "candidateManifest": str(candidate_path),
        "limitation": "A per-run layout-work reduction supports this controlled change, not a statistical latency claim.",
    }


def verify(root: Path) -> dict:
    reference = load_json(root / "fix-validation.json")
    if reference["schema"] != "winui-profiler-harness/fix-validation-v1":
        raise MeasurementError("Unsupported baseline-relative validation contract.")
    check_seals(reference["inputs"])
    validation_rule(reference["rule"])
    scenario = load_json(root / "scenario.json")
    protected = set(reference["allowedFiles"]) - set(scenario["editableFiles"])
    if set(reference["protectedHashes"]) != protected:
        raise MeasurementError("The validation contract must protect every non-editable input.")
    for name, expected in reference["protectedHashes"].items():
        if digest(source_file(root, name)) != expected:
            raise MeasurementError(f"A protected runner or scenario input changed: {name}")
    baseline_path = Path(reference["baselineManifest"])
    baseline_manifest, _ = read_capture(baseline_path, scenario)
    if scenario["minimumIterations"] < 10:
        raise MeasurementError("Fix acceptance requires at least ten iterations.")
    changed = []
    source_hashes = {}
    for name, expected in baseline_manifest["sourceHashes"].items():
        source_hashes[name] = digest(source_file(root, name)).lower()
        if source_hashes[name] != expected.lower():
            if name not in scenario["editableFiles"]:
                raise MeasurementError(f"The protected scenario or source changed: {name}")
            changed.append(name)
    allowed = set(reference["allowedFiles"]) | {"fix-validation.json"}
    for directory, folders, files in os.walk(root):
        folders[:] = [name for name in folders if name != ".git"]
        for name in files:
            path = Path(directory) / name
            relative = str(path.relative_to(root))
            if relative in allowed:
                continue
            if path.is_relative_to(root / "ade-output") and path.suffix in {".json", ".jsonl", ".md", ".txt", ".log", ".patch"}:
                continue
            raise MeasurementError(f"Unexpected file in the candidate working copy: {relative}")
    if not changed:
        return {"status": "Failed", "reason": "No app code change; the retained baseline cannot improve against itself."}
    measured = measure(root)
    if any(digest(source_file(root, name)).lower() != expected for name, expected in source_hashes.items()):
        raise MeasurementError("Candidate source changed while it was being built or measured.")
    if load_json(Path(measured["captureManifest"]))["sourceHashes"] != source_hashes:
        raise MeasurementError("The capture does not describe the candidate source that was built.")
    result = compare(baseline_path, Path(measured["captureManifest"]), scenario, reference["rule"])
    check_seals(reference["inputs"])
    result["measurement"] = measured
    result["report"] = str(Path(measured["report"]).with_name("fix-validation.json"))
    Path(result["report"]).write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description="Build and validate a local fix against its sealed, identical-scenario baseline.")
    parser.add_argument("--root", type=Path, required=True)
    arguments = parser.parse_args()
    try:
        result = verify(arguments.root.resolve())
    except (MeasurementError, OSError, ValueError, KeyError, TypeError, sqlite3.Error) as error:
        print(json.dumps({"status": "EnvironmentFailure", "error": str(error)}))
        return 2
    print(json.dumps(result, indent=2))
    return 0 if result["status"] == "Passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
