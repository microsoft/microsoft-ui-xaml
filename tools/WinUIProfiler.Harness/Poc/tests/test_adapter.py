import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


class AdapterTests(unittest.TestCase):
    @unittest.skipUnless(all(shutil.which(name) for name in ("pwsh", "dotnet", "logman")),
                         "The Windows capture prerequisites are required for preparation staging.")
    def test_preparation_refuses_plan_bytes_that_changed_after_chat_review(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            poc = Path(__file__).resolve().parents[1]
            repo = root / "repo"
            staged_poc = repo / "tools" / "WinUIProfiler.Harness" / "Poc"
            staged_poc.mkdir(parents=True)
            entry = staged_poc / "Invoke-AdePoc.ps1"
            shutil.copyfile(poc / "Invoke-AdePoc.ps1", entry)
            app = staged_poc / "ControlDemo"
            app.mkdir()
            source = poc / "ControlDemo"
            files = json.loads((source / "scenario.json").read_text(encoding="utf-8"))["sourceFiles"]
            for name in files + [".gitignore", "README.md"]:
                shutil.copyfile(poc / "ControlDemo" / name, app / name)
            for folder, files in (
                ("helpers", ["__init__.py", "capture.ps1", "metrics.py", "verify.py", "profiler_query.py"]),
                ("tests", ["__init__.py", "test_metrics.py"]),
            ):
                (staged_poc / folder).mkdir()
                for name in files:
                    shutil.copyfile(poc / folder / name, staged_poc / folder / name)
            launcher = repo / "eng" / "ade" / "ui-profiler" / "scripts" / "ade.ps1"
            launcher.parent.mkdir(parents=True)
            launcher.write_text(
                "Write-Output 'ADE-RESULT: {\"status\":\"Healthy\"}'\nexit 0\n", encoding="utf-8")
            plan = {
                "schema": "winui-profiler-control-scenario/v1", "status": "Ready",
                "request": "Populate 20 buttons, then clear all buttons.",
                "name": "Populate and clear", "initialButtonCount": 0,
                "steps": [{"action": "populate", "count": 20}, {"action": "clear"}],
            }
            path = root / "plan.json"
            path.write_text(json.dumps(plan), encoding="utf-8")
            profiler = root / "fixture-only.exe"
            profiler.write_text("not an executable", encoding="utf-8")
            completed = subprocess.run(
                ["pwsh", "-NoProfile", "-File", str(entry), "-Action", "Prepare",
                 "-Scenario", "Control", "-ScenarioPlan", str(path),
                 "-ScenarioPlanSha256", "0" * 64, "-ProfilerCli", str(profiler),
                 "-DotNet", shutil.which("dotnet"), "-ArtifactsRoot", str(root / "artifacts")],
                capture_output=True, text=True, timeout=30, check=False,
            )
            self.assertNotEqual(0, completed.returncode)
            self.assertIn("changed after review", completed.stdout + completed.stderr)
            staged = next((root / "artifacts").iterdir()) / "sample"
            self.assertFalse((staged / "scenario-plan.json").exists())
            self.assertFalse((staged / "measurement-contract.json").exists())

    @unittest.skipUnless(shutil.which("pwsh"), "PowerShell 7 is required for adapter staging.")
    def test_custom_measurement_cannot_reuse_the_seeded_fix_gate(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            poc = Path(__file__).resolve().parents[1]
            source = root / "sample"
            (source / "helpers").mkdir(parents=True)
            (source / "helpers" / "verify.py").write_text("# fixture", encoding="utf-8")
            state = root / "state.json"
            state.write_text(json.dumps({
                "Schema": "winui-profiler-ade-poc/state-v1", "Scenario": "Control",
                "SampleRoot": str(source), "RunId": None, "ScenarioPlanSha256": "frozen-plan",
            }), encoding="utf-8")
            completed = subprocess.run(
                ["pwsh", "-NoProfile", "-File", str(poc / "Invoke-AdePoc.ps1"),
                 "-Action", "Propose", "-StatePath", str(state)],
                capture_output=True, text=True, timeout=30, check=False,
            )
            self.assertNotEqual(0, completed.returncode)
            self.assertIn("measurement/investigation-only", completed.stdout + completed.stderr)

    @unittest.skipUnless(shutil.which("pwsh"), "PowerShell 7 is required for adapter staging.")
    def test_investigation_stages_shared_helpers_and_configured_sources(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            poc = Path(__file__).resolve().parents[1]
            repo = root / "repo"
            entry = repo / "tools" / "WinUIProfiler.Harness" / "Poc" / "Invoke-AdePoc.ps1"
            entry.parent.mkdir(parents=True)
            shutil.copyfile(poc / "Invoke-AdePoc.ps1", entry)
            launcher = repo / "eng" / "ade" / "ui-profiler" / "scripts" / "ade.ps1"
            launcher.parent.mkdir(parents=True)
            launcher.write_text(
                """$ErrorActionPreference = 'Stop'
if ($args[0] -eq 'doctor') {
    Write-Output 'ADE-RESULT: {"status":"Healthy"}'
} else {
    $args | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'arguments.json')
    Write-Output 'ADE-RESULT: {"runId":"fixture-only","proposalSha256":"fixture-only"}'
}
exit 0
""", encoding="utf-8")
            source = root / "staged app"
            source.mkdir()
            shutil.copytree(poc / "helpers", source / "helpers")
            (source / "scenario.json").write_text(json.dumps({
                "sourceSnapshots": ["OtherApp.cs"],
            }), encoding="utf-8")
            for name in ("OtherApp.cs", "profiler.exe", "database.sqlite", "app-result.json"):
                (source / name).write_text("synthetic fixture, not a capture or executable", encoding="utf-8")
            manifest = source / "capture-manifest.json"
            manifest.write_text(json.dumps({
                "profilerCli": str(source / "profiler.exe"),
                "iterations": [{"database": str(source / "database.sqlite"),
                                "appResult": str(source / "app-result.json")}],
            }), encoding="utf-8")
            baseline = source / "baseline.json"
            baseline.write_text(json.dumps({"captureManifest": str(manifest)}), encoding="utf-8")
            state = source / "state.json"
            state.write_text(json.dumps({
                "Schema": "winui-profiler-ade-poc/state-v1", "Scenario": "Control",
                "SampleRoot": str(source), "BaselineMeasurement": str(baseline), "Investigation": None,
            }), encoding="utf-8")
            completed = subprocess.run(
                ["pwsh", "-NoProfile", "-File", str(entry), "-Action", "Investigate",
                 "-StatePath", str(state)], capture_output=True, text=True, timeout=30, check=False,
            )
            self.assertEqual(0, completed.returncode, completed.stdout + completed.stderr)
            arguments = json.loads((launcher.parent / "arguments.json").read_text(encoding="utf-8-sig"))
            staged = Path(arguments[arguments.index("--repo") + 1])
            self.assertEqual({"scenario.json", "OtherApp.cs", "helpers", "investigation.json"},
                             {path.name for path in staged.iterdir()})
            self.assertEqual({"__init__.py", "metrics.py", "profiler_query.py"},
                             {path.name for path in (staged / "helpers").iterdir()})
            contract = json.loads((staged / "investigation.json").read_text(encoding="utf-8-sig"))
            self.assertEqual(str(source / "profiler.exe"), contract["profilerCli"])


if __name__ == "__main__":
    unittest.main()
