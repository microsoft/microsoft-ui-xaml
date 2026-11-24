# Expose bounded investigation tools to the planning agent; never approve or run a coding round.
import argparse
import asyncio
import copy
from contextlib import asynccontextmanager
import hashlib
import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path, PureWindowsPath
from typing import Any, Literal
from typing_extensions import TypedDict


class ToolResult(TypedDict):
    schema: Literal["ui-profiler/tool-result/v1"]
    operation: str
    state: Literal["planning", "planned", "measured", "analyzed", "recommended", "awaiting-approval", "completed"]
    data: dict[str, Any]


ROOT = Path(__file__).resolve().parent
STATES = {"planning", "planned", "measured", "analyzed", "recommended", "awaiting-approval", "completed"}
CONTROL_TYPES = {
    "Button", "Calendar", "CheckBox", "ComboBox", "Custom", "DataGrid", "DataItem",
    "Document", "Edit", "Group", "Header", "HeaderItem", "Hyperlink", "Image", "List",
    "ListItem", "Menu", "MenuBar", "MenuItem", "Pane", "ProgressBar", "RadioButton",
    "ScrollBar", "Separator", "Slider", "Spinner", "SplitButton", "StatusBar", "Tab",
    "TabItem", "Table", "Text", "Thumb", "TitleBar", "ToolBar", "ToolTip", "Tree",
    "TreeItem", "Window",
}
KEYS = re.compile(
    r"(?:(?:\{(?:DOWN|UP|LEFT|RIGHT|ENTER|TAB|ESC|HOME|END|PGDN|PGUP|SPACE|BACKSPACE|DELETE)\})|\^a)+"
)


def object_keys(value, required, optional=()):
    if not isinstance(value, dict) or set(value) - set(required) - set(optional):
        raise ValueError(f"Expected an object with only {sorted(set(required) | set(optional))}")
    if set(required) - set(value):
        raise ValueError(f"Missing fields: {sorted(set(required) - set(value))}")


def text(value, name, maximum=512):
    if not isinstance(value, str) or not value.strip() or len(value) > maximum or "\0" in value:
        raise ValueError(f"{name} must be nonempty text of at most {maximum} characters")
    return value


def integer(value, name, minimum, maximum):
    if type(value) is not int or not minimum <= value <= maximum:
        raise ValueError(f"{name} must be an integer in [{minimum}, {maximum}]")
    return value


def relative_path(value):
    text(value, "project")
    path = PureWindowsPath(value)
    if path.is_absolute() or path.drive or ".." in path.parts or ":" in value or not path.parts:
        raise ValueError("The project must be relative to the application repository")
    return str(path)


def validate_build(build):
    object_keys(build, ("project", "configuration", "executable"))
    project = relative_path(build["project"])
    if not project.lower().endswith(".csproj"):
        raise ValueError("This adapter supports .NET WinUI .csproj applications")
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_-]{0,79}", text(build["configuration"], "configuration", 80)):
        raise ValueError("Invalid build configuration")
    executable = text(build["executable"], "executable")
    if not re.fullmatch(r"[A-Za-z0-9_.-]+\.exe", executable, re.IGNORECASE):
        raise ValueError("The executable must be a leaf .exe filename")
    return {**build, "project": project}


def validate_window(window):
    object_keys(window, ("width", "height"))
    integer(window["width"], "width", 640, 3840)
    integer(window["height"], "height", 480, 2160)
    return copy.deepcopy(window)


def validate_selector(selector):
    object_keys(selector, (), ("name", "automationId", "className", "controlType", "index"))
    if not any(key in selector for key in ("name", "automationId", "className")):
        raise ValueError("A selector needs a name, automationId or className, not just a control type")
    for key in ("name", "automationId", "className"):
        if key in selector:
            text(selector[key], key)
    if "controlType" in selector and text(selector["controlType"], "controlType") not in CONTROL_TYPES:
        raise ValueError("Unknown UI Automation control type")
    if "index" in selector:
        integer(selector["index"], "index", 0, 511)
    return copy.deepcopy(selector)


def validate_step(step):
    object_keys(step, ("id", "action", "selector", "settleMilliseconds"), ("text", "keys"))
    if not re.fullmatch(r"[a-z0-9][a-z0-9-]{0,79}", text(step["id"], "step id", 80)):
        raise ValueError("A step id must be a lowercase slug")
    if text(step["action"], "action", 32) not in {"type-text", "keys", "invoke", "assert", "expand", "collapse"}:
        raise ValueError("Unsupported UI action; shell commands and arbitrary scripts are not actions")
    validate_selector(step["selector"])
    integer(step["settleMilliseconds"], "settleMilliseconds", 0, 2000)
    required = {"type-text": "text", "keys": "keys"}.get(step["action"])
    if set(step) & {"text", "keys"} != ({required} if required else set()):
        raise ValueError("Only type-text accepts text; only keys accepts keys")
    if required:
        text(step[required], required, 256)
        if any(ord(character) < 32 for character in step[required]):
            raise ValueError("UI input may not contain control characters")
    if required == "keys" and not KEYS.fullmatch(step["keys"]):
        raise ValueError("Only bounded navigation keys and Ctrl+A are permitted")
    return copy.deepcopy(step)


def selector_observed(selector, observations):
    for observation in observations:
        matches = [
            node for node in observation.get("elements", [])
            if not node.get("offscreen", False)
            and all(node.get(key) == value for key, value in selector.items() if key != "index")
        ]
        if "index" in selector and len(matches) > selector["index"]:
            return True
        if "index" not in selector and len(matches) == 1 and not observation.get("truncated", False):
            return True
    return False


def freeze_plan(plan, request, projects, observations):
    object_keys(plan, ("build", "window", "steps"))
    build = validate_build(plan["build"])
    if build["project"].casefold() not in {str(PureWindowsPath(p)).casefold() for p in projects}:
        raise ValueError("The selected project is not a tracked application project")
    validate_window(plan["window"])
    if not isinstance(plan["steps"], list) or not 1 <= len(plan["steps"]) <= 32:
        raise ValueError("A scenario needs between 1 and 32 steps")
    steps = [validate_step(step) for step in plan["steps"]]
    if len({step["id"] for step in steps}) != len(steps):
        raise ValueError("Step ids must be unique")
    for step in steps:
        if not selector_observed(step["selector"], observations):
            raise ValueError(f"Selector for {step['id']} was not uniquely resolved during the unrecorded preview")
    return {
        "schema": "ui-profiler/scenario/v1",
        "name": "user-scenario",
        "description": text(request["scenario"], "scenario", 8000),
        "build": build,
        "window": copy.deepcopy(plan["window"]),
        "steps": steps,
    }


def parse_agent_events(raw, required_server=None, required_tool=None, expected_database=None):
    events = []
    for line in raw.splitlines():
        if line.strip():
            event = json.loads(line)
            if not isinstance(event, dict):
                raise ValueError("An agent event must be an object")
            events.append(event)
    starts = {
        event["data"]["toolCallId"]: event["data"]
        for event in events
        if event.get("type") == "tool.execution_start" and "toolCallId" in event.get("data", {})
    }
    calls = []
    for event in events:
        data = event.get("data", {})
        if event.get("type") != "tool.execution_complete" or data.get("success") is not True:
            continue
        start = starts.get(data.get("toolCallId"), {})
        server = start.get("mcpServerName", "")
        tool = start.get("mcpToolName", start.get("toolName", ""))
        if required_server and server != required_server:
            continue
        if required_tool and tool not in {required_tool, f"{server}-{required_tool}", f"{server}_{required_tool}"}:
            continue
        arguments = start.get("arguments", {})
        if expected_database:
            session = arguments.get("sessionPath") if isinstance(arguments, dict) else None
            if not isinstance(session, str) or str(PureWindowsPath(session)).casefold() != str(
                PureWindowsPath(expected_database)
            ).casefold():
                continue
        calls.append({"server": server, "tool": tool, "id": data["toolCallId"], "arguments": arguments})
    answers = [
        event["data"]["content"] for event in events
        if event.get("type") == "assistant.message"
        and isinstance(event.get("data", {}).get("content"), str)
        and event["data"]["content"].strip()
    ]
    if not answers:
        raise ValueError("The agent produced no final assistant message")
    if required_server and not calls:
        raise ValueError(f"The agent did not complete a successful {required_server}/{required_tool or '*'} query")
    return {"answer": answers[-1], "queries": calls}


def validate_recommendation(value, source):
    object_keys(value, ("schema", "summary", "recommendations", "risks"))
    if value["schema"] != "ui-profiler/recommendations/v1":
        raise ValueError("Unsupported expert recommendation schema/verdict")
    if len(json.dumps(value, allow_nan=False).encode("utf-8")) > 1024 * 1024:
        raise ValueError("The recommendation report exceeds its one-MiB payload budget")
    text(value["summary"], "summary", 4000)
    if not isinstance(value["risks"], list) or not 1 <= len(value["risks"]) <= 16:
        raise ValueError("The report needs 1..16 risks")
    for risk in value["risks"]:
        text(risk, "risk", 4000)
    recommendations = value["recommendations"]
    if not isinstance(recommendations, list) or not 1 <= len(recommendations) <= 64:
        raise ValueError("The bounded recommendation report needs 1..64 entries")
    root = Path(source).resolve(strict=True)
    identifiers = set()
    for recommendation in recommendations:
        object_keys(recommendation, ("id", "title", "verdict", "summary", "observations", "hypotheses", "files", "risks"))
        identifier = text(recommendation["id"], "recommendation id", 80)
        if (not re.fullmatch(r"[a-z0-9][a-z0-9-]{0,79}", identifier) or identifier in identifiers
                or re.fullmatch(r"con|prn|aux|nul|com[1-9]|lpt[1-9]", identifier)):
            raise ValueError("Recommendation ids must be unique lowercase slugs")
        identifiers.add(identifier)
        text(recommendation["title"], "title", 256)
        text(recommendation["summary"], "summary", 4000)
        if recommendation["verdict"] not in {"propose", "insufficient-evidence"}:
            raise ValueError("Unsupported recommendation verdict")
        for field in ("observations", "hypotheses", "risks"):
            if not isinstance(recommendation[field], list) or not 1 <= len(recommendation[field]) <= 16:
                raise ValueError(f"{field} needs 1..16 entries")
            for entry in recommendation[field]:
                text(entry, field, 4000)
        files = recommendation["files"]
        if not isinstance(files, list) or (recommendation["verdict"] == "propose") != bool(files):
            raise ValueError("A proposed fix needs an explicit file scope; insufficient evidence authorizes no files")
        resolved = set()
        for item in files:
            object_keys(item, ("path", "change"))
            path = PureWindowsPath(relative_path(item["path"]))
            target = root.joinpath(*path.parts).resolve(strict=True)
            if not target.is_relative_to(root) or not target.is_file() or target.suffix.lower() not in {".cs", ".xaml"}:
                raise ValueError("The expert may recommend changes only to existing app C#/XAML files")
            if target in resolved or any(part.lower() == ".github" or part.lower().endswith(("test", "tests"))
                                         for part in path.parts):
                raise ValueError("Duplicate or protected expert file")
            resolved.add(target)
            text(item["change"], "change", 4000)
    return value


class Harness:
    def __init__(self, request=None):
        self.request = request or {}
        self.active_previews = set()
        self.contracts_sha256 = ""

    def resolve_directory(self, directory=None):
        directory = directory or self.request.get("directory")
        text(directory, "investigation directory")
        if not PureWindowsPath(directory).is_absolute():
            raise ValueError("The investigation directory must be an absolute Windows path")
        if self.request and str(PureWindowsPath(directory)).casefold() != str(
            PureWindowsPath(self.request["directory"])
        ).casefold():
            raise ValueError("This controller is bound to another investigation directory")
        return directory

    def call(self, operation, directory=None, **arguments):
        directory = self.resolve_directory(directory)
        payload = {"directory": directory, **arguments}
        with tempfile.TemporaryDirectory(prefix="ui-profiler-tool-") as temporary:
            path = Path(temporary) / "arguments.json"
            path.write_text(json.dumps(payload, allow_nan=False), encoding="utf-8")
            result = subprocess.run(
                ["pwsh", "-NoProfile", "-NonInteractive", "-File",
                 str(ROOT / "Invoke-HarnessTool.ps1"), "-Operation", operation, "-ArgumentsPath", str(path)],
                capture_output=True, encoding="utf-8", timeout=1800, check=False,
                env={**os.environ, "PATH": str(Path(sys.executable).parent) + os.pathsep + os.environ.get("PATH", "")},
            )
        lines = [
            line.removeprefix("UI-PROFILER-RESULT: ") for line in result.stdout.splitlines()
            if line.startswith("UI-PROFILER-RESULT: ")
        ]
        if result.returncode or not lines:
            raise RuntimeError(f"{operation} failed; no success was recorded.\n{result.stderr[-6000:]}")
        report = json.loads(lines[-1])
        object_keys(report, ("schema", "operation", "state", "data"))
        if report["schema"] != "ui-profiler/tool-result/v1" or report["operation"] != operation or report["state"] not in STATES:
            raise ValueError("Invalid harness tool output")
        if operation == "Preview":
            self.active_previews.add(directory)
        elif operation in {"ClosePreview", "Freeze"}:
            self.active_previews.discard(directory)
        return report


def create_server(request=None):
    from mcp.server.fastmcp import FastMCP
    from mcp.types import ToolAnnotations

    harness = Harness(request)

    @asynccontextmanager
    async def lifespan(_server):
        try:
            yield {}
        finally:
            for directory in list(harness.active_previews):
                try:
                    harness.call("ClosePreview", directory)
                except Exception as error:
                    print(f"Owned preview cleanup could not complete: {error}", file=sys.stderr)

    server = FastMCP("ui-profiler-harness", lifespan=lifespan)
    read_only = ToolAnnotations(readOnlyHint=True, destructiveHint=False, openWorldHint=False)
    controlled = ToolAnnotations(readOnlyHint=False, destructiveHint=False, openWorldHint=False)

    @server.tool(annotations=read_only)
    def harness_contracts(directory: str = "") -> ToolResult:
        """Read mandatory contracts before planning/pivoting; after begin, use the hash-bound run copy.

        This is guidance, not permission to override deterministic validation, freeze, capture or
        approval gates. The source contract path is fixed; the caller cannot supply another file.
        """
        selected = directory or harness.request.get("directory")
        if selected:
            selected = harness.resolve_directory(selected)
        if selected and (Path(selected) / "context.json").is_file():
            return harness.call("Contracts", selected)
        path = ROOT.parents[1] / "CONTRACTS.md"
        raw = path.read_bytes()
        harness.contracts_sha256 = hashlib.sha256(raw).hexdigest()
        return {"schema": "ui-profiler/tool-result/v1", "operation": "Contracts", "state": "planning",
                "data": {"path": str(path), "sha256": harness.contracts_sha256,
                         "text": raw.decode("utf-8-sig")}}

    @server.tool(annotations=controlled)
    def begin(
        directory: str = "", application_repository: str = "", scenario: str = "",
        knowledge_repository: str = "", expert_home: str = "", profiler_cli: str = "",
        dotnet: str = "", ade_copilot_cli: str = "", github_account: str = "",
        allow_preview: bool = False, allow_measurements: bool = False,
    ) -> ToolResult:
        """Bind the user's natural-language request and copy clean app source. No app launch/capture.

        For a request-bound CLI session, omit arguments. Otherwise supply user-approved absolute
        paths and the user's scenario verbatim. Never substitute a checked-in scenario file.
        """
        supplied = {
            "directory": directory, "applicationRepository": application_repository,
            "scenario": scenario, "knowledgeRepository": knowledge_repository,
            "expertHome": expert_home, "profilerCli": profiler_cli, "dotnet": dotnet,
            "adeCopilotCli": ade_copilot_cli, "githubAccount": github_account,
            "allowPreview": allow_preview, "allowMeasurements": allow_measurements,
        }
        selected = dict(harness.request)
        if not harness.contracts_sha256:
            raise ValueError("Read harness_contracts before beginning an investigation")
        if selected.get("contractsSha256", harness.contracts_sha256) != harness.contracts_sha256:
            raise ValueError("The contracts changed since the original request; use a new investigation")
        selected["contractsSha256"] = harness.contracts_sha256
        for key, value in supplied.items():
            if value and key in selected and selected[key] != value:
                raise ValueError(f"{key} differs from the original user request")
            if value:
                selected[key] = value
        for key in ("directory", "applicationRepository", "scenario", "knowledgeRepository", "expertHome", "profilerCli"):
            text(selected.get(key), key, 8000 if key == "scenario" else 1024)
        directory = selected.pop("directory")
        result = harness.call("Initialize", directory, request=selected)
        harness.request = {"directory": directory, **selected}
        return result

    @server.tool(annotations=read_only)
    def inspect_application(directory: str = "") -> ToolResult:
        """List tracked .NET application projects/instructions; inspect source using view, not shell."""
        return harness.call("Inspect", directory)

    @server.tool(annotations=controlled)
    def preview_application(build: dict, window: dict, directory: str = "") -> ToolResult:
        """Build a separate preview copy and launch exactly its owned app, unrecorded, for UI discovery.

        build: project(relative .csproj), configuration, executable(leaf .exe).
        window: width, height. One preview, at most 64 actions; no profiler/capture is started.
        """
        return harness.call("Preview", directory, build=validate_build(build), window=validate_window(window))

    @server.tool(annotations=controlled)
    def inspect_ui(directory: str = "") -> ToolResult:
        """Inspect up to 512 visible UIA elements of only the preview PID/executable/creation time."""
        return harness.call("InspectUi", directory)

    @server.tool(annotations=controlled)
    def act_on_ui(step: dict, directory: str = "") -> ToolResult:
        """Perform one safe UIA step on the owned preview; no capture. Then inspect_ui to observe it.

        A step has id, action, selector, settleMilliseconds and text/keys only when needed.
        Actions: type-text, keys, invoke, assert, expand, collapse. Selector: name/automationId/
        className, optional controlType/index. Only navigation keys/Ctrl+A, no scripts/shortcuts.
        """
        return harness.call("Act", directory, step=validate_step(step))

    @server.tool(annotations=controlled)
    def freeze_scenario(plan: dict, directory: str = "") -> ToolResult:
        """Validate and freeze a run-local plan derived from the user's language and observed UI.

        plan has build, window, steps (1..32). Every selector must have resolved in a preview
        inspection; ambiguous matches require an index. User text is preserved, not model-rewritten.
        Closes only the owned preview. The exact plan/hash is replayed at both measurements.
        """
        object_keys(plan, ("build", "window", "steps"))
        return harness.call("Freeze", directory, plan=plan)

    @server.tool(annotations=controlled)
    def close_preview(directory: str = "") -> ToolResult:
        """Close only the verified task-owned preview; never terminate another user's application."""
        return harness.call("ClosePreview", directory)

    @server.tool(annotations=controlled)
    def measure_baseline(directory: str = "") -> ToolResult:
        """Build/deploy and record the frozen scenario once; failed/reserved budgets never retry."""
        return harness.call("Measure", directory)

    @server.tool(annotations=controlled)
    def analyze_baseline(directory: str = "") -> ToolResult:
        """Export all dimensions deterministically and hash-bind reports, inputs and capability states."""
        return harness.call("Analyze", directory)

    @server.tool(annotations=controlled)
    def recommend_fix(directory: str = "") -> ToolResult:
        """Keep the actual expert's full ranked recommendation report against the shared baseline.

        Requires a successful real profiler evidence MCP call and a nonempty structured transcript.
        Each actionable item is an independent trial, never a cumulative fix. Insufficient-evidence
        entries stay in the report without authorizing edits. No app edit or additional capture.
        """
        return harness.call("Recommend", directory)

    @server.tool(annotations=controlled)
    def propose_fix(recommendation_id: str, directory: str = "") -> ToolResult:
        """Prepare one independent recommendation's ADE subagent/workstream and exact-hash proposal.

        Call once per actionable id in the frozen report. Every trial starts from original source
        and reuses the same baseline; its explicit file scope, not a two-file cap, limits edits.
        No approve/run/apply API; stop for human approval. Candidates are profiled sequentially.
        """
        text(recommendation_id, "recommendation id", 80)
        return harness.call("Propose", directory, recommendationId=recommendation_id)

    @server.tool(annotations=read_only)
    def recommendation_report(directory: str = "") -> ToolResult:
        """Read every recommendation/proposal/outcome and its shared-baseline comparison references."""
        return harness.call("Report", directory)

    @server.tool(annotations=read_only)
    def investigation_status(directory: str = "") -> ToolResult:
        """Read the actual phase, frozen hashes, budgets and last failure; never invent completion."""
        return harness.call("Status", directory)

    return server


async def probe_mcp(configuration_path):
    from mcp import ClientSession, StdioServerParameters
    from mcp.client.stdio import stdio_client

    configuration = json.loads(Path(configuration_path).read_text(encoding="utf-8-sig"))
    servers = {}
    for name, server in configuration["mcpServers"].items():
        parameters = StdioServerParameters(
            command=server["command"], args=server.get("args", []),
            env={**os.environ, **server.get("env", {})},
        )
        async with stdio_client(parameters) as streams:
            async with ClientSession(*streams) as session:
                await session.initialize()
                tools = (await session.list_tools()).tools
        if not tools:
            raise ValueError(f"The required {name} MCP server exposes no tools")
        if any(tool.annotations is None or tool.annotations.readOnlyHint is not True for tool in tools):
            raise ValueError(f"The required {name} MCP catalog must contain only read-only tools")
        servers[name] = [tool.name for tool in tools]
    if "winui-profiler" not in servers:
        raise ValueError("The original WinUIProfiler MCP server is required")
    return {"servers": servers, "queries": False, "captures": False}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--request")
    parser.add_argument("--validate-plan")
    parser.add_argument("--parse-agent-events")
    parser.add_argument("--required-server")
    parser.add_argument("--required-tool")
    parser.add_argument("--recommendation")
    parser.add_argument("--application-source")
    parser.add_argument("--probe-mcp")
    parser.add_argument("--expected-database")
    args = parser.parse_args()
    if args.validate_plan:
        bundle = json.loads(Path(args.validate_plan).read_text(encoding="utf-8-sig"))
        print(json.dumps(freeze_plan(**bundle), allow_nan=False))
    elif args.probe_mcp:
        print(json.dumps(asyncio.run(probe_mcp(args.probe_mcp)), allow_nan=False))
    elif args.recommendation:
        value = json.loads(Path(args.recommendation).read_text(encoding="utf-8-sig"))
        print(json.dumps(validate_recommendation(value, args.application_source), allow_nan=False))
    elif args.parse_agent_events:
        print(json.dumps(parse_agent_events(
            Path(args.parse_agent_events).read_text(encoding="utf-8-sig"),
            args.required_server, args.required_tool,
            args.expected_database,
        ), allow_nan=False))
    else:
        request = json.loads(Path(args.request).read_text(encoding="utf-8-sig")) if args.request else None
        create_server(request).run(transport="stdio")


if __name__ == "__main__":
    main()
