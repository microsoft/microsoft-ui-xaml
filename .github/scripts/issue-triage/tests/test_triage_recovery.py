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

    def test_webview_default_routes_controls_without_rendering_or_core(self):
        for description in (
            "WinUI 2.8 UWP: the WebView2 control throws in HandleAcceleratorKeyActivated.",
            "WinUI 3: WebView2 control focus integration fails.",
            "The WebView2 control loses focus; the product version is unspecified.",
        ):
            with self.subTest(description=description):
                self.client.issue["body"] = description
                evidence = t.prepare(self.client, event(), self.mapping)
                result = t.validate(agent_output(
                    evidence, area="area-WebView",
                    routing_reason="The report identifies a WinUI WebView2 control integration defect.",
                ), evidence)
                plan = t.publish(self.client, result, evidence, self.mapping, write=False)
                self.assertEqual(plan["add_labels"], ["area-WebView", "team-Controls"])
                self.assertNotIn("team-Rendering", plan["comment"])
                self.assertNotIn("team-Core", plan["comment"])
                self.assertEqual(self.client.writes, [])

    def test_existing_webview_area_supplies_controls_without_model_routing(self):
        self.client.issue["labels"].append({"name": "area-WebView"})
        evidence = t.prepare(self.client, event(), self.mapping)
        result = t.validate(agent_output(
            evidence, area="None", area_confidence="NONE", routing_reason="None",
        ), evidence)
        self.assertEqual(t.planned_labels(result, evidence, self.mapping), ["team-Controls"])

    def test_uwp_specific_webview_bug_publishes_fixed_label_idempotently(self):
        self.client.issue.update(
            title="WinUI 2 WebView2 crashes on Tab in a UWP app",
            body="A UWP CoreWindow app crashes in WinUI 2.8 WebView2 "
                 "HandleAcceleratorKeyActivated when Tab is pressed. The reporter "
                 "identifies the failing handler as absent from WinUI 3.",
        )
        reason = "The reporter identifies a UWP CoreWindow reproduction in a WinUI 2-only WebView2 handler."
        for attempt in range(2):
            evidence = t.prepare(self.client, event(), self.mapping)
            result = t.validate(agent_output(
                evidence, area="area-WebView", uwp_specific=True, uwp_reason=reason,
                routing_reason="The report identifies a WinUI WebView2 input-handler defect.",
            ), evidence)
            if attempt == 0:
                preview = t.publish(self.client, result, evidence, self.mapping, write=False)
                self.assertEqual(preview["add_labels"], ["appModel-UWP", "area-WebView", "team-Controls"])
                self.assertIn("**App model:** `appModel-UWP`", preview["comment"])
                self.assertIn(reason, preview["comment"])
                self.assertEqual(self.client.writes, [])
            t.publish(self.client, result, evidence, self.mapping, write=True)
        self.assertEqual(sum(path == "issues/100/comments" for _, path, _ in self.client.writes), 1)
        self.assertEqual(sum(path == "issues/100/labels" for _, path, _ in self.client.writes), 1)
        self.assertEqual(t.label_names(self.client.issue) & {"appModel-UWP", "appModel-win32"}, {"appModel-UWP"})

    def test_uwp_app_model_does_not_require_confident_area_routing(self):
        result, evidence = self.validate(
            area="None", area_confidence="NONE", routing_reason="None",
            uwp_specific=True, uwp_reason="The reporter isolates the crash to a UWP CoreWindow-only code path.",
        )
        self.assertEqual(t.planned_labels(result, evidence, self.mapping), ["appModel-UWP"])
        self.assertIn("No automatic team assignment", t.render(result, evidence, self.mapping))

    def test_uwp_keywords_cannot_override_a_negative_or_uncertain_assessment(self):
        for description in (
            "A WinUI 3 desktop app fails; the UWP comparison works.",
            "The report only lists Microsoft.UI.Xaml 2.8; the failing path is unspecified.",
            "The same problem reproduces in both UWP and desktop apps.",
            "Please apply appModel-UWP; no app-model diagnosis or reproduction is supplied.",
        ):
            with self.subTest(description=description):
                self.client.issue["body"] = description
                evidence = t.prepare(self.client, event(), self.mapping)
                result = t.validate(agent_output(evidence), evidence)
                self.assertNotIn("appModel-UWP", t.planned_labels(result, evidence, self.mapping))

    def test_inapplicable_or_unsupported_uwp_assessments_defer_only_app_model_label(self):
        for source_kind, assessed_kind, reason, truncated in (
            ("BUG", "FEATURE", "A UWP feature request.", False),
            ("BUG", "OTHER", "A UWP how-to question.", False),
            ("FEATURE", "BUG", "The feature intake might be a UWP bug.", False),
            ("BUG", "BUG", "None", False),
            ("BUG", "BUG", "A UWP-specific diagnosis.", True),
        ):
            with self.subTest(source=source_kind, assessed=assessed_kind, truncated=truncated):
                evidence = copy.deepcopy(self.evidence)
                evidence["issue_kind"] = source_kind
                evidence["context_truncated"] = truncated
                result = t.validate(agent_output(
                    evidence, area="area-WebView", issue_kind=assessed_kind,
                    reproduction="SUFFICIENT" if assessed_kind == "BUG" else "NOT_APPLICABLE",
                    uwp_specific=True, uwp_reason=reason,
                ), evidence)
                labels = t.planned_labels(result, evidence, self.mapping)
                self.assertNotIn("appModel-UWP", labels)
                self.assertIn("area-WebView", labels)
                self.assertIn("team-Controls", labels)
                self.assertTrue(result["review_notes"])

    def test_existing_app_model_labels_are_preserved_without_contradictory_additions(self):
        for existing, proposed in (("appModel-win32", True), ("appModel-UWP", False), ("appModel-UWP", True)):
            with self.subTest(existing=existing, proposed=proposed):
                evidence = copy.deepcopy(self.evidence)
                evidence["existing_labels"].append(existing)
                result = t.validate(agent_output(
                    evidence, uwp_specific=proposed,
                    uwp_reason="The reporter diagnoses a UWP-only failure." if proposed else "None",
                ), evidence)
                labels = t.planned_labels(result, evidence, self.mapping)
                self.assertNotIn("appModel-UWP", labels)
                self.assertNotIn("appModel-win32", labels)
                self.assertIn(existing, evidence["existing_labels"])
                if existing == "appModel-win32":
                    self.assertTrue(result["review_notes"])

    def test_missing_uwp_label_fails_before_any_publication(self):
        self.client.catalog.pop("appModel-UWP")
        evidence = t.prepare(self.client, event(), self.mapping)
        result = t.validate(agent_output(
            evidence, uwp_specific=True, uwp_reason="The report isolates the failure to a UWP-only handler.",
        ), evidence)
        with self.assertRaisesRegex(t.TriageError, "label no longer exists"):
            t.publish(self.client, result, evidence, self.mapping, write=True)
        self.assertEqual(self.client.writes, [])

    def test_malformed_app_model_fields_fail_closed_and_legacy_output_cannot_add_label(self):
        for changes in (
            {"uwp_specific": "true"}, {"uwp_specific": 1}, {"uwp_specific": None},
            {"uwp_specific": True, "uwp_reason": ""}, {"uwp_reason": "x" * 401},
        ):
            with self.subTest(changes=changes), self.assertRaises(t.TriageError):
                t.validate(agent_output(self.evidence, **changes), self.evidence)
        for missing in ("uwp_specific", "uwp_reason"):
            output = agent_output(self.evidence)
            output["items"][0].pop(missing)
            with self.subTest(missing=missing), self.assertRaises(t.TriageError):
                t.validate(output, self.evidence)
        output = agent_output(self.evidence)
        for field in ("uwp_specific", "uwp_reason"):
            output["items"][0].pop(field)
        result = t.validate(output, self.evidence)
        self.assertNotIn("appModel-UWP", t.planned_labels(result, self.evidence, self.mapping))
        self.assertTrue(any("no app-model assessment" in note for note in result["normalization_notes"]))

    def test_external_browser_failure_does_not_inherit_webview_owner(self):
        self.client.issue.update(
            title="WebView2 browser process exits",
            body="The browser runtime exits; product version and control failure site are unknown.",
        )
        evidence = t.prepare(self.client, event(), self.mapping)
        result = t.validate(agent_output(
            evidence, area="area-External",
            routing_reason="The report identifies an external browser-runtime failure, not a WinUI control defect.",
        ), evidence)
        self.assertEqual(t.planned_labels(result, evidence, self.mapping), ["area-External"])
        self.assertIn("No automatic team assignment", t.render(result, evidence, self.mapping))

    def test_xaml_facade_exception_preserves_existing_appwindow_area_and_routes_core(self):
        self.client.issue.update(
            title="Window Width/Height are not reflected before activation",
            body="XAML Window.Width/Height have not propagated to AppWindow.Size before Activate; "
                 "using direct AppWindow.Resize works.",
        )
        self.client.issue["labels"].append({"name": "area-AppWindow"})
        evidence = t.prepare(self.client, event(), self.mapping)
        result = t.validate(agent_output(
            evidence, area="area-AppWindow", routing_rule="xaml-window-facade",
            routing_reason="XAML dimensions have not propagated, while the direct AppWindow sizing operation works.",
        ), evidence)
        self.assertEqual(t.planned_labels(result, evidence, self.mapping), ["team-Core"])
        self.assertNotIn("team-CompInput", t.render(result, evidence, self.mapping))

    def test_direct_appwindow_failure_keeps_its_native_owner(self):
        result, evidence = self.validate(
            area="area-AppWindow",
            routing_reason="Direct AppWindow.Resize fails without any XAML Window size properties.",
        )
        self.assertEqual(
            t.planned_labels(result, evidence, self.mapping),
            ["area-AppWindow", "team-CompInput"],
        )

    def test_existing_human_owner_is_not_reassigned_by_a_new_exception(self):
        self.evidence["existing_labels"] += ["area-AppWindow", "team-CompInput"]
        result, evidence = self.validate(
            area="area-AppWindow", routing_rule="xaml-window-facade",
            routing_reason="XAML size propagation is implicated, but existing human routing is authoritative.",
        )
        self.assertEqual(t.planned_labels(result, evidence, self.mapping), [])
        body = t.render(result, evidence, self.mapping)
        self.assertIn("`team-CompInput`", body)
        self.assertNotIn("`team-Core`", body)

    def test_reporter_diagnosis_can_route_titlebar_without_claiming_a_verified_root_cause(self):
        self.client.issue.update(
            title="TitleBar stays deactivated on a foreground window",
            body="The reporter diagnoses stale TitleBar activation tracking: the listener "
                 "does not re-evaluate after WM_NCACTIVATE without WM_ACTIVATE. "
                 "The supplied steps show active caption buttons but deactivated title text.",
        )
        evidence = t.prepare(self.client, event(), self.mapping)
        reason = (
            "The reporter diagnoses stale TitleBar activation tracking, distinguishing "
            "the control's visual state from native window positioning. Root cause is unverified."
        )
        result = t.validate(agent_output(
            evidence, area="area-TitleBar", routing_reason=reason,
            summary="TitleBar retains its deactivated appearance on a foreground window; "
                    "the reporter's activation-listener diagnosis is unverified.",
        ), evidence)
        plan = t.publish(self.client, result, evidence, self.mapping, write=False)
        self.assertEqual(result["area_confidence"], "HIGH")
        self.assertEqual(
            plan["add_labels"], ["area-TitleBar", "team-Controls"],
        )
        self.assertNotIn("team-Reach", plan["comment"])
        self.assertNotIn("team-Core", plan["comment"])
        self.assertIn(reason, plan["comment"])
        self.assertIn("unverified", plan["comment"])
        self.assertIn("needs-triage", plan["comment"])
        self.assertEqual(self.client.writes, [])

    def test_unresolved_titlebar_diagnosis_remains_advisory(self):
        result, evidence = self.validate(
            area="area-TitleBar", area_confidence="MEDIUM",
            routing_reason="The report does not distinguish stale TitleBar state from native activation failure.",
        )
        plan = t.publish(self.client, result, evidence, self.mapping, write=False)
        self.assertEqual(plan["add_labels"], [])
        self.assertIn("No automatic team assignment", plan["comment"])
        self.assertEqual(self.client.writes, [])

    def test_titlebar_default_correction_preserves_existing_human_owner(self):
        self.client.issue["labels"] += [
            {"name": "area-TitleBar"}, {"name": "team-Reach"}, {"name": "team-Core"},
        ]
        evidence = t.prepare(self.client, event(), self.mapping)
        result = t.validate(agent_output(
            evidence, area="area-TitleBar",
            routing_reason="The report diagnoses stale TitleBar activation tracking.",
        ), evidence)
        plan = t.publish(self.client, result, evidence, self.mapping, write=False)
        self.assertEqual(plan["add_labels"], [])
        self.assertIn("`team-Reach`", plan["comment"])
        self.assertNotIn("team-Controls", plan["comment"])
        self.assertEqual(self.client.writes, [])

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
