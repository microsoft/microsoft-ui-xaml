import copy
import io
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
import urllib.error

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import triage as t

from test_find_duplicates import BODY, candidate


REPO = "example/winui"
MAPPING = {"area-Expander": "team-Controls", "area-Binding": None}
CATALOG = {
    "bug": "", "feature proposal": "", "needs-triage": "", "needs-repro": "",
    "needs-author-feedback": "", "area-Expander": "", "area-Binding": "",
    "team-Controls": "", "team-Core": "", "team-Markup": "",
}


def source(**changes):
    result = {
        "number": 100, "title": "Expander content flashes during first expansion",
        "body": BODY, "state": "open", "locked": False, "comments": 0,
        "created_at": "2026-09-01T00:00:00Z",
        "labels": [{"name": "bug"}, {"name": "needs-triage"}],
        "user": {"id": 1, "login": "reporter", "type": "User"},
    }
    result.update(changes)
    return result


def event():
    return {"repository": {"full_name": REPO}, "issue": {"number": 100}, "action": "opened"}


def comment(number=1, body="Here are more details", actor="reporter"):
    bot = actor == "github-actions[bot]"
    return {
        "id": number, "body": body,
        "user": {"id": 2 if bot else 1, "login": actor, "type": "Bot" if bot else "User"},
    }


class FakeGitHub:
    repository = REPO

    def __init__(self):
        self.issue = source()
        self.comments = []
        self.catalog = dict(CATALOG)
        self.candidates = [candidate()]
        self.writes = []
        self.searches = []
        self.after_write = None

    def get(self, suffix):
        if suffix == "issues/100":
            return copy.deepcopy(self.issue)
        raise AssertionError(suffix)

    def pages(self, suffix):
        if suffix == "labels":
            return [{"name": name, "description": value} for name, value in self.catalog.items()]
        if suffix == "issues/100/comments":
            return copy.deepcopy(self.comments)
        raise AssertionError(suffix)

    def search(self, query):
        self.searches.append(query)
        return copy.deepcopy(self.candidates)

    def write(self, method, suffix, payload):
        self.writes.append((method, suffix, copy.deepcopy(payload)))
        if suffix == "issues/100/comments" and method == "POST":
            self.comments.append(comment(200, payload["body"], "github-actions[bot]"))
        elif suffix.startswith("issues/comments/") and method == "PATCH":
            number = int(suffix.rsplit("/", 1)[1])
            next(item for item in self.comments if item["id"] == number)["body"] = payload["body"]
        elif suffix == "issues/100/labels" and method == "POST":
            self.issue["labels"] += [{"name": name} for name in payload["labels"]]
        else:
            raise AssertionError("Forbidden write: " + suffix)
        if self.after_write:
            self.after_write()


def agent_output(evidence, **changes):
    item = {
        "type": "publish_triage_summary",
        "input_sha256": evidence["input_sha256"],
        "summary": "The Expander content flashes during its first expansion.",
        "area": "area-Expander", "area_confidence": "HIGH", "issue_kind": "BUG",
        "reproduction": "SUFFICIENT", "missing_information": "None",
        "duplicate_candidates_json": "[]",
    }
    item.update(changes)
    return {"items": [item]}


class TriageFixture(unittest.TestCase):
    def setUp(self):
        self.client = FakeGitHub()
        self.evidence = t.prepare(self.client, event(), MAPPING)

    def result(self, **changes):
        return t.validate(agent_output(self.evidence, **changes), self.evidence)


class PreparationTests(TriageFixture):
    def test_context_contains_live_labels_and_bounded_candidates(self):
        self.assertTrue(self.evidence["should_process"])
        self.assertEqual(set(self.evidence["allowed_areas"]), set(MAPPING))
        self.assertEqual(self.evidence["candidates"][0]["number"], 1)

    def test_transferred_issue_creation_date_does_not_hide_lower_numbered_duplicate(self):
        self.client.issue["created_at"] = "2026-08-03T03:41:27Z"
        self.client.candidates[0]["created_at"] = "2026-08-03T10:36:17Z"
        with patch.object(
            self.client, "search",
            side_effect=lambda query: [] if "created:" in query else self.client.candidates,
        ):
            refreshed = t.prepare(self.client, event(), MAPPING)
        self.assertEqual(refreshed["candidates"][0]["number"], 1)

    def test_other_issue_kinds_do_not_pollute_candidate_set(self):
        self.client.candidates = [candidate(labels=[{"name": "feature proposal"}])]
        self.assertEqual(t.prepare(self.client, event(), MAPPING)["candidates"], [])

    def test_excluded_intake_does_not_search(self):
        for changes in (
            {"state": "closed"}, {"locked": True}, {"labels": []},
            {"labels": [{"name": "WinUI OSS"}, {"name": "bug"}]},
            {"title": "[WinUI OSS] Internal tracking task"},
            {"user": {"id": 1, "type": "Bot"}},
        ):
            with self.subTest(changes=changes):
                client = FakeGitHub()
                client.issue.update(changes)
                self.assertFalse(t.prepare(client, event(), MAPPING)["should_process"])
                self.assertEqual(client.searches, [])

    def test_reporter_followups_are_used_and_change_fingerprint(self):
        self.client.comments = [comment()]
        refreshed = t.prepare(self.client, event(), MAPPING)
        self.assertEqual(refreshed["author_followups"][0]["body"], "Here are more details")
        self.assertNotEqual(refreshed["input_sha256"], self.evidence["input_sha256"])

    def test_bot_comment_does_not_change_fingerprint(self):
        self.client.comments = [comment(2, t.MARKER + "\nBot text", "github-actions[bot]")]
        self.assertEqual(t.prepare(self.client, event(), MAPPING)["input_sha256"], self.evidence["input_sha256"])

    def test_human_label_changes_invalidate_fingerprint(self):
        self.client.issue["labels"].append({"name": "area-Binding"})
        self.assertNotEqual(t.prepare(self.client, event(), MAPPING)["input_sha256"], self.evidence["input_sha256"])

    def test_large_discussion_is_bounded_and_flagged(self):
        self.client.comments = [comment(n, "x" * 3000) for n in range(1, 22)]
        refreshed = t.prepare(self.client, event(), MAPPING)
        self.assertEqual(len(refreshed["author_followups"]), 10)
        self.assertTrue(refreshed["context_truncated"])
        self.assertTrue(all(len(item["body"]) <= 2000 for item in refreshed["author_followups"]))

    def test_oversized_report_is_flagged(self):
        self.client.issue["body"] = "x" * (t.MAX_BODY + 1)
        refreshed = t.prepare(self.client, event(), MAPPING)
        self.assertEqual(len(refreshed["body"]), t.MAX_BODY)
        self.assertTrue(refreshed["context_truncated"])

    def test_missing_team_in_live_catalog_fails_closed(self):
        del self.client.catalog["team-Controls"]
        with self.assertRaises(t.TriageError):
            t.prepare(self.client, event(), MAPPING)

    def test_non_author_followup_is_not_processed(self):
        trigger = event()
        trigger["comment"] = {"user": {"id": 99}}
        self.assertFalse(t.prepare(self.client, trigger, MAPPING)["should_process"])

    def test_feature_template_is_recognized_without_bug_reproduction(self):
        self.assertEqual(t.issue_kind(source(labels=[{"name": "feature proposal"}])), "FEATURE")
        self.assertEqual(t.issue_kind(source(labels=[], body="### Summary\nSomething\n### Rationale\nWhy")), "FEATURE")

    def test_limit_and_search_failures_are_errors_not_no_duplicates(self):
        self.client.issue["comments"] = 1000
        with self.assertRaises(t.TriageError):
            t.prepare(self.client, event(), MAPPING)
        self.client.issue["comments"] = 0
        with patch.object(self.client, "search", side_effect=t.TriageError("Search unavailable")):
            with self.assertRaises(t.TriageError):
                t.prepare(self.client, event(), MAPPING)


class EventTests(unittest.TestCase):
    def test_positive_dispatch_number(self):
        self.assertEqual(t.issue_number({"repository": {"full_name": REPO}, "inputs": {"issue_number": "42"}}, REPO), 42)

    def test_dispatch_rejects_shell_and_expression_injection(self):
        for value in ("1; echo secret", "0", "-1", "1\n2", "${{ secrets.TOKEN }}", True, 3, "01"):
            with self.subTest(value=value), self.assertRaises(t.TriageError):
                t.issue_number({"repository": {"full_name": REPO}, "inputs": {"issue_number": value}}, REPO)

    def test_repository_and_pr_scope_are_fixed(self):
        for trigger in (
            {"repository": {"full_name": "attacker/other"}, "issue": {"number": 100}},
            {"repository": {"full_name": REPO}, "issue": {"number": 100, "pull_request": {}}},
            {"repository": {"full_name": REPO}, "issue": {"number": True}},
        ):
            with self.subTest(trigger=trigger), self.assertRaises(t.TriageError):
                t.issue_number(trigger, REPO)


class ValidationTests(TriageFixture):
    def test_valid_high_confidence_duplicate(self):
        duplicate = {"number": 1, "reason": "Same first-expansion flash.", "confidence": "HIGH"}
        self.assertEqual(self.result(duplicate_candidates_json=json.dumps([duplicate]))["duplicates"], [duplicate])

    def test_stale_input_is_rejected(self):
        with self.assertRaises(t.TriageError):
            self.result(input_sha256="0" * 64)

    def test_extra_fields_and_other_safe_outputs_are_rejected(self):
        for output in (
            agent_output(self.evidence, labels=["duplicate"]),
            agent_output(self.evidence, type="close_issue"),
            {"items": []}, {"items": [agent_output(self.evidence)["items"][0]] * 2},
        ):
            with self.subTest(output=output), self.assertRaises(t.TriageError):
                t.validate(output, self.evidence)

    def test_label_confidence_and_string_boundaries(self):
        for changes in (
            {"area": "area-Invented"}, {"area": "needs-author-feedback"},
            {"area_confidence": "CERTAIN"}, {"area": "None"},
            {"summary": ""}, {"summary": "x" * 701}, {"summary": ["not a string"]},
            {"missing_information": None}, {"issue_kind": "FEATURE"},
            {"reproduction": "NOT_APPLICABLE"},
            {"reproduction": "INSUFFICIENT", "missing_information": "None"},
        ):
            with self.subTest(changes=changes), self.assertRaises(t.TriageError):
                self.result(**changes)

    def test_unknown_area_is_valid_but_never_auto_applied(self):
        result = self.result(area="None", area_confidence="NONE")
        self.assertEqual(t.planned_labels(result, self.evidence, MAPPING), [])

    def test_out_of_set_self_newer_non_integer_and_medium_duplicates_are_rejected(self):
        for value in (True, "1", 999, 100, 101):
            with self.subTest(value=value), self.assertRaises(t.TriageError):
                self.result(duplicate_candidates_json=json.dumps([
                    {"number": value, "reason": "Same report.", "confidence": "HIGH"}
                ]))
        with self.assertRaises(t.TriageError):
            self.result(duplicate_candidates_json=json.dumps([
                {"number": 1, "reason": "Maybe.", "confidence": "MEDIUM"}
            ]))

    def test_duplicate_count_shape_and_reason_are_bounded(self):
        duplicate = {"number": 1, "reason": "Same report.", "confidence": "HIGH"}
        for value in ([duplicate] * 6, [duplicate] * 2, {}, [True], [{**duplicate, "url": "https://example.com"}],
                      [{**duplicate, "reason": "x" * 401}]):
            with self.subTest(value=value), self.assertRaises(t.TriageError):
                self.result(duplicate_candidates_json=json.dumps(value))

    def test_malformed_duplicate_json_fails(self):
        with self.assertRaises(ValueError):
            self.result(duplicate_candidates_json="not json")

    def test_feature_never_asks_for_bug_reproduction(self):
        self.evidence["issue_kind"] = "FEATURE"
        with self.assertRaises(t.TriageError):
            self.result(issue_kind="FEATURE")
        result = self.result(issue_kind="FEATURE", reproduction="NOT_APPLICABLE")
        self.assertNotIn("needs-repro", t.planned_labels(result, self.evidence, MAPPING))


class RoutingTests(TriageFixture):
    def test_high_confidence_adds_one_area_and_derived_team(self):
        self.assertEqual(t.planned_labels(self.result(), self.evidence, MAPPING), ["area-Expander", "team-Controls"])

    def test_lower_confidence_is_comment_only(self):
        for confidence in ("MEDIUM", "LOW"):
            self.assertEqual(t.planned_labels(self.result(area_confidence=confidence), self.evidence, MAPPING), [])

    def test_existing_area_wins_over_model(self):
        self.evidence["existing_labels"] += ["area-Binding"]
        self.assertEqual(t.planned_labels(self.result(), self.evidence, MAPPING), [])

    def test_existing_area_can_supply_its_missing_team(self):
        self.evidence["existing_labels"] += ["area-Expander"]
        self.assertEqual(t.planned_labels(self.result(area="None", area_confidence="NONE"), self.evidence, MAPPING), ["team-Controls"])

    def test_existing_team_is_preserved_and_conflicting_area_is_not_added(self):
        self.evidence["existing_labels"] += ["team-Markup"]
        self.assertEqual(t.planned_labels(self.result(), self.evidence, MAPPING), [])

    def test_multiple_existing_areas_are_not_collapsed(self):
        self.evidence["existing_labels"] += ["area-Expander", "area-Binding"]
        self.assertEqual(t.planned_labels(self.result(), self.evidence, MAPPING), [])

    def test_missing_mapping_applies_area_without_guessing_team(self):
        self.assertEqual(t.planned_labels(self.result(area="area-Binding"), self.evidence, MAPPING), ["area-Binding"])

    def test_repro_only_no_author_feedback_and_no_removal(self):
        result = self.result(reproduction="INSUFFICIENT", missing_information="Please provide concrete steps.")
        self.evidence["existing_labels"] += ["area-Expander", "team-Controls", "needs-author-feedback"]
        self.assertEqual(t.planned_labels(result, self.evidence, MAPPING), ["needs-repro"])
        self.evidence["existing_labels"] += ["needs-repro"]
        self.assertEqual(t.planned_labels(self.result(), self.evidence, MAPPING), [])

    def test_truncated_context_never_applies_repro(self):
        self.evidence["context_truncated"] = True
        result = self.result(reproduction="INSUFFICIENT", missing_information="Please provide steps.")
        self.assertNotIn("needs-repro", t.planned_labels(result, self.evidence, MAPPING))

    def test_catalog_drift_cannot_create_labels(self):
        self.evidence["_catalog"].pop("needs-repro")
        with self.assertRaises(t.TriageError):
            t.planned_labels(self.result(reproduction="INSUFFICIENT", missing_information="Please provide steps."), self.evidence, MAPPING)

    def test_checked_in_map_is_explicit_and_uses_existing_team_names(self):
        mapping = t.read_json(str(t.MAP_PATH))
        teams = {"team-Controls", "team-Core", "team-Markup", "team-Reach", "team-Rendering", "team-Design", "team-CompInput"}
        self.assertTrue(all(area.startswith("area-") for area in mapping))
        self.assertTrue(all(team is None or team in teams for team in mapping.values()))
        self.assertIsNone(mapping["area-External"])


class PublicationTests(TriageFixture):
    def test_preview_has_no_writes(self):
        plan = t.publish(self.client, self.result(), self.evidence, MAPPING, write=False)
        self.assertFalse(plan["published"])
        self.assertEqual(self.client.writes, [])

    def test_create_and_repeat_maintain_one_canonical_comment(self):
        t.publish(self.client, self.result(), self.evidence, MAPPING, write=True)
        refreshed = t.prepare(self.client, event(), MAPPING)
        result = t.validate(agent_output(refreshed), refreshed)
        self.client.writes.clear()
        t.publish(self.client, result, refreshed, MAPPING, write=True)
        self.assertEqual(len(self.client.comments), 1)
        self.assertEqual(self.client.writes, [])

    def test_updated_summary_edits_bot_comment_instead_of_adding_one(self):
        self.client.comments = [comment(200, t.MARKER + "\nOld summary", "github-actions[bot]")]
        t.publish(self.client, self.result(), self.evidence, MAPPING, write=True)
        self.assertEqual(self.client.writes[0][0:2], ("PATCH", "issues/comments/200"))
        self.assertEqual(len(self.client.comments), 1)

    def test_spoofed_marker_does_not_select_reporter_comment(self):
        self.client.comments = [comment(201, t.MARKER + "\nForged summary")]
        refreshed = t.prepare(self.client, event(), MAPPING)
        result = t.validate(agent_output(refreshed), refreshed)
        t.publish(self.client, result, refreshed, MAPPING, write=True)
        self.assertEqual(self.client.writes[0][0:2], ("POST", "issues/100/comments"))

    def test_marker_embedded_in_quote_is_not_canonical(self):
        self.assertIsNone(t.canonical_comment([comment(200, "> " + t.MARKER + "\ntext", "github-actions[bot]")]))

    def test_multiple_bot_comments_fail_closed(self):
        self.client.comments = [comment(n, t.MARKER + "\ntext", "github-actions[bot]") for n in (1, 2)]
        with self.assertRaises(t.TriageError):
            t.publish(self.client, self.result(), self.evidence, MAPPING, write=True)
        self.assertEqual(self.client.writes, [])

    def test_stale_edit_or_human_routing_prevents_all_writes(self):
        for field, value in (("body", "Updated"), ("labels", [{"name": "bug"}, {"name": "area-Binding"}])):
            with self.subTest(field=field):
                client = FakeGitHub()
                client.issue[field] = value
                with self.assertRaises(t.TriageError):
                    t.publish(client, self.result(), self.evidence, MAPPING, write=True)
                self.assertEqual(client.writes, [])

    def test_closed_or_locked_issue_is_not_written(self):
        for changes in ({"state": "closed"}, {"locked": True}):
            with self.subTest(changes=changes):
                client = FakeGitHub()
                client.issue.update(changes)
                t.publish(client, self.result(), self.evidence, MAPPING, write=True)
                self.assertEqual(client.writes, [])

    def test_human_change_between_comment_and_labels_prevents_label_write(self):
        self.client.after_write = lambda: self.client.issue["labels"].append({"name": "area-Binding"})
        with self.assertRaises(t.TriageError):
            t.publish(self.client, self.result(), self.evidence, MAPPING, write=True)
        self.assertEqual(len(self.client.writes), 1)
        self.assertEqual(self.client.writes[0][1], "issues/100/comments")

    def test_only_comments_and_additive_allowlisted_labels_are_written(self):
        result = self.result(reproduction="INSUFFICIENT", missing_information="Please supply concrete steps.")
        t.publish(self.client, result, self.evidence, MAPPING, write=True)
        self.assertEqual([entry[1] for entry in self.client.writes], ["issues/100/comments", "issues/100/labels"])
        self.assertEqual(self.client.writes[1][2], {"labels": ["area-Expander", "needs-repro", "team-Controls"]})
        self.assertTrue(all("state" not in payload for _, _, payload in self.client.writes))

    def test_untrusted_prose_cannot_inject_mentions_links_or_markers(self):
        result = self.result(summary="@someone\n#999 [click](https://example.com/leak) <!-- marker --> <img src=x>")
        body = t.render(result, self.evidence, MAPPING)
        self.assertNotIn("@someone", body)
        self.assertNotIn("#999", body)
        self.assertNotIn("https://example.com", body)
        self.assertNotIn("<!-- marker -->", body)
        self.assertNotIn("<img", body)
        self.assertTrue(body.startswith(t.MARKER + "\n"))


class ApiTests(unittest.TestCase):
    def test_invalid_repository_cannot_redirect_credentials(self):
        for repo in ("https://example.com", "owner/repo/extra", "owner/repo\n", "../repo/secret"):
            with self.subTest(repo=repo), self.assertRaises(t.TriageError):
                t.GitHub(repo, "test-token")

    def test_redirects_fail_closed(self):
        with self.assertRaises(t.TriageError):
            t.NoRedirect().redirect_request(None, None, 301, "", {}, "https://example.com")

    def test_rate_limit_headers_and_malformed_delays(self):
        self.assertEqual(t.retry_delay({"Retry-After": "12"}, 0), 13)
        self.assertEqual(t.retry_delay({"Retry-After": "100000"}, 0), 75)
        self.assertEqual(t.retry_delay({"Retry-After": "soon"}, 1), 16)
        self.assertEqual(t.retry_delay({"Retry-After": "nan"}, 1), 16)
        with patch("triage.time.time", return_value=100):
            self.assertEqual(t.retry_delay({"x-ratelimit-remaining": "0", "x-ratelimit-reset": "120"}, 0), 21)

    def test_incomplete_search_is_failure_not_empty_result(self):
        client = t.GitHub(REPO, "test-token")
        with patch.object(client, "request", return_value={"incomplete_results": True, "items": []}):
            with self.assertRaises(t.TriageError):
                client.search("Expander")

    def test_failed_writes_are_not_retried(self):
        client = t.GitHub(REPO, "test-token")
        error = urllib.error.HTTPError("https://api.github.com", 503, "Unavailable", {}, None)
        with patch.object(client.opener, "open", side_effect=error) as request:
            with self.assertRaises(t.TriageError):
                client.write("POST", "issues/100/comments", {"body": "text"})
        self.assertEqual(request.call_count, 1)

    def test_unauthorized_reads_fail_without_retrying(self):
        client = t.GitHub(REPO, "test-token")
        error = urllib.error.HTTPError("https://api.github.com", 403, "Forbidden", {}, None)
        with patch.object(client.opener, "open", side_effect=error) as request:
            with self.assertRaises(t.TriageError):
                client.get("issues/100")
        self.assertEqual(request.call_count, 1)

    def test_pagination_is_bounded(self):
        client = t.GitHub(REPO, "test-token")
        with patch.object(client, "get", return_value=[{}] * 100) as request:
            with self.assertRaises(t.TriageError):
                client.pages("labels", max_pages=2)
        self.assertEqual(request.call_count, 2)

    def test_large_json_input_fails(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "large.json"
            path.write_text('{"body":"oversized"}', encoding="utf-8")
            with self.assertRaises(t.TriageError):
                t.read_json(str(path), limit=10)

    def test_cli_missing_token_fails_without_writing(self):
        with patch.dict(os.environ, {}, clear=True), patch.object(sys, "argv", ["triage.py", "publish", "event", "output"]):
            with patch("sys.stderr", new_callable=io.StringIO):
                self.assertEqual(t.main(), 1)


if __name__ == "__main__":
    unittest.main()
