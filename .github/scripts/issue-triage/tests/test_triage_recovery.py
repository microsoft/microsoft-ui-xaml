import copy
import unittest

import triage as t
from test_triage import FakeGitHub, agent_output, event


class AssessmentRecoveryTests(unittest.TestCase):
    def setUp(self):
        self.client = FakeGitHub()
        self.mapping = t.read_json(str(t.MAP_PATH))
        self.client.catalog.update(dict.fromkeys(self.mapping, ""))
        self.client.catalog.update(dict.fromkeys({
            "team-Controls", "team-Markup", "team-Rendering", "team-Reach",
            "team-CompInput", "team-Design", "team-Core",
        }, ""))
        self.evidence = t.prepare(self.client, event(), self.mapping)

    def validate(self, source_kind="BUG", legacy=False, **changes):
        evidence = copy.deepcopy(self.evidence)
        evidence["issue_kind"] = source_kind
        output = agent_output(evidence, **changes)
        if legacy:
            output["items"][0].pop("missing_information_kind")
        return t.validate(output, evidence), evidence

    def test_all_observed_rejection_shapes_are_recoverable(self):
        cases = [
            (708, "OTHER", dict(issue_kind="OTHER", reproduction="SUFFICIENT",
                                area="area-KeyboardAccelerators")),
            (10166, "OTHER", dict(issue_kind="OTHER", reproduction="NOT_APPLICABLE",
                                  area="area-RadialGradientBrush",
                                  routing_reason="The request concerns gradient animation.")),
            (10167, "BUG", dict(area="area-Tooling",
                                routing_reason="The issue concerns the debugging experience.")),
            (10621, "BUG", dict(area="area-CoreFramework", reproduction="INSUFFICIENT")),
            (10639, "BUG", dict(area="area-SystemBackdropEement",
                                routing_rule="compositor-resource",
                                routing_reason="Backdrop configuration uses compositor resources.")),
            (10737, "BUG", dict(area="area-Pointer", reproduction="INSUFFICIENT",
                                missing_information="Please provide Windows and Windows App SDK versions.",
                                routing_reason="Pointer movement affects the visual state.")),
            (10750, "OTHER", dict(issue_kind="OTHER", area="area-Performance",
                                  reproduction="INSUFFICIENT",
                                  missing_information="Please provide a complete minimal C++ reproduction project.")),
            (10971, "FEATURE", dict(issue_kind="BUG", area="area-External")),
            (11535, "BUG", dict(area="area-SystemBackdropEement",
                                routing_rule="compositor-resource",
                                routing_reason="Backdrop theme configuration affects compositor resources.")),
            (11796, "BUG", dict(area="area-Infrastructure", reproduction="NOT_APPLICABLE")),
            (12060, "FEATURE", dict(issue_kind="FEATURE", reproduction="NOT_APPLICABLE",
                                    area="area-DevInternal",
                                    routing_reason="The proposal concerns build and code-generation tooling.")),
        ]
        for number, source_kind, changes in cases:
            with self.subTest(issue=number):
                result, evidence = self.validate(source_kind, legacy=True, **changes)
                labels = t.planned_labels(result, evidence, self.mapping)
                self.assertNotIn("needs-repro", labels)
                self.assertNotIn("needs-author-feedback", labels)
                self.assertTrue(t.render(result, evidence, self.mapping).startswith(t.MARKER))

    def test_legal_assessment_combinations_never_require_fabricated_requests(self):
        for source_kind in ("BUG", "FEATURE", "OTHER"):
            for assessed_kind in ("BUG", "FEATURE", "OTHER"):
                for reproduction in ("SUFFICIENT", "INSUFFICIENT", "NOT_APPLICABLE"):
                    for category in ("NONE", "REPRODUCTION", "ENVIRONMENT", "REQUEST_DETAILS"):
                        request = "None" if category == "NONE" else "Please provide the stated information."
                        with self.subTest(source=source_kind, assessed=assessed_kind,
                                          reproduction=reproduction, category=category):
                            result, evidence = self.validate(
                                source_kind, issue_kind=assessed_kind, reproduction=reproduction,
                                missing_information=request, missing_information_kind=category,
                            )
                            should_label = (
                                assessed_kind == "BUG" and source_kind != "FEATURE"
                                and reproduction == "INSUFFICIENT" and category == "REPRODUCTION"
                            )
                            self.assertEqual(result["needs_repro"], should_label)
                            self.assertEqual(
                                "needs-repro" in t.planned_labels(result, evidence, self.mapping),
                                should_label,
                            )
                            if request == "None":
                                self.assertEqual(result["missing_information"], "None")

    def test_legacy_bug_without_type_metadata_can_be_assessed_as_bug(self):
        result, evidence = self.validate(
            "OTHER", issue_kind="BUG", reproduction="INSUFFICIENT",
            missing_information="Please provide exact steps and a minimal reproduction.",
            missing_information_kind="REPRODUCTION",
        )
        self.assertTrue(result["needs_repro"])
        self.assertEqual(result["source_issue_kind"], "OTHER")
        self.assertEqual(result["issue_kind"], "BUG")
        labels = t.planned_labels(result, evidence, self.mapping)
        self.assertIn("needs-repro", labels)
        self.assertNotIn("bug", labels)

    def test_non_bug_ratings_are_normalized_and_auditable(self):
        result, _ = self.validate("OTHER", issue_kind="OTHER", reproduction="SUFFICIENT")
        self.assertEqual(result["reproduction"], "NOT_APPLICABLE")
        self.assertEqual(result["reported_reproduction"], "SUFFICIENT")
        self.assertTrue(result["review_notes"])
        self.assertFalse(result["needs_repro"])

    def test_bug_tagged_question_does_not_get_a_reproduction_request(self):
        result, evidence = self.validate(issue_kind="OTHER", reproduction="NOT_APPLICABLE")
        self.assertEqual(result["source_issue_kind"], "BUG")
        self.assertTrue(result["review_notes"])
        self.assertNotIn("needs-repro", t.planned_labels(result, evidence, self.mapping))
        self.assertNotIn("Information needed", t.render(result, evidence, self.mapping))

    def test_bug_not_applicable_is_deferred_instead_of_failing(self):
        result, _ = self.validate(reproduction="NOT_APPLICABLE")
        self.assertEqual(result["reproduction"], "NOT_APPLICABLE")
        self.assertTrue(result["review_notes"])
        self.assertFalse(result["needs_repro"])

    def test_insufficient_without_actionable_request_does_not_invent_one(self):
        result, evidence = self.validate(reproduction="INSUFFICIENT")
        self.assertEqual(result["missing_information"], "None")
        self.assertFalse(result["needs_repro"])
        self.assertTrue(result["review_notes"])
        self.assertNotIn("Information needed", t.render(result, evidence, self.mapping))

    def test_environment_only_request_does_not_apply_needs_repro(self):
        result, evidence = self.validate(
            reproduction="INSUFFICIENT",
            missing_information="Please provide Windows and Windows App SDK versions.",
            missing_information_kind="ENVIRONMENT",
        )
        self.assertEqual(result["missing_information_kind"], "ENVIRONMENT")
        self.assertIn("Windows", result["missing_information"])
        self.assertFalse(result["needs_repro"])
        self.assertNotIn("needs-repro", t.planned_labels(result, evidence, self.mapping))

    def test_legacy_uncategorized_request_is_not_assumed_to_need_repro(self):
        result, _ = self.validate(
            legacy=True, reproduction="INSUFFICIENT",
            missing_information="Please provide additional information.",
        )
        self.assertEqual(result["missing_information_kind"], "UNSPECIFIED")
        self.assertFalse(result["needs_repro"])
        self.assertTrue(result["review_notes"])

    def test_feature_intake_blocks_bug_repro_request_even_when_model_says_bug(self):
        result, evidence = self.validate(
            "FEATURE", issue_kind="BUG", reproduction="INSUFFICIENT",
            missing_information="Please provide a minimal reproduction project.",
            missing_information_kind="REPRODUCTION",
        )
        self.assertEqual(result["reproduction"], "NOT_APPLICABLE")
        self.assertEqual(result["missing_information"], "None")
        self.assertIn("minimal reproduction", result["deferred_information_request"])
        self.assertFalse(result["needs_repro"])
        self.assertNotIn("Information needed", t.render(result, evidence, self.mapping))

    def test_general_feature_clarification_is_preserved(self):
        result, _ = self.validate(
            "FEATURE", issue_kind="FEATURE", reproduction="NOT_APPLICABLE",
            missing_information="Please describe the desired behavior and scenario.",
            missing_information_kind="REQUEST_DETAILS",
        )
        self.assertIn("desired behavior", result["missing_information"])
        self.assertFalse(result["needs_repro"])

    def test_conflicting_reproduction_request_is_deferred(self):
        result, _ = self.validate(
            reproduction="SUFFICIENT", missing_information="Please provide a minimal repro.",
            missing_information_kind="REPRODUCTION",
        )
        self.assertEqual(result["missing_information"], "None")
        self.assertIn("minimal repro", result["deferred_information_request"])
        self.assertTrue(result["review_notes"])

    def test_truncated_context_never_authorizes_repro_label(self):
        evidence = copy.deepcopy(self.evidence)
        evidence["context_truncated"] = True
        result = t.validate(agent_output(
            evidence, reproduction="INSUFFICIENT",
            missing_information="Please provide steps.", missing_information_kind="REPRODUCTION",
        ), evidence)
        self.assertFalse(result["needs_repro"])

    def test_review_is_visible_in_comment_and_triage_plan(self):
        result, evidence = self.validate(reproduction="INSUFFICIENT")
        evidence["existing_labels"] = ["bug", "area-Expander", "team-Controls"]
        self.assertEqual(t.planned_labels(result, evidence, self.mapping), ["needs-triage"])
        self.assertIn("Assessment needs review", t.render(result, evidence, self.mapping))

    def test_kind_labels_remain_unchanged_when_content_disagrees(self):
        result, _ = self.validate(issue_kind="OTHER", reproduction="NOT_APPLICABLE")
        t.publish(self.client, result, self.evidence, self.mapping, write=True)
        for method, path, payload in self.client.writes:
            self.assertNotIn("state", payload)
            if path.endswith("/labels"):
                self.assertEqual(method, "POST")
                self.assertNotIn("bug", payload["labels"])
                self.assertNotIn("feature proposal", payload["labels"])

    def test_same_owner_cross_area_rule_uses_unconditional_default(self):
        result, evidence = self.validate(
            area="area-SystemBackdropEement", routing_rule="compositor-resource",
            routing_reason="Backdrop configuration concerns compositor resources.",
        )
        self.assertEqual(result["routing_rule"], "default")
        self.assertEqual(result["original_routing_rule"], "compositor-resource")
        self.assertTrue(result["normalization_notes"])
        self.assertIn("team-CompInput", t.planned_labels(result, evidence, self.mapping))
        self.assertIn("Routing adjustment", t.render(result, evidence, self.mapping))

    def test_different_owner_cross_area_rule_still_fails_closed(self):
        with self.assertRaises(t.TriageError):
            self.validate(area="area-Resources", routing_rule="compositor-resource",
                          routing_reason="Use another owner.")

    def test_recovery_cannot_bypass_selected_area_conditions(self):
        with self.assertRaises(t.TriageError):
            self.validate(area="area-AppWindow", routing_rule="compositor-resource",
                          routing_reason="Use the default without assessing hosting.")

    def test_unknown_rules_and_ownerless_defaults_still_fail_closed(self):
        for area, rule in (
            ("area-SystemBackdropEement", "invented-rule"),
            ("area-Performance", "compositor-resource"),
        ):
            with self.subTest(area=area, rule=rule), self.assertRaises(t.TriageError):
                self.validate(area=area, routing_rule=rule, routing_reason="A guess.")

    def test_ambiguous_foreign_rule_name_cannot_be_recovered(self):
        evidence = copy.deepcopy(self.evidence)
        evidence["area_guidance"]["area-Expander"]["overrides"] = [{
            "id": "compositor-resource", "when": "Different condition", "team": "team-Rendering",
        }]
        with self.assertRaises(t.TriageError):
            t.validate(agent_output(
                evidence, area="area-SystemBackdropEement",
                routing_rule="compositor-resource", routing_reason="Ambiguous route.",
            ), evidence)


if __name__ == "__main__":
    unittest.main()
