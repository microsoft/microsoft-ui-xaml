import argparse
import hashlib
import json
from pathlib import Path
import sqlite3
import subprocess
import sys
import uuid

from .metrics import MeasurementError, markdown_report, summarize


def verify(source: Path) -> dict:
    source = source.resolve()
    contract = json.loads((source / "measurement-contract.json").read_text(encoding="utf-8"))
    scenario_path = source / "scenario.json"
    scenario = json.loads(scenario_path.read_text(encoding="utf-8-sig"))
    profiler = Path(contract["profilerCli"])
    if hashlib.sha256(profiler.read_bytes()).hexdigest() != contract["profilerSha256"]:
        raise MeasurementError("The approved WinUIProfiler binary changed.")
    output = Path(contract["artifactsRoot"]) / f"measurement-{uuid.uuid4().hex}"
    process = subprocess.run(
        [
            "pwsh", "-NoProfile", "-File", str(Path(__file__).with_name("capture.ps1")),
            "-SourceRoot", str(source), "-ScenarioFile", str(scenario_path),
            "-ProfilerCli", str(profiler), "-InitRun", contract["initRun"],
            "-DotNet", contract["dotnet"],
            "-RunDirectory", str(output),
            "-Iterations", str(scenario["minimumIterations"]),
        ],
        stdout=sys.stderr,
        stderr=sys.stderr,
        check=False,
    )
    if process.returncode != 0:
        raise MeasurementError(f"Capture/import failed with exit code {process.returncode}.")
    manifest = json.loads((output / "capture-manifest.json").read_text(encoding="utf-8"))
    result = summarize(manifest, scenario)
    result["captureManifest"] = str(output / "capture-manifest.json")
    result["report"] = str(output / "report.md")
    (output / "measurement.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    (output / "report.md").write_text(markdown_report(result), encoding="utf-8")
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description="Build, capture, and check a configured WinUI scenario.")
    parser.add_argument("--root", type=Path, required=True)
    arguments = parser.parse_args()
    try:
        result = verify(arguments.root)
    except (MeasurementError, OSError, ValueError, KeyError, TypeError, sqlite3.Error) as error:
        print(json.dumps({"status": "EnvironmentFailure", "error": str(error)}, indent=2))
        return 2
    print(json.dumps(result, indent=2))
    return 0 if result["status"] in ("Passed", "Measured") else 1


if __name__ == "__main__":
    raise SystemExit(main())
