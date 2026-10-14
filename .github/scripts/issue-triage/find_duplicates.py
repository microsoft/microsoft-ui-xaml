"""Bounded retrieval and ranking, not a duplicate verdict."""

from __future__ import annotations

import difflib
import re
from dataclasses import dataclass
from typing import Callable

MAX_CANDIDATES = 30
MAX_QUERIES = 6
MAX_SUGGESTIONS = 5
MAX_BODY = 16000

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

HTML_COMMENT = re.compile(r"<!--.*?(?:-->|$)", re.DOTALL)
FENCED_CODE = re.compile(r"```.*?```", re.DOTALL)
CONTROL_CHARS = re.compile(r"[\x00-\x08\x0b\x0c\x0e-\x1f\x7f\u202a-\u202e\u2066-\u2069]")
FORM_HEADING = re.compile(r"^###\s+(.+?)\s*$", re.MULTILINE)
WORD = re.compile(r"[A-Za-z][A-Za-z0-9_]{2,}")
IDENTIFIER = re.compile(r"\b[A-Z][a-zA-Z0-9]*[A-Z][a-z][a-zA-Z0-9]*\b")
ERROR_CODE = re.compile(r"\b0x[0-9A-Fa-f]{4,8}\b")
CODE_SPAN = re.compile(r"`([A-Za-z][A-Za-z0-9_.]{2,})`")
TITLE_TAG = re.compile(r"^\s*(?:\[[^\]]{1,40}\]\s*)+")
TITLE_STEP = re.compile(r"^\s*[A-Za-z]+\s+\d+\s*:\s*")
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


@dataclass(frozen=True)
class Issue:
    number: int
    title: str
    body: str
    state: str = "open"


def normalize_text(text: str) -> str:
    text = CONTROL_CHARS.sub(" ", text or "")
    text = HTML_COMMENT.sub(" ", text)
    text = FENCED_CODE.sub(" ", text)
    text = re.sub(r"!\[[^\]]*\]\([^)]*\)", " ", text)
    text = re.sub(r"\[([^\]]*)\]\([^)]*\)", r"\1", text)
    text = re.sub(r"https?://\S+", " ", text)
    return re.sub(r"^\s*(?:_No response_|n/?a)\s*$", " ", text, flags=re.I | re.M)


def signal_body(body: str) -> str:
    normalized = normalize_text(body[:MAX_BODY])
    headings = list(FORM_HEADING.finditer(normalized))
    if not headings:
        return normalized
    kept = []
    for index, heading in enumerate(headings):
        end = headings[index + 1].start() if index + 1 < len(headings) else len(normalized)
        if heading.group(1).strip().lower() in SIGNAL_SECTIONS:
            kept.append(normalized[heading.end() : end])
    # A metadata-only form is not evidence of the same defect.
    return "\n".join(kept)


def version_fields(body: str) -> dict[str, str]:
    headings = list(FORM_HEADING.finditer(body))
    fields = {}
    for index, heading in enumerate(headings):
        name = heading.group(1).strip().lower()
        if name not in ("nuget package version", "windows version"):
            continue
        end = headings[index + 1].start() if index + 1 < len(headings) else len(body)
        fields[name] = normalize_text(body[heading.end() : end]).strip()[:500]
    return fields


def tokenize(text: str) -> set[str]:
    tokens = set()
    for raw in WORD.findall(normalize_text(text)):
        if raw.lower() in STOPWORDS:
            continue
        tokens.add(raw.lower())
        for part in re.findall(r"[A-Z][a-z0-9]+|[a-z0-9]+", raw):
            if len(part) > 2 and part.lower() not in STOPWORDS:
                tokens.add(part.lower())
    return tokens


def extract_identifiers(text: str) -> set[str]:
    cleaned = normalize_text(text)
    found = set(IDENTIFIER.findall(cleaned))
    found.update(code.lower() for code in ERROR_CODE.findall(cleaned))
    found.update(CODE_SPAN.findall(HTML_COMMENT.sub(" ", text or "")))
    return {item for item in found if item.lower() not in STOPWORDS}


def strip_title_prefix(title: str) -> str:
    stripped = TITLE_STEP.sub("", TITLE_TAG.sub("", title or "")).strip()
    return stripped if len(stripped) >= 10 else (title or "").strip()


def _jaccard(left: set[str], right: set[str]) -> float:
    return len(left & right) / len(left | right) if left and right else 0.0


def similarity(source: Issue, candidate: Issue) -> tuple[float, list[str]]:
    source_title = strip_title_prefix(normalize_text(source.title[:256]))
    candidate_title = strip_title_prefix(normalize_text(candidate.title[:256]))
    overlap = _jaccard(tokenize(source_title), tokenize(candidate_title))
    ratio = difflib.SequenceMatcher(None, source_title.lower(), candidate_title.lower()).ratio()
    source_body, candidate_body = signal_body(source.body), signal_body(candidate.body)
    source_ids = extract_identifiers(source_title + "\n" + source_body)
    candidate_ids = extract_identifiers(candidate_title + "\n" + candidate_body)
    score = (
        0.60 * max(overlap, ratio * 0.95)
        + 0.25 * _jaccard(tokenize(source_body), tokenize(candidate_body))
        + 0.15 * _jaccard(source_ids, candidate_ids)
    )
    return round(min(score, 1.0), 4), sorted(source_ids & candidate_ids)


def build_queries(source: Issue) -> list[str]:
    title = strip_title_prefix(source.title[:256])
    tokens = sorted(tokenize(title), key=lambda token: (-len(token), token))
    identifiers = sorted(extract_identifiers(title + "\n" + signal_body(source.body)))
    queries = []
    # Never interpolate raw issue text as GitHub search syntax.
    phrase = " ".join(re.findall(r"[A-Za-z0-9_]+", title))[:180]
    if len(phrase) >= 15:
        queries.append(f'in:title "{phrase}"')
    if len(tokens) >= 2:
        queries.append("in:title " + " ".join(tokens[:3]))
    queries.extend(f'"{identifier}"' for identifier in identifiers[:2])
    queries.extend("in:title " + token for token in tokens[:4])
    return list(dict.fromkeys(queries))[:MAX_QUERIES]


def retrieve_candidates(source: Issue, search: Callable[[str], list[dict]]) -> list[dict]:
    candidates = {}
    for query in build_queries(source):
        for item in search(query):
            number = item["number"]
            labels = {label["name"] for label in item.get("labels", [])}
            # Repository issue numbers define the canonical ordering. Transferred
            # issues retain creation timestamps that can disagree with that order.
            if (
                "pull_request" in item
                or type(number) is not int
                or not 0 < number < source.number
                or "WinUI OSS" in labels
                or (item.get("title") or "").lower().startswith("[winui oss]")
            ):
                continue
            candidate = Issue(
                number,
                (item.get("title") or "")[:256],
                (item.get("body") or "")[:MAX_BODY],
                item["state"],
            )
            score, identifiers = similarity(source, candidate)
            candidates[number] = {
                "number": number,
                "title": candidate.title,
                "body": signal_body(candidate.body)[:1600],
                "state": candidate.state,
                "labels": sorted(labels),
                "versions": version_fields(item.get("body") or ""),
                "retrieval_score": score,
                "shared_identifiers": identifiers[:10],
            }
    return sorted(
        candidates.values(), key=lambda item: (-item["retrieval_score"], item["number"])
    )[:MAX_CANDIDATES]
