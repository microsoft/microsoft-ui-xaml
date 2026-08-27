# Issue triage and duplicate detection automation

## Summary

This document proposes a phased replacement for the current similar-issues bot with a safer issue-triage system that:

* finds likely duplicate issues;
* identifies missing reproduction information;
* keeps one up-to-date triage summary instead of adding repeated bot comments;
* gives maintainers a review queue and explicit control over issue closure; and
* can be evaluated in a public fork before it writes to issues in this repository.

The design adapts the issue-triage automation used by
[Microsoft PowerToys](https://github.com/microsoft/PowerToys) to WinUI's issue templates, labels, and reproduction
requirements. The first production rollout is advisory only. It must not close issues or apply resolution labels.

## Motivation

The repository already has useful pieces of an automated issue lifecycle:

* `.github/workflows/similar-issues-bot.yml` comments on newly opened issues with similar results.
* `.github/workflows/needs-repro-command.yml` lets maintainers request a minimal reproduction.
* `.github/policies/resourceManagement.yml` manages `needs-triage`, `needs-author-feedback`, and stale issues.

The current similar-issues workflow has several limitations:

* it references a third-party action by the mutable `main` branch;
* it runs only when an issue is opened, so corrected titles and descriptions are not reassessed;
* it adds a new comment rather than maintaining one canonical triage result;
* it does not distinguish retrieval similarity from an actual duplicate verdict;
* it does not validate model output against a bounded set of candidates; and
* it has no maintainer digest, accuracy measurement, or staged rollout mechanism.

Duplicate suggestions are especially valuable in this repository because reports often describe the same control
behavior with different application code, Windows versions, or Windows App SDK package versions. Incorrect duplicate
closure is also costly because superficially similar XAML symptoms can have different causes. The system therefore
needs strong retrieval and conservative decisions.

## Goals

* Surface up to five likely duplicate candidates with a short, evidence-based reason and confidence.
* Parse the existing bug and feature proposal forms before using a model.
* Suggest the best matching existing `area-*` label and optionally apply it after the advisory pilot.
* Detect whether a bug includes actionable reproduction steps, actual and expected behavior, package version, and
  Windows version.
* Recommend `needs-author-feedback` or `needs-repro` when investigation is blocked.
* Maintain one recognizable triage comment per issue.
* Re-evaluate an issue when its original title or body changes.
* Re-check the current issue state and content immediately before publishing.
* Provide a daily review surface for maintainers without automatically resolving issues.
* Bound model usage, permissions, candidate count, issue text, and workflow concurrency.
* Measure usefulness and false positives before enabling any write beyond comments and pilot labels.

## Non-goals

* Automatically closing issues during the initial rollout.
* Replacing maintainer judgment about severity, priority, roadmap fit, or API design.
* Automatically setting priority, severity, milestones, assignees, or roadmap labels.
* Sending public issue content, attachments, or reproduction projects to a third-party service.
* Mirroring issue comments or attachments into the public pilot repository.
* Changing the existing internal bug mirroring process.
* Automatically assigning an engineer.

## Proposed experience

### Issue author

When an issue is opened or its original content is edited, the workflow creates or updates one comment:

```markdown
<!-- winui-ai-triage:canonical:v1 -->

## Automated triage

**Summary:** The first expansion of an Expander briefly renders with an incorrect transition.

**Suggested area:** `area-Expander`

**Information needed:** Please attach a minimal WinUI 3 reproduction or provide XAML and code-behind that reproduces
the first-expansion behavior.

### Possible duplicates

<details>
<summary>#12345 - Expander content flashes during its first expansion</summary>

**Why this may be a duplicate:** Both reports describe a first-use Expander transition with the same visible glitch.

**Confidence:** Medium
</details>

_This is an automated suggestion. A maintainer will decide whether the issues are duplicates._
```

The comment mentions the author only when information is required. Editing the issue updates the existing comment
instead of creating another notification.

### Maintainer

A daily digest groups open reports under the strongest proposed canonical issue:

```markdown
## Daily duplicate digest

### Keep #12345 - Expander content flashes during its first expansion

* Review #12367 - Expander animation glitch the first time it expands
* Review #12371 - Expander first-open transition is incorrect
```

Each entry links to the issue and its canonical triage comment. The digest is a review queue, not a source of new
duplicate judgments.

After the advisory phase has demonstrated acceptable precision, the workflow may submit GitHub's native duplicate
suggestion. This is the GitHub issue action that renders an accept/decline chip near the top of the issue, allowing a
maintainer to confirm the duplicate and close it in one click. The workflow must not directly close the issue.

## Architecture

The system separates deterministic processing, model judgment, validation, and publication.

### 1. Deterministic issue context

A repository-owned script reads the event and GitHub API responses, then produces bounded evidence:

* issue number, author, state, title, and original body;
* issue kind: bug, feature proposal, or other;
* parsed issue-form fields;
* existing labels;
* an allowlist of existing `area-*` labels and deterministic area candidates;
* package and Windows versions;
* reproduction completeness;
* a content hash of the title and original body; and
* a bounded list of duplicate candidates.

For bug reports, the parser uses the existing sections:

* `Describe the bug`
* `Why is this important?`
* `Steps to reproduce the bug`
* `Actual behavior`
* `Expected behavior`
* `NuGet package version`
* `Windows version`
* `Additional context`

For feature proposals, it uses `Title`, `Summary`, `Rationale`, `Scope`, `Important Notes`, and `Open Questions`.

All issue text is untrusted data. HTML comments, control characters, mentions, and oversized sections are removed or
escaped before text is republished.

### 2. Candidate retrieval

Retrieval should favor recall but remain deterministic. It can combine:

* exact technical identifiers, such as control, API, event, property, exception, and error-code names;
* focused GitHub issue searches generated from title and issue-form fields;
* matching issue-kind and area labels when available;
* open and closed issues, because a closed issue can be the canonical report; and
* a configurable creation-date boundary.

Candidates are scored and capped before model execution. The retrieval score is evidence for ranking only and must not
be presented as a duplicate verdict.

For the pilot, the maximum candidate set is 30 issues and the model may return at most five suggestions.

### 3. Bounded model judgment

The model receives only the triggering issue and the deterministic candidate set. It may:

* summarize the report;
* determine whether required reproduction information is present;
* compare the report with supplied duplicate candidates; and
* explain each suggested duplicate in one short sentence.

The model may not search GitHub, run arbitrary shell commands, edit files, publish comments, apply labels, or close
issues. Its response uses a strict schema.

### 4. Area and ownership classification

The repository currently has more than 120 `area-*` labels and a smaller set of `team-*` ownership labels. The
automation should use the existing taxonomy rather than inventing labels.

Area classification combines:

* exact control, API, event, property, namespace, and exception names;
* the issue title and parsed form sections;
* labels on strongly related duplicate candidates; and
* a bounded model choice from the deterministic candidate set.

The model may return one primary area and up to two alternatives, but every value must exactly match an existing label.
The publication policy is conservative:

* never create a new area or team label;
* never remove or replace an area label applied by a human;
* apply at most one primary `area-*` label when the issue has none and confidence is high;
* show medium-confidence classifications only in the canonical comment;
* do not change priority, severity, milestones, assignees, or resolution labels; and
* do not remove `needs-triage` during the initial rollout.

`team-*` labels should not be selected directly by the model. If the repository wants ownership routing, a
maintainer-owned configuration maps stable area labels to teams. This keeps ownership changes reviewable and avoids
encoding the organization chart in prompts.

### 5. Verification

Repository-owned code verifies that:

* the issue content hash still matches;
* every suggested duplicate was in the deterministic candidate set;
* no more than five candidates were returned;
* issue numbers and confidence values have valid types;
* recommended labels are existing allowlisted `area-*` or workflow labels;
* output fields are bounded; and
* the issue is still open.

If verification fails, the workflow publishes nothing.

### 6. Publication

A separate job with `issues: write` rebuilds current evidence and performs the write. It:

* finds the existing comment by `<!-- winui-ai-triage:canonical:v1 -->`;
* creates or updates that one comment;
* optionally applies one validated area label and only pilot-approved workflow labels;
* skips closed issues; and
* records enough hidden state to support the digest without trusting rendered Markdown.

The model job has read-only issue access. Only the validated publication job receives write permission.

### 7. Digest

The daily digest reads validated hidden state from canonical triage comments. It does not run duplicate detection.
Every run re-checks that:

* the reported issue is still open;
* the proposed canonical issue still exists;
* resolved duplicates no longer appear; and
* a manually dismissed candidate is not immediately reintroduced.

The previous digest is closed when a replacement is created. If no candidates remain, no new digest is opened.

## Workflow files

The intended production implementation has these surfaces:

| File | Responsibility |
| --- | --- |
| `.github/workflows/issue-triage.md` | Human-readable source for the agentic workflow. |
| `.github/workflows/issue-triage.lock.yml` | Generated GitHub Actions workflow. |
| `.github/workflows/dedupe-digest.yml` | Daily maintainer review queue. |
| `.github/scripts/issue-triage/issue-context.py` | Template parsing, candidate retrieval, and evidence generation. |
| `.github/scripts/issue-triage/verify-agent-output.py` | Fail-closed validation of model output. |
| `.github/scripts/issue-triage/tests/` | Unit and workflow-contract tests. |

The existing `/needs-repro` command and resource-management policy remain the source of truth for the author-feedback
lifecycle. The triage workflow can recommend or apply those existing labels, but must not create a competing stale
policy.

## Security and safety requirements

* Pin every third-party action to a full commit SHA.
* Use the minimum job-level permissions.
* Treat issue titles, bodies, labels, comments, and attachment names as prompt-injection input.
* Do not pass raw attachments or reproduction archives to the model.
* Do not upload issue content as workflow artifacts.
* Escape Markdown and neutralize mentions before republishing user text.
* Use per-issue concurrency with `cancel-in-progress: true`.
* Add per-user and daily model-credit limits.
* Rebuild evidence in the write job instead of trusting files from the model job.
* Never use model-provided issue numbers, labels, URLs, or hashes unless they match deterministic evidence.
* Fail closed when threat detection, evidence refresh, schema validation, or state checks fail.
* Never reopen an issue unless closure can be attributed to the same workflow request.

## Public fork pilot

The pilot runs in [niels9001/microsoft-ui-xaml](https://github.com/niels9001/microsoft-ui-xaml).

### Corpus

The test repository is populated with the 50 most recently created upstream issues, excluding pull requests. Each copy:

* preserves the original title and issue body for realistic retrieval;
* links to the upstream issue and records its original state;
* neutralizes user and team mentions;
* does not copy comments, reactions, assignees, attachments, or internal mirror state; and
* receives a `triage-pilot` label.

The corpus includes an initial known duplicate case: upstream issues
[#11646](https://github.com/microsoft/microsoft-ui-xaml/issues/11646),
[#11647](https://github.com/microsoft/microsoft-ui-xaml/issues/11647),
[#11648](https://github.com/microsoft/microsoft-ui-xaml/issues/11648), and
[#11649](https://github.com/microsoft/microsoft-ui-xaml/issues/11649) have the same title. The pilot should rank the
older reports as strong candidates for the newest report without automatically closing any copy.

### Pilot modes

1. **Candidate retrieval:** Run deterministic title/body retrieval.
2. **Comment-only intake:** Parse the WinUI issue form and update the same canonical comment.
3. **Legacy GenAI comparison:** Run the pinned GitHub Models dedupe action previously used by PowerToys.
4. **Digest:** Create a maintainer-facing digest from validated comments in a later pilot iteration.

The deterministic and comment-only modes establish a reliable comparison point. The legacy action is retained only to
document its current behavior and is not the proposed production architecture.

Initial public runs:

* [Deterministic retrieval and intake run](https://github.com/niels9001/microsoft-ui-xaml/actions/runs/33071945420)
* [Legacy GenAI comparison run](https://github.com/niels9001/microsoft-ui-xaml/actions/runs/33071662766)

The GenAI comparison did not produce a usable verdict. Every model request returned HTTP 410 because GitHub Models was
in a scheduled retirement brownout. The action nevertheless completed successfully and reported no duplicates. This
success-shaped failure confirms that the production design must own model invocation, fail closed on model errors, and
must not treat an empty legacy-action result as evidence that an issue is unique.

### Public examples

Pilot issue links and observed results will be added here after the workflows have run:

| Scenario | Pilot issue | Expected result | Observed result |
| --- | --- | --- | --- |
| Exact-title duplicate cluster | [Fork #4](https://github.com/niels9001/microsoft-ui-xaml/issues/4#issuecomment-5439086935) | Find the other copied `#11646`-`#11649` reports. | Ranked fork issues #10, #6, and #2 as high-confidence exact-title matches. |
| Unique bug | [Fork #5](https://github.com/niels9001/microsoft-ui-xaml/issues/5#issuecomment-5439087067) | Return no duplicate when the threshold is not met. | Returned no candidates. |
| Incomplete bug form | [Fork #5](https://github.com/niels9001/microsoft-ui-xaml/issues/5#issuecomment-5439087067) | Identify missing or very short investigation fields without changing labels. | Identified `Expected behavior` as incomplete and left labels unchanged. |
| Feature proposal | [Fork #35](https://github.com/niels9001/microsoft-ui-xaml/issues/35#issuecomment-5439088490) | Avoid bug-only reproduction guidance. | Classified the issue as a feature proposal and did not request bug reproduction information. |
| Legacy GenAI action | [Actions run](https://github.com/niels9001/microsoft-ui-xaml/actions/runs/33071662766) | Compare model judgments with deterministic retrieval. | Model calls returned HTTP 410 during a retirement brownout, but the action still concluded successfully with no duplicates. |

## Evaluation

The pilot records the following for each issue:

* candidates retrieved;
* candidates suggested by the model;
* candidate ordering and confidence;
* whether the current bot returned a result;
* whether a maintainer marks each suggestion correct, related, or incorrect;
* workflow duration and model cost; and
* whether the author-facing guidance was actionable.

The minimum bar to move from the fork to an upstream advisory pilot is:

* at least 90 percent precision for high-confidence duplicate suggestions;
* no issue closures or resolution-label changes;
* no unauthorized mentions or malformed Markdown;
* no suggestions outside the deterministic candidate set;
* no repeated comments after issue edits; and
* all fail-closed and workflow-contract tests passing.

Recall is measured but is not a rollout gate. Missing a duplicate is preferable to confidently suggesting the wrong
canonical issue.

## Rollout

### Phase 0: Public fork

Run the four pilot modes on the copied corpus. Review failures and add public examples to this document.

### Phase 1: Upstream shadow mode

Run deterministic retrieval on new and edited issues, but publish only workflow summaries visible to maintainers.
Compare results with normal triage for two weeks.

### Phase 2: Advisory comments

Enable the canonical comment for a percentage of new issues. Include the suggested area and alternatives, but do not
apply labels or submit duplicate suggestions.

### Phase 3: Triage labels and digest

Allow the validated publication job to apply one high-confidence `area-*` label, plus `needs-repro` and
`needs-author-feedback` when required. Enable the daily digest. Existing resource-management rules continue to handle
author responses and inactivity. Keep `needs-triage` until maintainers decide that automated area routing is reliable
enough to replace that queue.

### Phase 4: Native duplicate suggestions

If precision remains above the agreed threshold, allow the workflow to submit GitHub's native duplicate suggestion for
high-confidence matches. GitHub displays the accept/decline action chip on the issue; accepting it performs the linked
duplicate closure. Maintainer approval remains mandatory.

## Open questions

* Which area-to-team mappings should be repository-owned configuration, and who approves changes to that map?
* Should more than one `area-*` label ever be applied automatically?
* Should closed issues be eligible as canonical reports indefinitely or only within a time window?
* Should edited issues be reassessed immediately or after a short debounce period?
* Who owns the daily digest, and should it be an issue, discussion, or project view?
* What maintainer action records that a suggested candidate was rejected?
* Should `needs-repro` require a minimal project for crash and input issues, while accepting a snippet for visual bugs?
* Which model and daily credit budget are acceptable for an upstream advisory pilot?
