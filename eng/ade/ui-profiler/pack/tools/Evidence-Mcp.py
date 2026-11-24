# Query the team's hash-verified derived reports without extending or replacing WinUIProfiler MCP.
import argparse
import hashlib
import json
import math
from pathlib import Path
from typing import Any, Literal
from typing_extensions import NotRequired, TypedDict


class EvidenceQueryResult(TypedDict):
    schema: Literal["ui-profiler/evidence-query/v1"]
    view: Literal["capabilities", "metrics", "details"]
    provenance: dict[str, Any]
    interpretation: str
    capabilities: NotRequired[dict[str, Any] | None]
    scopes: NotRequired[dict[str, str] | None]
    detailSections: NotRequired[dict[str, Any] | None]
    capability: NotRequired[dict[str, Any] | None]
    section: NotRequired[str | None]
    total: NotRequired[int | None]
    offset: NotRequired[int | None]
    returned: NotRequired[int | None]
    rows: NotRequired[list[dict[str, Any]] | None]
    nextOffset: NotRequired[int | None]


MAX_REPORT_BYTES = 128 * 1024 * 1024
MAX_REPLY_BYTES = 64 * 1024


def parse_json(raw):
    def reject_constant(value):
        raise ValueError(f"Non-finite JSON value: {value}")

    def finite_float(value):
        number = float(value)
        if not math.isfinite(number):
            raise ValueError(f"Non-finite JSON number: {value}")
        return number

    return json.loads(raw, parse_constant=reject_constant, parse_float=finite_float)


def load_json(path):
    return parse_json(Path(path).read_bytes())


def digest(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def verify_manifest(path, active=None, depth=0):
    path = Path(path).resolve(strict=True)
    active = set() if active is None else active
    if path in active or depth > 32:
        raise ValueError("Cyclic or excessively deep evidence manifest")
    if path.stat().st_size > 4 * 1024 * 1024:
        raise ValueError("Evidence manifest is too large")
    active.add(path)
    try:
        manifest = load_json(path)
        if manifest.get("schema") != "ui-profiler/evidence-manifest/v1":
            raise ValueError("Unsupported evidence manifest")
        if not all(isinstance(manifest.get(key), list) for key in ("inputs", "outputs")):
            raise ValueError("A manifest requires input and output references")
        references = manifest["inputs"] + manifest["outputs"]
        if len(references) > 20000:
            raise ValueError("Too many evidence references")
        for item in references:
            target = Path(item["path"])
            if not target.is_absolute():
                raise ValueError("Evidence references must be absolute")
            target = target.resolve(strict=True)
            if "bytes" in item and item["bytes"] != target.stat().st_size:
                raise ValueError(f"Evidence byte count changed: {target}")
            if digest(target) != item["sha256"]:
                raise ValueError(f"Evidence changed: {target}")
        for item in manifest["inputs"]:
            if item.get("origin") == "evidence-manifest":
                verify_manifest(item["path"], active, depth + 1)
        return manifest
    finally:
        active.remove(path)


def read_report(analysis_path):
    path = Path(analysis_path).resolve(strict=True)
    if path.stat().st_size > MAX_REPORT_BYTES:
        raise ValueError("Derived report exceeds the 128 MiB reader limit")
    manifest_path = path.with_name("manifest.json")
    manifest = verify_manifest(manifest_path)
    output = [item for item in manifest["outputs"] if Path(item["path"]).resolve() == path]
    if len(output) != 1:
        raise ValueError("The report is not a uniquely bound manifest output")
    raw = path.read_bytes()
    if hashlib.sha256(raw).hexdigest() != output[0]["sha256"]:
        raise ValueError("The report changed while reading")
    report = parse_json(raw)
    if report.get("schema") != "ui-profiler/analysis/v1":
        raise ValueError("Unsupported derived analysis schema")
    for key in ("capabilities", "scopeAvailability", "details"):
        if not isinstance(report.get(key), dict):
            raise ValueError(f"Missing report object: {key}")
    if not isinstance(report.get("metrics"), list):
        raise ValueError("Missing report metrics")
    for key in ("measurement", "database", "actions"):
        reference = report[key]
        if not any(
            Path(item["path"]).resolve() == Path(reference["path"]).resolve()
            and item["sha256"] == reference["sha256"] for item in manifest["inputs"]
        ):
            raise ValueError(f"Report {key} is not bound to a manifest input")
    return report, {
        "kind": "team-derived-report", "analysis": output[0],
        "manifest": {"path": str(manifest_path), "sha256": digest(manifest_path)},
        "measurement": report["measurement"], "database": report["database"],
        "verification": "Input/output hashes matched at query time; integrity checking, not a signature.",
        "queries": [], "computed": "Reads the team's deterministic exported analysis; does not execute profiler SQL.",
    }


def pointer_token(token):
    return str(token).replace("~", "~0").replace("/", "~1")


def shape(value, pointer):
    if isinstance(value, (list, dict)):
        return {"pointer": pointer, "kind": "array" if isinstance(value, list) else "object", "count": len(value)}
    if isinstance(value, str) and len(value) > 2048:
        return {"pointer": pointer, "kind": "string", "characters": len(value), "omitted": True}
    if type(value) is int and abs(value) > 2**53 - 1:
        return {"pointer": pointer, "kind": "integer", "value": value, "exactDecimal": str(value)}
    return value


def query(analysis_path, view="capabilities", family="", scope="", metric="", section="", offset=0, limit=50):
    if type(offset) is not int or offset < 0 or type(limit) is not int or not 1 <= limit <= 200:
        raise ValueError("Offset must be nonnegative and limit must be 1..200")
    if view not in {"capabilities", "metrics", "details"}:
        raise ValueError("View must be capabilities, metrics or details")
    report, provenance = read_report(analysis_path)
    result = {
        "schema": "ui-profiler/evidence-query/v1", "view": view, "provenance": provenance,
        "interpretation": report["interpretation"],
    }
    if view == "capabilities":
        if family and family not in report["capabilities"]:
            raise ValueError("An exact reported family is required")
        scopes = [
            (key, value) for key, value in report["scopeAvailability"].items()
            if (not family or key.startswith(family + "/")) and (not scope or scope in key)
        ]
        result.update(
            capabilities={family: report["capabilities"][family]} if family else report["capabilities"],
            detailSections={key: shape(value, "/" + pointer_token(key)) for key, value in report["details"].items()},
            total=len(scopes), offset=offset,
        )
        selected = {}
        for key, value in scopes[offset:offset + limit]:
            candidate = {**result, "scopes": {**selected, key: value}, "returned": len(selected) + 1,
                         "nextOffset": offset + len(selected) + 1}
            if len(json.dumps(candidate, allow_nan=False).encode("utf-8")) > MAX_REPLY_BYTES:
                break
            selected[key] = value
        result.update(scopes=selected, returned=len(selected),
                      nextOffset=offset + len(selected) if offset + len(selected) < len(scopes) else None)
        if (not selected and offset < len(scopes)) or len(
            json.dumps(result, allow_nan=False).encode("utf-8")
        ) > MAX_REPLY_BYTES:
            raise ValueError("Capabilities exceed the reply limit; request an exact family or narrower scope")
        return result
    if view == "metrics":
        if not family or family not in report["capabilities"]:
            raise ValueError("An exact reported family is required for metrics")
        rows = [
            {**row, **({"valueExact": str(row["value"])} if type(row["value"]) is int
                      and abs(row["value"]) > 2**53 - 1 else {})}
            for row in report["metrics"] if row["family"] == family
            and (not scope or row["scope"] == scope)
            and (not metric or metric.casefold() in row["metric"].casefold())
        ]
        result["capability"] = report["capabilities"][family]
    else:
        if not section.startswith("/") or len(section) > 1024:
            raise ValueError("Details require a JSON pointer relative to details, e.g. /nativeSeries")
        node = report["details"]
        for token in section[1:].split("/"):
            token = token.replace("~1", "/").replace("~0", "~")
            if isinstance(node, list):
                if not token.isdecimal():
                    raise ValueError("Array pointers require a nonnegative index")
                node = node[int(token)]
            elif isinstance(node, dict):
                node = node[token]
            else:
                raise ValueError("Cannot traverse a scalar detail")
        if isinstance(node, dict):
            rows = [
                {"key": key, "value": shape(value, section + "/" + pointer_token(key))}
                for key, value in node.items()
            ]
        elif isinstance(node, list):
            rows = [
                {"index": index, "value": {
                    key: shape(value, f"{section}/{index}/{pointer_token(key)}") for key, value in row.items()
                } if isinstance(row, dict) else shape(row, f"{section}/{index}")}
                for index, row in enumerate(node)
            ]
        else:
            rows = [{"value": shape(node, section)}]
        result["section"] = section
    selected = []
    for row in rows[offset:offset + limit]:
        if len(json.dumps({**result, "rows": [*selected, row]}, allow_nan=False).encode("utf-8")) > MAX_REPLY_BYTES:
            break
        selected.append(row)
    if not selected and offset < len(rows):
        raise ValueError("One row exceeds the reply limit; use a more specific detail pointer")
    result.update(
        total=len(rows), offset=offset, returned=len(selected), rows=selected,
        nextOffset=offset + len(selected) if offset + len(selected) < len(rows) else None,
    )
    if len(json.dumps(result, allow_nan=False).encode("utf-8")) > MAX_REPLY_BYTES:
        raise ValueError("Page metadata exceeds the reply limit; request a smaller limit")
    return result


def create_server(analysis_path):
    from mcp.server.fastmcp import FastMCP
    from mcp.types import ToolAnnotations

    server = FastMCP("ui-profiler-evidence")

    @server.tool(annotations=ToolAnnotations(readOnlyHint=True, destructiveHint=False, openWorldHint=False))
    def evidence(
        view: str = "capabilities", family: str = "", scope: str = "", metric: str = "",
        section: str = "", offset: int = 0, limit: int = 50,
    ) -> EvidenceQueryResult:
        """Query only this run's hash-verified derived evidence; original WinUIProfiler MCP is separate.

        capabilities: availability/detail pointers for every family, paged scopes; optional family/scope filter.
        metrics: exact family, optional scope/metric substring, bounded offset/limit (max 200).
        details: a JSON pointer relative to details, e.g. /nativeSeries or /textureCheckpoints/0;
        nested objects/arrays return pointers to drill down, not enormous payloads.
        Preserves nulls, units, statuses and integer bytes; large integers include exact decimal strings.
        No SQL, launch, capture, writes or approval.
        """
        return query(analysis_path, view, family, scope, metric, section, offset, limit)

    return server


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--analysis", required=True)
    create_server(parser.parse_args().analysis).run(transport="stdio")
