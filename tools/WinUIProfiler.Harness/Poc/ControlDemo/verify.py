import hashlib
import json
from pathlib import Path
import sqlite3
import subprocess
import sys
import uuid

from metrics import MeasurementError, markdown_report, summarize


def verify() -> dict:
    source = Path(__file__).resolve().parent
    contract = json.loads((source / "measurement-contract.json").read_text(encoding="utf-8"))
    profiler = Path(contract["profilerCli"])
    if hashlib.sha256(profiler.read_bytes()).hexdigest() != contract["profilerSha256"]:
        raise MeasurementError("The approved WinUIProfiler binary changed.")
    output = Path(contract["artifactsRoot"]) / f"measurement-{uuid.uuid4().hex}"
    process = subprocess.run(
        [
            "pwsh", "-NoProfile", "-File", str(source / "capture.ps1"),
            "-ProfilerCli", str(profiler), "-InitRun", contract["initRun"],
            "-DotNet", contract["dotnet"],
            "-RunDirectory", str(output),
            "-Iterations", "10",
        ],
        stdout=sys.stderr,
        stderr=sys.stderr,
        check=False,
    )
    if process.returncode != 0:
        raise MeasurementError(f"Capture/import failed with exit code {process.returncode}.")
    manifest = json.loads((output / "capture-manifest.json").read_text(encoding="utf-8"))
    result = summarize(manifest, contract["expectedControls"], contract["maxLayoutPasses"])
    result["captureManifest"] = str(output / "capture-manifest.json")
    result["report"] = str(output / "report.md")
    (output / "measurement.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    (output / "report.md").write_text(markdown_report(result), encoding="utf-8")
    return result


def main() -> int:
    try:
        result = verify()
    except (MeasurementError, OSError, ValueError, KeyError, TypeError, sqlite3.Error) as error:
        print(json.dumps({"status": "EnvironmentFailure", "error": str(error)}, indent=2))
        return 2
    print(json.dumps(result, indent=2))
    return 0 if result["status"] == "Passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
