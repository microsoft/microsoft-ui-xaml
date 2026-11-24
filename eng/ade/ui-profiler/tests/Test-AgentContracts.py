"""Harness contracts, not generated app-code tests. Synthetic files and local MCP only; zero captures."""
import asyncio
import copy
import importlib.util
import json
import pathlib
import subprocess
import sys
import tempfile
import unittest
from unittest import mock


TOOLS = pathlib.Path(__file__).parents[1] / "pack" / "tools"


def module(name):
    spec = importlib.util.spec_from_file_location(name.replace("-", "_"), TOOLS / (name + ".py"))
    value = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(value)
    return value


harness = module("Harness-Mcp")
evidence = module("Evidence-Mcp")


def step():
    return {"id": "open", "action": "invoke", "selector": {"automationId": "Open", "controlType": "Button"},
            "settleMilliseconds": 100}


def plan():
    return {"build": {"project": "Example\\Example.csproj", "configuration": "Release-Unpackaged",
                      "executable": "Example.exe"}, "window": {"width": 1280, "height": 900}, "steps": [step()]}


def events(success=True):
    return "\n".join(json.dumps(item) for item in [
        {"type": "tool.execution_start", "data": {"toolCallId": "query-1", "mcpServerName": "ui-profiler-evidence",
                                                 "mcpToolName": "evidence"}},
        {"type": "tool.execution_complete", "data": {"toolCallId": "query-1", "success": success}},
        {"type": "assistant.message", "data": {"content": "An evidence-grounded answer"}},
    ])


class PlanContracts(unittest.TestCase):
    def setUp(self):
        self.request = {"scenario": "Open my app's details and invoke Open."}
        self.projects = ["Example/Example.csproj"]
        self.observations = [{"elements": [{"automationId": "Open", "controlType": "Button", "offscreen": False}]}]

    def freeze(self, value=None):
        return harness.freeze_plan(value or plan(), self.request, self.projects, self.observations)

    def test_user_language_is_preserved_in_generated_run_local_plan(self):
        result = self.freeze()
        self.assertEqual(result["description"], self.request["scenario"])
        self.assertEqual(result["build"], plan()["build"])
        self.assertEqual(result["steps"], plan()["steps"])
        self.assertNotIn("Gallery", json.dumps(result))

    def test_unknown_fields_actions_and_os_shortcuts_are_rejected(self):
        for update in ({"script": "anything"}, {"action": "shell"}, {"action": "keys", "keys": "%{F4}"},
                       {"action": "keys", "keys": "^{ESC}"}, {"action": "keys", "keys": "{DOWN}" * 100},
                       {"action": "type-text", "text": "line\nenter"}):
            with self.subTest(update=update), self.assertRaises(ValueError):
                harness.validate_step({**step(), **update})
        value = {**step(), "action": "keys", "keys": "{DOWN}{ENTER}"}
        self.assertEqual(harness.validate_step(value), value)

    def test_project_paths_and_command_injection_are_rejected(self):
        for project in ("..\\outside.csproj", "C:\\outside.csproj", "C:outside.csproj", "\\\\server\\a.csproj"):
            with self.subTest(project=project), self.assertRaises(ValueError):
                harness.validate_build({**plan()["build"], "project": project})
        for update in ({"configuration": "Release;command"}, {"executable": "..\\other.exe"},
                       {"project": "not-tracked.csproj"}):
            value = plan()
            value["build"].update(update)
            with self.subTest(update=update), self.assertRaises(ValueError):
                self.freeze(value)

    def test_ambiguous_missing_offscreen_and_truncated_selectors_are_not_proof(self):
        node = copy.deepcopy(self.observations[0]["elements"][0])
        for observation in ({"elements": []}, {"elements": [{**node, "offscreen": True}]},
                            {"elements": [node, node]}, {"elements": [node], "truncated": True}):
            self.observations[:] = [observation]
            with self.subTest(observation=observation), self.assertRaises(ValueError):
                self.freeze()
        self.observations[:] = [{"elements": [node, node]}]
        value = plan()
        value["steps"][0]["selector"]["index"] = 1
        self.assertEqual(self.freeze(value)["steps"][0]["selector"]["index"], 1)

    def test_step_window_and_selector_bounds_fail_closed(self):
        for update in ({"window": {"width": 0, "height": 900}}, {"steps": []},
                       {"steps": [step()] * 33}, {"steps": [step(), step()]}):
            with self.subTest(update=update), self.assertRaises(ValueError):
                self.freeze({**plan(), **update})
        for selector in ({"processId": 123}, {"controlType": "Button"}, {"name": "a", "controlType": "Script"},
                         {"automationId": "Open", "index": -1}):
            with self.subTest(selector=selector), self.assertRaises(ValueError):
                harness.validate_selector(selector)

    def test_agent_text_without_a_successful_real_mcp_call_is_rejected(self):
        self.assertEqual(len(harness.parse_agent_events(events(), "ui-profiler-evidence", "evidence")["queries"]), 1)
        for value in (events(False), '{"type":"assistant.message","data":{"content":"I queried it"}}',
                      events().replace("ui-profiler-evidence", "some-other-server")):
            with self.subTest(value=value), self.assertRaises(ValueError):
                harness.parse_agent_events(value, "ui-profiler-evidence", "evidence")

    def test_native_queries_must_target_the_exact_measured_database(self):
        expected = "C:\\Runs\\baseline\\recording.sqlite"
        native = [json.loads(line) for line in events().replace("ui-profiler-evidence", "winui-profiler").splitlines()]
        for arguments in ({}, {"sessionPath": "C:\\Other\\recording.sqlite"}):
            native[0]["data"]["arguments"] = arguments
            with self.subTest(arguments=arguments), self.assertRaises(ValueError):
                harness.parse_agent_events("\n".join(map(json.dumps, native)), "winui-profiler", expected_database=expected)
        native[0]["data"]["arguments"] = {"sessionPath": expected}
        result = harness.parse_agent_events("\n".join(map(json.dumps, native)), "winui-profiler", expected_database=expected)
        self.assertEqual(result["queries"][0]["arguments"]["sessionPath"], expected)

    def test_expert_output_requires_a_safe_explicit_verdict_and_existing_files(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            for name in ("App.xaml", "Page.xaml", "Page.xaml.cs"):
                (root / name).write_text("synthetic app code", encoding="utf-8")
            entry = {"id": "defer-work", "title": "Defer hidden work", "verdict": "propose",
                     "summary": "Defer hidden work.", "observations": ["Measured evidence"],
                     "hypotheses": ["Uncertain cause"], "risks": ["Check accessibility."],
                     "files": [{"path": name, "change": "Load only when visible."}
                               for name in ("App.xaml", "Page.xaml", "Page.xaml.cs")]}
            value = {"schema": "ui-profiler/recommendations/v1", "summary": "Independent trials.",
                     "recommendations": [entry, {**entry, "id": "other-variant"}], "risks": ["Variants are not combined."]}
            self.assertEqual(harness.validate_recommendation(value, root), value)
            for update in ({"files": []}, {"files": [{"path": "..\\outside.cs", "change": "bad"}]},
                           {"verdict": "insufficient-evidence"}, {"observations": []}, {"approved": True},
                           {"id": "../escape"}):
                with self.subTest(update=update), self.assertRaises((ValueError, FileNotFoundError)):
                    harness.validate_recommendation({**value, "recommendations": [{**entry, **update}]}, root)
            self.assertEqual(harness.validate_recommendation(
                {**value, "recommendations": [{**entry, "verdict": "insufficient-evidence", "files": []}]},
                root)["recommendations"][0]["files"], [])
            with self.assertRaises(ValueError):
                harness.validate_recommendation({**value, "recommendations": [entry, entry]}, root)
            self.assertEqual(len(value["recommendations"][0]["files"]), 3)

    def test_unstructured_failed_or_wrong_stage_tool_output_cannot_be_success(self):
        client = harness.Harness({"directory": "C:\\Runs\\bound"})
        with self.assertRaises(ValueError):
            client.call("Status", "C:\\Runs\\different")
        for code, output in ((1, ""), (0, "Success!"), (0, 'UI-PROFILER-RESULT: {"schema":"other"}')):
            with self.subTest(code=code, output=output), mock.patch.object(
                subprocess, "run", return_value=subprocess.CompletedProcess([], code, output, "failure")
            ), self.assertRaises((RuntimeError, ValueError)):
                client.call("Status")

    def test_real_adapter_binds_source_freezes_plan_and_denies_unapproved_measurement(self):
        with tempfile.TemporaryDirectory(prefix="ui-profiler-adapter-contract-") as temporary:
            root = pathlib.Path(temporary)
            app, knowledge, expert, profiler, run = (root / name for name in ("app", "knowledge", "expert", "profiler", "run"))
            (app / "Example").mkdir(parents=True)
            (app / "Example" / "Example.csproj").write_text("<Project/>", encoding="utf-8")
            (app / "Example" / "App.xaml").write_text("<Grid/>", encoding="utf-8")
            launcher = knowledge / "tools" / "WinUIProfiler" / "scripts" / "Start-McpServer.ps1"
            launcher.parent.mkdir(parents=True)
            launcher.write_text("# synthetic test fixture; never executed", encoding="utf-8")
            (expert / "rag").mkdir(parents=True)
            (expert / "winui-expert.agent.md").write_text("Synthetic existence fixture; never invoked.", encoding="utf-8")
            (expert / "rag" / "rag.py").write_text("# never invoked", encoding="utf-8")
            profiler.mkdir()
            (profiler / "WinUIProfiler.Cli.exe").write_bytes(b"never executed or substituted for a recording")
            for arguments in (
                ["init", "-q"], ["add", "--", "Example/Example.csproj", "Example/App.xaml"],
                ["-c", "user.name=Harness Test", "-c", "user.email=test@example.invalid", "commit", "-qm",
                 "Synthetic contract fixture\n\nCo-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>\n"
                 "Copilot-Session: 895bb146-abbd-4f74-a580-f5970b30689d"],
            ):
                subprocess.run(["git", "-C", str(app), *arguments], check=True, capture_output=True)
            client = harness.Harness({"directory": str(run)})
            request = {"applicationRepository": str(app), "scenario": self.request["scenario"],
                       "knowledgeRepository": str(knowledge), "expertHome": str(expert),
                       "profilerCli": str(profiler / "WinUIProfiler.Cli.exe"), "dotnet": sys.executable,
                       "allowPreview": True, "allowMeasurements": False}
            initialized = client.call("Initialize", request=request)
            self.assertEqual(initialized["state"], "planning")
            self.assertEqual(client.call("Inspect")["data"]["scenario"], self.request["scenario"])
            context_path = run / "context.json"
            context = json.loads(context_path.read_text(encoding="utf-8-sig"))
            self.assertEqual(client.call("Contracts")["data"]["sha256"], context["contracts"]["sha256"])
            observation = run / "synthetic-ui.json"
            observation.write_text(json.dumps(self.observations[0]), encoding="utf-8")
            context["preview"] = {
                "identity": {"processId": 2147483000, "creationTimeFileTime": 0},
                "executable": str(run / "never-launched.exe"), "openedUtc": "2999-01-01T00:00:00Z",
                "closed": False, "build": plan()["build"], "window": plan()["window"],
                "actions": [], "inspections": [{"path": str(observation), "sha256": evidence.digest(observation)}],
            }
            context_path.write_text(json.dumps(context), encoding="utf-8")
            frozen = client.call("Freeze", plan=plan())
            self.assertEqual(frozen["state"], "planned")
            self.assertEqual(frozen["data"]["scenario"]["description"], self.request["scenario"])
            before = (run / "scenario.json").read_bytes()
            with self.assertRaises(RuntimeError):
                client.call("Freeze", plan=plan())
            with self.assertRaises(RuntimeError):
                client.call("Measure")
            self.assertEqual((run / "scenario.json").read_bytes(), before)
            self.assertFalse((run / "baseline" / "capture-budget.json").exists())
            self.assertEqual((app / "Example" / "App.xaml").read_text(encoding="utf-8"), "<Grid/>")
            contract_path = pathlib.Path(context["contracts"]["path"])
            original_contract = contract_path.read_bytes()
            contract_path.write_text("Changed contract snapshot", encoding="utf-8")
            with self.assertRaises(RuntimeError):
                client.call("Contracts")
            contract_path.write_bytes(original_contract)
            pathlib.Path(context["module"]["path"]).write_text("throw 'Never execute a changed adapter'", encoding="utf-8")
            with self.assertRaisesRegex(RuntimeError, "frozen harness module"):
                client.call("Contracts")


class EvidenceContracts(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="ui-profiler-mcp-contract-")
        self.root = pathlib.Path(self.temporary.name)
        self.path = self.root / "analysis.json"
        self.database = self.root / "recording.sqlite"
        self.database.write_bytes(b"synthetic bytes, not a profiler recording")
        self.actions = self.root / "actions.json"
        self.actions.write_text("{}", encoding="utf-8")
        self.measurement = self.root / "measurement.json"
        self.measurement.write_text('{"synthetic":true}', encoding="utf-8")
        self.unrelated = self.root / "queries" / "manifest.json"
        self.unrelated.parent.mkdir()
        self.unrelated.write_text('{"payload":"not an evidence manifest"}', encoding="utf-8")
        self.report = {
            "schema": "ui-profiler/analysis/v1", "interpretation": "Observed, not statistical significance.",
            "measurement": self.reference(self.measurement), "database": self.reference(self.database),
            "actions": self.reference(self.actions),
            "capabilities": {"memory": {"status": "available"}, "resources": {"status": "unavailable"}},
            "scopeAvailability": {"memory/scenario": "available", "resources/scenario": "unavailable"},
            "metrics": [
                {"family": "memory", "scope": "scenario", "metric": f"bytes.{index}", "value": 2**64 - 1 - index,
                 "unit": "bytes", "status": "available", "direction": "lower"} for index in range(3)
            ] + [{"family": "resources", "scope": "scenario", "metric": "bytes", "value": None,
                  "unit": "bytes", "status": "unavailable", "direction": "context"}],
            "details": {"series": [{"value": 5, "samples": [1, 2, 3]}], "launch/definitions": {"Count": 1}},
        }
        self.seal()

    def tearDown(self):
        self.temporary.cleanup()

    def reference(self, path, origin="recorded"):
        return {"path": str(path), "sha256": evidence.digest(path), "bytes": path.stat().st_size, "origin": origin}

    def seal(self):
        self.path.write_text(json.dumps(self.report), encoding="utf-8")
        self.manifest = {
            "schema": "ui-profiler/evidence-manifest/v1",
            "inputs": [self.reference(path) for path in (self.database, self.actions, self.measurement, self.unrelated)],
            "outputs": [self.reference(self.path, "derived")],
        }
        self.save_manifest()

    def save_manifest(self):
        self.path.with_name("manifest.json").write_text(json.dumps(self.manifest), encoding="utf-8")

    def test_all_capabilities_and_unavailable_states_remain_explicit(self):
        value = evidence.query(self.path)
        self.assertEqual(value["capabilities"]["resources"]["status"], "unavailable")
        self.assertEqual(value["provenance"]["kind"], "team-derived-report")
        self.assertEqual(value["provenance"]["queries"], [])
        row = evidence.query(self.path, "metrics", family="resources")["rows"][0]
        self.assertIsNone(row["value"])
        self.assertEqual(row["status"], "unavailable")

    def test_integer_bytes_paging_scope_and_metric_filters_are_exact(self):
        value = evidence.query(self.path, "metrics", family="memory", scope="scenario", offset=1, limit=1)
        self.assertEqual(value["rows"][0]["value"], 2**64 - 2)
        self.assertEqual(value["rows"][0]["valueExact"], str(2**64 - 2))
        self.assertEqual(value["total"], 3)
        self.assertEqual(value["nextOffset"], 2)
        self.assertEqual(evidence.query(self.path, "metrics", family="memory", metric="bytes.2")["returned"], 1)
        self.assertEqual(evidence.query(self.path, "metrics", family="memory", scope="other")["returned"], 0)

    def test_capability_scopes_are_paged_and_every_reply_is_bounded(self):
        value = evidence.query(self.path, offset=1, limit=1)
        self.assertEqual(value["scopes"], {"resources/scenario": "unavailable"})
        self.assertEqual(value["total"], 2)
        self.assertIsNone(value["nextOffset"])
        self.assertEqual(evidence.query(self.path, family="memory")["scopes"], {"memory/scenario": "available"})
        self.report["interpretation"] = "x" * (evidence.MAX_REPLY_BYTES + 1)
        self.seal()
        with self.assertRaisesRegex(ValueError, "reply limit"):
            evidence.query(self.path)

    def test_nonfinite_report_numbers_are_rejected_and_large_detail_bytes_remain_exact(self):
        self.report["details"]["bytes"] = 2**64 - 1
        self.seal()
        value = evidence.query(self.path, "details", section="/bytes")["rows"][0]["value"]
        self.assertEqual(value["exactDecimal"], str(2**64 - 1))
        for number in (float("nan"), float("inf"), -float("inf")):
            self.report["metrics"][0]["value"] = number
            self.seal()
            with self.subTest(number=number), self.assertRaisesRegex(ValueError, "Non-finite"):
                evidence.query(self.path)
        with self.assertRaisesRegex(ValueError, "Non-finite"):
            evidence.parse_json('{"value":1e400}')

    def test_details_are_bounded_and_expandable_without_dumping_nested_trees(self):
        value = evidence.query(self.path, "details", section="/series")
        self.assertEqual(value["rows"][0]["value"]["samples"], {"pointer": "/series/0/samples", "kind": "array", "count": 3})
        self.assertEqual(evidence.query(self.path, "details", section="/series/0/samples")["rows"][1]["value"], 2)
        self.assertEqual(evidence.query(self.path, "details", section="/launch~1definitions")["rows"][0]["value"], 1)
        for arguments in ({"limit": 201}, {"offset": -1}, {"view": "sql"},
                          {"view": "metrics"}, {"view": "details", "section": "..\\file"}):
            with self.subTest(arguments=arguments), self.assertRaises(ValueError):
                evidence.query(self.path, **arguments)

    def test_report_and_every_input_tampering_is_rejected(self):
        for target in (self.path, self.database, self.actions, self.unrelated):
            original = target.read_bytes()
            target.write_bytes(original + b" ")
            with self.subTest(target=target), self.assertRaises(ValueError):
                evidence.query(self.path)
            target.write_bytes(original)

    def test_unbound_reports_unsupported_schema_and_wrong_references_are_rejected(self):
        self.manifest["outputs"] = []
        self.save_manifest()
        with self.assertRaisesRegex(ValueError, "bound"):
            evidence.query(self.path)
        self.report["schema"] = "future-unknown"
        self.seal()
        with self.assertRaisesRegex(ValueError, "schema"):
            evidence.query(self.path)
        self.report["schema"] = "ui-profiler/analysis/v1"
        self.report["database"]["sha256"] = "0" * 64
        self.seal()
        with self.assertRaisesRegex(ValueError, "database"):
            evidence.query(self.path)

    def test_only_declared_manifest_origins_recurse_and_cycles_fail_closed(self):
        evidence.query(self.path)  # unrelated queries\\manifest.json is not recursively interpreted
        child = self.root / "child.json"
        child.write_text(json.dumps({"schema": "ui-profiler/evidence-manifest/v1", "inputs": [], "outputs": []}), encoding="utf-8")
        self.manifest["inputs"].append(self.reference(child, "evidence-manifest"))
        self.save_manifest()
        evidence.query(self.path)
        child.write_text("{}", encoding="utf-8")
        with self.assertRaises(ValueError):
            evidence.query(self.path)
        with self.assertRaisesRegex(ValueError, "Cyclic"):
            evidence.verify_manifest(child, active={child.resolve()})

    def test_real_stdio_mcp_catalog_and_error_contracts(self):
        from mcp import ClientSession, StdioServerParameters
        from mcp.client.stdio import stdio_client

        async def run():
            arguments = ["-B", str(TOOLS / "Evidence-Mcp.py"), "--analysis", str(self.path)]
            async with stdio_client(StdioServerParameters(command=sys.executable, args=arguments)) as streams:
                async with ClientSession(*streams) as session:
                    await session.initialize()
                    tools = (await session.list_tools()).tools
                    self.assertEqual([tool.name for tool in tools], ["evidence"])
                    self.assertTrue(tools[0].annotations.readOnlyHint)
                    result = await session.call_tool("evidence", {"view": "metrics", "family": "memory", "limit": 1})
                    self.assertFalse(result.isError, result.model_dump_json())
                    self.assertEqual(result.structuredContent["rows"][0]["value"], 2**64 - 1)
                    rejected = await session.call_tool("evidence", {"view": "sql"})
                    self.assertTrue(rejected.isError)
            async with stdio_client(StdioServerParameters(
                command=sys.executable, args=["-B", str(TOOLS / "Harness-Mcp.py")]
            )) as streams:
                async with ClientSession(*streams) as session:
                    await session.initialize()
                    names = {tool.name for tool in (await session.list_tools()).tools}
                    self.assertTrue({"harness_contracts", "begin", "freeze_scenario", "propose_fix"} <= names)
                    self.assertFalse({"approve", "run", "apply", "capture_candidate"} & names)
                    unguided = await session.call_tool("begin", {})
                    self.assertTrue(unguided.isError)
                    contracts = await session.call_tool("harness_contracts", {})
                    self.assertFalse(contracts.isError, contracts.model_dump_json())
                    reference = contracts.structuredContent["data"]
                    self.assertEqual(reference["sha256"], evidence.digest(TOOLS.parents[1] / "CONTRACTS.md"))
                    self.assertIn("Legal pivot", reference["text"])
                    rejected = await session.call_tool("act_on_ui", {"step": {**step(), "action": "shell"}})
                    self.assertTrue(rejected.isError)
            configuration = self.root / "wrong-native-mcp.json"
            configuration.write_text(json.dumps({"mcpServers": {"winui-profiler": {
                "command": sys.executable, "args": ["-B", str(TOOLS / "Harness-Mcp.py")],
            }}}), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "only read-only"):
                await harness.probe_mcp(configuration)

        asyncio.run(run())


if __name__ == "__main__":
    unittest.main(verbosity=2)
