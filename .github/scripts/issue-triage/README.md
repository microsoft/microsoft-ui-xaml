# Issue triage automation

The `AI Issue Triage` workflow prepares a bounded issue context, uses GitHub
Copilot to analyze it, and independently verifies the result before making any
issue changes. The Markdown workflow is the source of truth; its compiled
`issue-triage.lock.yml` is the workflow GitHub Actions executes.

## Behavior

| Situation | Behavior |
| --- | --- |
| Clear primary area, no existing area label | Add one existing `area-*` label at HIGH confidence. |
| One area with a configured owner, no existing team label | Derive and add its `team-*` label from `area-team-map.json`. |
| Uncertain area or an unmapped team | Comment with the suggestion; leave the uncertain routing to a maintainer. |
| Existing area or team labels | Preserve them. Do not replace a human decision or add a contradictory mapped area. |
| Multiple existing areas | Preserve all of them; do not infer one team. |
| Bug with insufficient reproduction information | Ask for the specific missing information and add `needs-repro`. |
| Missing version but otherwise actionable reproduction | Ask for the version in the comment; do not add `needs-repro` for that alone. |
| Feature proposal | Assess the problem and desired outcome, not bug repro requirements. |
| Strong evidence of a duplicate | Suggest up to five older issues in the canonical comment, for human confirmation. |
| Subsequent author follow-up or title/body edit | Re-analyze and update the same bot-owned comment. |
| Invalid model output, API failure, or changed input | Fail closed without publishing that result. |

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

## Ownership mapping

Ownership changes are reviewed in pull requests, like other configuration
changes. The workflow uses the checked-in `area-team-map.json` immediately;
there is no separate mapping approval or activation step.

The [public issue-label evidence](area-team-map.md) supports reviewing the initial
map, which covers all 123 area labels: 65 have a proposed owner and
58 explicitly use `null` because the sampled history is sparse or conflicting.
These are reviewable historical inferences, not an authoritative organization
chart. No team is inferred by the model at runtime.

Maintainers can change individual mappings in `area-team-map.json` as ownership
changes. An absent or `null` entry means "do not auto-assign a team." Team names
must already exist in the live label catalog. The publisher fails visibly if a
configured team has been removed rather than creating or guessing a replacement.

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
- Only allowlisted routing labels and `needs-repro` can be added, at most three
  per run. The publisher uses additive label writes, never replacement.
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
