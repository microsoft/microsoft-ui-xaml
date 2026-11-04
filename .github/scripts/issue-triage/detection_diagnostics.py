"""Read-only detector diagnostics; never creates or authorizes a verdict."""

import argparse
import json
import math
import os
from pathlib import Path
import sys


def diagnose(root, limit, run_id, attempt):
    if not math.isfinite(limit) or limit <= 0:
        raise ValueError("Detector credit limit must be finite and positive")
    result = {
        "run_id": run_id, "run_attempt": attempt, "max_ai_credits": limit,
        "ai_credits": None,
        "result_file_present": (root / "threat-detection/detection_result.json").is_file(),
    }
    receipt = root / "threat-detection/execution.json"
    if not receipt.is_file():
        return {**result, "reason": "execution_not_recorded"}
    with receipt.open("rb") as stream:
        raw_identity = stream.read(4097)
    if len(raw_identity) > 4096:
        raise ValueError("Detector execution identity exceeds the diagnostic byte limit")
    identity = json.loads(raw_identity)
    if not isinstance(identity, dict) or (
        identity.get("component") != "detection"
        or identity.get("run_id") != run_id
        or identity.get("run_attempt") != attempt
    ):
        raise ValueError("Detector execution identity does not match this run/attempt")
    if identity.get("state") == "not_started":
        return {**result, "reason": "execution_not_started"}
    if identity.get("state") != "started":
        raise ValueError("Invalid detector execution state")
    candidates = [
        root / "sandbox/firewall/logs/api-proxy-logs/token-usage.jsonl",
        root / "sandbox/firewall-audit-logs/api-proxy-logs/token-usage.jsonl",
        root / "sandbox/firewall/audit/api-proxy-logs/token-usage.jsonl",
    ]
    usage = next((path for path in candidates if path.is_file()), None)
    if usage is None:
        return {**result, "reason": "usage_unavailable"}
    with usage.open("rb") as stream:
        raw = stream.read(2 * 1024 * 1024 + 1)
    if len(raw) > 2 * 1024 * 1024:
        raise ValueError("Detector usage exceeds the diagnostic byte limit")
    total = 0.0
    seen = set()
    cumulative = None
    for line in raw.splitlines():
        if not line.strip():
            continue
        row = json.loads(line)
        if not isinstance(row, dict):
            raise ValueError("Invalid detector usage record")
        request = row.get("request_id")
        charge = row.get("ai_credits_this_response")
        cumulative = row.get("ai_credits_total")
        if (
            not isinstance(request, str) or not request or request in seen
            or any(
                isinstance(value, bool) or not isinstance(value, (int, float))
                or not math.isfinite(value) or value < 0
                for value in (charge, cumulative)
            )
        ):
            raise ValueError("Invalid or duplicate detector credit record")
        seen.add(request)
        total += charge
        if not math.isclose(total, cumulative, rel_tol=0, abs_tol=(len(seen) + 1) * 0.0000005):
            raise ValueError("Detector response charges disagree with cumulative usage")
    if cumulative is None:
        return {**result, "reason": "usage_unavailable"}
    credits = round(cumulative, 6)
    reason = (
        "result_file_present" if result["result_file_present"]
        else "credit_budget_exhausted" if credits >= limit
        else "missing_result"
    )
    return {**result, "reason": reason, "ai_credits": credits}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("artifact_root", type=Path)
    parser.add_argument("max_ai_credits", type=float)
    args = parser.parse_args()
    try:
        result = diagnose(
            args.artifact_root, args.max_ai_credits,
            int(os.environ["GITHUB_RUN_ID"]), int(os.environ["GITHUB_RUN_ATTEMPT"]),
        )
    except (OSError, ValueError, KeyError) as error:
        print(f"::error::Detector diagnostics failed: {error}", file=sys.stderr)
        return 1
    print("TRIAGE_DETECTION_DIAGNOSTICS: " + json.dumps(result, sort_keys=True))
    if result["reason"] != "result_file_present":
        print(
            f"::warning::Detector diagnostics: {result['reason']}; "
            f"credits={result['ai_credits']}, limit={result['max_ai_credits']}. "
            "No security verdict is inferred; publication still requires detection_success."
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
