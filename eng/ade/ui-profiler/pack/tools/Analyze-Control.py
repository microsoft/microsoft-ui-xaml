"""Add a control evidence lens to a sealed ADE analysis, without capturing or changing it.

CLI: --analysis PATH --control TYPE --directory NEW [--profiler-cli PINNED_EXE].
Outputs: control-analysis.json (ui-profiler/control-analysis/v1), control-analysis.md, manifest.json,
and optional bounded native timeline query exports. No SQL is accepted or executed.
Object IDs are recording-local; snapshot IDs are capture-local. A snapshot object
link is accepted only from the canonical snapshot engine's saved histories.
Source/model hypotheses belong in the explicit inferred/unverified channels, not
in measured values. This module does not replace the expert's source analysis.
"""

import argparse
import collections
import importlib.util
import pathlib
import subprocess
import sys


_spec = importlib.util.spec_from_file_location(
    "control_profiler_analysis", pathlib.Path(__file__).with_name("Analyze-Profiler.py"))
profiler = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(profiler)

MAX_INSTANCES = 64
MAX_HISTORY_EVENTS = 500
QUERY_TIMEOUT_SECONDS = 30


def is_type(actual, requested):
    """Same full/short exact type semantics as the native MCP objects tool."""
    return bool(actual) and (actual.casefold() == requested.casefold() or
                             actual.rsplit(".", 1)[-1].casefold() == requested.casefold())


def unavailable(reason, telemetry):
    return {"status": "not captured/unsupported", "value": None,
            "reason": reason, "extraProfilerTelemetryNeeded": telemetry}


def as_list(value):
    # Saved PowerShell engine exports can collapse singleton arrays to scalars.
    return [] if value is None else value if isinstance(value, list) else [value]


def descendants(nodes, start):
    children = collections.defaultdict(list)
    for node in nodes.values():
        if node.get("parentId") is not None:
            if node["parentId"] not in nodes:
                raise ValueError("Snapshot has a missing parent")
            children[node["parentId"]].append(node["id"])
    result, pending = set(), [start]
    while pending:
        current = pending.pop()
        if current in result:
            raise ValueError("Cyclic snapshot subtree")
        result.add(current)
        pending.extend(children[current])
    return result


def ancestry(nodes, node):
    result, seen = [], {node["id"]}
    cursor = node.get("parentId")
    while cursor is not None:
        if cursor in seen or cursor not in nodes:
            raise ValueError("Invalid snapshot ancestry")
        seen.add(cursor)
        parent = nodes[cursor]
        result.append({key: parent.get(key) for key in ("id", "type", "name")})
        cursor = parent.get("parentId")
    return result


def snapshot_evidence(analysis, documents, control):
    result = []
    session = analysis["session"]
    for snapshot, document in documents:
        if (document["processId"] != session["ProcessId"] or
                document["creationTimeFileTime"] != profiler.filetime(session["ProcessCreatedUtc"]) or
                document["qpcFrequency"] != session["QpcFrequency"]):
            raise ValueError("Snapshot process instance/clock does not match analysis")
        xaml = {node["id"]: node for node in document["xamlNodes"]}
        visuals = {node["id"]: node for node in document["compositionNodes"]}
        histories = {(row["tree"], row["nodeId"]): row for row in snapshot.get("histories", [])}
        roots = {root["id"]: root for root in document["roots"]}
        targets = []
        for node in xaml.values():
            if not is_type(node.get("type"), control):
                continue
            element_ids = descendants(xaml, node["id"])
            parents = ancestry(xaml, node)
            nested = [parent for parent in parents if is_type(parent.get("type"), control)]
            linked_visuals = set()
            for identity in element_ids:
                history = histories.get(("Xaml", identity))
                # matchingIds was exported by SnapshotViewerData.SelectNodes.
                if history is not None:
                    linked_visuals.update(as_list(history.get("matchingIds")))
            if not linked_visuals <= visuals.keys():
                raise ValueError("Canonical snapshot correlation names an absent visual")
            visual_ids = set()
            for identity in linked_visuals:
                visual_ids.update(descendants(visuals, identity))
            root = roots[node["rootId"]]
            history = histories.get(("Xaml", node["id"]))
            object_id = history.get("objectId") if history and history.get("status") == "matched" else None
            correlations_complete = all(("Xaml", identity) in histories for identity in element_ids)
            targets.append({
                "captureId": document["captureId"], "nodeId": node["id"], "rootId": node["rootId"],
                "type": node["type"], "name": node.get("name"), "ancestors": parents,
                "placement": "nested-same-type" if nested else "outermost-same-type",
                "sameTypeAncestorNodeIds": [parent["id"] for parent in nested],
                "canonicalObjectId": object_id,
                "identityStatus": "canonical matched" if object_id is not None else
                    "unavailable; not matched by name, address, or ordinal",
                "sampleInterval": {"startQpc": root["startQpc"], "endQpc": root["endQpc"]},
                "properties": node.get("properties", {}),
                "subtree": {
                    "status": "partial", "scope": "public realized snapshot only; includes self",
                    "xamlElements": len(element_ids), "xamlNodeIds": sorted(element_ids),
                    "directlyCorrelatedVisuals": len(linked_visuals) if correlations_complete else None,
                    "correlatedVisualDescendants": len(visual_ids) if correlations_complete else None,
                    "visualNodeIds": sorted(visual_ids) if correlations_complete else None,
                    "canonicalCorrelationExportComplete": correlations_complete,
                    "ownership": "structural reachability, not exclusive ownership; nested subtrees overlap",
                    "fullLogicalOrPrivateSubtree": "unknown/incomplete; popup/unrealized/private nodes are not assigned",
                },
            })
        result.append({
            "path": snapshot["path"], "phase": snapshot["phase"], "captureId": document["captureId"],
            "startQpc": document["startQpc"], "endQpc": document["endQpc"],
            "limitations": document.get("limitations", []),
            "resourceSnapshot": document.get("resourceSnapshot", document.get("memorySnapshot")),
            "instances": targets,
        })
    return result


def history_evidence(indexed, query, session, windows):
    """Only indexed ObjectPointer start/stop pairs establish per-object elapsed spans."""
    if query is None:
        return {"status": "unavailable", "reason": "No bounded canonical timeline export",
                "events": [], "spans": [], "unpairedBoundaries": [], "actionOverlap": []}
    if len(query) != 1 or query[0]["Object"]["ObjectId"] != indexed["ObjectId"]:
        raise ValueError("Native timeline does not identify the requested object")
    timeline = query[0]
    native_object = timeline["Object"]
    if native_object["Address"] != indexed["Address"] or native_object["Kind"] != indexed["Kind"]:
        raise ValueError("Native object identity disagrees with sealed analysis")
    frequency = session["QpcFrequency"]
    pending = collections.defaultdict(list)
    spans, events, unpaired = [], [], []
    for row in timeline["Events"]:
        event = row["Event"]
        if event["ProcessId"] != session["ProcessId"]:
            raise ValueError("Native timeline contains another process")
        fields = {item["Name"]: item["Value"] for item in event["Fields"]}
        events.append({"qpc": event["TimestampQpc"], "relativeMs": event["RelativeMilliseconds"],
                       "threadId": event["ThreadId"], "provider": event["Provider"], "name": event["Name"],
                       "identityFields": row["Fields"], "fields": fields, "frameNumber": row.get("FrameNumber")})
        # A parent/child reference or an unrelated pointer is not timing ownership.
        if ("ObjectPointer" not in row["Fields"] or fields.get("ObjectPointer") != indexed["Address"] or
                not isinstance(fields.get("IsStart"), bool)):
            continue
        key = (event["ThreadId"], event["Provider"], event["Name"])
        if fields["IsStart"]:
            pending[key].append(event["TimestampQpc"])
        elif pending[key]:
            start = pending[key].pop()
            end = event["TimestampQpc"]
            if end < start:
                raise ValueError("Native timeline has a negative span")
            spans.append({"threadId": key[0], "provider": key[1], "name": key[2],
                          "startQpc": start, "endQpc": end, "elapsedMs": (end - start) * 1000 / frequency,
                          "attribution": "indexed ObjectPointer on both boundaries; elapsed, not exclusive CPU",
                          "actionOverlap": [window["id"] for window in windows
                                            if window["id"].startswith("action:") and
                                            start < window["endQpc"] and end > window["startQpc"]]})
        else:
            unpaired.append({"threadId": key[0], "provider": key[1], "name": key[2], "boundary": "stop",
                             "qpc": event["TimestampQpc"]})
    for key, starts in pending.items():
        unpaired.extend({"threadId": key[0], "provider": key[1], "name": key[2], "boundary": "start",
                         "qpc": start} for start in starts)
    total = timeline["Total"]
    complete = timeline["Offset"] == 0 and len(events) == total
    counts = collections.Counter((event["provider"], event["name"]) for event in events)
    return {
        "status": "complete" if complete else "partial",
        "offset": timeline["Offset"], "returned": len(events), "total": total,
        "events": events, "eventCounts": [{"provider": key[0], "name": key[1], "count": value}
                                         for key, value in sorted(counts.items())],
        "spans": spans, "unpairedBoundaries": unpaired,
        "actionOverlap": [{"id": window["id"], "mentionsInReturnedPage": sum(
            profiler.in_window(event["qpc"], window) for event in events),
            "provesActionTarget": False} for window in windows if window["id"].startswith("action:")],
        "creation": unavailable("First retained mention is not allocation or creation.",
                                "Object creation/destruction events with stable lifetime IDs and QPC timestamps."),
    }


def timing_capabilities(instances):
    spans = [span for instance in instances for span in instance["history"]["spans"]]
    # This is an event contract, not a control-name heuristic.
    templates = [span for span in spans if span["provider"] == "Xaml" and
                 span["name"] == "FrameworkElement_ApplyTemplate"]
    result = {}
    if templates:
        complete = all(instance["history"]["status"] == "complete" and not any(
            boundary["provider"] == "Xaml" and boundary["name"] == "FrameworkElement_ApplyTemplate"
            for boundary in instance["history"]["unpairedBoundaries"]) for instance in instances)
        result["templating"] = {
            "status": "available" if complete else "partial", "unit": "ms",
            "scope": "observed indexed instances; nested elapsed spans, not exclusive control cost",
            "observedPairs": len(templates), "observedElapsed": profiler.summarize(
                [span["elapsedMs"] for span in templates]),
        }
    else:
        result["templating"] = unavailable("No proven per-instance template boundary pair.",
                                           "Paired ApplyTemplate events with indexed ObjectPointer on both boundaries.")
    for name in ("measure", "arrange"):
        result[name] = unavailable(
            "No canonical per-object " + name + " boundary contract in these exports; global layout is not assigned.",
            "Paired " + name + " start/stop events carrying stable indexed element/lifetime identity.")
    return result


def build_report(analysis, control, documents, timelines, actions, measurement):
    if not control or not control.strip():
        raise ValueError("--control requires a nonempty type name")
    details, session = analysis["details"], analysis["session"]
    activity = details.get("objectActivity", {})
    # Recording scope is canonical. Older derived evidence may lack it: union
    # the existing windows without pretending a mention is a lifetime.
    indexed = {}
    for row in activity.get("recording", []):
        if row["Kind"] == "Element" and is_type(row.get("Type"), control):
            indexed[row["ObjectId"]] = row
    if "recording" not in activity:
        for rows in activity.values():
            for row in rows:
                if row["Kind"] == "Element" and is_type(row.get("Type"), control):
                    indexed.setdefault(row["ObjectId"], row)
    instances = []
    tick_context = {}
    for identity, row in sorted(indexed.items(), key=lambda pair: (pair[1]["FirstQpc"], pair[0])):
        first = (row["FirstQpc"] - session["StartQpc"]) * 1000 / session["QpcFrequency"]
        ticks = [tick for tick in details.get("ticks", [])
                 if tick["StartMs"] <= first < tick["EndMs"]]
        for tick in ticks:
            if tick["TickId"] in tick_context:
                continue
            grouped = collections.defaultdict(list)
            for span in details.get("eventSpans", []):
                start = (span["startQpc"] - session["StartQpc"]) * 1000 / session["QpcFrequency"]
                end = (span["endQpc"] - session["StartQpc"]) * 1000 / session["QpcFrequency"]
                if (span["threadId"] == tick["ThreadId"] and
                        tick["StartMs"] <= start <= end <= tick["EndMs"]):
                    grouped[(span["provider"], span["name"])].append(end - start)
            tick_context[tick["TickId"]] = [
                {"provider": key[0], "name": key[1], "elapsedMs": profiler.summarize(values)}
                for key, values in sorted(grouped.items())]
        instances.append({
            **row, "firstMentionMs": first,
            "firstMentionActions": [window["id"] for window in analysis["windows"]
                                   if window["id"].startswith("action:") and
                                   profiler.in_window(row["FirstQpc"], window)],
            "firstMentionTickContext": ticks,
            "firstMentionTickSpans": [{"tickId": tick["TickId"], "attribution": "non-exclusive thread/tick context",
                                      "spans": tick_context[tick["TickId"]]} for tick in ticks],
            "actionActivity": [{"id": scope, "evidence": next(
                (item for item in rows if item["ObjectId"] == identity), None)}
                               for scope, rows in activity.items() if scope.startswith("action:")],
            "history": history_evidence(row, timelines.get(identity), session, analysis["windows"]),
        })
    snapshots = snapshot_evidence(analysis, documents, control)
    sampling = details.get("cpuSampling") or {}
    denominator = (sampling.get("Samples") or 0) + (sampling.get("Dropped") or 0)
    cpu_quality = {"sampling": sampling, "droppedPercent": 100 * sampling.get("Dropped", 0) / denominator
                   if denominator else None,
                   "symbols": dict(collections.Counter(
                       module.get("SymbolStatus") or "NotAttempted" for module in details.get("cpuModules", [])))}
    needs = {
        "cpuOwnership": unavailable(
            "Process CPU and sampled function stacks have no object-identity ownership join.",
            "CPU samples/activity intervals correlated to stable element/lifetime IDs; symbols and loss accounting."),
        "ownedHeapBytes": unavailable(
            "No per-control allocation, retained heap, or byte ownership was captured; process growth is not ownership.",
            "Allocation/free size, allocation stack, stable allocation/lifetime ID and an explicit element-owner relation."),
        "graphicsOwnership": unavailable(
            "Snapshot element/visual reachability does not prove element-to-surface-to-allocation byte ownership.",
            "Stable element -> visual -> surface -> texture/allocation lifetime links and shared-allocation accounting."),
        "creationLifetime": unavailable(
            "Object index stores first/last mentions; same-type address reuse can be undetectable.",
            "Explicit creation/destruction events and stable lifetime IDs, not inferred from addresses."),
    }
    return {
        "schema": "ui-profiler/control-analysis/v1", "control": control,
        "question": "What does this recording prove about instances of the supplied control type, their time and structure?",
        "setupActionsWaitsValidity": {
            "session": session, "window": analysis.get("window"), "actions": actions,
            "waits": "Action elapsed time includes automation waits. No unrecorded wait duration is invented.",
            "measurementHealth": measurement.get("health"), "cpuQuality": cpu_quality,
            "snapshotValidity": "Same process creation time and QPC frequency verified; roots are not globally atomic.",
            "instanceValidity": "Object IDs are recording-local. Snapshot IDs are capture-local. Names do not establish identity.",
            "actionTargetValidity": "Temporal overlap does not establish which instance automation invoked.",
        },
        "time": {"perInstanceCapabilities": timing_capabilities(instances),
                 "interpretation": "Tick/layout/action timings are non-exclusive context; no counts are converted to time."},
        "cpu": {"quality": cpu_quality, "controlOwned": needs["cpuOwnership"]},
        "memoryObjectsGraphics": {
            **needs, "indexedInstances": instances, "snapshots": snapshots,
            "snapshotCompleteness": "Public realized subtrees only; absent instances/subtrees are unknown, not zero.",
            "resourceCounters": "See retained resourceSnapshot status/HRESULT; global counters never become control-owned bytes.",
        },
        "measured": {
            "sourceCapabilities": analysis["capabilities"], "sourceScopeAvailability": analysis.get("scopeAvailability", {}),
            "contextMetrics": analysis["metrics"],
            "contextEvidence": "All original metrics are retained unchanged, with original family/scope/status/unit. "
                               "They are app/thread/action/recording context unless explicitly scoped above.",
        },
        "inferredSourceModel": {
            "status": "expert analysis permitted, not performed by this deterministic evidence lens",
            "guidance": "Use recorded type/name/ancestry and source to develop causal hypotheses. Label source/model "
                        "reasoning separately from measured values; numerical ownership is not required for hypotheses.",
        },
        "unverifiedEstimated": {
            "status": "No estimated duration, owned bytes, exclusive CPU, or causal improvement is presented as measured.",
            "gaps": needs,
        },
        "recommendationFix": {
            "status": "hypothesis; requires expert source/model analysis and validation",
            "recommendation": "Investigate the largest observed object-scoped template spans and their containing "
                              "action/tick/layout context. Inspect source for avoidable eager realization or repeated "
                              "work; structural nesting and first mentions alone do not prove a defect.",
            "validation": "Test a single proposed change with the same actions, waits, window and profiler payload. "
                          "Preserve unavailable dimensions and evaluate all retained metrics for regressions.",
        },
        "beforeAfter": {"status": "baseline only; no candidate supplied",
                        "before": "this recording", "after": None, "absoluteChange": None, "percentChange": None},
        "regressions": {"status": "not evaluated; no before/after pair", "measured": []},
        "evidenceLinks": [],
        "draftPr": {
            "status": "not drafted; no validated fix or candidate; repository template must be read before drafting a PR",
            "requiredEvidence": ["Question", "Setup/Actions/Waits/Validity", "Time", "CPU", "Memory/Objects/Graphics",
                                 "Measured vs inferred/source+model vs unverified/estimated", "Recommendation/fix",
                                 "Before/after absolute and percent change", "Regressions", "Evidence links"],
        },
    }


def markdown(report):
    sections = [
        "# Control evidence: " + report["control"], "", "## Question", report["question"], "",
        "## Setup/Actions/Waits/Validity",
        report["setupActionsWaitsValidity"]["waits"],
        report["setupActionsWaitsValidity"]["instanceValidity"],
        report["setupActionsWaitsValidity"]["actionTargetValidity"], "",
        "| Action | State | Elapsed including waits (ms) |", "|---|---|---:|",
    ]
    setup = report["setupActionsWaitsValidity"]
    for action in setup["actions"].get("steps", []):
        elapsed = (action["endedQpc"] - action["startedQpc"]) * 1000 / setup["session"]["QpcFrequency"]
        sections.append(f"| {action['id']} | {action['state']} | {elapsed:.4f} |")
    sections += ["", "## Time", report["time"]["interpretation"]]
    for name, capability in report["time"]["perInstanceCapabilities"].items():
        sections.append(f"- {name}: {capability['status']}; " +
                        (f"{capability['observedPairs']} proven pairs, observed total "
                         f"{capability['observedElapsed']['sum']:.4f} ms (nested/non-exclusive)."
                         if "observedPairs" in capability else capability["reason"]))
    sections += ["", "### Object-identity-proven elapsed spans",
                 "| Object | Name | Event | Elapsed (ms) | Action overlap (not target proof) |",
                 "|---:|---|---|---:|---|"]
    for instance in report["memoryObjectsGraphics"]["indexedInstances"]:
        for span in instance["history"]["spans"]:
            sections.append(f"| {instance['ObjectId']} | {instance.get('Name') or '<unnamed>'} | "
                            f"{span['provider']}/{span['name']} | {span['elapsedMs']:.4f} | "
                            f"{', '.join(span['actionOverlap']) or '<outside actions>'} |")
    sections += ["", "### First-mention tick context (not exclusive control time)",
                 "| Tick | Thread | Tick elapsed (ms) | New indexed mentions |",
                 "|---:|---:|---:|---|"]
    tick_instances = collections.defaultdict(list)
    tick_rows = {}
    for instance in report["memoryObjectsGraphics"]["indexedInstances"]:
        for tick in instance["firstMentionTickContext"]:
            tick_rows[tick["TickId"]] = tick
            tick_instances[tick["TickId"]].append(str(instance["ObjectId"]))
    for tick_id, tick in sorted(tick_rows.items()):
        sections.append(f"| {tick_id} | {tick['ThreadId']} | {tick['TickMs']:.4f} | "
                        f"{', '.join(tick_instances[tick_id])} |")
    seen_ticks = set()
    sections += ["", "### Largest nested spans in first-mention ticks (thread context only)",
                 "| Tick | Provider/event | Count | Max (ms) | Nested sum (ms) |",
                 "|---:|---|---:|---:|---:|"]
    for instance in report["memoryObjectsGraphics"]["indexedInstances"]:
        for context in instance["firstMentionTickSpans"]:
            if context["tickId"] in seen_ticks:
                continue
            seen_ticks.add(context["tickId"])
            for span in sorted(context["spans"], key=lambda row: row["elapsedMs"]["max"], reverse=True)[:5]:
                stats = span["elapsedMs"]
                sections.append(f"| {context['tickId']} | {span['provider']}/{span['name']} | {stats['count']} | "
                                f"{stats['max']:.4f} | {stats['sum']:.4f} |")
    sections += ["", "### Action/scenario elapsed and tick context (not control cost)",
                 "| Scope | Family/metric | Value | Unit | Status |",
                 "|---|---|---:|---|---|"]
    for metric in report["measured"]["contextMetrics"]:
        if ((metric["scope"] == "scenario" or metric["scope"].startswith("action:")) and
                ((metric["family"] == "scenario" and metric["metric"] == "elapsed") or
                 (metric["family"] == "ticks" and metric["metric"] in
                  ("duration.count", "duration.mean", "duration.p95", "duration.max")))):
            value = metric["value"]
            sections.append(f"| {metric['scope']} | {metric['family']}/{metric['metric']} | "
                            f"{value if value is not None else 'unavailable'} | {metric['unit']} | {metric['status']} |")
    sections += ["", "## CPU", report["cpu"]["controlOwned"]["reason"],
                 f"Sample loss: {report['cpu']['quality']['droppedPercent']}%; "
                 f"symbol states: {report['cpu']['quality']['symbols']}.", "",
                 "## Memory/Objects/Graphics"]
    memory = report["memoryObjectsGraphics"]
    for key in ("ownedHeapBytes", "graphicsOwnership", "creationLifetime"):
        sections.append("- " + memory[key]["status"] + ": " + memory[key]["reason"])
    sections += ["", "### Distinct indexed instances",
                 "| Object ID | Name | First mention (ms) | Events | History |",
                 "|---:|---|---:|---:|---|"]
    for instance in memory["indexedInstances"]:
        sections.append(f"| {instance['ObjectId']} | {instance.get('Name') or '<unnamed>'} | "
                        f"{instance['firstMentionMs']:.4f} | {instance.get('EventCount')} | "
                        f"{instance['history']['status']} |")
    for snapshot in memory["snapshots"]:
        sections += ["", "### Snapshot: " + snapshot["phase"],
                     "Structural reachability includes self; nested subtrees overlap. Full/private/unrealized ownership unknown.",
                     "Resource diagnostics: " + str(snapshot["resourceSnapshot"]),
                     "| Node | Name | Placement | Canonical object ID | XAML elements | Correlated visual descendants |",
                     "|---:|---|---|---|---:|---|"]
        for instance in snapshot["instances"]:
            tree = instance["subtree"]
            sections.append(f"| {instance['nodeId']} | {instance.get('name') or '<unnamed>'} | "
                            f"{instance['placement']} | {instance['canonicalObjectId']} | "
                            f"{tree['xamlElements']} | {tree['correlatedVisualDescendants']} |")
    sections += ["", "### Process CPU, memory and GPU context (never control ownership)",
                 "| Scope | Family/metric | Value | Unit | Status |",
                 "|---|---|---:|---|---|"]
    for metric in report["measured"]["contextMetrics"]:
        if (metric["scope"] == "scenario" and metric["family"] in ("cpu", "memory", "gpu", "gpuMemory") and
                metric["metric"].endswith((".mean", ".peak", ".change", ".coveragePercent", ".maxSamplingGap"))):
            sections.append(f"| {metric['scope']} | {metric['family']}/{metric['metric']} | "
                            f"{metric['value'] if metric['value'] is not None else 'unavailable'} | "
                            f"{metric['unit']} | {metric['status']} |")
    sections += ["", "## Measured vs inferred/source+model vs unverified/estimated",
                 "- Measured: canonical indexed mentions, explicitly identity-paired spans, snapshot reachability, "
                 "and unchanged source metrics with their original scopes/status.",
                 "- Inferred/source+model: expert hypotheses are permitted; this evidence lens does not replace source analysis.",
                 "- Unverified/estimated: no guessed owned bytes, exclusive CPU or durations from counts.", "",
                 "## Recommendation/fix", report["recommendationFix"]["recommendation"],
                 report["recommendationFix"]["validation"], "",
                 "## Before/after + absolute/% change",
                 "Baseline only. Candidate, absolute change and percent change: unavailable.", "",
                 "## Regressions", report["regressions"]["status"], "",
                 "## Evidence links"]
    sections.extend(f"- [{pathlib.Path(item['path']).name}]({pathlib.Path(item['path']).as_uri()}) "
                    f"(SHA-256 `{item['sha256']}`)" for item in report["evidenceLinks"])
    sections += ["", "Full instance histories, action overlap, ancestry, snapshot node IDs, unavailable capabilities, "
                 "extra telemetry requirements and all source metrics are in control-analysis.json.", "",
                 "## Draft PR", report["draftPr"]["status"], ""]
    return "\n".join(sections)


def pinned_closure(cli, engine, measurement, bound):
    cli = pathlib.Path(cli).resolve()
    if cli.name != "WinUIProfiler.Cli.exe":
        raise ValueError("Only the exact recorded WinUIProfiler.Cli.exe is accepted")
    if engine.get("measurementSha256") != measurement["sha256"]:
        raise ValueError("Canonical engine export is not bound to this measurement")
    tools = engine.get("tools", [])
    root = cli.parent
    closure = {pathlib.Path(item["path"]).resolve(): item for item in tools
               if pathlib.Path(item["path"]).resolve().is_relative_to(root)}
    actual = {path.resolve() for path in root.rglob("*") if path.is_file()}
    if cli not in closure or actual != closure.keys():
        raise ValueError("Pinned profiler closure differs from the canonical saved export")
    for path in sorted(actual):
        if path.is_symlink():
            raise ValueError("Profiler closure links are not accepted")
        bound(path)
    profiler.verify_references(closure.values())
    return [profiler.reference(path, "pinned-native-cli") for path in sorted(actual)]


def analyze(analysis_path, control, directory, cli=None):
    if not control or not control.strip():
        raise ValueError("--control requires a nonempty type name")
    analysis_path, directory = pathlib.Path(analysis_path).resolve(), pathlib.Path(directory).resolve()
    if directory.exists():
        raise ValueError("Use a fresh output directory; existing evidence is never overwritten")
    manifest_path = analysis_path.parent / "manifest.json"
    manifest = profiler.verify_manifest(manifest_path)
    sealed = {pathlib.Path(item["path"]).resolve(): item for item in manifest["inputs"] + manifest["outputs"]}

    def bound(path):
        path = pathlib.Path(path).resolve()
        if path not in sealed:
            raise ValueError("Referenced evidence is not bound by the source manifest: " + str(path))
        return sealed[path]

    if analysis_path not in {pathlib.Path(item["path"]).resolve() for item in manifest["outputs"]}:
        raise ValueError("Analysis is not an output of the supplied evidence manifest")
    analysis = profiler.load_json(analysis_path)
    if analysis.get("schema") != "ui-profiler/analysis/v1":
        raise ValueError("Unsupported source analysis schema")
    measurement_ref = bound(analysis["measurement"]["path"])
    measurement = profiler.load_json(measurement_ref["path"])
    database = bound(analysis["database"]["path"])
    if pathlib.Path(measurement["database"]["path"]).resolve() != pathlib.Path(database["path"]).resolve():
        raise ValueError("Measurement and analysis reference different databases")
    profiler.verify_references([analysis["database"], analysis["measurement"], analysis["actions"]])
    actions = profiler.load_json(bound(analysis["actions"]["path"])["path"])
    if actions["qpcFrequency"] != analysis["session"]["QpcFrequency"]:
        raise ValueError("Action and recording clocks differ")
    documents = [(snapshot, profiler.load_json(bound(snapshot["path"])["path"]))
                 for snapshot in analysis["details"].get("snapshots", [])]
    inputs = [profiler.reference(manifest_path, "evidence-manifest"),
              profiler.reference(analysis_path, "source-analysis")]
    implementation_sources = [profiler.reference(__file__, "control-analyzer"),
                              profiler.reference(profiler.__file__, "shared-analyzer")]
    if cli:
        engine = profiler.load_json(bound(analysis_path.parent / "queries" / "engine.json")["path"])
        inputs += pinned_closure(cli, engine, measurement_ref, bound)
    directory.mkdir(parents=True)
    implementation_directory = directory / "implementation"
    implementation_directory.mkdir()
    for source in implementation_sources:
        original = pathlib.Path(source["path"])
        saved = implementation_directory / original.name
        with saved.open("xb") as file:
            file.write(original.read_bytes())
        inputs.append({**profiler.reference(saved, source["origin"]), "sourcePath": source["path"]})
    timelines, queries = {}, []
    if cli:
        query_directory = directory / "queries"
        query_directory.mkdir()
        targets = [row for row in analysis["details"].get("objectActivity", {}).get("recording", [])
                   if row["Kind"] == "Element" and is_type(row.get("Type"), control)]
        for row in sorted(targets, key=lambda item: (item["FirstQpc"], item["ObjectId"]))[:MAX_INSTANCES]:
            identity = row["ObjectId"]
            if not isinstance(identity, int) or identity <= 0:
                raise ValueError("Invalid canonical object ID")
            arguments = [str(pathlib.Path(cli).resolve()), "timeline", database["path"], "--object", str(identity),
                         "--offset", "0", "--count", str(MAX_HISTORY_EVENTS), "--json"]
            output = subprocess.run(arguments, capture_output=True, timeout=QUERY_TIMEOUT_SECONDS, check=True)
            path = query_directory / f"object-{identity}.json"
            with path.open("xb") as file:
                file.write(output.stdout)
            if output.stderr:
                with path.with_suffix(".stderr.log").open("xb") as file:
                    file.write(output.stderr)
                inputs.append(profiler.reference(path.with_suffix(".stderr.log"), "native-query-diagnostics"))
            timelines[identity] = profiler.load_json(path)
            ref = profiler.reference(path, "canonical-native-query")
            queries.append({"command": arguments, "result": ref, "timeoutSeconds": QUERY_TIMEOUT_SECONDS})
            inputs.append(ref)
    report = build_report(analysis, control, documents, timelines, actions, measurement)
    report["queryBudget"] = {"maxInstances": MAX_INSTANCES, "maxEventsPerInstance": MAX_HISTORY_EVENTS,
                             "timeoutSecondsPerQuery": QUERY_TIMEOUT_SECONDS,
                             "queriedInstances": len(timelines), "queries": queries,
                             "noCapture": True, "noSql": True,
                             "incomplete": len(timelines) < len(report["memoryObjectsGraphics"]["indexedInstances"]) or
                             any(instance["history"]["status"] != "complete"
                                 for instance in report["memoryObjectsGraphics"]["indexedInstances"])}
    report["evidenceLinks"] = [profiler.reference(analysis_path, "source-analysis"),
                               profiler.reference(manifest_path, "evidence-manifest"),
                               profiler.reference(database["path"], "recorded")] + [query["result"] for query in queries]
    json_path, markdown_path = directory / "control-analysis.json", directory / "control-analysis.md"
    profiler.write_json(json_path, report)
    with markdown_path.open("x", encoding="utf-8", newline="\n") as file:
        file.write(markdown(report))
    # Revalidate the old database, closure and every source reference after queries.
    profiler.verify_manifest(manifest_path)
    profiler.verify_references(implementation_sources)
    profiler.seal(directory, inputs, [json_path, markdown_path])
    profiler.verify_manifest(directory / "manifest.json")
    return json_path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--analysis", required=True)
    parser.add_argument("--control", required=True)
    parser.add_argument("--directory", required=True)
    parser.add_argument("--profiler-cli")
    args = parser.parse_args()
    print(analyze(args.analysis, args.control, args.directory, args.profiler_cli))


if __name__ == "__main__":
    try:
        main()
    except (ValueError, KeyError, OSError, subprocess.SubprocessError) as error:
        sys.exit(str(error))
