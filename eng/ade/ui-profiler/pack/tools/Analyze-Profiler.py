"""Read-only, multidimensional analysis of MUX WinUIProfiler evidence."""

import argparse
import calendar
import collections
import csv
import datetime
import decimal
import hashlib
import json
import math
import pathlib
import sqlite3
import statistics
import sys


FAMILIES = {
    "scenario": "Scenario and individual automation actions",
    "ticks": "Elapsed UI ticks, draw flags and tick cadence",
    "spans": "Thread-local XAML event spans, including all layout passes",
    "regions": "Canonical regions, navigation, focus and operations",
    "events": "All retained provider/event activity",
    "cpu": "Native process CPU utilization",
    "cpuStacks": "Sampled CPU stacks, modules, functions and symbol quality",
    "memory": "Working set, private working set and private commit",
    "gpu": "Busiest process GPU-engine utilization",
    "gpuMemory": "Event-driven process GPU usage and budget",
    "graphics": "DirectX lifetimes, allocations and correlation intervals",
    "textures": "Canonical point-in-time texture descriptors and allocations",
    "objects": "Indexed element/visual activity and snapshot history",
    "trees": "Realized XAML/composition trees, properties and correlations",
    "resources": "Runtime allocator and rendering-resource diagnostics",
    "launch": "Launch milestones and the first ten drawn XAML ticks",
    "presentTiming": "GPU execution, present latency and displayed-frame timing",
    "heap": "Heap objects, allocation stacks and retained-object ownership",
    "textureOwnership": "Complete XAML element-to-surface-to-texture ownership",
    "texturePixels": "Texture pixels and image contents",
}
NATIVE = {
    "cpu": ("cpu", "percent"),
    "gpu": ("gpu", "percent"),
    "workingSet": ("memory", "MiB"),
    "privateWorkingSet": ("memory", "MiB"),
    "privateCommit": ("memory", "MiB"),
}
INTERPRETATION = (
    "One observed baseline/candidate pair, not statistical evidence. Action elapsed time includes "
    "automation waits. UI ticks and nested XAML spans are elapsed time, not CPU or displayed-frame "
    "time. Counts and structural differences are workload context, not independently improvements. "
    "No single score hides regressions or unavailable dimensions."
)


def load_json(path, exact=False):
    with pathlib.Path(path).open(encoding="utf-8-sig") as file:
        return json.load(file, parse_float=decimal.Decimal if exact else float)


def json_default(value):
    if isinstance(value, decimal.Decimal):
        return int(value) if value == value.to_integral_value() else float(value)
    raise TypeError(type(value).__name__)


def write_json(path, value):
    with pathlib.Path(path).open("x", encoding="utf-8", newline="\n") as file:
        json.dump(value, file, indent=2, ensure_ascii=True, allow_nan=False, default=json_default)
        file.write("\n")


def sha256(path):
    digest = hashlib.sha256()
    with pathlib.Path(path).open("rb") as file:
        for chunk in iter(lambda: file.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def reference(path, origin="analysis-time"):
    path = pathlib.Path(path).resolve()
    return {"path": str(path), "sha256": sha256(path), "bytes": path.stat().st_size, "origin": origin}


def verify_references(references):
    for item in references:
        if sha256(item["path"]) != item["sha256"]:
            raise ValueError("Evidence changed: " + item["path"])


def verify_manifest(path, visited=None):
    visited = set() if visited is None else visited
    path = pathlib.Path(path).resolve()
    if path in visited:
        raise ValueError("Cyclic evidence manifest")
    visited.add(path)
    manifest = load_json(path)
    if manifest["schema"] != "ui-profiler/evidence-manifest/v1":
        raise ValueError("Unsupported evidence manifest")
    verify_references(manifest["inputs"] + manifest["outputs"])
    for item in manifest["inputs"]:
        child = pathlib.Path(item["path"]).resolve()
        if item.get("origin") == "evidence-manifest":
            verify_manifest(child, visited)
    visited.remove(path)
    return manifest


def seal(directory, inputs, outputs):
    verify_references(inputs)
    write_json(pathlib.Path(directory) / "manifest.json", {
        "schema": "ui-profiler/evidence-manifest/v1",
        "inputs": inputs, "outputs": [reference(path, "derived") for path in outputs],
    })


def percentile(values, fraction=0.95):
    ordered = sorted(values)
    return ordered[math.ceil(len(ordered) * fraction) - 1] if ordered else None


def filetime(text):
    whole, _, fraction = text.rstrip("Z").partition(".")
    seconds = calendar.timegm(datetime.datetime.fromisoformat(whole).timetuple())
    return (seconds + 11644473600) * 10000000 + int((fraction + "0000000")[:7])


def make_windows(actions, session):
    frequency = actions["qpcFrequency"]
    if frequency <= 0 or frequency != session["QpcFrequency"] or session["Source"] != "Live":
        raise ValueError("Actions and recording do not share the live QPC clock")
    if actions["state"] != "complete" or not actions["steps"]:
        raise ValueError("A completed action audit is required")
    windows = [{"id": "scenario", "startQpc": actions["startedQpc"], "endQpc": actions["endedQpc"]}]
    previous = actions["startedQpc"]
    seen = set()
    for step in actions["steps"]:
        if (step["state"] != "passed" or step["id"] in seen or
                not previous <= step["startedQpc"] <= step["endedQpc"] <= actions["endedQpc"]):
            raise ValueError("Invalid, repeated or unordered action intervals")
        seen.add(step["id"])
        previous = step["endedQpc"]
        windows.append({"id": "action:" + step["id"], "startQpc": step["startedQpc"], "endQpc": step["endedQpc"]})
    for window in windows:
        if not session["StartQpc"] <= window["startQpc"] < window["endQpc"]:
            raise ValueError("Action interval is outside the recording clock")
        window["startMs"] = (window["startQpc"] - session["StartQpc"]) * 1000 / frequency
        window["endMs"] = (window["endQpc"] - session["StartQpc"]) * 1000 / frequency
    return windows


def in_window(qpc, window):
    return window["startQpc"] <= qpc < window["endQpc"]


def summarize(values):
    return {
        "count": len(values), "min": min(values) if values else None,
        "mean": statistics.fmean(values) if values else None,
        "median": statistics.median(values) if values else None,
        "p95": percentile(values), "max": max(values) if values else None,
        "sum": sum(values),
    }


def gauge(events, start, end, field):
    """Carry in only an observed value; uncovered time never becomes zero."""
    events = sorted(events, key=lambda row: (row["qpc"], row["seq"]))
    prior = [row for row in events if row["qpc"] <= start]
    value = prior[-1][field] if prior else None
    first = value
    cursor, covered, area = start, 0, 0
    observed = [] if value is None else [value]
    changes = 0
    for row in events:
        if not start < row["qpc"] < end:
            continue
        if value is not None:
            elapsed = row["qpc"] - cursor
            area += value * elapsed
            covered += elapsed
        cursor, value = row["qpc"], row[field]
        observed.append(value)
        changes += 1
    if value is not None:
        area += value * (end - cursor)
        covered += end - cursor
    return {
        "start": first, "end": value, "peak": max(observed) if observed else None,
        "mean": int((decimal.Decimal(area) / covered).to_integral_value()) if covered else None,
        "change": value - first if value is not None and first is not None else None,
        "coveragePercent": 100 * covered / (end - start), "changes": changes,
    }


class Analysis:
    def __init__(self, measurement, actions, connection, engine):
        self.measurement = measurement
        self.actions = actions
        self.db = connection
        self.engine = engine
        self.session = dict(connection.execute("SELECT * FROM SessionInfo").fetchone())
        self.windows = make_windows(actions, self.session)
        last_qpc = connection.execute("SELECT MAX(Qpc) FROM Events").fetchone()[0]
        if last_qpc is not None and last_qpc >= self.session["StartQpc"]:
            self.windows.append({
                "id": "recording", "startQpc": self.session["StartQpc"], "endQpc": last_qpc + 1,
                "startMs": 0, "endMs": (last_qpc + 1 - self.session["StartQpc"]) * 1000 / self.session["QpcFrequency"],
            })
        self.thread = actions["window"]["threadId"]
        self.tables = {row[0] for row in connection.execute(
            "SELECT name FROM sqlite_master WHERE type IN ('table','view')")}
        self.metrics, self.details, self.scopes = [], {}, {}
        self.capabilities = {
            family: {"label": label, "status": "unavailable", "notes": "No retained evidence."}
            for family, label in FAMILIES.items()
        }
        for family in ("presentTiming", "heap", "textureOwnership", "texturePixels"):
            self.capability(family, "unsupported", "Not supplied by this profiler evidence contract; never inferred.")

    def capability(self, family, status, notes):
        self.capabilities[family].update(status=status, notes=notes)

    def scope(self, family, scope, status):
        key = family + "/" + scope
        previous = self.scopes.get(key)
        self.scopes[key] = status if previous is None or previous == status else "partial"

    def add(self, family, scope, metric, value, unit="count", status="available",
            direction="context", zero=False, primary=False):
        self.metrics.append({
            "family": family, "scope": scope, "metric": metric, "unit": unit, "value": value,
            "status": status if value is not None else "unavailable", "direction": direction,
            "zeroWhenAbsent": zero, "primary": primary,
        })

    def stats(self, family, scope, name, values, unit="ms", status="available", zero=False, primary=False,
              additive=True, direction="lower"):
        for statistic, value in summarize(values).items():
            if statistic == "sum" and not additive:
                continue
            self.add(family, scope, name + "." + statistic, value, "count" if statistic == "count" else unit,
                     status, direction if statistic != "count" else "context",
                     zero and statistic in ("count", "sum"), primary and statistic in ("mean", "p95", "max", "count"))

    def timing(self):
        self.capability("scenario", "available", "Half-open QPC action windows; elapsed time includes waits. "
                        "The recording scope is the retained event span, not an independently measured wall-clock duration.")
        self.capability("ticks", "available", "Complete ticks wholly inside each window. Cadence is not present timing.")
        ticks = [dict(row) for row in self.db.execute("SELECT * FROM Ticks")]
        self.details["ticks"] = ticks
        for window in self.windows:
            scope = window["id"]
            for family in ("scenario", "ticks"):
                self.scope(family, scope, "available")
            self.add("scenario", scope, "elapsed", window["endMs"] - window["startMs"], "ms",
                     direction="lower" if scope != "recording" else "context", primary=scope != "recording")
            selected = [row for row in ticks if row["ThreadId"] == self.thread and
                        row["StartMs"] is not None and row["EndMs"] is not None and
                        window["startMs"] <= row["StartMs"] <= row["EndMs"] <= window["endMs"]]
            self.stats("ticks", scope, "duration", [row["TickMs"] for row in selected], primary=True)
            for name in ("Drew", "Slow"):
                self.add("ticks", scope, name.lower(), sum(row[name] == 1 for row in selected), zero=True)
                self.add("ticks", scope, name.lower() + ".unknown", sum(row[name] is None for row in selected), zero=True)
            for name in ("Events", "Xaml", "Composition", "Other"):
                self.add("ticks", scope, name.lower(), sum(row[name] for row in selected), zero=True)
            starts = sorted(row["StartMs"] for row in selected)
            self.stats("ticks", scope, "startInterval", [b - a for a, b in zip(starts, starts[1:])],
                       additive=False, direction="context")
            overlapping = [row for row in ticks if row["ThreadId"] == self.thread and
                           (row["EndMs"] is None or row["EndMs"] > window["startMs"]) and
                           (row["StartMs"] is None or row["StartMs"] < window["endMs"])]
            selected_ids = {row["TickId"] for row in selected}
            self.add("ticks", scope, "boundaryOrIncomplete",
                     sum(row["TickId"] not in selected_ids for row in overlapping), zero=True)
        main = [row for row in self.metrics if row["scope"] == "scenario"]
        lookup = {row["metric"]: row["value"] for row in main}
        for metric, key in (("completedTicks", "duration.count"), ("meanTickMs", "duration.mean"),
                            ("p95TickMs", "duration.p95"), ("maxTickMs", "duration.max"), ("totalTickMs", "duration.sum")):
            if not math.isclose(self.measurement["metrics"][metric], lookup[key], abs_tol=0.0001):
                raise ValueError("Recorded tick summary disagrees with SQLite: " + metric)

    def events_and_spans(self):
        self.capability("events", "available", "All retained event types; counts are activity, not exclusive costs.")
        self.capability("spans", "available",
                        "XAML /Start-/Stop pairs use a per-thread/provider/name LIFO stack. "
                        "Nested durations must not be summed as exclusive CPU. Unpaired boundaries remain explicit.")
        counts = {window["id"]: collections.Counter() for window in self.windows}
        stacks = collections.defaultdict(list)
        spans, unpaired, gpu = [], collections.Counter(), collections.defaultdict(list)
        event_types = {row["TypeId"]: dict(row) for row in self.db.execute("SELECT * FROM EventTypes")}
        for row in self.db.execute("SELECT Seq,Qpc,ThreadId,TypeId,Payload FROM Events ORDER BY Qpc,Seq"):
            event = event_types[row["TypeId"]]
            key = (event["Provider"], event["Name"], event["Opcode"], event["Launcher"])
            for window in self.windows:
                if in_window(row["Qpc"], window):
                    counts[window["id"]][key] += 1
            provider, name = event["Provider"], event["Name"]
            if provider in ("Xaml", "XamlControls") and name.endswith(("/Start", "/Stop")):
                span_key = (row["ThreadId"], provider, name.rsplit("/", 1)[0])
                if name.endswith("/Start"):
                    stacks[span_key].append((row["Qpc"], row["Seq"]))
                elif stacks[span_key]:
                    start, seq = stacks[span_key].pop()
                    spans.append({"threadId": row["ThreadId"], "provider": provider, "name": span_key[2],
                                  "startQpc": start, "endQpc": row["Qpc"], "startSeq": seq, "endSeq": row["Seq"]})
                else:
                    unpaired[(provider, span_key[2], "stop")] += 1
            if provider == "DxgKrnl" and name in ("VidMmProcessUsageChange", "VidMmProcessBudgetChange"):
                fields = dict(zip(event["FieldNames"].split(","), json.loads(row["Payload"])))
                if fields.get("ProcessId") != self.session["ProcessId"]:
                    continue
                required = ("pDxgAdapter", "PhysicalAdapterIndex", "MemorySegmentGroup")
                if any(not isinstance(fields.get(field), int) for field in required):
                    raise ValueError("Incomplete GPU adapter/segment identity")
                gauge_name = "usage" if name == "VidMmProcessUsageChange" else "budget"
                field = "NewUsage" if gauge_name == "usage" else "NewBudget"
                if not isinstance(fields.get(field), int) or fields[field] < 0:
                    raise ValueError("Invalid exact GPU byte value")
                gpu[(fields["PhysicalAdapterIndex"], fields["MemorySegmentGroup"], fields["pDxgAdapter"])].append({
                    "qpc": row["Qpc"], "seq": row["Seq"], "gauge": gauge_name, "value": fields[field], "fields": fields})
        for key, values in stacks.items():
            unpaired[(key[1], key[2], "start")] += len(values)
        if sum(unpaired.values()):
            self.capability("spans", "partial", self.capabilities["spans"]["notes"] + " Some pairs are incomplete.")
        self.details["unpairedSpans"] = [{"provider": key[0], "name": key[1], "boundary": key[2], "count": value}
                                       for key, value in sorted(unpaired.items()) if value]
        self.details["eventSpans"] = spans
        for window in self.windows:
            scope = window["id"]
            for family in ("events", "spans"):
                self.scope(family, scope, self.capabilities[family]["status"])
            self.add("events", scope, "total", sum(counts[scope].values()), zero=True, primary=True)
            for key, count in sorted(counts[scope].items()):
                label = "/".join(map(str, key))
                self.add("events", scope, "event:" + label, count, zero=True)
            grouped = collections.defaultdict(list)
            for span in spans:
                if (span["threadId"] == self.thread and window["startQpc"] <= span["startQpc"] and
                        span["endQpc"] <= window["endQpc"]):
                    grouped[span["provider"] + "/" + span["name"]].append(
                        (span["endQpc"] - span["startQpc"]) * 1000 / self.session["QpcFrequency"])
            for name, durations in sorted(grouped.items()):
                provider, event_name = name.split("/", 1)
                status = "partial" if unpaired[(provider, event_name, "start")] or unpaired[(provider, event_name, "stop")] else "available"
                self.stats("spans", scope, name, durations, status=status, zero=True)
        self.gpu_memory(gpu)

    def regions(self):
        if "InterestingRows" not in self.tables:
            return
        rows = [dict(row) for row in self.db.execute("SELECT * FROM InterestingRows")]
        self.details["interestingRows"] = rows
        self.capability("regions", "available", "Canonical InterestingRows; overlapping/nested spans are not additive CPU time.")
        for window in self.windows:
            scope = window["id"]
            self.scope("regions", scope, "available")
            groups = collections.defaultdict(list)
            for row in rows:
                if (row["ThreadId"] == self.thread and row["StopMs"] is not None and
                        row["StartMs"] < window["endMs"] and row["StopMs"] >= window["startMs"]):
                    groups[row["Type"] + "/" + row["Name"]].append(
                        max(0, min(row["StopMs"], window["endMs"]) - max(row["StartMs"], window["startMs"])))
            self.add("regions", scope, "overlappingRows", sum(map(len, groups.values())), zero=True)
            for name, durations in sorted(groups.items()):
                self.stats("regions", scope, name + ".clipped", durations, zero=True)

    def native(self):
        readings = self.measurement.get("sampledMetrics", [])
        by_metric = collections.defaultdict(list)
        creation = filetime(self.session["ProcessCreatedUtc"])
        previous = -1
        for packet in readings:
            identity = packet["process"]
            if identity["processId"] != self.session["ProcessId"] or identity["creationTimeFileTime"] != creation:
                raise ValueError("Native counters belong to a different process instance")
            timestamp = packet["timestampQpc"]
            values = packet["values"]
            if timestamp < previous or len(values) != 5 or {value["metric"] for value in values} != set(NATIVE):
                raise ValueError("Incomplete or unordered native acquisition")
            previous = timestamp
            for entry in values:
                metric, sample = entry["metric"], entry["sample"]
                value = sample["value"]
                if (entry["unit"] != NATIVE[metric][1] or sample["timestampQpc"] != timestamp or
                        not sample["source"] or
                        (value is None and not sample["unavailableReason"]) or
                        (value is not None and (not math.isfinite(value) or value < 0 or
                                              (entry["unit"] == "percent" and value > 100)))):
                    raise ValueError("Invalid native metric contract")
                by_metric[metric].append(sample)
        self.details["nativeSeries"] = by_metric
        self.details["nativeAcquisitions"] = [
            {"timestampQpc": packet["timestampQpc"], "totalSamples": packet.get("totalSamples")} for packet in readings]
        for family in ("cpu", "memory", "gpu"):
            self.capability(family, "available" if readings else "unavailable",
                            "250 ms timestamped point samples; warmup/missing values stay null. "
                            "CPU is processor-normalized; GPU is the busiest engine, not the sum. "
                            "Private commit is not residency. Point means are not integrated CPU/GPU time. "
                            "Large time/ordinal gaps qualify the affected window; the host stream can omit acquisitions.")
        for window in self.windows:
            scope = window["id"]
            ordinals = [packet.get("totalSamples") for packet in readings if in_window(packet["timestampQpc"], window)]
            skipped = sum(max(0, b - a - 1) for a, b in zip(ordinals, ordinals[1:])) if (
                len(ordinals) > 1 and all(isinstance(value, int) for value in ordinals)) else None
            for metric, (family, unit) in NATIVE.items():
                selected = [row for row in by_metric[metric] if in_window(row["timestampQpc"], window)]
                valid = [row for row in selected if row["value"] is not None]
                status = "unavailable" if not valid else "partial" if len(valid) < len(selected) else "available"
                boundaries = [window["startQpc"]] + [row["timestampQpc"] for row in valid] + [window["endQpc"]]
                gap = max(b - a for a, b in zip(boundaries, boundaries[1:])) * 1000 / self.session["QpcFrequency"]
                if status == "available" and (gap > 500 or skipped):
                    status = "partial"
                self.scope(family, scope, status)
                values = [row["value"] for row in valid]
                self.stats(family, scope, metric, values, unit, status, primary=scope == "scenario", additive=False)
                for statistic, value in (("first", values[0] if values else None), ("last", values[-1] if values else None),
                                         ("change", values[-1] - values[0] if len(values) > 1 else None)):
                    self.add(family, scope, metric + "." + statistic, value, unit, status,
                             "lower" if statistic != "change" else "context", primary=scope == "scenario")
                self.add(family, scope, metric + ".missing", len(selected) - len(valid), zero=True)
                self.add(family, scope, metric + ".unemittedAcquisitionsBetweenPoints", skipped)
                self.add(family, scope, metric + ".maxSamplingGap", gap, "ms")
        for family in ("cpu", "memory", "gpu"):
            statuses = [value for key, value in self.scopes.items() if key.startswith(family + "/")]
            if statuses and any(value != "available" for value in statuses):
                self.capabilities[family]["status"] = "partial" if any(value != "unavailable" for value in statuses) else "unavailable"

    def cpu_stacks(self):
        if not {"CpuSampling", "CpuSamples", "CpuFrames"} <= self.tables:
            return
        row = self.db.execute("SELECT * FROM CpuSampling").fetchone()
        if row is None:
            return
        metadata = dict(row)
        status = "partial" if metadata["Dropped"] or metadata["CompressedStacks"] else "available"
        if metadata["Samples"] == 0:
            status = "unavailable"
        self.capability("cpuStacks", status,
                        "CPU drops are separate from ETW event loss. Sample counts/estimates are observed, "
                        "not complete CPU cost when stacks were dropped. Unresolved frames aggregate by module, "
                        "never by ASLR address; inclusive recursion is counted once per sample.")
        frames = {row["StackId"]: dict(row) for row in self.db.execute("SELECT * FROM CpuFrames")}
        cache = {}

        def stack(node):
            if node in cache:
                return cache[node]
            result, seen = [], set()
            while node is not None:
                if node in seen or node not in frames:
                    raise ValueError("Cyclic or incomplete stored CPU stack")
                seen.add(node)
                result.append(frames[node])
                node = frames[node]["ParentId"]
            cache[result[0]["StackId"]] = result
            return result

        samples = [dict(row) for row in self.db.execute("SELECT * FROM CpuSamples WHERE ProcessId=?", (self.session["ProcessId"],))]
        self.details["cpuSampling"] = metadata
        self.details["cpuModules"] = [dict(row) for row in self.db.execute("SELECT * FROM Modules")]
        self.details["cpuFrames"] = list(frames.values())
        module_statuses = collections.Counter(module["SymbolStatus"] or "NotAttempted" for module in self.details["cpuModules"])
        self.scope("cpuStacks", "recording", status)
        self.add("cpuStacks", "recording", "modules", len(self.details["cpuModules"]), status=status)
        for symbol_status, count in sorted(module_statuses.items()):
            self.add("cpuStacks", "recording", "moduleSymbols:" + symbol_status, count, zero=True)
        if "CodeAddresses" in self.tables:
            total, named = self.db.execute(
                "SELECT COUNT(*),COALESCE(SUM(Function IS NOT NULL),0) FROM CodeAddresses").fetchone()
            self.add("cpuStacks", "recording", "codeAddresses", total)
            self.add("cpuStacks", "recording", "namedCodeAddresses", named)
            self.add("cpuStacks", "recording", "namedAddressPercent", 100 * named / total if total else None,
                     "percent", primary=True)
        for name in ("Samples", "Dropped", "CompressedStacks"):
            self.add("cpuStacks", "recording", name, metadata[name], status=status, primary=True)
        attempted = metadata["Samples"] + metadata["Dropped"]
        self.add("cpuStacks", "recording", "dropPercent", 100 * metadata["Dropped"] / attempted if attempted else None,
                 "percent", status, primary=True)
        for window in self.windows:
            scope = window["id"]
            self.scope("cpuStacks", scope, status)
            selected = [sample for sample in samples if in_window(sample["Qpc"], window)]
            for group, subset in (("allThreads", selected), ("uiThread", [sample for sample in selected if sample["ThreadId"] == self.thread])):
                self.add("cpuStacks", scope, group + ".samples", len(subset), status=status, zero=True, primary=True)
                self.add("cpuStacks", scope, group + ".estimated", len(subset) * metadata["IntervalMs"]
                         if metadata["IntervalMs"] is not None else None, "ms", status, primary=scope == "scenario")
                exclusive, inclusive = collections.Counter(), collections.Counter()
                unresolved = 0
                for sample in subset:
                    path = stack(sample["StackId"])
                    if not path[0]["Function"]:
                        unresolved += 1
                    label = lambda frame: frame["Module"] + "!" + (frame["Function"] or "<unresolved>")
                    exclusive[label(path[0])] += 1
                    inclusive.update({label(frame) for frame in path})
                self.add("cpuStacks", scope, group + ".unresolvedLeafSamples", unresolved, status=status, zero=True)
                self.add("cpuStacks", scope, group + ".unresolvedLeafPercent",
                         100 * unresolved / len(subset) if subset else None, "percent", status)
                for kind, counts in (("exclusive", exclusive), ("inclusive", inclusive)):
                    for label, count in sorted(counts.items()):
                        self.add("cpuStacks", scope, group + "." + kind + ":" + label, count, status=status, zero=True)
        if any(not frame["Function"] for frame in frames.values()):
            self.capabilities["cpuStacks"]["notes"] += " Symbols are incomplete; no unresolved address is a source function."

    def gpu_memory(self, groups):
        if not groups:
            return
        aliases = collections.defaultdict(set)
        for adapter, segment, pointer in groups:
            aliases[(adapter, segment)].add(pointer)
        ambiguous = any(len(pointers) != 1 for pointers in aliases.values())
        self.capability("gpuMemory", "partial" if ambiguous else "available",
                        "Exact byte counters, carried in only from observed events. Local/nonlocal segments "
                        "remain separate. PhysicalAdapterIndex is an ordinal, not a proven hardware LUID; "
                        "multiple pointers for one ordinal/segment are not cross-run comparable. "
                        "Time-weighted means are rounded to the nearest byte. A reported usage of zero is "
                        "not proof of no GPU allocations.")
        self.details["gpuMemorySeries"] = [
            {"adapterIndex": adapter, "segment": segment, "adapterPointer": pointer, "events": rows}
            for (adapter, segment, pointer), rows in sorted(groups.items())]
        for window in self.windows:
            scope = window["id"]
            self.scope("gpuMemory", scope, "partial" if ambiguous else "available")
            for adapter, segment, pointer in sorted(groups):
                label = "adapter:" + str(adapter) + "/" + {0: "local", 1: "nonlocal"}.get(segment, str(segment))
                comparable = len(aliases[(adapter, segment)]) == 1
                if not comparable:
                    label += "/ambiguous:" + str(pointer)
                for metric in ("usage", "budget"):
                    events = [row for row in groups[(adapter, segment, pointer)] if row["gauge"] == metric]
                    values = gauge(events, window["startQpc"], window["endQpc"], "value")
                    status = "not_comparable" if not comparable else "available" if values["coveragePercent"] == 100 else "partial"
                    for name, value in values.items():
                        unit = "percent" if name == "coveragePercent" else "count" if name == "changes" else "bytes"
                        self.add("gpuMemory", scope, label + "." + metric + "." + name, value, unit, status,
                                 "lower" if metric == "usage" and name in ("start", "end", "peak", "mean") else "context",
                                 primary=scope == "scenario")
            for segment in ("local", "nonlocal"):
                if not any(key[1] == (0 if segment == "local" else 1) for key in groups):
                    self.add("gpuMemory", scope, segment + ".usage", None, "bytes")

    def graphics(self):
        if not {"GraphicsLifetimes", "GraphicsResourceAllocations", "GraphicsLinks"} <= self.tables:
            return
        objects = [dict(row) for row in self.db.execute("SELECT * FROM GraphicsLifetimes")]
        links = [dict(row) for row in self.db.execute(
            "SELECT l.*,b.Qpc AS BeginQpc,e.Qpc AS EndQpc FROM GraphicsLinks l "
            "LEFT JOIN Events b ON b.Seq=l.StartSeq LEFT JOIN Events e ON e.Seq=l.EndSeq")]
        self.details["graphicsLifetimes"] = objects
        self.details["graphicsLinks"] = links
        self.details["graphicsResourceAllocations"] = [dict(row) for row in self.db.execute("SELECT * FROM GraphicsResourceAllocations")]
        self.capability("graphics", "available" if objects else "unavailable",
                        "Recording-local lifetimes and time-bounded links, not cross-run handle matches. "
                        "Open-ended lifetimes are observed candidates, not proof of liveness/leaks. "
                        "Recorded allocation bytes are exact size metadata, not residency or texture pixel-size estimates.")
        kinds = {row["Kind"] for row in objects}
        for window in self.windows:
            scope = window["id"]
            self.scope("graphics", scope, self.capabilities["graphics"]["status"])
            for kind in sorted(kinds):
                overlap = [row for row in objects if row["Kind"] == kind and
                           min(row["StartQpc"] if row["StartQpc"] is not None else row["KnownFromQpc"], row["KnownFromQpc"]) < window["endQpc"] and
                           (row["EndQpc"] is None or row["EndQpc"] > window["startQpc"])]
                self.add("graphics", scope, kind + ".overlappingLifetimes", len(overlap), zero=True)
                for boundary, qpc in (("start", window["startQpc"]), ("end", window["endQpc"])):
                    observed = [row for row in overlap if (row["StartQpc"] if row["StartQpc"] is not None else row["KnownFromQpc"]) <= qpc and
                                (row["EndQpc"] is None or row["EndQpc"] > qpc)]
                    self.add("graphics", scope, kind + ".observedOpen." + boundary, len(observed), zero=True,
                             primary=scope == "scenario")
                for name, predicate in (
                    ("created", lambda row: row["StartQpc"] is not None and in_window(row["StartQpc"], window)),
                    ("destroyed", lambda row: row["StopQpc"] is not None and in_window(row["StopQpc"], window)),
                    ("unknownStart", lambda row: row["StartQpc"] is None),
                    ("unknownStop", lambda row: row["StopQpc"] is None),
                    ("handleReuseEnd", lambda row: row["EndReason"] == "HandleReuse"),
                    ("trimmed", lambda row: bool(row["Trimmed"])),
                ):
                    self.add("graphics", scope, kind + "." + name, sum(predicate(row) for row in overlap), zero=True,
                             primary=scope == "scenario" and name in ("created", "destroyed", "unknownStop"))
                if kind == "AdapterAllocation":
                    known = [int(row["SizeBytes"]) for row in overlap if row["SizeBytes"] is not None]
                    self.add("graphics", scope, kind + ".knownSizeRows", len(known), zero=True)
                    self.add("graphics", scope, kind + ".unknownSizeRows", len(overlap) - len(known), zero=True)
                    self.add("graphics", scope, kind + ".recordedKnownSizeSum", sum(known) if known else None, "bytes",
                             "partial" if len(known) != len(overlap) else "available", primary=scope == "scenario")
            for kind in ("ResourceAllocation", "AllocationGlobal"):
                active = [row for row in links if row["Kind"] == kind and row["BeginQpc"] is not None and
                          row["BeginQpc"] < window["endQpc"] and
                          (row["EndSeq"] is None or row["EndQpc"] is not None and row["EndQpc"] > window["startQpc"])]
                self.add("graphics", scope, kind + ".overlappingLinks", len(active), zero=True)

    def texture_checkpoints(self):
        checkpoints = self.engine.get("textureCheckpoints", [])
        available = [row for row in checkpoints if row["capture"]["available"]]
        self.capability("textures", "available" if len(available) == len(checkpoints) and available else "partial" if available else "unavailable",
                        "Uses MUX SessionFile.ReadTextureSnapshots, including historical descriptors, unique "
                        "resource claims, matching endpoints and deduplication of shared adapter allocations. "
                        "QPC query checkpoints are not additional tree captures. Unknown links/sizes stay unknown.")
        self.details["textureCheckpoints"] = checkpoints
        for row in checkpoints:
            scope, capture, boundary = row["scope"], row["capture"], row["boundary"]
            status = "available" if capture["available"] and not capture["trimmed"] else "unavailable"
            self.scope("textures", scope, status)
            self.add("textures", scope, boundary + ".count", len(capture["textures"]) if capture["available"] else None,
                     status=status, primary=scope == "scenario")
            self.add("textures", scope, boundary + ".knownLinkedBytes", capture["knownLinkedBytes"], "bytes",
                     status, primary=scope == "scenario")
            missing = sum(texture["linkedBytes"] is None for texture in capture["textures"])
            self.add("textures", scope, boundary + ".unknownLinkedTextureCount", missing if capture["available"] else None,
                     status=status, primary=scope == "scenario")
            descriptors = collections.Counter()
            for texture in capture["textures"]:
                descriptor = json.loads(texture["descriptor"])
                for name, value in descriptor.items():
                    descriptors[(texture["kind"], name, json.dumps(value, sort_keys=True))] += 1
            for key, count in sorted(descriptors.items()):
                self.add("textures", scope, boundary + ".descriptor:" + "/".join(key), count, zero=True)

    def object_activity(self):
        if not {"Objects", "ObjectEvents"} <= self.tables:
            return
        self.capability("objects", "available",
                        "Indexed mentions, not every allocated/live object. Object IDs and addresses are "
                        "recording-local; cross-run comparisons use type/name aggregates. Snapshot histories "
                        "are matched only by the profiler's process/time-aware identity implementation.")
        details = {}
        for window in self.windows:
            scope = window["id"]
            self.scope("objects", scope, "available")
            rows = [dict(row) for row in self.db.execute(
                "SELECT o.*,COUNT(DISTINCT e.Seq) AS EventCount,COUNT(*) AS Mentions,MIN(e.Qpc) AS FirstQpc,MAX(e.Qpc) AS LastQpc "
                "FROM Objects o JOIN ObjectEvents m ON m.ObjectId=o.ObjectId JOIN Events e ON e.Seq=m.Seq "
                "WHERE e.Qpc>=? AND e.Qpc<? GROUP BY o.ObjectId", (window["startQpc"], window["endQpc"]))]
            details[scope] = rows
            self.add("objects", scope, "mentionedObjects", len(rows), zero=True, primary=scope == "scenario")
            self.add("objects", scope, "mentions", sum(row["Mentions"] for row in rows), zero=True)
            groups = collections.defaultdict(list)
            for row in rows:
                groups[row["Kind"] + "/" + (row["Type"] or "<unknown>")].append(row)
            for name, group in sorted(groups.items()):
                self.add("objects", scope, "type:" + name + ".objects", len(group), zero=True)
                self.add("objects", scope, "type:" + name + ".mentions", sum(row["Mentions"] for row in group), zero=True)
        self.details["objectActivity"] = details

    def snapshots(self):
        snapshots = self.engine.get("snapshots", [])
        self.capability("trees", "available" if snapshots else "unavailable",
                        "Realized public trees, not logical/unrealized objects. Roots and resource counters "
                        "are not atomic. Cross-run type/property distributions only; exact node diffs are "
                        "restricted to two snapshots of the same process instance.")
        resource_available = False
        phases = set()
        self.details["snapshots"] = snapshots
        for snapshot in snapshots:
            document = load_json(snapshot["path"])
            phase = snapshot["phase"]
            if phase in phases:
                raise ValueError("Ambiguous snapshot phase; do not pair capture IDs across processes")
            phases.add(phase)
            self.scope("trees", phase, "available")
            for key in ("xamlNodes", "compositionNodes"):
                nodes = document[key]
                self.add("trees", phase, key + ".count", len(nodes), zero=True, primary=True)
                by_id = {node["id"]: node for node in nodes}
                depths = {}

                def depth(node_id):
                    trail, seen, cursor = [], set(), node_id
                    while cursor is not None and cursor not in depths:
                        if cursor in seen or cursor not in by_id:
                            raise ValueError("Invalid captured tree")
                        seen.add(cursor)
                        trail.append(cursor)
                        cursor = by_id[cursor]["parentId"]
                    value = depths.get(cursor, -1)
                    for item in reversed(trail):
                        value += 1
                        depths[item] = value
                    return depths[node_id]

                self.add("trees", phase, key + ".maxDepth", max((depth(node["id"]) for node in nodes), default=0))
                types, errors, numeric, categorical = collections.Counter(), collections.Counter(), collections.defaultdict(list), collections.Counter()

                def property_values(node_type, path, value):
                    if isinstance(value, dict):
                        if "error" in value:
                            errors[(node_type, path, str(value["error"]))] += 1
                        else:
                            for name, child in value.items():
                                property_values(node_type, path + "." + name, child)
                    elif isinstance(value, list):
                        for index, child in enumerate(value):
                            property_values(node_type, path + "[" + str(index) + "]", child)
                    elif isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(value):
                        numeric[(node_type, path)].append(value)
                    else:
                        categorical[(node_type, path, json.dumps(value, ensure_ascii=True, sort_keys=True))] += 1

                for node in nodes:
                    types[node["type"]] += 1
                    property_values(node["type"], "properties", node["properties"])
                for name, count in sorted(types.items()):
                    self.add("trees", phase, key + ".type:" + name, count, zero=True)
                for name, values in sorted(numeric.items()):
                    for statistic in ("min", "mean", "max", "count"):
                        self.add("trees", phase, key + "." + "/".join(name) + "." + statistic,
                                 summarize(values)[statistic], "count" if statistic == "count" else "property",
                                 zero=statistic == "count")
                for label, count in sorted(categorical.items()):
                    self.add("trees", phase, key + ".property:" + "/".join(label), count, zero=True)
                for label, count in sorted(errors.items()):
                    self.add("trees", phase, key + ".error:" + "/".join(label), count, zero=True)
            self.add("trees", phase, "correlations", len(document["correlations"]), zero=True, primary=True)
            self.add("trees", phase, "captureElapsed", (document["endQpc"] - document["startQpc"]) * 1000 / document["qpcFrequency"], "ms")
            matched = sum(row["objectId"] is not None for row in snapshot.get("histories", []))
            self.add("objects", phase, "snapshotHistoryMatched", matched, zero=True)
            self.add("objects", phase, "snapshotHistoryUnavailable", len(snapshot.get("histories", [])) - matched, zero=True)
            self.scope("objects", phase, "available")
            resource = document.get("resourceSnapshot", document.get("memorySnapshot"))
            status = "available" if resource and resource["status"] == "available" and resource["values"] is not None else "unavailable"
            resource_available |= status == "available"
            self.scope("resources", phase, status)
            for row in snapshot["resources"]:
                name = (row["resourceName"] or str(row["resourceIndex"]) if row["resourceIndex"] is not None else "allocator")
                value = row["after"] if status == "available" and row["isTracked"] else None
                if value is not None:
                    value = int(value)
                self.add("resources", phase, name + "." + row["metric"], value,
                         "bytes" if row["isBytes"] else "count", status,
                         primary=row["resourceIndex"] is None)
        for phase in ("before-scenario", "after-scenario"):
            if phase not in phases:
                self.scope("trees", phase, "unavailable")
                self.scope("resources", phase, "unavailable")
                for key in ("xamlNodes", "compositionNodes"):
                    self.add("trees", phase, key + ".count", None, primary=True)
        self.capability("resources", "available" if resource_available else "unavailable",
                        "Canonical SnapshotResourceComparison rows. HRESULT/status and untracked/all-zero "
                        "slots stay explicit. AllocatedMemory is a GC heuristic, not process heap. "
                        "Creation-only resource slots are not live counts.")
        for snapshot in snapshots:
            for path in snapshot.get("diffPaths", []):
                diff = load_json(path)
                scope = "within-run:" + snapshot["phase"]
                self.scope("trees", scope, "available")
                counts = collections.Counter(change for node in diff["nodes"] for change in node["changes"])
                for change in ("Added", "Removed", "Properties", "Moved", "Reordered"):
                    self.add("trees", scope, diff["tree"] + "." + change, counts[change], zero=True)
                self.details.setdefault("treeDiffs", []).append({"path": path, "tree": diff["tree"],
                                                               "counts": dict(counts)})
        if "after-scenario" not in phases:
            self.capabilities["trees"]["status"] = "partial" if snapshots else "unavailable"
            self.capabilities["trees"]["notes"] += " No retained end-of-scenario tree; no structural effect of the scenario is claimed."

    def launch(self):
        if "LaunchSummary" not in self.tables:
            return
        rows = [dict(row) for row in self.db.execute("SELECT * FROM LaunchSummary")]
        self.details["launchDrawnTicks"] = rows
        self.details["launch"] = self.engine.get("launch")
        self.capability("launch", "available" if rows else "unavailable",
                        "First drawn XAML ticks and profiler milestone definitions; neither proves "
                        "presentation or user responsiveness. Missing milestones remain unavailable.")
        self.scope("launch", "launch", self.capabilities["launch"]["status"])
        for row in rows:
            self.add("launch", "launch", "drawnTick:" + str(row["Position"]) + "." + row["MeasuredFrom"], row["Ms"], "ms",
                     direction="lower", primary=row["Position"] == 1)
        launch = self.engine.get("launch", {})
        definitions = {}
        for row in launch.get("Milestones", []):
            definitions[row["Label"]] = row["Definition"]
            self.add("launch", "launch", "milestone:" + row["Label"], row["Milliseconds"], "ms",
                     direction="lower", primary=row["Label"] in ("Window shown", "First frame drawn"))
            self.add("launch", "launch", "milestoneCount:" + row["Label"], row["Count"], zero=True)
        for row in launch.get("NotReached", []):
            definitions[row["Label"]] = row
            self.add("launch", "launch", "milestone:" + row["Label"], None, "ms")
        self.details["launchDefinitions"] = definitions

    def run(self):
        for action in (self.timing, self.events_and_spans, self.regions, self.native, self.cpu_stacks,
                       self.graphics, self.texture_checkpoints, self.object_activity, self.snapshots, self.launch):
            action()
        return {
            "schema": "ui-profiler/analysis/v1", "interpretation": INTERPRETATION,
            "inputs": {**{key: self.measurement[key] for key in
                         ("scenarioSha256", "profilerSha256", "frameworkSha256", "sourceSha256", "executableSha256")},
                       "captureSchema": self.measurement["schema"]},
            "session": self.session, "window": self.actions["window"], "windows": self.windows,
            "steps": [{key: step.get(key) for key in ("id", "action", "from", "to")} for step in self.actions["steps"]],
            "capabilities": self.capabilities, "scopeAvailability": self.scopes,
            "metrics": self.metrics, "details": self.details,
            **({"nativeRuntime": self.measurement["nativeRuntime"]} if self.measurement.get("nativeRuntime") else {}),
        }


def validate_native_pair(before, after):
    runtimes = [report.get("nativeRuntime") for report in (before, after)]
    if not any(runtimes):
        return False
    if not all(isinstance(runtime, dict) for runtime in runtimes):
        raise ValueError("Both native measurements must retain private-runtime provenance")
    for name in ("trialId", "trialDirectory", "level"):
        if not runtimes[0].get(name) or runtimes[0][name] != runtimes[1].get(name):
            raise ValueError("Different independent native trial: " + name)
    if runtimes[0]["level"] not in ("control", "framework"):
        raise ValueError("Invalid native trial level")
    for name in ("sourceSha256", "executableSha256"):
        if not before["inputs"].get(name) or before["inputs"][name] != after["inputs"].get(name):
            raise ValueError("A native trial cannot also change the application: " + name)
    for report, runtime in zip((before, after), runtimes):
        verify_references([runtime["manifest"]])
        manifest = load_json(runtime["manifest"]["path"])
        if (manifest["schema"] != "ui-profiler/native-runtime/v1" or manifest["level"] != runtime["level"]
                or manifest["source"]["sha256"] != runtime["sourceSha256"]):
            raise ValueError("Native source/runtime provenance mismatch")
        verify_references([{"path": item["source"], "sha256": item["sha256"]} for item in manifest["files"]])
        framework = [item for item in manifest["files"]
                     if pathlib.PureWindowsPath(item["path"]).name.casefold() == "microsoft.ui.xaml.dll"]
        if len(framework) != 1 or framework[0]["sha256"] != report["inputs"]["frameworkSha256"]:
            raise ValueError("Measured framework differs from the built native runtime")
        for stage in ("loadedBefore", "loadedAfter"):
            loaded = runtime.get(stage, [])
            verify_references(loaded)
            main = [item for item in loaded
                    if pathlib.PureWindowsPath(item["path"]).name.casefold() == "microsoft.ui.xaml.dll"]
            if len(main) != 1 or main[0]["sha256"] != framework[0]["sha256"]:
                raise ValueError("Private runtime was not verified before and after the scenario")
    return True


def comparison_rows(before, after):
    native_pair = validate_native_pair(before, after)
    for name in ("scenarioSha256", "profilerSha256", "frameworkSha256", "captureSchema"):
        if name == "frameworkSha256" and native_pair:
            continue
        if before["inputs"].get(name) != after["inputs"].get(name):
            raise ValueError("Incomparable inputs: " + name)
    for name in ("dpi", "width", "height"):
        if before["window"][name] != after["window"][name]:
            raise ValueError("Incomparable window conditions: " + name)
    if before["steps"] != after["steps"]:
        raise ValueError("Different actions or expansion transitions")
    key = lambda row: (row["family"], row["scope"], row["metric"])
    left, right = {key(row): row for row in before["metrics"]}, {key(row): row for row in after["metrics"]}
    if len(left) != len(before["metrics"]) or len(right) != len(after["metrics"]):
        raise ValueError("Duplicate metric identities")
    result = []
    for identity in sorted(left.keys() | right.keys()):
        a, b = left.get(identity), right.get(identity)
        template = a or b

        def side(row, report):
            if row is not None:
                return row["value"], row["status"]
            availability = report["scopeAvailability"].get(identity[0] + "/" + identity[1], "unavailable")
            if template["zeroWhenAbsent"] and availability in ("available", "partial"):
                return 0, availability
            return None, "unavailable"

        av, ast = side(a, before)
        bv, bst = side(b, after)
        allowed = av is not None and bv is not None and "not_comparable" not in (ast, bst)
        if a and b and a["unit"] != b["unit"]:
            allowed = False
        if identity[0] == "launch" and identity[2].startswith("milestone:"):
            label = identity[2].split(":", 1)[1]
            if before["details"]["launchDefinitions"].get(label) != after["details"]["launchDefinitions"].get(label):
                allowed = False
        delta = bv - av if allowed else None
        reduction = float(100 * (decimal.Decimal(str(av)) - decimal.Decimal(str(bv))) / decimal.Decimal(str(av))) if allowed and av != 0 else None
        status = "available" if allowed and ast == bst == "available" else "partial" if allowed else "not_comparable"
        change = "unavailable" if not allowed else "unchanged" if delta == 0 else "increase" if delta > 0 else "decrease"
        if allowed and status == "available" and template["direction"] == "lower" and delta != 0:
            change = "regression" if delta > 0 else "observed-improvement"
        result.append({
            "family": identity[0], "scope": identity[1], "metric": identity[2], "unit": template["unit"],
            "baseline": av, "candidate": bv, "baselineStatus": ast, "candidateStatus": bst,
            "status": status, "delta": delta, "reductionPercent": reduction,
            "change": change, "direction": template["direction"], "primary": template["primary"],
        })
    return result


def display(value):
    if value is None:
        return "unavailable"
    return str(value) if isinstance(value, int) else f"{value:.4f}"


def markdown(report, comparison=False):
    lines = ["# Multidimensional profiler " + ("comparison" if comparison else "analysis"), "", INTERPRETATION, "",
             "| Evidence family | Baseline | Candidate |" if comparison else "| Evidence family | Availability |",
             "|---|---|---|" if comparison else "|---|---|"]
    for family, item in report["capabilities"].items():
        lines.append("| " + FAMILIES[family] + " | " + (
            item["baseline"] + " | " + item["candidate"] if comparison else item["status"]) + " |")
    lines += ["", "## Scenario measurements", "",
              "| Family / scope / metric | Unit | Baseline | Candidate | Delta | Status / change |" if comparison else
              "| Family / scope / metric | Unit | Value | Status |",
              "|---|---|---:|---:|---:|---|" if comparison else "|---|---|---:|---|"]
    for row in report["metrics"]:
        if not row["primary"]:
            continue
        label = (row["family"] + " / " + row["scope"] + " / " + row["metric"]).replace("|", "\\|")
        if comparison:
            lines.append(f"| {label} | {row['unit']} | {display(row['baseline'])} | {display(row['candidate'])} | "
                         f"{display(row['delta'])} | {row['status']} / {row['change']} |")
        else:
            lines.append(f"| {label} | {row['unit']} | {display(row['value'])} | {row['status']} |")
    lines += ["", "## Interpretation and drilldown", "",
              "The JSON and CSV contain every comparison row, not just this primary-metric table. "
              "The per-side analysis JSON retains all event-span, object, graphics, CPU, GPU, snapshot and texture drilldowns. "
              "Original SQLite, native NDJSON, snapshots and query exports are linked and hashed by manifest.json."]
    for family, item in report["capabilities"].items():
        notes = item["notes"] if not comparison else "Baseline: " + item["baselineNotes"] + " Candidate: " + item["candidateNotes"]
        lines += ["", "**" + FAMILIES[family] + ":** " + notes]
    if report.get("legacyEvidence"):
        lines += ["", "**Legacy evidence:** Secondary files were first sealed at reanalysis time, not at the original approval. "
                  "The original database, action and measurement hashes were verified; the approved run was not modified."]
    lines += ["", "## Evidence files", ""]
    if comparison:
        for side in ("baseline", "candidate"):
            database = report[side + "Database"]
            lines.append(f"{side.capitalize()} SQLite: `{database['path']}` (SHA-256 `{database['sha256']}`).")
    else:
        lines.append(f"SQLite: `{report['database']['path']}` (SHA-256 `{report['database']['sha256']}`).")
    return "\n".join(lines) + "\n"


def analyze(measurement_path, engine_path, directory):
    measurement_path, engine_path, directory = map(pathlib.Path, (measurement_path, engine_path, directory))
    measurement = load_json(measurement_path)
    if measurement["schema"] not in ("ui-profiler/measurement/v1", "ui-profiler/measurement/v2") or measurement["state"] != "complete":
        raise ValueError("A completed versioned measurement is required")
    inputs = [reference(measurement_path, "recorded"), reference(__file__, "analyzer")]
    for name in ("database", "actions"):
        verify_references([measurement[name]])
        inputs.append(reference(measurement[name]["path"], "recorded"))
    evidence = measurement.get("evidence", [])
    verify_references(evidence)
    if measurement["schema"].endswith("/v2") and not evidence:
        raise ValueError("Version 2 measurements require a secondary-evidence manifest")
    recorded = {str(pathlib.Path(item["path"]).resolve()): item for item in evidence}
    for path in measurement_path.parent.iterdir():
        if path.is_file() and path != measurement_path and (path.suffix == ".json" or path.name == "live.ndjson"):
            inputs.append(reference(path, "recorded" if str(path.resolve()) in recorded else "analysis-time"))
    for path in engine_path.parent.iterdir():
        if path.is_file():
            inputs.append(reference(path, "derived-query"))
    engine = load_json(engine_path, exact=True)
    if engine["measurementSha256"] != sha256(measurement_path):
        raise ValueError("Engine export belongs to another measurement")
    verify_references(engine["tools"])
    captures = load_json(engine["textureCapturesPath"], exact=True)
    for checkpoint in engine["textureCheckpoints"]:
        checkpoint["capture"] = captures[checkpoint.pop("captureIndex")]
    engine["launch"] = load_json(engine["launchPath"])
    for snapshot in engine["snapshots"]:
        verify_references([snapshot])
        snapshot["resources"] = load_json(snapshot["resourcesPath"], exact=True)
    stream_path = measurement_path.parent / "live.ndjson"
    if stream_path.exists():
        streamed = [load for line in stream_path.read_text(encoding="utf-8-sig").splitlines()
                    if (load := json.loads(line))["type"] == "metrics"]
        if [packet["data"] for packet in streamed] != measurement.get("sampledMetrics", []):
            raise ValueError("Native metric evidence disagrees with the retained NDJSON")
    inputs += engine["tools"]
    actions = load_json(measurement["actions"]["path"])
    connection = sqlite3.connect(pathlib.Path(measurement["database"]["path"]).resolve().as_uri() + "?mode=ro", uri=True)
    connection.row_factory = sqlite3.Row
    try:
        connection.execute("PRAGMA query_only=ON")
        connection.execute("BEGIN")
        if connection.execute("PRAGMA user_version").fetchone()[0] != 1:
            raise ValueError("Unsupported profiler SQLite schema")
        session = dict(connection.execute("SELECT * FROM SessionInfo").fetchone())
        if (session["LostByEtw"] != 0 or session["DroppedByProfiler"] != 0 or
                session["CompleteFromSeq"] is not None or session["CompleteFromQpc"] is not None or
                session["EndedUtc"] is None):
            raise ValueError("Lost, dropped, trimmed or unfinished event recording")
        result = Analysis(measurement, actions, connection, engine).run()
    finally:
        connection.close()
    result.update(measurement=reference(measurement_path, "recorded"), database=measurement["database"],
                  actions=measurement["actions"], legacyEvidence=not evidence)
    directory.mkdir(parents=True, exist_ok=True)
    json_path, md_path = directory / "analysis.json", directory / "analysis.md"
    write_json(json_path, result)
    with md_path.open("x", encoding="utf-8", newline="\n") as file:
        file.write(markdown(result))
    seal(directory, inputs, [json_path, md_path])
    return json_path


def compare(before_path, after_path, directory):
    before_path, after_path, directory = map(pathlib.Path, (before_path, after_path, directory))
    verify_manifest(before_path.parent / "manifest.json")
    verify_manifest(after_path.parent / "manifest.json")
    before, after = load_json(before_path), load_json(after_path)
    rows = comparison_rows(before, after)
    result = {
        "schema": "ui-profiler/comparison/v2", "interpretation": INTERPRETATION,
        "baseline": before["measurement"], "candidate": after["measurement"],
        "baselineDatabase": before["database"], "candidateDatabase": after["database"],
        "legacyEvidence": before["legacyEvidence"] or after["legacyEvidence"],
        "capabilities": {
            family: {"baseline": before["capabilities"][family]["status"],
                     "candidate": after["capabilities"][family]["status"],
                     "baselineNotes": before["capabilities"][family]["notes"],
                     "candidateNotes": after["capabilities"][family]["notes"]}
            for family in FAMILIES
        },
        "metrics": rows,
        "counts": dict(collections.Counter(row["change"] for row in rows)),
        "baselineAnalysis": str(before_path), "candidateAnalysis": str(after_path),
    }
    native_inputs = []
    if before.get("nativeRuntime"):
        result["nativeRuntime"] = {"baseline": before["nativeRuntime"], "candidate": after["nativeRuntime"]}
        result["interpretation"] += " Native runtime changes are intentional; application source and executable are unchanged."
        for report in (before, after):
            runtime = report["nativeRuntime"]
            native_inputs.append(reference(runtime["manifest"]["path"], "native-runtime"))
            manifest = load_json(runtime["manifest"]["path"])
            native_inputs.extend(reference(item["source"], "native-runtime") for item in manifest["files"])
            native_inputs.extend(reference(item["path"], "loaded-native-runtime")
                                 for stage in ("loadedBefore", "loadedAfter") for item in runtime[stage])
    directory.mkdir(parents=True, exist_ok=True)
    json_path, md_path, csv_path = (directory / name for name in ("comparison.json", "comparison.md", "comparison.csv"))
    write_json(json_path, result)
    with md_path.open("x", encoding="utf-8", newline="\n") as file:
        file.write(markdown(result, True))
    with csv_path.open("x", encoding="utf-8", newline="") as file:
        writer = csv.DictWriter(file, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    seal(directory, [reference(before_path.parent / "manifest.json", "evidence-manifest"),
                     reference(after_path.parent / "manifest.json", "evidence-manifest"),
                     reference(before_path), reference(after_path), reference(__file__, "analyzer")] + native_inputs,
         [json_path, md_path, csv_path])
    return json_path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    analysis = subparsers.add_parser("analyze")
    analysis.add_argument("--measurement", required=True)
    analysis.add_argument("--engine", required=True)
    analysis.add_argument("--directory", required=True)
    comparison = subparsers.add_parser("compare")
    comparison.add_argument("--baseline", required=True)
    comparison.add_argument("--candidate", required=True)
    comparison.add_argument("--directory", required=True)
    verify = subparsers.add_parser("verify")
    verify.add_argument("--manifest", required=True)
    args = parser.parse_args()
    if args.command == "analyze":
        print(analyze(args.measurement, args.engine, args.directory))
    elif args.command == "compare":
        print(compare(args.baseline, args.candidate, args.directory))
    else:
        verify_manifest(args.manifest)
        print("Evidence manifest verified; no capture.")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, sqlite3.Error) as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
