"""SQLite-only GUI capture contracts; no GUI, app, native sampler or capture is started."""
import importlib.util
import pathlib
import sqlite3
import tempfile
import unittest
import uuid
from contextlib import closing

path = pathlib.Path(__file__).parents[1] / "pack" / "tools" / "Read-GuiRecording.py"
spec = importlib.util.spec_from_file_location("gui_recording", path)
gui = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gui)


class GuiRecordingTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.path = pathlib.Path(self.temporary.name) / "gui.sqlite"
        self.instance = str(uuid.uuid4())
        with closing(sqlite3.connect(self.path)) as connection, connection:
            connection.executescript("""
                CREATE TABLE SessionInfo(ProcessId INTEGER, EndedUtc TEXT, LostByEtw INTEGER,
                    DroppedByProfiler INTEGER, CompleteFromSeq INTEGER, QpcFrequency INTEGER);
                INSERT INTO SessionInfo VALUES(123,'finalized',0,0,NULL,10000000);
                CREATE TABLE ProcessMetricRetention(CompleteFromQpc INTEGER);
                INSERT INTO ProcessMetricRetention VALUES(NULL);
                CREATE TABLE ProcessMetrics(AcquisitionId INTEGER, Qpc INTEGER, ProcessId INTEGER,
                    CreationTimeFileTime INTEGER, InstanceId TEXT, Metric TEXT, Value REAL,
                    Unit TEXT, Source TEXT, UnavailableReason TEXT);
            """)
            for name, unit in gui.METRICS.items():
                connection.execute("INSERT INTO ProcessMetrics VALUES(1,100,123,456,?,?,?,?,?,?)",
                                   (self.instance, name, None if name == "gpu" else 0.0, unit,
                                    "native source", "warming up" if name == "gpu" else None))

    def read(self):
        return gui.readings(self.path, 123, 456, self.instance)

    def mutate(self, sql, values=()):
        with closing(sqlite3.connect(self.path)) as connection, connection:
            connection.execute(sql, values)

    def test_persisted_zero_missing_reason_clock_and_provenance(self):
        result = self.read()
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]["timestampQpc"], 100)
        self.assertEqual(result[0]["process"]["creationTimeFileTime"], 456)
        values = {row["metric"]: row for row in result[0]["values"]}
        self.assertEqual(values["privateCommit"]["sample"]["value"], 0)
        self.assertEqual(values["privateCommit"]["unit"], "MiB")
        self.assertIsNone(values["gpu"]["sample"]["value"])
        self.assertEqual(values["gpu"]["sample"]["unavailableReason"], "warming up")
        self.assertEqual(values["cpu"]["sample"]["source"], "native source")

    def test_rejects_mixed_pid_creation_time_or_instance(self):
        for column, value, original in [("ProcessId", 124, 123), ("CreationTimeFileTime", 457, 456),
                                        ("InstanceId", str(uuid.uuid4()), self.instance)]:
            with self.subTest(column=column):
                self.mutate(f"UPDATE ProcessMetrics SET {column}=? WHERE Metric='cpu'", (value,))
                with self.assertRaises(ValueError):
                    self.read()
                self.mutate(f"UPDATE ProcessMetrics SET {column}=? WHERE Metric='cpu'", (original,))

    def test_rejects_unfinished_lost_trimmed_or_missing_quality(self):
        for column, value, original in [("EndedUtc", None, "finalized"), ("LostByEtw", 1, 0),
                                        ("DroppedByProfiler", None, 0), ("CompleteFromSeq", 1, None)]:
            with self.subTest(column=column):
                self.mutate(f"UPDATE SessionInfo SET {column}=?", (value,))
                with self.assertRaises(ValueError):
                    self.read()
                self.mutate(f"UPDATE SessionInfo SET {column}=?", (original,))

    def test_rejects_incomplete_acquisition_or_missing_sqlite_metrics(self):
        self.mutate("DELETE FROM ProcessMetrics WHERE Metric='cpu'")
        with self.assertRaisesRegex(ValueError, "missing a metric"):
            self.read()
        self.mutate("DROP TABLE ProcessMetrics")
        with self.assertRaisesRegex(ValueError, "chart values are not evidence"):
            self.read()

    def test_rejects_wrong_units_and_unexplained_null(self):
        self.mutate("UPDATE ProcessMetrics SET Unit='bytes' WHERE Metric='privateCommit'")
        with self.assertRaises(ValueError):
            self.read()
        self.mutate("UPDATE ProcessMetrics SET Unit='MiB' WHERE Metric='privateCommit'")
        self.mutate("UPDATE ProcessMetrics SET UnavailableReason=NULL WHERE Metric='gpu'")
        with self.assertRaises(ValueError):
            self.read()

    def test_rejects_trimmed_metric_timeline(self):
        self.mutate("UPDATE ProcessMetricRetention SET CompleteFromQpc=100")
        with self.assertRaisesRegex(ValueError, "gauge history was trimmed"):
            self.read()


if __name__ == "__main__":
    unittest.main(verbosity=2)
