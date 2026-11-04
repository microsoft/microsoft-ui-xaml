import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import find_duplicates as fd


BODY = """### Describe the bug
The Expander flashes the first time its content expands.

### Steps to reproduce the bug
1. Add an Expander containing a TextBlock.
2. Launch the app and click its chevron.

### Actual behavior
The content flashes.

### Expected behavior
The content animates smoothly.

### NuGet package version
1.8.260317003

### Windows version
Windows 11 (24H2): Build 26100
"""


def issue(number=100, title="Expander content flashes during first expansion", body=BODY):
    return fd.Issue(number, title, body)


def candidate(number=1, **changes):
    result = {
        "number": number,
        "title": issue().title,
        "body": BODY,
        "state": "open",
        "labels": [{"name": "bug"}, {"name": "area-Expander"}],
    }
    result.update(changes)
    return result


class NormalizationTests(unittest.TestCase):
    def test_hidden_comments_code_links_and_controls_are_not_retrieval_signal(self):
        text = "visible <!-- secret --> ```ignored code``` [docs](https://example.com) \x00"
        result = fd.normalize_text(text)
        for excluded in ("secret", "ignored", "example.com", "\x00"):
            self.assertNotIn(excluded, result)
        self.assertIn("visible", result)
        self.assertIn("docs", result)

    def test_unterminated_hidden_comment_is_removed(self):
        self.assertEqual(fd.normalize_text("visible <!-- hide the rest"), "visible  ")

    def test_metadata_only_form_has_no_defect_signal(self):
        self.assertEqual(fd.signal_body("### Windows version\nWindows 11"), "")

    def test_versions_do_not_contribute_to_body_similarity(self):
        signal = fd.signal_body(BODY)
        self.assertIn("flashes", signal)
        self.assertNotIn("26100", signal)
        self.assertNotIn("260317003", signal)

    def test_versions_are_separately_available_for_regression_judgment(self):
        fields = fd.version_fields(BODY)
        self.assertEqual(fields["nuget package version"], "1.8.260317003")
        self.assertIn("26100", fields["windows version"])

    def test_plain_reports_remain_searchable(self):
        self.assertEqual(fd.signal_body("An Expander flashes."), "An Expander flashes.")

    def test_placeholders_and_noise(self):
        self.assertEqual(fd.tokenize("WinUI bug on Windows"), set())
        self.assertNotIn("response", fd.tokenize("_No response_"))

    def test_camel_case_and_identifiers(self):
        self.assertIn("navigationview", fd.tokenize("NavigationView"))
        self.assertIn("navigation", fd.tokenize("NavigationView"))
        identifiers = fd.extract_identifiers("TabView COMException 0x80070057 `Expander`")
        self.assertTrue({"TabView", "COMException", "0x80070057", "Expander"} <= identifiers)
        self.assertNotIn("Windows", fd.extract_identifiers("Windows"))

    def test_hidden_code_span_does_not_become_identifier(self):
        self.assertNotIn("FakeControl", fd.extract_identifiers("<!-- `FakeControl` -->"))

    def test_tracking_title_prefix_is_not_similarity_signal(self):
        title = "[WinUI OSS] Phase 4: Update metadata factory layer"
        self.assertEqual(fd.strip_title_prefix(title), "Update metadata factory layer")
        self.assertEqual(fd.strip_title_prefix("[WinUI OSS] Crash"), "[WinUI OSS] Crash")


class RetrievalTests(unittest.TestCase):
    def test_identical_reports_rank_above_unrelated_reports(self):
        same_score, _ = fd.similarity(issue(), issue(1))
        other_score, _ = fd.similarity(issue(), issue(2, "Please add a Ribbon", "A new control"))
        self.assertGreater(same_score, .8)
        self.assertLess(other_score, .62)
        self.assertLessEqual(same_score, 1)

    def test_query_budget_and_identifiers(self):
        queries = fd.build_queries(issue(title="NavigationView selection lost after Frame navigation"))
        self.assertLessEqual(len(queries), fd.MAX_QUERIES)
        self.assertEqual(len(queries), len(set(queries)))
        self.assertTrue(any("NavigationView" in query for query in queries))

    def test_search_operators_cannot_escape_the_fixed_query(self):
        queries = fd.build_queries(issue(title='Crash" repo:attacker/other OR is:pr in:body'))
        for query in queries:
            self.assertNotIn("repo:", query)
            self.assertNotIn("is:pr", query)
            self.assertNotIn("in:body", query)
            self.assertEqual(query.count('"') % 2, 0)

    def test_old_closed_issues_are_candidates_but_not_verdicts(self):
        results = fd.retrieve_candidates(issue(), lambda query: [candidate(state="closed")])
        self.assertEqual(len(results), 1)
        self.assertEqual(results[0]["state"], "closed")
        self.assertIn("versions", results[0])
        self.assertNotIn("confidence", results[0])

    def test_excludes_self_newer_pull_requests_and_tracking_issues(self):
        inputs = [
            candidate(100), candidate(101), candidate(2, pull_request={}),
            candidate(3, title="[WinUI OSS] Phase 4: Refactor infrastructure"),
            candidate(4, labels=[{"name": "WinUI OSS"}]),
            candidate(5),
        ]
        results = fd.retrieve_candidates(issue(), lambda query: inputs)
        self.assertEqual([result["number"] for result in results], [5])

    def test_duplicates_are_deduplicated_bounded_and_relevance_ranked(self):
        inputs = [candidate(n) for n in range(1, 40)]
        inputs += [candidate(99, title="Unrelated Slider issue", body="Unrelated scrolling")]
        results = fd.retrieve_candidates(issue(), lambda query: inputs)
        self.assertEqual(len(results), fd.MAX_CANDIDATES)
        self.assertEqual(len({result["number"] for result in results}), fd.MAX_CANDIDATES)
        self.assertNotIn(99, [result["number"] for result in results])

    def test_large_candidate_body_is_bounded(self):
        results = fd.retrieve_candidates(issue(), lambda query: [candidate(body="example " * 10000)])
        self.assertLessEqual(len(results[0]["body"]), 1600)


if __name__ == "__main__":
    unittest.main()
