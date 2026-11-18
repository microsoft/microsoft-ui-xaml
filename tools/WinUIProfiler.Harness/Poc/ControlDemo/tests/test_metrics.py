from contextlib import closing
import json
from pathlib import Path
import sqlite3
import tempfile
import unittest

from metrics import MARKER_PROVIDER, XAML_PROVIDER, MeasurementError, extract_iteration


class MetricsTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.database = Path(self.directory.name) / "fixture.sqlite"
        self.app = {
            "passed": True,
            "processId": 123,
            "uiThreadId": 456,
            "controlCount": 40,
            "frameworkPath": "fixture-only.dll",
            "frameworkVersion": "fixture-only",
        }
        with closing(sqlite3.connect(self.database)) as db, db:
            db.executescript(
                """
                PRAGMA user_version=1;
                CREATE TABLE SessionInfo(
                    ProcessId INTEGER, QpcFrequency INTEGER, Source TEXT, EndReason TEXT,
                    LostByEtw INTEGER, DroppedByProfiler INTEGER,
                    CompleteFromSeq INTEGER, CompleteFromQpc INTEGER
                );
                INSERT INTO SessionInfo VALUES(123, 1000000, 'Trace', 'End of trace', 0, 0, NULL, NULL);
                CREATE TABLE EventTypes(TypeId INTEGER, ProviderId TEXT, EventId INTEGER, TraceLogging INTEGER);
                CREATE TABLE Events(Seq INTEGER, Qpc INTEGER, ThreadId INTEGER, TypeId INTEGER, Payload TEXT);
                """
            )
            db.executemany(
                "INSERT INTO EventTypes VALUES(?,?,?,0)",
                [(1, MARKER_PROVIDER, 1), (2, MARKER_PROVIDER, 2),
                 (3, XAML_PROVIDER, 41), (4, XAML_PROVIDER, 42),
                 (5, "unrelated-provider", 41), (6, "unrelated-provider", 42)],
            )
            db.executemany(
                "INSERT INTO Events VALUES(?,?,?,?,?)",
                [
                    (1, 10, 456, 3, "[]"), (2, 20, 456, 4, "[]"),
                    (10, 100, 456, 1, json.dumps([40])),
                    (20, 200, 456, 3, "[]"),
                    (21, 210, 999, 3, "[]"), (22, 220, 456, 5, "[]"),
                    (28, 480, 456, 6, "[]"), (29, 490, 999, 4, "[]"),
                    (30, 500, 456, 4, "[]"),
                    (40, 900, 456, 2, json.dumps([40, True])),
                ],
            )

    def change(self, statement):
        with closing(sqlite3.connect(self.database)) as db, db:
            db.execute(statement)

    def test_scopes_to_process_ui_thread_provider_and_markers(self):
        result = extract_iteration(self.database, self.app, 40)
        self.assertEqual(result["layoutPassCount"], 1)
        self.assertEqual(result["scenarioMs"], 0.8)
        self.assertEqual(result["layoutBusyMs"], 0.3)

    def test_counts_nested_passes_without_double_counting_busy_time(self):
        self.change("INSERT INTO Events VALUES(24,240,456,3,'[]')")
        self.change("INSERT INTO Events VALUES(26,260,456,4,'[]')")
        result = extract_iteration(self.database, self.app, 40)
        self.assertEqual(result["layoutPassCount"], 2)
        self.assertEqual(result["layoutBusyMs"], 0.3)

    def test_rejects_missing_or_ambiguous_markers(self):
        for statement in (
            "DELETE FROM Events WHERE Seq=40",
            "INSERT INTO Events VALUES(50,1000,456,2,'[40,true]')",
            "UPDATE Events SET ThreadId=999 WHERE Seq=40",
        ):
            with self.subTest(statement=statement):
                with closing(sqlite3.connect(self.database)) as db, db:
                    db.execute(statement)
                try:
                    with self.assertRaises(MeasurementError):
                        extract_iteration(self.database, self.app, 40)
                finally:
                    with closing(sqlite3.connect(self.database)) as db, db:
                        db.execute("DELETE FROM Events WHERE Seq IN (40,50)")
                        db.execute("INSERT INTO Events VALUES(40,900,456,2,'[40,true]')")

    def test_rejects_lost_trimmed_or_wrong_process_sessions(self):
        for statement in (
            "UPDATE SessionInfo SET LostByEtw=1",
            "UPDATE SessionInfo SET DroppedByProfiler=1",
            "UPDATE SessionInfo SET CompleteFromSeq=1",
            "UPDATE SessionInfo SET ProcessId=999",
        ):
            with self.subTest(statement=statement):
                self.change(statement)
                with self.assertRaises(MeasurementError):
                    extract_iteration(self.database, self.app, 40)
                self.change(
                    "UPDATE SessionInfo SET ProcessId=123,LostByEtw=0,"
                    "DroppedByProfiler=0,CompleteFromSeq=NULL"
                )

    def test_rejects_missing_or_unbalanced_layout_instead_of_reporting_zero(self):
        self.change("DELETE FROM Events WHERE Seq=30")
        with self.assertRaises(MeasurementError):
            extract_iteration(self.database, self.app, 40)
        self.change("DELETE FROM Events WHERE Seq=20")
        with self.assertRaises(MeasurementError):
            extract_iteration(self.database, self.app, 40)

    def test_rejects_incorrect_control_tree(self):
        self.app["controlCount"] = 39
        with self.assertRaises(MeasurementError):
            extract_iteration(self.database, self.app, 40)


if __name__ == "__main__":
    unittest.main()
