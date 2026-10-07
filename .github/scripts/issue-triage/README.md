# Issue triage automation

The `AI Issue Triage` workflow prepares a bounded issue context, uses GitHub
Copilot to analyze it, and independently verifies the result before making any
issue changes. The Markdown workflow is the source of truth; its compiled
`issue-triage.lock.yml` is the workflow GitHub Actions executes.

## Behavior

| Situation | Behavior |
| --- | --- |
| Clear primary area, no existing area label | Add one existing `area-*` label at HIGH confidence. |
| One area with a configured owner, no existing team label | Derive and add its `team-*` label from the selected configured routing rule. |
| Evidence clearly meets an area's conditional rule | Use that rule's configured owner rather than the default; the model cannot invent a team. |
| Uncertain area or an unmapped team | Comment with the suggestion; leave the uncertain routing to a maintainer. |
| Low-confidence or unresolved automatic routing | Keep or add `needs-triage` for human confirmation. Already-established human routing is not reset. |
| Existing area or team labels | Preserve them. Do not replace a human decision or add a contradictory mapped area. |
| Multiple existing areas | Preserve all of them; do not infer one team. |
| BUG assessment with an insufficient repro and an actionable REPRODUCTION request | Ask for the specified details and add `needs-repro`, unless the intake is a feature or context is truncated. |
| Missing version but otherwise actionable reproduction | Ask for the version in the comment; do not add `needs-repro` for that alone. |
| Feature proposal | Assess the problem and desired outcome, not bug repro requirements. |
| Intake type and content assessment disagree | Preserve existing type labels and surface the discrepancy for human triage; do not fail otherwise valid routing. |
| Non-bug reproduction rating or incomplete repro without an actionable request | Reconcile applicability or defer the affected request/label; keep useful summary/routing and explain the review requirement. |
| Strong evidence of a duplicate | Suggest up to five older issues in the canonical comment, for human confirmation. |
| Subsequent author follow-up or title/body edit | Re-analyze and update the same bot-owned comment. |
| Malformed output, unsafe/unconfigured routing, API failure, or changed input | Fail closed without publishing that result. |

The workflow **never** creates labels, removes labels, applies
`needs-author-feedback` or `duplicate`, closes an issue, submits a duplicate-close
suggestion, changes assignees/milestones, or sets severity or priority. It leaves
`needs-triage` in place.

Existing `needs-repro` labels remain until a maintainer confirms the new
reproduction. Applying or clearing `needs-author-feedback` remains a human
decision: the existing repository policy enrolls that label in stale closure.
This workflow does not change that policy or the `/needs-repro` command.

One comment is recognized by `<!-- winui-ai-triage:canonical:v1 -->`, at the
beginning of a comment authored by `github-actions[bot]`. A marker in an issue
author's comment cannot make the publisher edit it.

## Comment presentation

The canonical comment uses a compact, decorated triage-summary layout:

- An **issue-author** section appears only when there is an actual information
  request. The request stays visible outside collapsed details, without adding
  new mention/notification behavior.
- The **WinUI-team** section shows area, team, issue-kind, and optional reported
  package-version badges, followed by the summary.
- **Investigation details** contains reproduction status, routing/confidence,
  conditional rationale, and triage notes. It opens automatically when an
  assessment needs review or the analyzed context was truncated.
- **Possible duplicates** is a separate expandable section when suggestions
  exist, preserving issue state, confidence, rationale, and human confirmation.

The metadata uses the preserved/planned routing, not a model suggestion that
would be ignored. Sufficient reproduction details are not presented as a claim
that the automation reproduced the bug. All issue-derived prose and package
metadata remain escaped, and the canonical marker remains the first line so
updates do not create additional comments.

## Ownership mapping

Ownership changes are reviewed in pull requests, like other configuration
changes. The workflow uses the checked-in `area-team-map.json` immediately;
there is no separate mapping approval or activation step.

The team-maintained configuration maps area labels to their default owners;
`area-External` and `area-Performance` deliberately have no default owner.
It also provides keyword guidance, ownership confidence, and nine
conditional routes: eight declared override rules and the explicit
AnimatedVisualPlayer compositor case. Teams are derived from this configuration,
never accepted as arbitrary model output.

Each area entry contains:

- `team`: an existing team label, or `null` for no default owner.
- `confidence`: `high`, `medium`, or `low` ownership confidence, separate from
  the model's confidence in its area classification.
- `match`: control/API names and scenario keywords used for area selection.
- Optional `note`: additional area-selection guidance.
- Optional `overrides`: entries with a stable `id`, an evidence-based `when`
  condition, and an existing `team` label.

The model picks an area and either `default` or a rule ID configured for that
area. Conditional choices require a short evidence-based explanation. The
default rule may also carry a useful explanation; it does not require an empty
rationale. Unknown rule IDs remain errors. A cross-area rule can be normalized
to `default` only if the selected area has no conditional rules and its fixed
default owner exactly matches the unambiguous owner of that known rule. This
cannot introduce a new owner or bypass a selected area's conditions, and the
adjustment is recorded in the result, comment, and workflow log.
For example, `area-Scrolling` normally maps to `team-Controls`, but its
`compositor-input` rule maps compositor-thread interaction tracker/input-latency
problems to `team-CompInput`.

Maintainers can change individual mappings in `area-team-map.json` as ownership
changes. An absent entry or a `null` default team means "do not auto-assign a
default owner." Team names must already exist in the live label catalog. The
publisher fails visibly if a configured team has been removed rather than
creating or guessing a replacement.
If a pre-existing area has conditional routes but the model assessed a different
or uncertain area, no default team is guessed: it remains for human routing.

## Assessment reconciliation

The prepared issue kind is a label/template-based hint, not a verdict about the
report's content. The model assesses BUG, FEATURE, or OTHER independently, while
the publisher never changes the existing bug/feature labels. Disagreements with
a known intake category stay in `needs-triage`.

Non-bug reproduction ratings are treated as `NOT_APPLICABLE`; the original
rating is retained for audit. Bug reports marked not applicable, or marked
insufficient without an actionable request, go to human review instead of
failing the entire issue or inventing a generic request.

`missing_information_kind` separates reproduction gaps from environment/version
requests and other clarification. `needs-repro` requires a BUG assessment, an
INSUFFICIENT reproduction, an actual REPRODUCTION request, and untruncated
context. A feature intake never receives that automatic label. Ambiguous or
contradictory repro requests are deferred; unrelated valid triage remains usable.
Older output artifacts without a request category are accepted conservatively,
but their free-form prose alone cannot authorize a `needs-repro` label.

These recoveries are explicit, not silent defaults: review/normalization notes
appear in the comment and job log. Schema/type errors, unknown labels, genuinely
unconfigured routes, and out-of-set duplicate suggestions still fail closed.

## Operation

**Automatic intake starts when the workflow merges to the default branch.**
The approved mapping is picked up from the trusted workflow commit. There is
no repository activation variable or post-merge mapping review process.

GitHub Actions and Copilot agentic workflow access must be permitted under the
organization's policy, including the Copilot CLI policy **Allow use of Copilot
CLI billed to the organization**. The generated analysis job requests
`copilot-requests: write`; issue reads and writes use job-scoped `GITHUB_TOKEN`.
Do not add a broad personal access token to make the analysis job work.

An optional manual dispatch with an existing `issue_number` and `publish=false`
(the manual default) runs analysis but only writes the validated comment and
proposed label list to the Actions job summary. It does not disable or gate
automatic intake. Preview still consumes AI credits. A fork with the same label
catalog can exercise `publish=true` without affecting real reporter issues; the
workflow must be on the fork's default branch to receive automatic issue events.

Intake runs on issue open, edit, reopen, and the
addition of `needs-triage`. It also runs when the issue author creates or edits
a comment. It excludes pull requests, closed/locked issues, bot-authored issues,
`WinUI OSS` tracking issues, and reports without `bug`, `feature proposal`, or
`needs-triage`. Bot comments and the labels this workflow adds do not retrigger it.

For an emergency stop, disable `AI Issue Triage` in Actions and cancel active
runs. Re-enable that workflow in Actions to resume intake.

The initial rollout does not modify project boards, internal mirroring, existing
stale handling, or the maintainer's acceptance/closure process.

## Trust boundaries and limits

- Issue content, comments, code samples, links, and candidate text are untrusted.
  The model cannot search GitHub, download attachments, run sample code, or write
  directly to issues. It can only return a structured triage result.
- Retrieval performs at most six scoped searches of twenty results each, then
  ranks and retains at most thirty unique older issues. Both open and closed
  issues are considered. Similarity scores are retrieval evidence, not duplicate
  confidence; shared metadata alone is not sufficient. Canonical ordering uses
  the repository's issue numbers, not creation timestamps, which transferred
  issues can retain from another repository.
- Evidence includes up to 16,000 characters of the report and up to ten author
  comments (the first five and last five, at most 2,000 characters each).
  Truncated context is flagged and cannot cause an automatic `needs-repro`.
  Issues with 1,000 or more comments require manual triage instead.
- The analysis job has no issue-write permission. A separate job with
  `issues: write` checks out the trusted workflow commit, rebuilds current
  evidence, and verifies the exact output schema, hash, labels, and candidate
  membership. It never trusts model-written evidence files.
- The fingerprint includes title, body, author comments, and current labels.
  The publisher checks it again immediately before commenting and before
  additive label writes. GitHub does not provide a transaction across these
  operations; a concurrent edit can stop label publication after the comment
  was written. A later run safely refreshes the single comment.
- Only allowlisted routing labels, `needs-triage`, and `needs-repro` can be added,
  at most four per run. The publisher uses additive label writes, never replacement.
- Per-issue concurrency cancels superseded runs. Actor rate limiting allows five
  runs per hour. Ignored label events and other people's comments use separate
  concurrency groups so they cannot cancel an active intake. The triage agent
  is bounded to five turns and ten AI credits;
  the separate output-safety analysis has its own ten-credit budget. A
  one-hundred-credit daily guardrail stops starting further analyses after the
  measured threshold is reached; concurrent runs may already be in flight.
  GitHub API rate limits use bounded backoff.
- Action references and generated runtime containers are immutable pins. Only
  GitHub services are used for issue retrieval and AI analysis.

## Development

Python 3.12+ and the Python standard library are sufficient for scripts/tests.
From the repository root:

```powershell
python -m unittest discover -s .github\scripts\issue-triage\tests -v
gh extension install github/gh-aw --pin v0.89.21
gh aw compile issue-triage --strict --no-check-update
```

Commit the Markdown source, compiled `.lock.yml`, and `.github/aw/actions-lock.json`
together. Do not hand-edit the generated workflow. The focused CI workflow runs
the tests and checks that the pinned compiler reproduces the committed lockfile.
Product binaries do not need rebuilding for these automation-only changes.

`find_duplicates.py` implements deterministic retrieval and ranking.
`triage.py prepare EVENT_PATH CONTEXT_PATH` builds model input.
`triage.py publish EVENT_PATH AGENT_OUTPUT_PATH` independently validates and
previews unless `TRIAGE_PUBLISH` is explicitly `true`.

Both commands require `GITHUB_REPOSITORY` and a job-scoped `GITHUB_TOKEN`.
For local read-only checks, use an existing authenticated GitHub environment;
never put a token in a fixture, command output, source file, or commit.
