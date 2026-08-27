# Issue triage scripts

Deterministic duplicate-issue detection used by
[`.github/workflows/duplicate-issue-detection.yml`](../../workflows/duplicate-issue-detection.yml).

## Behavior

The workflow runs on every issue that is opened, edited, or reopened.

1. `detect` (read-only) retrieves up to 30 candidate issues through the GitHub search API and
   scores them with deterministic text similarity.
2. `publish` (`issues: write`) runs **only** when at least one candidate scores at or above the
   threshold. It creates or updates a single canonical comment identified by
   `<!-- winui-duplicate-detection:v1 -->` and applies the `possible-duplicate` label.

When no candidate clears the threshold, nothing is commented and no label is applied.
Issues are never closed and no resolution label is applied.

## Scoring

`similarity()` returns a value in `[0, 1]`:

| Component | Weight | Source |
| --- | --- | --- |
| Title agreement | 0.60 | Token overlap and character ratio of the titles |
| Body agreement | 0.25 | Token overlap of the defect-describing issue-form sections |
| Shared identifiers | 0.15 | Control/API names, exception names, and `0x` error codes |

Version and metadata sections (`NuGet package version`, `Windows version`, `Screenshots`,
`Additional context`) are excluded so two unrelated reports on the same Windows build do not
score as similar. Confidence bands: High `>= 0.80`, Medium `>= 0.70`, otherwise Low.

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
