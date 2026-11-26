"""Export native process gauges from a finalized GUI SQLite, never from UI chart text."""
import argparse
import json
import math
import sqlite3
import uuid
from contextlib import closing
from pathlib import Path


METRICS = {"cpu": "percent", "gpu": "percent", "workingSet": "MiB",
           "privateWorkingSet": "MiB", "privateCommit": "MiB"}


def readings(database, process_id, creation_time, instance_id):
    uri = Path(database).resolve().as_uri() + "?mode=ro&immutable=1"
    with closing(sqlite3.connect(uri, uri=True)) as connection:
        connection.row_factory = sqlite3.Row
        info = connection.execute("SELECT * FROM SessionInfo").fetchall()
        if len(info) != 1 or info[0]["ProcessId"] != process_id:
            raise ValueError("GUI SQLite belongs to a different process")
        info = info[0]
        if (not info["EndedUtc"] or info["LostByEtw"] != 0 or info["DroppedByProfiler"] != 0
                or info["CompleteFromSeq"] is not None or not info["QpcFrequency"]
                or info["QpcFrequency"] <= 0):
            raise ValueError("GUI SQLite is unfinished, lost, trimmed or lacks its clock")
        if connection.execute("PRAGMA quick_check").fetchone()[0] != "ok":
            raise ValueError("GUI SQLite integrity check failed")
        if not connection.execute(
                "SELECT 1 FROM sqlite_master WHERE type='table' AND name='ProcessMetrics'").fetchone():
            raise ValueError("GUI SQLite did not retain native process gauges; chart values are not evidence")
        if connection.execute(
                "SELECT CompleteFromQpc FROM ProcessMetricRetention WHERE CompleteFromQpc IS NOT NULL").fetchone():
            raise ValueError("GUI SQLite process gauge history was trimmed")
        rows = connection.execute("""
            SELECT AcquisitionId, Qpc, ProcessId, CreationTimeFileTime, InstanceId,
                   Metric, Value, Unit, Source, UnavailableReason
            FROM ProcessMetrics ORDER BY AcquisitionId, Metric LIMIT 25001
        """).fetchall()
        if not rows or len(rows) > 25000:
            raise ValueError("GUI metric acquisition count is empty or outside its bound")
    expected_instance = uuid.UUID(str(instance_id))
    result = []
    current = None
    last_qpc = -1
    for row in rows:
        if (row["ProcessId"] != process_id or row["CreationTimeFileTime"] != creation_time
                or uuid.UUID(row["InstanceId"]) != expected_instance):
            raise ValueError("GUI metric row belongs to another process instance")
        if row["AcquisitionId"] != current:
            if result and {entry["metric"] for entry in result[-1]["values"]} != set(METRICS):
                raise ValueError("GUI acquisition is missing a metric")
            if row["Qpc"] < 0 or row["Qpc"] < last_qpc:
                raise ValueError("GUI acquisitions have unordered timestamps")
            current = row["AcquisitionId"]
            last_qpc = row["Qpc"]
            result.append({"process": {"processId": process_id, "creationTimeFileTime": creation_time,
                                       "instanceId": str(expected_instance)},
                           "timestampQpc": last_qpc, "values": []})
        if row["Qpc"] != last_qpc or row["Metric"] not in METRICS or row["Unit"] != METRICS[row["Metric"]]:
            raise ValueError("GUI metric has an inconsistent clock, name or unit")
        if any(value["metric"] == row["Metric"] for value in result[-1]["values"]):
            raise ValueError("GUI acquisition duplicates a metric")
        value = row["Value"]
        if value is not None and (not math.isfinite(value) or value < 0
                                  or row["Unit"] == "percent" and value > 100):
            raise ValueError("GUI metric is nonfinite or outside its range")
        if not row["Source"] or value is None and not row["UnavailableReason"]:
            raise ValueError("GUI metric is missing provenance or an unavailable reason")
        result[-1]["values"].append({
            "metric": row["Metric"], "unit": row["Unit"],
            "sample": {"timestampQpc": row["Qpc"], "value": value, "source": row["Source"],
                       "unavailableReason": row["UnavailableReason"]},
        })
    if {entry["metric"] for entry in result[-1]["values"]} != set(METRICS):
        raise ValueError("GUI acquisition is missing a metric")
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--database", required=True)
    parser.add_argument("--process-id", required=True, type=int)
    parser.add_argument("--creation-time", required=True, type=int)
    parser.add_argument("--instance-id", required=True)
    args = parser.parse_args()
    for reading in readings(args.database, args.process_id, args.creation_time, args.instance_id):
        print(json.dumps({"type": "metrics", "data": reading}, allow_nan=False), flush=True)
