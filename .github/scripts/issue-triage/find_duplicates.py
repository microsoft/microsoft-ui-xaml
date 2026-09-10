"""Deterministic duplicate-issue detection for the WinUI repository.

Reads the triggering issue from the workflow environment, retrieves a bounded set of
candidate issues through the GitHub search API, scores them with deterministic text
similarity, and emits a rendered Markdown comment when confident duplicates exist.

The script performs no writes. It fails closed: any retrieval or parsing error exits
non-zero so the publishing job is skipped.
"""

from __future__ import annotations

import difflib
import json
import os
import re
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from dataclasses import dataclass, field

API_ROOT = "https://api.github.com"

MAX_CANDIDATES = 30
MAX_SUGGESTIONS = 5
MAX_QUERIES = 6
DEFAULT_THRESHOLD = 0.62
HIGH_CONFIDENCE = 0.80
MEDIUM_CONFIDENCE = 0.70

# The search API allows 30 requests per minute. Pace requests to stay under that limit
# and retry with header-driven backoff when a secondary limit is still hit.
MIN_SEARCH_INTERVAL_SECONDS = 2.2
MAX_SEARCH_ATTEMPTS = 4
MAX_BACKOFF_SECONDS = 75

COMMENT_MARKER = "<!-- winui-duplicate-detection:v1 -->"

# Terms that carry no discriminating signal in this repository.
STOPWORDS = frozenset(
    """
    a about after all also an and any are as at be because been before being between both but by
    can cannot could did do does doing done down during each either else for from further get gets
    getting had has have having he her here hers him his how however if in into is it its itself
    just me more most my no nor not of off on once only or other our out over own same she should
    so some such than that the their them then there these they this those through to too under
    until up use used uses using very was way we were what when where which while who whom why will
    with without would you your
    bug issue issues problem problems repro reproduce reproduction steps expected actual behavior
    behaviour describe description context additional screenshot screenshots version versions
    windows winui xaml app apps application applications sample code project title important
    nuget package build sdk microsoft
    """.split()
)

# Placeholder text emitted by the issue forms when the author leaves a field untouched.
PLACEHOLDER_PATTERNS = (
    re.compile(r"^\s*_no response_\s*$", re.IGNORECASE | re.MULTILINE),
    re.compile(r"^\s*n/?a\s*$", re.IGNORECASE | re.MULTILINE),
)

HTML_COMMENT = re.compile(r"<!--.*?-->", re.DOTALL)
FENCED_CODE = re.compile(r"```.*?```", re.DOTALL)
MARKDOWN_IMAGE = re.compile(r"!\[[^\]]*\]\([^)]*\)")
MARKDOWN_LINK = re.compile(r"\[([^\]]*)\]\([^)]*\)")
BARE_URL = re.compile(r"https?://\S+")
CONTROL_CHARS = re.compile(r"[\x00-\x08\x0b\x0c\x0e-\x1f\x7f]")
FORM_HEADING = re.compile(r"^###\s+(.+?)\s*$", re.MULTILINE)
WORD = re.compile(r"[A-Za-z][A-Za-z0-9_]{2,}")

# Structural title prefixes such as "[WinUI OSS] Phase 4: ". Epic sub-tasks share these,
# which inflates title similarity between issues that are deliberately distinct work items.
TITLE_TAG = re.compile(r"^\s*(?:\[[^\]]{1,40}\]\s*)+")
TITLE_STEP = re.compile(r"^\s*[A-Za-z]+\s+\d+\s*:\s*")

# Control, API, and exception names such as NavigationView or COMException. The pattern
# requires an internal capital followed by a lowercase letter, so plain words such as
# "Windows" are not treated as identifiers while acronym prefixes such as "COM" are kept.
IDENTIFIER = re.compile(r"\b[A-Z][a-zA-Z0-9]*[A-Z][a-z][a-zA-Z0-9]*\b")
ERROR_CODE = re.compile(r"\b0x[0-9A-Fa-f]{4,8}\b")

# Authors routinely write single-word control names in backticks, for example `Expander`.
CODE_SPAN = re.compile(r"`([A-Za-z][A-Za-z0-9_.]{2,})`")

# Body sections that describe the defect. Version and metadata fields are excluded so
# that two unrelated reports on the same Windows build do not score as similar.
SIGNAL_SECTIONS = frozenset(
    {
        "describe the bug",
        "steps to reproduce the bug",
        "actual behavior",
        "expected behavior",
        "summary",
        "rationale",
        "scope",
    }
)

NOISE_SECTIONS = frozenset(
    {
        "nuget package version",
        "windows version",
        "screenshots",
        "additional context",
        "why is this important?",
        "important notes",
        "open questions",
    }
)


class RetrievalError(RuntimeError):
    """Raised when GitHub data cannot be retrieved. Causes a fail-closed exit."""


@dataclass(frozen=True)
class Issue:
    number: int
    title: str
    body: str
    state: str = "open"
    url: str = ""


@dataclass
class Match:
    issue: Issue
    score: float
    shared_identifiers: tuple = field(default_factory=tuple)

    @property
    def confidence(self) -> str:
        if self.score >= HIGH_CONFIDENCE:
            return "High"
        if self.score >= MEDIUM_CONFIDENCE:
            return "Medium"
        return "Low"


def normalize_text(text: str) -> str:
    """Strip markup, links, and placeholder content from untrusted issue text."""
    if not text:
        return ""
    text = CONTROL_CHARS.sub(" ", text)
    text = HTML_COMMENT.sub(" ", text)
    text = FENCED_CODE.sub(" ", text)
    text = MARKDOWN_IMAGE.sub(" ", text)
    text = MARKDOWN_LINK.sub(r"\1", text)
    text = BARE_URL.sub(" ", text)
    for pattern in PLACEHOLDER_PATTERNS:
        text = pattern.sub(" ", text)
    return text


def signal_body(body: str) -> str:
    """Return only the issue-form sections that describe the defect.

    Bodies that do not use the issue forms are returned unchanged so that manually
    written reports still participate in retrieval.
    """
    normalized = normalize_text(body)
    headings = list(FORM_HEADING.finditer(normalized))
    if not headings:
        return normalized

    kept = []
    for index, heading in enumerate(headings):
        name = heading.group(1).strip().lower()
        end = headings[index + 1].start() if index + 1 < len(headings) else len(normalized)
        section = normalized[heading.end() : end]
        if name in NOISE_SECTIONS or name not in SIGNAL_SECTIONS:
            continue
        kept.append(section)
    return "\n".join(kept) if kept else normalized


def tokenize(text: str) -> set:
    """Split text into lowercase content words, splitting CamelCase identifiers apart."""
    tokens = set()
    for raw in WORD.findall(normalize_text(text)):
        lowered = raw.lower()
        # Skip sub-tokens of stopwords so "WinUI" does not contribute "win".
        if lowered in STOPWORDS:
            continue
        tokens.add(lowered)
        for part in re.findall(r"[A-Z][a-z0-9]+|[a-z0-9]+", raw):
            part = part.lower()
            if len(part) > 2 and part not in STOPWORDS:
                tokens.add(part)
    return tokens


def extract_identifiers(text: str) -> set:
    """Return technical identifiers: control/API names and hexadecimal error codes.

    Code spans are read from the raw text because normalization removes the backticks.
    """
    raw = CONTROL_CHARS.sub(" ", text or "")
    cleaned = normalize_text(text)
    found = {match.group(0) for match in IDENTIFIER.finditer(cleaned)}
    found |= {match.group(0).lower() for match in ERROR_CODE.finditer(cleaned)}
    found |= {match.group(1) for match in CODE_SPAN.finditer(raw)}
    return {item for item in found if item.lower() not in STOPWORDS}


def _jaccard(left: set, right: set) -> float:
    if not left or not right:
        return 0.0
    return len(left & right) / len(left | right)


def strip_title_prefix(title: str) -> str:
    """Remove leading bracketed tags and numbered step prefixes from a title.

    Tracking epics generate sub-tasks such as "[WinUI OSS] Phase 4: Update metadata
    factory layer". The shared prefix is boilerplate, not evidence of duplication, so it
    is removed before titles are compared. If stripping would leave nothing meaningful,
    the original title is kept.
    """
    stripped = TITLE_TAG.sub("", title or "")
    stripped = TITLE_STEP.sub("", stripped)
    stripped = stripped.strip()
    return stripped if len(stripped) >= 10 else (title or "").strip()


def similarity(source: Issue, candidate: Issue):
    """Score two issues in [0, 1] and return the identifiers they share.

    Title agreement dominates because WinUI reports repeat the same control name and
    symptom. Body agreement and shared technical identifiers refine the ranking.
    """
    source_title = strip_title_prefix(normalize_text(source.title))
    candidate_title = strip_title_prefix(normalize_text(candidate.title))

    title_overlap = _jaccard(tokenize(source_title), tokenize(candidate_title))
    title_ratio = difflib.SequenceMatcher(
        None, source_title.lower().strip(), candidate_title.lower().strip()
    ).ratio()
    title_score = max(title_overlap, title_ratio * 0.95)

    source_body = signal_body(source.body)
    candidate_body = signal_body(candidate.body)
    body_score = _jaccard(tokenize(source_body), tokenize(candidate_body))

    source_ids = extract_identifiers(source.title + "\n" + source_body)
    candidate_ids = extract_identifiers(candidate.title + "\n" + candidate_body)
    shared = tuple(sorted(source_ids & candidate_ids))
    identifier_score = _jaccard(source_ids, candidate_ids)

    score = 0.60 * title_score + 0.25 * body_score + 0.15 * identifier_score
    return round(min(score, 1.0), 4), shared


def select_duplicates(
    source: Issue,
    candidates: list,
    threshold: float = DEFAULT_THRESHOLD,
    limit: int = MAX_SUGGESTIONS,
) -> list:
    """Rank candidates and keep those at or above the threshold."""
    matches = []
    seen = {source.number}
    for candidate in candidates:
        if candidate.number in seen:
            continue
        seen.add(candidate.number)
        score, shared = similarity(source, candidate)
        if score >= threshold:
            matches.append(Match(issue=candidate, score=score, shared_identifiers=shared))
    matches.sort(key=lambda match: (-match.score, match.issue.number))
    return matches[:limit]


def escape_markdown(text: str) -> str:
    """Neutralize mentions and Markdown control characters in republished text."""
    text = CONTROL_CHARS.sub("", text or "")
    text = re.sub(r"[@#]([A-Za-z0-9_\-])", lambda m: m.group(0)[0] + "\u200b" + m.group(1), text)
    text = re.sub(r"([\\`*_\[\]<>|])", r"\\\1", text)
    return " ".join(text.split())[:200]


def render_comment(matches: list) -> str:
    """Render the canonical duplicate-detection comment."""
    lines = [
        COMMENT_MARKER,
        "",
        "### Possible duplicates",
        "",
        "This issue looks similar to the reports below. Please check whether one of them "
        "already covers your problem, and add any new details there if it does.",
        "",
    ]
    for match in matches:
        state = "" if match.issue.state == "open" else " _(" + match.issue.state + ")_"
        lines.append(
            "* #{number} - {title}{state} - confidence: **{confidence}**".format(
                number=match.issue.number,
                title=escape_markdown(match.issue.title),
                state=state,
                confidence=match.confidence,
            )
        )
        if match.shared_identifiers:
            shared = ", ".join(
                "`" + escape_markdown(name) + "`" for name in match.shared_identifiers[:5]
            )
            lines.append("  * Shared references: " + shared)
    lines += [
        "",
        "_This is an automated suggestion based on text similarity. A maintainer decides "
        "whether these issues are duplicates; nothing is closed automatically._",
    ]
    return "\n".join(lines)


def _request(url: str, token: str) -> dict:
    request = urllib.request.Request(
        url,
        headers={
            "Accept": "application/vnd.github+json",
            "Authorization": "Bearer " + token,
            "User-Agent": "winui-duplicate-detection",
            "X-GitHub-Api-Version": "2022-11-28",
        },
    )
    with urllib.request.urlopen(request, timeout=30) as response:
        return json.loads(response.read().decode("utf-8"))


def _retry_delay(error: urllib.error.HTTPError, attempt: int) -> float:
    """Derive a wait time from GitHub rate-limit headers, falling back to backoff.

    Secondary rate limits send `Retry-After`; primary limits send `x-ratelimit-reset`.
    """
    headers = getattr(error, "headers", None)
    retry_after = headers.get("Retry-After") if headers else None
    if retry_after:
        try:
            return min(float(retry_after) + 1, MAX_BACKOFF_SECONDS)
        except ValueError:
            pass

    reset = headers.get("x-ratelimit-reset") if headers else None
    remaining = headers.get("x-ratelimit-remaining") if headers else None
    if reset and remaining == "0":
        try:
            return min(max(float(reset) - time.time() + 1, 1.0), MAX_BACKOFF_SECONDS)
        except ValueError:
            pass

    return min(8.0 * (2**attempt), MAX_BACKOFF_SECONDS)


_last_search_at = 0.0


def _throttle() -> None:
    global _last_search_at
    elapsed = time.time() - _last_search_at
    if _last_search_at and elapsed < MIN_SEARCH_INTERVAL_SECONDS:
        time.sleep(MIN_SEARCH_INTERVAL_SECONDS - elapsed)
    _last_search_at = time.time()


def search_issues(repo: str, query: str, token: str, per_page: int = 20) -> list:
    params = urllib.parse.urlencode(
        {"q": "repo:" + repo + " is:issue " + query, "per_page": per_page, "sort": "updated"}
    )
    payload = None
    for attempt in range(MAX_SEARCH_ATTEMPTS):
        _throttle()
        try:
            payload = _request(API_ROOT + "/search/issues?" + params, token)
            break
        except urllib.error.HTTPError as error:
            if error.code in (403, 429) and attempt < MAX_SEARCH_ATTEMPTS - 1:
                delay = _retry_delay(error, attempt)
                print("Rate limited (%s). Waiting %.1fs before retry." % (error.code, delay))
                time.sleep(delay)
                continue
            raise RetrievalError("search failed (%s) for query: %s" % (error.code, query))
        except urllib.error.URLError:
            raise RetrievalError("search failed for query: " + query)
    if payload is None:
        raise RetrievalError("search exhausted retries for query: " + query)

    results = []
    for item in payload.get("items", []):
        if "pull_request" in item:
            continue
        results.append(
            Issue(
                number=int(item["number"]),
                title=item.get("title") or "",
                body=item.get("body") or "",
                state=item.get("state") or "open",
                url=item.get("html_url") or "",
            )
        )
    return results


def build_queries(source: Issue) -> list:
    """Build a small, deterministic set of retrieval queries.

    GitHub issue search treats space-separated terms as AND, so several narrow queries
    are issued instead of one broad query in order to favor recall.
    """
    title_tokens = sorted(tokenize(source.title), key=lambda token: (-len(token), token))
    identifiers = sorted(extract_identifiers(source.title + "\n" + signal_body(source.body)))

    queries = []
    quoted_title = normalize_text(source.title).strip().replace('"', "")
    if len(quoted_title) >= 15:
        queries.append('in:title "' + quoted_title[:180] + '"')
    if len(title_tokens) >= 2:
        queries.append("in:title " + " ".join(title_tokens[:3]))
    for token in title_tokens[:4]:
        queries.append("in:title " + token)
    for identifier in identifiers[:3]:
        queries.append('"' + identifier + '"')

    deduped = []
    for query in queries:
        if query not in deduped:
            deduped.append(query)
    return deduped[:MAX_QUERIES]


def retrieve_candidates(repo: str, source: Issue, token: str) -> list:
    candidates = {}
    for query in build_queries(source):
        for issue in search_issues(repo, query, token):
            if issue.number == source.number:
                continue
            candidates.setdefault(issue.number, issue)
        if len(candidates) >= MAX_CANDIDATES:
            break
    ordered = sorted(candidates.values(), key=lambda issue: -issue.number)
    return ordered[:MAX_CANDIDATES]


def write_output(found: bool, comment: str, matches: list) -> None:
    output_path = os.environ.get("GITHUB_OUTPUT")
    top = matches[0] if matches else None
    payload = {
        "found": "true" if found else "false",
        "count": str(len(matches)),
        "numbers": json.dumps([match.issue.number for match in matches]),
        # The strongest match drives the optional duplicate suggestion chip.
        "top_number": str(top.issue.number) if top else "",
        "top_confidence": top.confidence.upper() if top else "",
        "top_reason": (
            "Deterministic text similarity scored this report %.2f against #%d."
            % (top.score, top.issue.number)
            if top
            else ""
        ),
    }
    if not output_path:
        print(json.dumps(payload, indent=2))
        print(comment)
        return
    with open(output_path, "a", encoding="utf-8") as handle:
        for key, value in payload.items():
            handle.write(key + "=" + value + "\n")
        handle.write("comment<<WINUI_EOF\n" + comment + "\nWINUI_EOF\n")


def main() -> int:
    token = os.environ.get("GITHUB_TOKEN", "")
    repo = os.environ.get("GITHUB_REPOSITORY", "")
    if not token or not repo:
        print("::error::GITHUB_TOKEN and GITHUB_REPOSITORY are required", file=sys.stderr)
        return 1

    try:
        number = int(os.environ["ISSUE_NUMBER"])
    except (KeyError, ValueError):
        print("::error::ISSUE_NUMBER is missing or not an integer", file=sys.stderr)
        return 1

    source = Issue(
        number=number,
        title=os.environ.get("ISSUE_TITLE", ""),
        body=os.environ.get("ISSUE_BODY", ""),
    )
    threshold = float(os.environ.get("SIMILARITY_THRESHOLD", DEFAULT_THRESHOLD))

    try:
        candidates = retrieve_candidates(repo, source, token)
    except RetrievalError as error:
        print("::error::" + str(error), file=sys.stderr)
        return 1

    matches = select_duplicates(source, candidates, threshold=threshold)
    print("Retrieved %d candidates, kept %d above %s." % (len(candidates), len(matches), threshold))
    for match in matches:
        print("  #%d score=%s confidence=%s" % (match.issue.number, match.score, match.confidence))

    write_output(bool(matches), render_comment(matches) if matches else "", matches)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
