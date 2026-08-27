# Issue triage scripts

Deterministic duplicate-issue detection used by
[`.github/workflows/duplicate-issue-detection.yml`](../../workflows/duplicate-issue-detection.yml).

## Behavior

The workflow runs on every issue that is opened, edited, or reopened, **provided the issue
carries `needs-triage`, `bug`, or `feature proposal`**. Both issue templates apply
`needs-triage`, so this covers real reports while excluding tracking-epic sub-tasks, which
are generated in near-identical families and produced almost only false positives during
evaluation.

1. `detect` (read-only) retrieves up to 30 candidate issues through the GitHub search API and
   scores them with deterministic text similarity.
2. `publish` (`issues: write`) runs **only** when at least one candidate scores at or above the
   threshold. It creates or updates a single canonical comment identified by
   `<!-- winui-duplicate-detection:v1 -->`.
3. When the strongest match is **High** confidence, `publish` can additionally submit GitHub's
   native duplicate suggestion, which renders an accept/decline chip on the issue. This step is
   **off by default**; see below.

When no candidate clears the threshold, nothing is commented. The workflow never applies or
removes labels, never closes issues, and never edits the issue body.

## Duplicate suggestion chip

The chip is submitted with:

```
PATCH /repos/{owner}/{repo}/issues/{issue_number}
X-GitHub-Api-Version: 2026-03-10

{
  "state": { "value": "closed", "rationale": "...", "confidence": "HIGH", "suggest": true },
  "state_reason": "duplicate",
  "duplicate_issue_id": <id of the canonical issue>
}
```

`suggest: true` tells GitHub to hold the close for human review rather than applying it. The
approach is adapted from the issue-triage workflow in
[microsoft/PowerToys](https://github.com/microsoft/PowerToys/blob/main/.github/workflows/issue-triage.md).

**This step is disabled by default** (`ENABLE_DUPLICATE_SUGGESTION: "false"`). Direct probing
against a repository without the duplicate-detection preview produced this result:

| Request | Result |
| --- | --- |
| `suggest: true` | HTTP 200, issue stays open, no chip, nothing recorded in REST or GraphQL |
| `suggest: false` | Issue is closed with `state_reason: duplicate` |

The payload is therefore understood, and only the suggestion behavior is gated behind the
preview. Enabling this step on a repository without the preview would produce a green workflow
run that has no effect. Turn it on only after confirming the preview is active, and verify the
chip visually the first time it runs.

Because this relies on a preview API, the step also verifies the response: if GitHub reports the
issue as `closed`, and the closure is attributable to this request, the workflow reopens the
issue and fails.

## Scoring

`similarity()` returns a value in `[0, 1]`:

| Component | Weight | Source |
| --- | --- | --- |
| Title agreement | 0.60 | Token overlap and character ratio of the titles |
| Body agreement | 0.25 | Token overlap of the defect-describing issue-form sections |
| Shared identifiers | 0.15 | Control/API names, exception names, and `0x` error codes |

Version and metadata sections (`NuGet package version`, `Windows version`, `Screenshots`,
`Additional context`) are excluded so two unrelated reports on the same Windows build do not
score as similar. Leading structural title prefixes such as `[WinUI OSS] Phase 4:` are stripped
before titles are compared, because epic sub-tasks share them as boilerplate.
Confidence bands: High `>= 0.80`, Medium `>= 0.70`, otherwise Low.

The default threshold is `0.62`, set through `SIMILARITY_THRESHOLD` in the workflow.

## Safety

* Issue text is untrusted. HTML comments, fenced code, images, links, and control characters are
  stripped before scoring, and mentions and Markdown control characters are escaped before any
  text is republished.
* The detection job has no write permission. The publishing job re-reads the issue state before
  writing and refuses to publish a body that lacks the canonical marker.
* Retrieval failures exit non-zero, so `publish` is skipped rather than posting a misleading
  "no duplicates found" result.
* Third-party actions are pinned to full commit SHAs.

## Running locally

```powershell
$env:GITHUB_TOKEN = (gh auth token)
$env:GITHUB_REPOSITORY = "microsoft/microsoft-ui-xaml"
$env:ISSUE_NUMBER = "11649"
$env:ISSUE_TITLE = "WinUI Key event does not correctly recognize French keyboard special characters"
$env:ISSUE_BODY = ""
python .github/scripts/issue-triage/find_duplicates.py
```

Without `GITHUB_OUTPUT` set, the script prints the result and the rendered comment to stdout
instead of writing workflow outputs.

## Tests

```powershell
python -m unittest discover -s .github/scripts/issue-triage/tests -v
```
