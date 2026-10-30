import copy
import os
import unittest
from unittest.mock import patch

import triage as t
from test_triage import FakeGitHub, MAPPING, agent_output, event


class ProjectFake:
    def __init__(self):
        self.item = None
        self.mutations = []
        self.title = "WinUI"
        self.closed = False
        self.options = [{"id": name + "_id", "name": name} for name in ("P0", "P1", "P2")]
        self.after_add = None
        self.fail_update = False
        self.queries = []

    def graphql(self, query, variables):
        self.queries.append((query, variables))
        if "addProjectV2ItemById" in query:
            self.mutations.append(("add", variables))
            self.item = {
                "id": "item_id", "isArchived": False,
                "project": {"id": "project_id"}, "fieldValueByName": None,
            }
            if self.after_add:
                self.after_add()
            return {"addProjectV2ItemById": {"item": {"id": "item_id"}}}
        if "updateProjectV2ItemFieldValue" in query:
            self.mutations.append(("update", variables))
            if self.fail_update:
                raise t.TriageError("Project update failed")
            self.item["fieldValueByName"] = {
                "__typename": "ProjectV2ItemFieldSingleSelectValue",
                "optionId": variables["option"], "name": variables["option"].removesuffix("_id"),
            }
            return {"updateProjectV2ItemFieldValue": {"projectV2Item": {"id": self.item["id"]}}}
        if "organization(" in query:
            return {"organization": {"projectV2": {
                "id": "project_id", "title": self.title, "closed": self.closed,
                "fields": {
                    "pageInfo": {"hasNextPage": False},
                    "nodes": [{"id": "priority_id", "name": "Priority", "options": self.options}],
                },
            }}}
        return {"node": {
            "id": variables["issue"],
            "projectItems": {
                "pageInfo": {"hasNextPage": False, "endCursor": None},
                "nodes": [copy.deepcopy(self.item)] if self.item else [],
            },
        }}


class MetadataTests(unittest.TestCase):
    def setUp(self):
        self.client = FakeGitHub()
        self.project = ProjectFake()
        self.environment = patch.dict(os.environ, {"TRIAGE_PROJECT_TOKEN": ""})
        self.environment.start()
        self.addCleanup(self.environment.stop)

    def assess(self, **changes):
        evidence = t.prepare(self.client, event(), MAPPING)
        result = t.validate(agent_output(evidence, **changes), evidence)
        return result, evidence

    def publish(self, changes=None, write=True):
        result, evidence = self.assess(
            impact="BROKEN_FUNCTIONALITY", priority_reason="The reported item search returns no group headers.",
            **(changes or {}),
        )
        with (
            patch.dict(os.environ, {"TRIAGE_PROJECT_TOKEN": "fixture-token"}),
            patch.object(t, "GitHub", return_value=self.project),
        ):
            return t.publish(self.client, result, evidence, MAPPING, write)

    def test_priority_precedence_and_no_mentioned_workaround_default(self):
        for impact, workaround, priority in (
            ("CRASH", False, "P0"), ("CRASH", True, "P0"),
            ("LATEST_RELEASE_REGRESSION", False, "P0"),
            ("LATEST_RELEASE_REGRESSION", True, "P0"),
            ("BROKEN_FUNCTIONALITY", False, "P1"),
            ("BROKEN_FUNCTIONALITY", True, "P2"), ("OTHER", False, None),
        ):
            with self.subTest(impact=impact, workaround=workaround):
                result, _ = self.assess(
                    impact=impact, workaround=workaround,
                    priority_reason="The reporter describes this failure and any bypass.",
                    regression_release="microsoft/WindowsAppSDK@v2.5.1",
                )
                self.assertEqual(result["priority"], priority)

    def test_latest_release_regression_requires_exact_trusted_release(self):
        for release in ("None", "latest", "microsoft/WindowsAppSDK@v2.4", "attacker/repo@v2.5.1"):
            with self.subTest(release=release):
                result, _ = self.assess(
                    impact="LATEST_RELEASE_REGRESSION", priority_reason="Reported regression.",
                    regression_release=release,
                )
                self.assertIsNone(result["priority"])
                self.assertTrue(result["review_notes"])
        self.client.release_tag = "v2.6"
        result, _ = self.assess(
            impact="LATEST_RELEASE_REGRESSION", priority_reason="Reported regression.",
            regression_release="microsoft/WindowsAppSDK@v2.5.1",
        )
        self.assertIsNone(result["priority"])

    def test_release_changes_and_type_changes_invalidate_prepared_assessment(self):
        for change in ("release", "type"):
            with self.subTest(change=change):
                client = FakeGitHub()
                evidence = t.prepare(client, event(), MAPPING)
                result = t.validate(agent_output(evidence), evidence)
                if change == "release":
                    client.release_tag = "v2.6"
                else:
                    client.issue["type"] = {"id": 2, "name": "Task"}
                with self.assertRaises(t.TriageError):
                    t.publish(client, result, evidence, MAPPING, True)
                self.assertEqual(client.writes, [])

    def test_nonbugs_conflicting_types_truncation_and_missing_reason_defer_priority(self):
        for kind, source_kind, existing_type, truncated, reason in (
            ("FEATURE", "BUG", "Bug", False, "Feature request."),
            ("OTHER", "BUG", "Bug", False, "How-to question."),
            ("BUG", "FEATURE", None, False, "Contradicts feature intake."),
            ("BUG", "BUG", "Task", False, "Contradicts existing Type."),
            ("BUG", "BUG", None, True, "Omitted evidence."),
            ("BUG", "BUG", None, False, "None"),
        ):
            with self.subTest(kind=kind, source=source_kind, existing_type=existing_type, truncated=truncated):
                evidence = t.prepare(self.client, event(), MAPPING)
                evidence.update(
                    issue_kind=source_kind, existing_issue_type=existing_type,
                    context_truncated=truncated,
                )
                result = t.validate(agent_output(
                    evidence, issue_kind=kind, impact="CRASH", priority_reason=reason,
                ), evidence)
                self.assertIsNone(result["priority"])
                self.assertTrue(result["review_notes"])

    def test_priority_is_independent_of_area_and_reproduction_confidence(self):
        result, _ = self.assess(
            area="None", area_confidence="NONE", routing_reason="None",
            impact="CRASH", priority_reason="The reported action terminates the process.",
            reproduction="INSUFFICIENT", missing_information="Please supply the triggering steps.",
        )
        self.assertEqual(result["priority"], "P0")

    def test_malformed_or_partial_priority_fields_fail_and_legacy_cannot_authorize_priority(self):
        evidence = t.prepare(self.client, event(), MAPPING)
        for changes in (
            {"impact": "P0"}, {"workaround": "false"}, {"workaround": 1},
            {"priority_reason": ""}, {"priority_reason": "x" * 401},
            {"regression_release": "x" * 161},
        ):
            with self.subTest(changes=changes), self.assertRaises(t.TriageError):
                t.validate(agent_output(evidence, **changes), evidence)
        fields = ("impact", "workaround", "priority_reason", "regression_release")
        for missing in fields:
            output = agent_output(evidence)
            output["items"][0].pop(missing)
            with self.subTest(missing=missing), self.assertRaises(t.TriageError):
                t.validate(output, evidence)
        output = agent_output(evidence)
        for field in fields:
            output["items"][0].pop(field)
        result = t.validate(output, evidence)
        self.assertIsNone(result["priority"])
        self.assertTrue(any("no priority assessment" in note for note in result["normalization_notes"]))

    def test_bug_type_is_real_issue_type_not_a_bug_label_and_is_idempotent(self):
        self.client.issue["type"] = None
        self.client.issue["labels"] = [{"name": "needs-triage"}]
        for _ in range(2):
            result, evidence = self.assess()
            t.publish(self.client, result, evidence, MAPPING, True)
        writes = [w for w in self.client.writes if w[1] == "issues/100"]
        self.assertEqual(writes, [("PATCH", "issues/100", {"type": "Bug"})])
        self.assertEqual(self.client.issue["type"]["name"], "Bug")
        self.assertNotIn("bug", t.label_names(self.client.issue))

    def test_existing_type_and_non_bug_assessments_are_not_changed(self):
        for existing, kind in (("Task", "BUG"), ("Feature", "BUG"), ("Bug", "BUG"), (None, "FEATURE"), (None, "OTHER")):
            with self.subTest(existing=existing, kind=kind):
                self.client = FakeGitHub()
                self.client.issue["type"] = {"name": existing} if existing else None
                result, evidence = self.assess(issue_kind=kind)
                t.publish(self.client, result, evidence, MAPPING, True)
                self.assertFalse(any(path == "issues/100" for _, path, _ in self.client.writes))

    def test_preview_has_type_and_priority_plan_but_no_mutations(self):
        self.client.issue["type"] = None
        plan = self.publish(write=False)
        self.assertEqual(plan["issue_type"], "Bug")
        self.assertEqual(plan["project_priority"], "P1")
        self.assertFalse(plan["published"])
        self.assertEqual(self.client.writes, [])
        self.assertEqual(self.project.mutations, [])

    def test_unconfigured_projects_keep_assessment_but_allow_bug_type_and_normal_triage(self):
        self.client.issue["type"] = None
        result, evidence = self.assess(impact="CRASH", priority_reason="The app terminates.")
        plan = t.publish(self.client, result, evidence, MAPPING, True)
        self.assertEqual(plan["project_priority"], "P0")
        self.assertFalse(plan["project_priority_enabled"])
        self.assertEqual(self.client.issue["type"]["name"], "Bug")
        self.assertTrue(plan["published"])

    def test_new_project_item_is_added_then_priority_set_and_verified(self):
        plan = self.publish()
        self.assertEqual([name for name, _ in self.project.mutations], ["add", "update"])
        self.assertEqual(self.project.mutations[0][1], {"project": "project_id", "issue": "I_example"})
        self.assertEqual(self.project.mutations[1][1]["option"], "P1_id")
        self.assertTrue(plan["published"])
        self.publish()
        self.assertEqual(len(self.project.mutations), 2)

    def test_existing_priority_including_a_nonstandard_value_and_archive_are_preserved(self):
        for value, archived in (("P0", False), ("P2", False), ("P3", False), (None, True), (None, False)):
            with self.subTest(value=value, archived=archived):
                self.project = ProjectFake()
                self.project.item = {
                    "id": "existing", "isArchived": archived, "project": {"id": "project_id"},
                    "fieldValueByName": {"name": value, "optionId": "human"} if value else None,
                }
                self.publish()
                self.assertEqual(len(self.project.mutations), int(value is None and not archived))
                self.assertFalse(any(name == "add" for name, _ in self.project.mutations))

    def test_human_priority_appearing_after_insertion_is_not_overwritten(self):
        def human_update():
            self.project.item["fieldValueByName"] = {"name": "P2", "optionId": "P2_id"}
        self.project.after_add = human_update
        self.publish()
        self.assertEqual([name for name, _ in self.project.mutations], ["add"])

    def test_project_configuration_or_access_failure_prevents_all_writes(self):
        for mode in ("wrong_project", "closed", "wrong_option", "access"):
            with self.subTest(mode=mode):
                self.client = FakeGitHub()
                self.project = ProjectFake()
                if mode == "wrong_project":
                    self.project.title = "Other"
                elif mode == "closed":
                    self.project.closed = True
                elif mode == "wrong_option":
                    self.project.options[0]["name"] = "Urgent"
                else:
                    self.project.graphql = lambda *_: (_ for _ in ()).throw(t.TriageError("Forbidden"))
                with self.assertRaises(t.TriageError):
                    self.publish()
                self.assertEqual(self.client.writes, [])
                self.assertEqual(self.project.mutations, [])

    def test_failed_project_mutation_is_not_retried_or_marked_successful(self):
        self.project.fail_update = True
        with self.assertRaises(t.TriageError):
            self.publish()
        self.assertEqual([name for name, _ in self.project.mutations], ["add", "update"])

    def test_human_type_change_during_label_write_prevents_metadata_overwrite(self):
        self.client.issue["type"] = None
        def change():
            if any(path.endswith("/labels") for _, path, _ in self.client.writes):
                self.client.issue["type"] = {"id": 2, "name": "Task"}
        self.client.after_write = change
        with self.assertRaises(t.TriageError):
            self.publish()
        self.assertFalse(any(path == "issues/100" for _, path, _ in self.client.writes))
        self.assertEqual(self.project.mutations, [])

    def test_feature_intake_and_truncated_context_cannot_set_bug_type(self):
        for feature, truncated in ((True, False), (False, True)):
            with self.subTest(feature=feature, truncated=truncated):
                self.client = FakeGitHub()
                self.client.issue["type"] = None
                if feature:
                    self.client.issue["labels"].append({"name": "feature proposal"})
                if truncated:
                    self.client.issue["body"] = "x" * (t.MAX_BODY + 1)
                result, evidence = self.assess()
                t.publish(self.client, result, evidence, MAPPING, True)
                self.assertFalse(any(path == "issues/100" for _, path, _ in self.client.writes))

    def test_project_memberships_are_paginated_and_duplicate_target_items_fail_closed(self):
        project = ProjectFake()
        original = project.graphql
        item = {
            "id": "later", "isArchived": False,
            "project": {"id": "project_id"}, "fieldValueByName": None,
        }
        def pages(query, variables):
            if "organization(" in query:
                return original(query, variables)
            more = variables["cursor"] is None
            return {"node": {
                "id": "I_example",
                "projectItems": {
                    "pageInfo": {"hasNextPage": more, "endCursor": "next" if more else None},
                    "nodes": [] if more else [item],
                },
            }}
        with patch.object(project, "graphql", side_effect=pages):
            self.assertEqual(t.project_priority_snapshot(project, "I_example")["item"]["id"], "later")
        def duplicates(query, variables):
            data = original(query, variables)
            if "organization(" not in query:
                data["node"]["projectItems"]["nodes"] = [item, item]
            return data
        with patch.object(project, "graphql", side_effect=duplicates):
            with self.assertRaisesRegex(t.TriageError, "Multiple WinUI project items"):
                t.project_priority_snapshot(project, "I_example")

    def test_graphql_errors_are_not_accepted_as_partial_success(self):
        client = t.GitHub("example/winui", "fixture-token")
        with patch.object(client, "request", return_value={"data": {"partial": True}, "errors": [{}]}):
            with self.assertRaises(t.TriageError):
                client.graphql("query { viewer { login } }", {})


if __name__ == "__main__":
    unittest.main()
