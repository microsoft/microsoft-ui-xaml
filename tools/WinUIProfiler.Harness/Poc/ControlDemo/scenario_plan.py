import argparse
import hashlib
import json
from pathlib import Path
import sys


class ScenarioError(Exception):
    pass


def read_plan(path: Path) -> dict:
    return parse_plan(path.read_bytes())


def parse_plan(data: bytes) -> dict:
    if len(data) > 16384:
        raise ScenarioError("The scenario plan exceeds 16 KiB.")
    def unique_fields(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ScenarioError(f"The plan contains a duplicate field: {key}")
            result[key] = value
        return result
    return json.loads(data.decode("utf-8-sig"), object_pairs_hook=unique_fields)


def integer(value, minimum: int, maximum: int, label: str) -> int:
    if type(value) is not int or not minimum <= value <= maximum:
        raise ScenarioError(f"{label} must be an integer from {minimum} to {maximum}.")
    return value


def validate_plan(plan: dict, capabilities: dict, request: str | None = None) -> dict:
    if not isinstance(plan, dict) or plan.get("schema") != capabilities["planSchema"]:
        raise ScenarioError("Unsupported scenario plan schema.")
    if not isinstance(plan.get("request"), str) or not plan["request"].strip():
        raise ScenarioError("The plan must retain the requested natural-language scenario.")
    if request is not None and plan["request"] != request:
        raise ScenarioError("The plan does not match the protected scenario request.")
    status = plan.get("status")
    if status in ("Unsupported", "NeedsClarification"):
        if set(plan) != {"schema", "status", "request", "reason"}:
            raise ScenarioError("A non-ready plan must contain only schema, status, request and reason.")
        if not isinstance(plan["reason"], str) or not plan["reason"].strip():
            raise ScenarioError("An unsupported or ambiguous request requires an explanation.")
        return {"status": status, "reason": plan["reason"]}
    if status != "Ready" or set(plan) != {
        "schema", "status", "request", "name", "initialButtonCount", "steps",
    }:
        raise ScenarioError("Expected a Ready plan with only the documented fields.")
    if not isinstance(plan["name"], str) or not plan["name"].strip() or len(plan["name"]) > 120:
        raise ScenarioError("A scenario name must contain 1-120 characters.")
    limits = capabilities["limits"]
    count = integer(plan["initialButtonCount"], 0, limits["maxButtons"], "initialButtonCount")
    steps = plan["steps"]
    if not isinstance(steps, list) or not 1 <= len(steps) <= limits["maxSteps"]:
        raise ScenarioError(f"A scenario requires 1-{limits['maxSteps']} steps.")
    for index, step in enumerate(steps, 1):
        if not isinstance(step, dict):
            raise ScenarioError(f"Step {index} must be an action object.")
        action = step.get("action")
        if action == "populate" and set(step) == {"action", "count"}:
            count = integer(step["count"], 1, limits["maxButtons"], f"Step {index} count")
        elif action == "clear" and set(step) == {"action"}:
            count = 0
        elif action == "resize" and set(step) == {"action", "width", "height"}:
            integer(step["width"], limits["minWidth"], limits["maxWidth"], f"Step {index} width")
            integer(step["height"], limits["minHeight"], limits["maxHeight"], f"Step {index} height")
        elif action == "scroll" and set(step) == {"action", "position"}:
            if step["position"] not in ("start", "end") or count == 0:
                raise ScenarioError(f"Step {index} scroll requires start/end and a nonempty panel.")
        else:
            raise ScenarioError(f"Step {index} has an unsupported action or unexpected fields.")
    return {"status": "Ready", "finalButtonCount": count, "stepCount": len(steps)}


def review(root: Path, path: Path, request_path: Path) -> dict:
    capabilities = json.loads((root / "scenario-capabilities.json").read_text(encoding="utf-8-sig"))
    request = json.loads(request_path.read_text(encoding="utf-8-sig"))
    data = path.read_bytes()
    plan = parse_plan(data)
    result = validate_plan(plan, capabilities, request["text"])
    if result["status"] == "Ready":
        result.update(name=plan["name"], request=plan["request"],
                      initialButtonCount=plan["initialButtonCount"], steps=plan["steps"],
                      iterations=10, mode="MeasurementOnly")
    return {**result, "plan": str(path.resolve()),
            "planSha256": hashlib.sha256(data).hexdigest()}


def stage_plan(root: Path, path: Path, expected_sha256: str | None = None) -> dict:
    capabilities = json.loads((root / "scenario-capabilities.json").read_text(encoding="utf-8-sig"))
    data = path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if expected_sha256 is not None and digest != expected_sha256.lower():
        raise ScenarioError("The scenario plan changed after review. Review it again before execution.")
    plan = parse_plan(data)
    result = validate_plan(plan, capabilities)
    if result["status"] != "Ready":
        raise ScenarioError(f"{result['status']}: {result['reason']}")
    frozen = root / "scenario-plan.json"
    with frozen.open("xb") as output:
        output.write(data)
    scenario_path = root / "scenario.json"
    scenario = json.loads(scenario_path.read_text(encoding="utf-8-sig"))
    scenario["name"] = plan["name"]
    scenario["expectedResults"] = {
        "controlCount": result["finalButtonCount"],
        "scenarioPlanSha256": digest,
        "scenarioStepCount": result["stepCount"],
    }
    scenario["markers"]["startPayload"] = [result["finalButtonCount"]]
    scenario["markers"]["stopPayload"] = [result["finalButtonCount"], True]
    scenario["sourceFiles"].append("scenario-plan.json")
    scenario["sourceSnapshots"].append("scenario-plan.json")
    scenario["launchArguments"] += ["--scenario", "{sourceRoot}\\scenario-plan.json"]
    scenario["allowZeroLayout"] = True
    scenario["maxLayoutPasses"] = None
    scenario["limitations"] = [
        "This is a user-selected action-plan measurement, not a performance acceptance or fix-validation gate.",
        "Steps run through app-owned hooks, not physical mouse/keyboard input; startup is outside the markers.",
        "Scenario duration is informational, not presented-frame latency or a statistical performance claim.",
    ]
    scenario_path.write_text(json.dumps(scenario, indent=2) + "\n", encoding="utf-8")
    return {**result, "plan": str(frozen.resolve()), "planSha256": digest,
            "mode": "MeasurementOnly"}


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate and freeze bounded ControlDemo action plans.")
    parser.add_argument("action", choices=["review", "stage"])
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--plan", type=Path)
    parser.add_argument("--request", type=Path)
    parser.add_argument("--expected-sha256")
    arguments = parser.parse_args()
    try:
        if arguments.plan is None:
            raise ScenarioError("An explicit --plan is required.")
        if arguments.action == "review":
            if arguments.request is None:
                raise ScenarioError("review requires --request with the original conversation request.")
            result = review(arguments.root.resolve(), arguments.plan, arguments.request)
        else:
            result = stage_plan(arguments.root.resolve(), arguments.plan, arguments.expected_sha256)
    except (ScenarioError, OSError, ValueError, KeyError, TypeError) as error:
        print(json.dumps({"status": "ScenarioFailure", "error": str(error)}), file=sys.stderr)
        return 2
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
