"""Unit tests for the deterministic duplicate detector.

Run from the repository root:

    python -m unittest discover -s .github/scripts/issue-triage/tests
"""

from __future__ import annotations

import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

import find_duplicates as fd  # noqa: E402


def make_issue(number, title, body="", state="open"):
    return fd.Issue(number=number, title=title, body=body, state=state)


BUG_BODY = """### Describe the bug

The Expander control flashes when it is expanded for the first time.

### Why is this important?

It looks broken to end users.

### Steps to reproduce the bug

1. Add an Expander to a page
2. Click the chevron

### Actual behavior

The content flashes.

### Expected behavior

The content should animate smoothly.

### NuGet package version

1.8.260317003

### Windows version

Windows 11 (24H2): Build 26100
"""


class NormalizeTests(unittest.TestCase):
    def test_strips_html_comments_and_code_fences(self):
        text = "before <!-- hidden instruction --> after ```ignored code``` end"
        result = fd.normalize_text(text)
        self.assertNotIn("hidden instruction", result)
        self.assertNotIn("ignored code", result)
        self.assertIn("before", result)
        self.assertIn("end", result)

    def test_keeps_link_text_but_drops_urls(self):
        result = fd.normalize_text("see [the docs](https://example.com/page) now")
        self.assertIn("the docs", result)
        self.assertNotIn("example.com", result)

    def test_handles_empty_input(self):
        self.assertEqual(fd.normalize_text(""), "")


class SignalBodyTests(unittest.TestCase):
    def test_drops_version_and_metadata_sections(self):
        result = fd.signal_body(BUG_BODY)
        self.assertIn("flashes", result)
        self.assertNotIn("1.8.260317003", result)
        self.assertNotIn("Build 26100", result)

    def test_freeform_body_is_returned_unchanged(self):
        body = "Just a plain description with no issue form headings."
        self.assertIn("plain description", fd.signal_body(body))


class TokenizeTests(unittest.TestCase):
    def test_splits_camel_case_and_drops_stopwords(self):
        tokens = fd.tokenize("The NavigationView is broken")
        self.assertIn("navigationview", tokens)
        self.assertIn("navigation", tokens)
        self.assertIn("view", tokens)
        self.assertNotIn("the", tokens)
        self.assertNotIn("is", tokens)

    def test_repository_noise_words_are_removed(self):
        self.assertEqual(fd.tokenize("WinUI bug on Windows"), set())


class IdentifierTests(unittest.TestCase):
    def test_extracts_control_names_and_error_codes(self):
        found = fd.extract_identifiers("TabView throws COMException 0x80070057")
        self.assertIn("TabView", found)
        self.assertIn("COMException", found)
        self.assertIn("0x80070057", found)

    def test_ignores_single_word_capitals(self):
        self.assertNotIn("Windows", fd.extract_identifiers("Windows is fine"))

    def test_reads_backticked_control_names(self):
        self.assertIn("Expander", fd.extract_identifiers("the `Expander` flashes"))


class SimilarityTests(unittest.TestCase):
    def test_identical_titles_score_high(self):
        left = make_issue(1, "Expander content flashes during first expansion", BUG_BODY)
        right = make_issue(2, "Expander content flashes during first expansion", BUG_BODY)
        score, _ = fd.similarity(left, right)
        self.assertGreaterEqual(score, fd.HIGH_CONFIDENCE)

    def test_shared_identifiers_are_reported(self):
        body = "### Describe the bug\n\n`NavigationView` throws COMException.\n"
        left = make_issue(1, "NavigationView crash", body)
        right = make_issue(2, "NavigationView crash on load", body)
        _, shared = fd.similarity(left, right)
        self.assertIn("NavigationView", shared)
        self.assertIn("COMException", shared)

    def test_unrelated_issues_score_low(self):
        left = make_issue(1, "Expander content flashes during first expansion", BUG_BODY)
        right = make_issue(2, "Add a Ribbon control to the toolkit", "Please add a Ribbon.")
        score, _ = fd.similarity(left, right)
        self.assertLess(score, fd.DEFAULT_THRESHOLD)

    def test_same_windows_version_alone_is_not_similar(self):
        shared_metadata = "### NuGet package version\n\n1.8.260317003\n\n### Windows version\n\nWindows 11 (24H2): Build 26100\n"
        left = make_issue(1, "ListView scrolling stutters", "### Describe the bug\n\nStutter.\n" + shared_metadata)
        right = make_issue(2, "TeachingTip never dismisses", "### Describe the bug\n\nStays open.\n" + shared_metadata)
        score, _ = fd.similarity(left, right)
        self.assertLess(score, fd.DEFAULT_THRESHOLD)

    def test_score_is_bounded(self):
        issue = make_issue(1, "NavigationView selection lost", BUG_BODY)
        score, _ = fd.similarity(issue, issue)
        self.assertLessEqual(score, 1.0)


class SelectDuplicatesTests(unittest.TestCase):
    def setUp(self):
        self.source = make_issue(100, "Expander content flashes during first expansion", BUG_BODY)

    def test_excludes_the_source_issue(self):
        matches = fd.select_duplicates(self.source, [self.source])
        self.assertEqual(matches, [])

    def test_returns_no_matches_when_nothing_is_similar(self):
        candidates = [make_issue(1, "Add Ribbon control", "unrelated request")]
        self.assertEqual(fd.select_duplicates(self.source, candidates), [])

    def test_ranks_by_descending_score_and_caps_results(self):
        candidates = [
            make_issue(n, "Expander content flashes during first expansion", BUG_BODY)
            for n in range(1, 9)
        ]
        matches = fd.select_duplicates(self.source, candidates)
        self.assertEqual(len(matches), fd.MAX_SUGGESTIONS)
        scores = [match.score for match in matches]
        self.assertEqual(scores, sorted(scores, reverse=True))

    def test_deduplicates_repeated_candidates(self):
        candidate = make_issue(7, "Expander content flashes during first expansion", BUG_BODY)
        matches = fd.select_duplicates(self.source, [candidate, candidate])
        self.assertEqual(len(matches), 1)

    def test_threshold_is_respected(self):
        candidates = [make_issue(1, "Expander flashes on first expansion", BUG_BODY)]
        self.assertEqual(fd.select_duplicates(self.source, candidates, threshold=1.01), [])


class RenderTests(unittest.TestCase):
    def test_comment_contains_marker_and_issue_links(self):
        match = fd.Match(issue=make_issue(42, "Expander flashes"), score=0.91)
        body = fd.render_comment([match])
        self.assertIn(fd.COMMENT_MARKER, body)
        self.assertIn("#42", body)
        self.assertIn("High", body)

    def test_closed_state_is_shown(self):
        match = fd.Match(issue=make_issue(42, "Expander flashes", state="closed"), score=0.75)
        self.assertIn("_(closed)_", fd.render_comment([match]))

    def test_mentions_are_neutralized(self):
        match = fd.Match(issue=make_issue(42, "cc @octocat and #1234"), score=0.75)
        body = fd.render_comment([match])
        self.assertNotIn("@octocat", body)
        self.assertIn("\u200b", body)

    def test_markdown_injection_is_escaped(self):
        match = fd.Match(issue=make_issue(42, "title with `code` and [link](x)"), score=0.75)
        body = fd.render_comment([match])
        self.assertIn("\\`", body)
        self.assertIn("\\[", body)


class ConfidenceTests(unittest.TestCase):
    def test_bands(self):
        issue = make_issue(1, "x")
        self.assertEqual(fd.Match(issue=issue, score=0.95).confidence, "High")
        self.assertEqual(fd.Match(issue=issue, score=0.72).confidence, "Medium")
        self.assertEqual(fd.Match(issue=issue, score=0.65).confidence, "Low")


class QueryTests(unittest.TestCase):
    def test_queries_are_bounded_and_unique(self):
        source = make_issue(1, "NavigationView selection is lost after Frame navigation", BUG_BODY)
        queries = fd.build_queries(source)
        self.assertLessEqual(len(queries), 8)
        self.assertEqual(len(queries), len(set(queries)))
        self.assertTrue(any("NavigationView" in query for query in queries))

    def test_short_title_skips_exact_phrase_query(self):
        queries = fd.build_queries(make_issue(1, "Crash"))
        self.assertFalse(any(query.startswith('in:title "') for query in queries))


if __name__ == "__main__":
    unittest.main()
