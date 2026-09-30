import copy
import json
from html.parser import HTMLParser
import unittest

import triage as t
from test_triage import MAPPING, TriageFixture, agent_output


class DetailsParser(HTMLParser):
    def __init__(self):
        super().__init__()
        self.stack = []
        self.details = []
        self.unexpected = []

    def handle_starttag(self, tag, attrs):
        if tag == "details":
            self.stack.append(tag)
            self.details.append(dict(attrs))
        elif tag not in {"summary"}:
            self.unexpected.append(tag)

    def handle_endtag(self, tag):
        if tag == "details":
            if not self.stack:
                raise AssertionError("Unbalanced details closing tag")
            self.stack.pop()


class DecoratedCommentTests(TriageFixture):
    def test_marker_remains_first_and_layout_uses_audience_sections(self):
        body = t.render(self.result(), self.evidence, MAPPING)
        self.assertTrue(body.startswith(t.MARKER + "\n"))
        self.assertIn("## \U0001f9ed Triage summary", body)
        self.assertIn("### \U0001f6e0\ufe0f For the WinUI team", body)
        self.assertIn("**\U0001f9e9 `area-Expander`**", body)
        self.assertIn("**\U0001f465 `team-Controls`**", body)
        self.assertIn("**\U0001f41e Bug**", body)
        self.assertIn("WinUI maintainers make final decisions", body)
        self.assertIn("never closes issues automatically", body)
        self.assertNotIn("For the issue author", body)

    def test_author_request_is_visible_before_team_investigation(self):
        result = self.result(
            reproduction="INSUFFICIENT",
            missing_information="Please provide concrete steps and a minimal reproduction.",
        )
        body = t.render(result, self.evidence, MAPPING)
        self.assertIn("### \U0001f64b For the issue author", body)
        self.assertIn("- **Needed:** Please provide concrete steps", body)
        self.assertLess(body.index("For the issue author"), body.index("For the WinUI team"))
        self.assertLess(body.index("**Needed:**"), body.index("<details"))
        self.assertIn("\u26a0\ufe0f Needs more detail", body)
        self.assertNotIn("@reporter", body)

    def test_environment_request_does_not_claim_reproduction_is_missing(self):
        result = self.result(
            missing_information="Please provide the Windows App SDK version.",
            missing_information_kind="ENVIRONMENT",
        )
        body = t.render(result, self.evidence, MAPPING)
        self.assertIn("**Needed:** Please provide", body)
        self.assertIn("\u2705 Sufficient detail for initial investigation", body)
        self.assertNotIn("Needs more detail", body)

    def test_sufficient_details_do_not_claim_the_bug_was_reproduced(self):
        body = t.render(self.result(), self.evidence, MAPPING)
        self.assertIn("Sufficient detail for initial investigation", body)
        self.assertNotIn("Successfully reproduced", body)
        self.assertIn("<details>\n<summary>\U0001f9ea Investigation details</summary>\n\n", body)

    def test_feature_and_question_badges_are_distinct(self):
        for kind, badge in (("FEATURE", "\u2728 Feature proposal"), ("OTHER", "\U0001f4ac Question / other")):
            with self.subTest(kind=kind):
                evidence = copy.deepcopy(self.evidence)
                evidence["issue_kind"] = kind
                result = t.validate(
                    agent_output(evidence, issue_kind=kind, reproduction="NOT_APPLICABLE"),
                    evidence,
                )
                body = t.render(result, evidence, MAPPING)
                self.assertIn(badge, body)
                self.assertIn("\u2139\ufe0f Not applicable", body)
                self.assertNotIn("For the issue author", body)

    def test_preserved_routing_drives_the_metadata_not_an_ignored_suggestion(self):
        self.evidence["existing_labels"] += ["area-Binding", "team-Core"]
        body = t.render(self.result(), self.evidence, MAPPING)
        metadata = body.split("<details", 1)[0]
        self.assertIn("`area-Binding`", metadata)
        self.assertIn("`team-Core`", metadata)
        self.assertNotIn("`team-Controls`", metadata)
        self.assertIn("Existing area labels take precedence", body)
        self.assertNotIn("maintainer routing is needed", body)

    def test_reported_package_version_is_shown_without_inventing_missing_metadata(self):
        body = t.render(self.result(), self.evidence, MAPPING)
        self.assertIn("\U0001f4e6 Package: 1.8.260317003", body)
        self.assertIn("Package version (reported)", body)
        for text in ("", "### NuGet package version\n\n_No response_\n"):
            evidence = {**self.evidence, "body": text}
            body = t.render(self.result(), evidence, MAPPING)
            self.assertNotIn("\U0001f4e6", body)
            self.assertNotIn("Package version (reported)", body)

    def test_long_package_metadata_keeps_the_badge_compact(self):
        evidence = {**self.evidence, "body": "### NuGet package version\n\n" + "v" * 150}
        body = t.render(self.result(), evidence, MAPPING)
        metadata = body.split("<details", 1)[0]
        self.assertIn("v" * 80 + "...", metadata)
        self.assertNotIn("v" * 81, metadata)
        self.assertIn("Package version (reported):** " + "v" * 150, body)

    def test_review_and_truncation_warnings_are_expanded(self):
        result = self.result(reproduction="INSUFFICIENT", missing_information="None")
        body = t.render(result, self.evidence, MAPPING)
        self.assertIn("<details open>", body)
        self.assertIn("Assessment needs review", body)
        self.assertIn("Needs maintainer review", body)
        self.assertNotIn("For the issue author", body)
        self.evidence["context_truncated"] = True
        body = t.render(self.result(), self.evidence, MAPPING)
        self.assertIn("<details open>", body)
        self.assertIn("Bounded excerpts were used", body)

    def test_duplicate_details_keep_state_confidence_and_human_confirmation(self):
        self.evidence["candidates"][0]["state"] = "closed"
        result = self.result(duplicate_candidates_json=json.dumps([
            {"number": 1, "reason": "The same first-expansion flash is described.", "confidence": "HIGH"}
        ]))
        body = t.render(result, self.evidence, MAPPING)
        self.assertIn("<summary>\U0001f50e Possible duplicates (1)</summary>", body)
        self.assertIn("#1 (closed)", body)
        self.assertIn("**Confidence: high.**", body)
        self.assertIn("A maintainer confirms duplicate relationships", body)
        parser = DetailsParser()
        parser.feed(body)
        self.assertEqual(len(parser.details), 2)
        self.assertEqual(parser.stack, [])
        self.assertEqual(parser.unexpected, [])

    def test_untrusted_content_cannot_escape_the_decorated_structure(self):
        payload = "@intruder </details><script>alert(1)</script> [link](https://example.com) `**"
        evidence = {**self.evidence, "body": "### NuGet package version\n\n" + payload}
        result = self.result(
            summary=payload, missing_information=payload,
            missing_information_kind="REQUEST_DETAILS",
        )
        body = t.render(result, evidence, MAPPING)
        parser = DetailsParser()
        parser.feed(body)
        self.assertEqual(parser.stack, [])
        self.assertEqual(parser.unexpected, [])
        self.assertNotIn("@intruder", body)
        self.assertNotIn("<script>", body)
        self.assertNotIn("https://example.com", body)
        self.assertEqual(body.count(t.MARKER), 1)


if __name__ == "__main__":
    unittest.main()
