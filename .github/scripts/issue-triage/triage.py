"""Prepare bounded evidence and independently validate/publish agentic issue triage."""

from __future__ import annotations

import hashlib
import json
import math
import os
from pathlib import Path
import re
import sys
import time
import urllib.error
import urllib.parse
import urllib.request

from find_duplicates import (
    CONTROL_CHARS,
    FORM_HEADING,
    HTML_COMMENT,
    Issue,
    MAX_BODY,
    MAX_SUGGESTIONS,
    retrieve_candidates,
)

MARKER = "<!-- winui-ai-triage:canonical:v1 -->"
MAP_PATH = Path(__file__).with_name("area-team-map.json")
MAX_RESPONSE_BYTES = 8 * 1024 * 1024
MAX_COMMENTS = 1000
MAX_AUTHOR_COMMENTS = 10
MAX_COMMENT_BODY = 2000
CORE_TEAMS = {"team-Markup", "team-Reach", "team-Rendering"}


class TriageError(RuntimeError):
    pass


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        raise TriageError("GitHub redirected a request; refusing to change repository scope")


class GitHub:
    def __init__(self, repository: str, token: str):
        if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", repository) or not token:
            raise TriageError("GITHUB_REPOSITORY and GITHUB_TOKEN are required")
        self.repository = repository
        self.token = token
        self.last_search = 0.0
        self.opener = urllib.request.build_opener(NoRedirect)

    def request(self, method: str, path: str, payload=None):
        request = urllib.request.Request(
            "https://api.github.com/" + path,
            method=method,
            data=json.dumps(payload).encode("utf-8") if payload is not None else None,
            headers={
                "Accept": "application/vnd.github+json",
                "Authorization": "Bearer " + self.token,
                "Content-Type": "application/json",
                "User-Agent": "winui-issue-triage",
                "X-GitHub-Api-Version": "2022-11-28",
            },
        )
        for attempt in range(4):
            if path.startswith("search/"):
                time.sleep(max(0, 2.2 - (time.monotonic() - self.last_search)))
                self.last_search = time.monotonic()
            try:
                with self.opener.open(request, timeout=30) as response:
                    body = response.read(MAX_RESPONSE_BYTES + 1)
                if len(body) > MAX_RESPONSE_BYTES:
                    raise TriageError("GitHub response exceeded the response-size limit")
                return json.loads(body)
            except urllib.error.HTTPError as error:
                limited = error.code == 429 or (
                    error.code == 403
                    and (
                        error.headers.get("Retry-After")
                        or error.headers.get("x-ratelimit-remaining") == "0"
                    )
                )
                if method != "GET" or attempt == 3 or not (limited or error.code in (502, 503)):
                    raise TriageError(f"GitHub {method} failed (HTTP {error.code})") from error
                delay = retry_delay(error.headers, attempt)
                print(f"GitHub rate limit/transient error ({error.code}); retrying in {delay:.0f}s")
                time.sleep(delay)
            except (urllib.error.URLError, TimeoutError) as error:
                raise TriageError(f"GitHub {method} could not complete") from error
        raise TriageError("GitHub request exhausted its retry budget")

    def repo_path(self, suffix: str) -> str:
        return f"repos/{self.repository}/{suffix}"

    def get(self, suffix: str):
        return self.request("GET", self.repo_path(suffix))

    def write(self, method: str, suffix: str, payload: dict):
        return self.request(method, self.repo_path(suffix), payload)

    def pages(self, suffix: str, max_pages: int = 10) -> list[dict]:
        result = []
        for page in range(1, max_pages + 1):
            items = self.get(f"{suffix}?per_page=100&page={page}")
            if not isinstance(items, list):
                raise TriageError("GitHub returned an invalid collection")
            result.extend(items)
            if len(items) < 100:
                return result
        raise TriageError(f"Pagination limit reached for {suffix}; manual triage is required")

    def search(self, query: str) -> list[dict]:
        params = urllib.parse.urlencode(
            {"q": f'repo:{self.repository} is:issue -label:"WinUI OSS" {query}', "per_page": 20}
        )
        response = self.request("GET", "search/issues?" + params)
        if response.get("incomplete_results") or not isinstance(response.get("items"), list):
            raise TriageError("GitHub issue search was incomplete; refusing partial evidence")
        return response["items"]


def retry_delay(headers, attempt: int) -> float:
    for value, absolute in (
        (headers.get("Retry-After"), False),
        (
            headers.get("x-ratelimit-reset")
            if headers.get("x-ratelimit-remaining") == "0"
            else None,
            True,
        ),
    ):
        if value is not None:
            try:
                delay = float(value) - (time.time() if absolute else 0) + 1
                if math.isfinite(delay):
                    return min(max(delay, 1), 75)
            except ValueError:
                continue
    return min(8 * 2**attempt, 75)


def issue_number(event: dict, repository: str) -> int:
    if event.get("repository", {}).get("full_name") != repository:
        raise TriageError("Event repository does not match GITHUB_REPOSITORY")
    if "issue" in event:
        if "pull_request" in event["issue"]:
            raise TriageError("Pull requests are not issue-triage targets")
        number = event["issue"].get("number")
    else:
        value = event.get("inputs", {}).get("issue_number", "")
        if not isinstance(value, str) or not re.fullmatch(r"[1-9][0-9]{0,8}", value):
            raise TriageError("workflow_dispatch requires a positive integer issue_number")
        number = int(value)
    if type(number) is not int or not 0 < number < 1_000_000_000:
        raise TriageError("Invalid issue number")
    return number


def label_names(issue: dict) -> set[str]:
    return {label["name"] for label in issue.get("labels", [])}


def eligible(issue: dict) -> bool:
    labels = label_names(issue)
    return (
        "pull_request" not in issue
        and issue["state"] == "open"
        and not issue.get("locked")
        and issue["user"].get("type") != "Bot"
        and "WinUI OSS" not in labels
        and not (issue.get("title") or "").lower().startswith("[winui oss]")
        and bool(labels & {"bug", "feature proposal", "needs-triage"})
    )


def issue_kind(issue: dict) -> str:
    labels = label_names(issue)
    if "feature proposal" in labels:
        return "FEATURE"
    if "bug" in labels:
        return "BUG"
    headings = {match.lower() for match in FORM_HEADING.findall(issue.get("body") or "")}
    if {"describe the bug", "steps to reproduce the bug"} <= headings:
        return "BUG"
    if {"summary", "rationale"} <= headings:
        return "FEATURE"
    return "OTHER"


def snapshot(client: GitHub, number: int) -> dict:
    issue = client.get(f"issues/{number}")
    if issue.get("number") != number or "pull_request" in issue:
        raise TriageError("GitHub returned an unexpected issue")
    if issue.get("comments", 0) >= MAX_COMMENTS:
        raise TriageError("Comment limit reached; manual triage is required")
    comments = client.pages(f"issues/{number}/comments")
    author_comments = [
        {"id": comment["id"], "body": comment.get("body") or ""}
        for comment in comments
        if comment["user"]["id"] == issue["user"]["id"]
        and comment["user"].get("type") != "Bot"
    ]
    content = {
        "number": number,
        "title": issue.get("title") or "",
        "body": issue.get("body") or "",
        "labels": sorted(label_names(issue)),
        "author_comments": author_comments,
    }
    digest = hashlib.sha256(
        json.dumps(content, sort_keys=True, ensure_ascii=True).encode("utf-8")
    ).hexdigest()
    return {"issue": issue, "comments": comments, "content": content, "input_sha256": digest}


def evidence_text(value: str, limit: int) -> str:
    return CONTROL_CHARS.sub(" ", HTML_COMMENT.sub(" ", value))[:limit]


def prepare(client: GitHub, event: dict, mapping: dict) -> dict:
    number = issue_number(event, client.repository)
    current = snapshot(client, number)
    issue = current["issue"]
    if not eligible(issue):
        return {"should_process": False, "reason": "Issue is closed, locked, or outside intake scope"}
    if "comment" in event and event["comment"]["user"]["id"] != issue["user"]["id"]:
        return {"should_process": False, "reason": "Only author follow-ups trigger re-triage"}

    catalog = {label["name"]: label.get("description") or "" for label in client.pages("labels")}
    areas = {name: description for name, description in catalog.items() if name.startswith("area-")}
    if not areas:
        raise TriageError("No existing area labels were found")
    for area, team in mapping.items():
        if not isinstance(area, str) or not area.startswith("area-"):
            raise TriageError("Invalid area-to-team configuration")
        if team is not None and (
            not isinstance(team, str) or not team.startswith("team-") or team not in catalog
        ):
            raise TriageError(f"Ownership map references an invalid or missing team for {area}")

    content = current["content"]
    author_comments = content["author_comments"]
    selected = {comment["id"]: comment for comment in author_comments[:5] + author_comments[-5:]}
    kind = issue_kind(issue)

    def search(query):
        return [
            item for item in client.search(query)
            if issue_kind(item) in (kind, "OTHER") or kind == "OTHER"
        ]

    candidates = retrieve_candidates(Issue(number, content["title"], content["body"]), search)
    result = {
        "should_process": True,
        "repository": client.repository,
        "number": number,
        "input_sha256": current["input_sha256"],
        "issue_kind": kind,
        "title": evidence_text(content["title"], 256),
        "body": evidence_text(content["body"], MAX_BODY),
        "author_followups": [
            {"id": comment["id"], "body": evidence_text(comment["body"], MAX_COMMENT_BODY)}
            for comment in selected.values()
        ],
        "context_truncated": (
            len(content["body"]) > MAX_BODY
            or len(author_comments) > MAX_AUTHOR_COMMENTS
            or any(len(comment["body"]) > MAX_COMMENT_BODY for comment in selected.values())
        ),
        "existing_labels": content["labels"],
        "allowed_areas": areas,
        "candidates": candidates,
    }
    # Private to the deterministic publisher; never accepted from agent output.
    result["_snapshot"] = current
    result["_catalog"] = catalog
    return result


def required_string(item: dict, name: str, limit: int) -> str:
    value = item.get(name)
    if not isinstance(value, str) or not value.strip() or len(value) > limit:
        raise TriageError(f"Invalid or oversized {name}")
    return value.strip()


def validate(output: dict, evidence: dict) -> dict:
    items = output.get("items")
    if not isinstance(items, list) or len(items) != 1 or not isinstance(items[0], dict):
        raise TriageError("Expected exactly one structured triage output")
    item = items[0]
    expected = {
        "type", "input_sha256", "summary", "area", "area_confidence", "issue_kind",
        "reproduction", "missing_information", "duplicate_candidates_json",
    }
    if set(item) != expected or item["type"] != "publish_triage_summary":
        raise TriageError("Unexpected triage output schema or operation")
    if item["input_sha256"] != evidence["input_sha256"]:
        raise TriageError("Issue content, follow-ups, or labels changed; refusing stale output")
    result = {
        "summary": required_string(item, "summary", 700),
        "area": required_string(item, "area", 100),
        "area_confidence": item["area_confidence"],
        "issue_kind": item["issue_kind"],
        "reproduction": item["reproduction"],
        "missing_information": required_string(item, "missing_information", 700),
    }
    if result["issue_kind"] != evidence["issue_kind"]:
        raise TriageError("Issue kind does not match deterministic evidence")
    if result["area_confidence"] not in ("HIGH", "MEDIUM", "LOW", "NONE"):
        raise TriageError("Invalid area confidence")
    if result["area"] == "None":
        if result["area_confidence"] != "NONE":
            raise TriageError("An unknown area must have NONE confidence")
    elif result["area"] not in evidence["allowed_areas"] or result["area_confidence"] == "NONE":
        raise TriageError("Area is not an existing allowlisted label")
    if result["reproduction"] not in ("SUFFICIENT", "INSUFFICIENT", "NOT_APPLICABLE"):
        raise TriageError("Invalid reproduction classification")
    if (result["reproduction"] == "NOT_APPLICABLE") != (result["issue_kind"] != "BUG"):
        raise TriageError("Reproduction requests apply only to bug reports")
    if result["reproduction"] == "INSUFFICIENT" and result["missing_information"] == "None":
        raise TriageError("An insufficient reproduction requires an actionable request")
    raw_duplicates = required_string(item, "duplicate_candidates_json", 6000)
    duplicates = json.loads(raw_duplicates)
    if not isinstance(duplicates, list) or len(duplicates) > MAX_SUGGESTIONS:
        raise TriageError("Too many duplicate suggestions or invalid duplicate array")
    allowed = {candidate["number"] for candidate in evidence["candidates"]}
    seen = set()
    for duplicate in duplicates:
        if not isinstance(duplicate, dict) or set(duplicate) != {"number", "reason", "confidence"}:
            raise TriageError("Invalid duplicate schema")
        number = duplicate["number"]
        if type(number) is not int or number not in allowed or number in seen:
            raise TriageError("Duplicate is repeated or outside the retrieved candidate set")
        if duplicate["confidence"] != "HIGH":
            raise TriageError("Only HIGH-confidence duplicate suggestions may be published")
        required_string(duplicate, "reason", 400)
        seen.add(number)
    result["duplicates"] = duplicates
    return result


def planned_labels(result: dict, evidence: dict, mapping: dict) -> list[str]:
    existing = set(evidence["existing_labels"])
    areas = sorted(label for label in existing if label.startswith("area-"))
    teams = {label for label in existing if label.startswith("team-")}
    additions = set()
    if not areas and result["area_confidence"] == "HIGH":
        area = result["area"]
        mapped_team = mapping.get(area)
        compatible = not teams or not mapped_team or mapped_team in teams
        compatible |= teams == {"team-Core"} and mapped_team in CORE_TEAMS
        if compatible:
            additions.add(area)
            areas = [area]
    if len(areas) == 1 and not teams and mapping.get(areas[0]):
        additions.add(mapping[areas[0]])
    if (
        result["issue_kind"] == "BUG"
        and result["reproduction"] == "INSUFFICIENT"
        and not evidence["context_truncated"]
    ):
        additions.add("needs-repro")
    if not additions <= set(evidence["_catalog"]):
        raise TriageError("A proposed label no longer exists")
    return sorted(additions - existing)


def plain_text(value: str) -> str:
    value = CONTROL_CHARS.sub("", HTML_COMMENT.sub("", value))
    value = re.sub(r"https?://\S+", "[link omitted]", value)
    value = " ".join(value.split())
    value = value.replace("@", "@\u200b")
    value = re.sub(r"#(?=\d)", "#\u200b", value)
    value = value.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    return re.sub(r"([\\`*_\[\]|])", r"\\\1", value)


def render(result: dict, evidence: dict, mapping: dict) -> str:
    lines = [
        MARKER, "", "## Automated triage", "",
        "**Summary:** " + plain_text(result["summary"]), "",
    ]
    if result["area"] != "None":
        area = result["area"]
        lines.append(f"**Suggested area:** `{area}` ({result['area_confidence'].lower()} confidence).")
        if mapping.get(area):
            lines.append(f"**Mapped team:** `{mapping[area]}`.")
        else:
            lines.append("**Team:** Needs maintainer routing; no reviewed mapping is configured.")
    else:
        lines.append("**Routing:** Needs maintainer review; the area is unclear.")
    if result["missing_information"] != "None":
        lines.extend(["", "**Information needed:** " + plain_text(result["missing_information"])])
    if evidence["context_truncated"]:
        lines.extend(["", "_Long report or discussion: this analysis used bounded excerpts. "
                      "No reproduction label is applied from incomplete context._"])
    if result["duplicates"]:
        candidates = {item["number"]: item for item in evidence["candidates"]}
        lines.extend(["", "### Possible duplicates", ""])
        for duplicate in result["duplicates"]:
            candidate = candidates[duplicate["number"]]
            lines.append(
                f"- #{candidate['number']} ({candidate['state']}) - "
                + plain_text(duplicate["reason"]) + " **Confidence: high.**"
            )
    lines.extend([
        "",
        "_Automated suggestions, not a triage decision. Existing labels are preserved. "
        "A maintainer confirms ownership, reproduction, and duplicates; this workflow never closes issues._",
    ])
    return "\n".join(lines)


def canonical_comment(comments: list[dict]):
    matches = [
        comment for comment in comments
        if comment["user"].get("login") == "github-actions[bot]"
        and comment["user"].get("type") == "Bot"
        and (comment.get("body") or "").startswith(MARKER + "\n")
    ]
    if len(matches) > 1:
        raise TriageError("Multiple canonical bot comments found; manual cleanup is required")
    return matches[0] if matches else None


def ensure_current(client: GitHub, evidence: dict) -> dict | None:
    current = snapshot(client, evidence["number"])
    if not eligible(current["issue"]):
        print("Issue is no longer eligible; no further writes will be made")
        return None
    if current["input_sha256"] != evidence["input_sha256"]:
        raise TriageError("Issue changed immediately before publication; refusing stale output")
    return current


def publish(client: GitHub, result: dict, evidence: dict, mapping: dict, write: bool) -> dict:
    body = render(result, evidence, mapping)
    labels = planned_labels(result, evidence, mapping)
    plan = {"comment": body, "add_labels": labels, "published": False}
    summary_path = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary_path:
        with open(summary_path, "a", encoding="utf-8") as stream:
            stream.write(("## Publish plan\n\n" if write else "## Preview only - no issue writes\n\n"))
            stream.write(body + "\n\nLabels to add: " + ", ".join(labels) + "\n")
    if not write:
        print("Preview only: validated triage; no issue writes were made")
        return plan
    current = ensure_current(client, evidence)
    if current is None:
        return plan
    comment = canonical_comment(current["comments"])
    number = evidence["number"]
    if comment is None:
        client.write("POST", f"issues/{number}/comments", {"body": body})
    elif comment["body"] != body:
        client.write("PATCH", f"issues/comments/{comment['id']}", {"body": body})
    if labels and ensure_current(client, evidence) is not None:
        client.write("POST", f"issues/{number}/labels", {"labels": labels})
    plan["published"] = True
    return plan


def read_json(path: str, limit: int = MAX_RESPONSE_BYTES):
    with open(path, "rb") as stream:
        raw = stream.read(limit + 1)
    if len(raw) > limit:
        raise TriageError("JSON input exceeded its size limit")
    return json.loads(raw.decode("utf-8-sig"))


def main() -> int:
    try:
        if len(sys.argv) != 4 or sys.argv[1] not in ("prepare", "publish"):
            raise TriageError("Usage: triage.py prepare|publish EVENT_PATH CONTEXT_OR_AGENT_OUTPUT")
        mode, event_path, output_path = sys.argv[1:]
        client = GitHub(os.environ.get("GITHUB_REPOSITORY", ""), os.environ.get("GITHUB_TOKEN", ""))
        event = read_json(event_path)
        mapping = read_json(str(MAP_PATH))
        evidence = prepare(client, event, mapping)
        if mode == "prepare":
            public = {key: value for key, value in evidence.items() if not key.startswith("_")}
            destination = Path(output_path)
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_text(json.dumps(public, indent=2, ensure_ascii=True), encoding="utf-8")
            print(f"Prepared bounded evidence (should_process={evidence['should_process']})")
            return 0
        if not evidence["should_process"]:
            print(evidence["reason"] + "; no writes will be made")
            return 0
        result = validate(read_json(output_path, 32768), evidence)
        publish_value = os.environ.get("TRIAGE_PUBLISH", "false")
        if publish_value not in ("true", "false"):
            raise TriageError("TRIAGE_PUBLISH must be true or false")
        publish(client, result, evidence, mapping, write=publish_value == "true")
        return 0
    except (TriageError, KeyError, TypeError, ValueError, OSError) as error:
        print("::error::Issue triage failed: " + plain_text(str(error)), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
