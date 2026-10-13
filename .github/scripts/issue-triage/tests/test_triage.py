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
MAPPING = {
    "area-Expander": {"team": "team-Controls", "confidence": "medium", "match": ["Expander"]},
    "area-Binding": {"team": None, "confidence": "low", "match": ["Binding", "x:Bind"]},
}
CATALOG = {
    "bug": "", "feature proposal": "", "needs-triage": "", "needs-repro": "",
    "needs-author-feedback": "", "area-Expander": "", "area-Binding": "",
    "team-Controls": "", "team-Core": "", "team-Markup": "", "team-Rendering": "",
    "team-CompInput": "", "team-Reach": "",
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
        "routing_rule": "default", "routing_reason": "None",
        "reproduction": "SUFFICIENT", "missing_information": "None",
        "missing_information_kind": "NONE",
        "duplicate_candidates_json": "[]",
    }
    item.update(changes)
    if "missing_information_kind" not in changes and item["missing_information"] != "None":
        item["missing_information_kind"] = (
            "REPRODUCTION" if item["reproduction"] == "INSUFFICIENT" else "REQUEST_DETAILS"
        )
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
        self.assertEqual(self.evidence["area_guidance"], MAPPING)

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
            {"missing_information": None}, {"issue_kind": "INVALID"},
            {"missing_information_kind": "INVALID"},
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
        recovered = self.result(issue_kind="FEATURE")
        self.assertEqual(recovered["reproduction"], "NOT_APPLICABLE")
        self.assertTrue(recovered["review_notes"])
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
        t.validate_mapping(mapping, dict.fromkeys(set(mapping) | teams, ""))
        self.assertEqual(
            {area for area, entry in mapping.items() if entry["team"] is None},
            {"area-External", "area-Performance"},
        )


class ConditionalRoutingTests(TriageFixture):
    def setUp(self):
        super().setUp()
        self.mapping = copy.deepcopy(MAPPING)
        self.mapping["area-Expander"]["overrides"] = [{
            "id": "rendering-artifact",
            "when": "The failure is in visual composition.",
            "team": "team-Rendering",
        }]
        self.evidence = t.prepare(self.client, event(), self.mapping)

    def conditional_result(self, **changes):
        return self.result(
            routing_rule="rendering-artifact",
            routing_reason="The report describes a composition clipping failure.",
            **changes,
        )

    def test_default_and_conditional_owners_are_derived_from_configuration(self):
        self.assertEqual(
            t.planned_labels(self.result(), self.evidence, self.mapping),
            ["area-Expander", "team-Controls"],
        )
        self.assertEqual(
            t.planned_labels(self.conditional_result(), self.evidence, self.mapping),
            ["area-Expander", "team-Rendering"],
        )

    def test_unknown_rule_is_rejected(self):
        with self.assertRaises(t.TriageError):
            self.result(routing_rule="invented", routing_reason="A guess.")

    def test_rule_cannot_be_used_for_another_area(self):
        with self.assertRaises(t.TriageError):
            self.conditional_result(area="area-Binding")

    def test_conditional_rule_requires_evidence(self):
        with self.assertRaises(t.TriageError):
            self.result(routing_rule="rendering-artifact", routing_reason="None")
        with self.assertRaises(t.TriageError):
            self.result(routing_rule="rendering-artifact", routing_reason="x" * 401)

    def test_default_rule_accepts_explanations_without_changing_its_owner(self):
        result = self.result(routing_reason="This is a control-specific issue.")
        self.assertEqual(
            t.planned_labels(result, self.evidence, self.mapping),
            ["area-Expander", "team-Controls"],
        )
        self.assertIn("Routing rationale", t.render(result, self.evidence, self.mapping))

    def test_model_cannot_supply_a_team_directly(self):
        with self.assertRaises(t.TriageError):
            self.result(team="team-Rendering")

    def test_medium_confidence_does_not_apply_an_override(self):
        result = self.conditional_result(area_confidence="MEDIUM")
        self.assertEqual(t.planned_labels(result, self.evidence, self.mapping), [])

    def test_team_compatibility_uses_the_selected_override_not_the_default(self):
        self.evidence["existing_labels"].append("team-Rendering")
        self.assertEqual(
            t.planned_labels(self.conditional_result(), self.evidence, self.mapping),
            ["area-Expander"],
        )
        self.assertEqual(t.planned_labels(self.result(), self.evidence, self.mapping), [])

    def test_parent_team_is_preserved_for_a_conditional_child_owner(self):
        self.evidence["existing_labels"].append("team-Core")
        self.assertEqual(
            t.planned_labels(self.conditional_result(), self.evidence, self.mapping),
            ["area-Expander"],
        )

    def test_existing_area_uses_its_assessed_condition(self):
        self.evidence["existing_labels"].append("area-Expander")
        self.assertEqual(
            t.planned_labels(self.conditional_result(), self.evidence, self.mapping),
            ["team-Rendering"],
        )

    def test_unassessed_conditional_area_does_not_guess_default_team(self):
        self.evidence["existing_labels"] = ["bug", "area-Expander"]
        result = self.result(area="None", area_confidence="NONE")
        self.assertEqual(
            t.planned_labels(result, self.evidence, self.mapping), ["needs-triage"]
        )

    def test_different_area_assessment_cannot_override_existing_conditional_area(self):
        self.evidence["existing_labels"] = ["bug", "area-Expander"]
        result = self.result(area="area-Binding")
        self.assertEqual(
            t.planned_labels(result, self.evidence, self.mapping), ["needs-triage"]
        )

    def test_low_confidence_ownership_keeps_automatic_routing_in_triage(self):
        self.mapping["area-Expander"]["confidence"] = "low"
        self.evidence["existing_labels"] = ["bug"]
        result = self.result(
            reproduction="INSUFFICIENT", missing_information="Please provide concrete steps."
        )
        self.assertEqual(
            t.planned_labels(result, self.evidence, self.mapping),
            ["area-Expander", "needs-repro", "needs-triage", "team-Controls"],
        )

    def test_confirmed_human_routing_is_not_reset_for_low_mapping_confidence(self):
        self.mapping["area-Expander"]["confidence"] = "low"
        self.evidence["existing_labels"] = ["bug", "area-Expander", "team-Controls"]
        self.assertEqual(t.planned_labels(self.result(), self.evidence, self.mapping), [])

    def test_unknown_area_retains_triage_without_guessing_an_owner(self):
        self.evidence["existing_labels"] = ["bug"]
        result = self.result(area="None", area_confidence="NONE")
        self.assertEqual(t.planned_labels(result, self.evidence, self.mapping), ["needs-triage"])

    def test_evidence_contains_keywords_confidence_and_exact_rule_conditions(self):
        self.assertEqual(
            self.evidence["area_guidance"]["area-Expander"], self.mapping["area-Expander"]
        )

    def test_rendered_team_agrees_with_the_conditional_label_plan(self):
        body = t.render(self.conditional_result(), self.evidence, self.mapping)
        self.assertIn("`team-Rendering`", body)
        self.assertNotIn("`team-Controls`", body)
        self.assertIn("composition clipping", body)

    def test_rendered_comment_does_not_call_an_existing_team_unrouted(self):
        self.evidence["existing_labels"] += ["area-Binding", "team-Core"]
        body = t.render(self.result(), self.evidence, self.mapping)
        self.assertIn("`area-Binding`", body)
        self.assertIn("`team-Core`", body)
        self.assertIn("Existing area labels take precedence", body)
        self.assertNotIn("maintainer routing is needed", body)
        self.assertNotIn("`team-Controls`", body)

    def test_conditional_publication_stays_idempotent(self):
        t.publish(self.client, self.conditional_result(), self.evidence, self.mapping, write=True)
        refreshed = t.prepare(self.client, event(), self.mapping)
        result = t.validate(agent_output(
            refreshed, routing_rule="rendering-artifact",
            routing_reason="The report describes a composition clipping failure.",
        ), refreshed)
        self.client.writes.clear()
        t.publish(self.client, result, refreshed, self.mapping, write=True)
        self.assertEqual(self.client.writes, [])
        self.assertEqual(len(self.client.comments), 1)

    def test_low_confidence_publication_stays_idempotent(self):
        self.mapping["area-Expander"]["confidence"] = "low"
        self.client.issue["labels"] = [{"name": "bug"}]
        evidence = t.prepare(self.client, event(), self.mapping)
        result = t.validate(agent_output(evidence), evidence)
        t.publish(self.client, result, evidence, self.mapping, write=True)
        refreshed = t.prepare(self.client, event(), self.mapping)
        self.client.writes.clear()
        t.publish(
            self.client, t.validate(agent_output(refreshed), refreshed),
            refreshed, self.mapping, write=True,
        )
        self.assertEqual(self.client.writes, [])


class MappingConfigurationTests(unittest.TestCase):
    def test_invalid_shapes_and_confidence_fail_closed(self):
        for entry in (
            None, "team-Controls", {},
            {"team": "team-Controls", "confidence": "certain", "match": ["Expander"]},
            {"team": "team-Controls", "confidence": "high", "match": []},
            {"team": "team-Controls", "confidence": "high", "match": [""]},
            {"team": "team-Controls", "confidence": "high", "match": ["Expander"], "extra": True},
        ):
            with self.subTest(entry=entry), self.assertRaises(t.TriageError):
                t.validate_mapping({"area-Expander": entry}, CATALOG)

    def test_unknown_area_or_non_team_label_is_not_accepted(self):
        for area, team in (("area-Invented", "team-Controls"), ("area-Expander", "needs-author-feedback")):
            with self.subTest(area=area, team=team), self.assertRaises(t.TriageError):
                t.validate_mapping({
                    area: {"team": team, "confidence": "high", "match": ["Expander"]}
                }, CATALOG)

    def test_invalid_conditional_rules_fail_closed(self):
        rule = {"id": "visual", "when": "Visual composition is affected.", "team": "team-Rendering"}
        for overrides in (
            {}, [None], [{**rule, "id": "default"}], [rule, rule],
            [{**rule, "id": " visual"}], [{**rule, "id": "visual;command"}],
            [{**rule, "team": "team-Invented"}],
            [{**rule, "team": "needs-author-feedback"}],
            [{**rule, "when": ""}], [{**rule, "extra": "unexpected"}],
        ):
            mapping = copy.deepcopy(MAPPING)
            mapping["area-Expander"]["overrides"] = overrides
            with self.subTest(overrides=overrides), self.assertRaises(t.TriageError):
                t.validate_mapping(mapping, CATALOG)

    def test_team_defaults_cover_each_ownership_family(self):
        mapping = t.read_json(str(t.MAP_PATH))
        expected = {
            "area-NavigationView": "team-Controls",
            "area-TableView": "team-Controls",
            "area-Charting": "team-Controls",
            "area-Inking": "team-Controls",
            "area-CoreFramework": "team-Markup",
            "area-Tooling": "team-Markup",
            "area-WebView": "team-Rendering",
            "area-Accessibility": "team-Reach",
            "area-Islands": "team-Reach",
            "area-Windowing": "team-CompInput",
            "area-SystemBackdropEement": "team-CompInput",
            "area-C++/WinRT": "team-Core",
            "area-DesignDiscussion": "team-Design",
        }
        for area, team in expected.items():
            with self.subTest(area=area):
                self.assertEqual(mapping[area]["team"], team)
        self.assertIn("IXamlDiagnostics", mapping["area-LiveVisualTree"]["match"])
        self.assertIn("x:Bind", mapping["area-Binding"]["match"])

    def test_all_configured_conditional_routes_resolve_the_declared_team(self):
        mapping = t.read_json(str(t.MAP_PATH))
        expected = {
            "area-AnimatedVisualPlayer": ("compositor-failure", "team-CompInput"),
            "area-Flyouts": ("rendering-artifact", "team-Rendering"),
            "area-Icon": ("glyph-rendering", "team-Rendering"),
            "area-Popup": ("composition-layering", "team-Rendering"),
            "area-Scrolling": ("compositor-input", "team-CompInput"),
            "area-Layouts": ("layout-engine", "team-Core"),
            "area-Materials": ("compositor-resource", "team-CompInput"),
            "area-AppWindow": ("xaml-hosting", "team-Reach"),
            "area-TitleBar": ("pointer-handling", "team-CompInput"),
        }
        self.assertEqual(
            {area for area, entry in mapping.items() if entry.get("overrides")}, set(expected)
        )
        for area, (rule_id, team) in expected.items():
            with self.subTest(area=area):
                self.assertEqual(t.route_team(mapping, area, rule_id), team)


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
