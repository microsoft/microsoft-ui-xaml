# Area-based reviewer suggestions

`.github/workflows/area-reviewers.yml` requests **optional** reviewers on a PR: when a file is
added or changed, whoever owns that area gets added. Nothing here blocks a merge - the
`* @microsoft/tm-winui` entry in [`CODEOWNERS`](../CODEOWNERS) remains the authoritative
reviewer requirement.

## Files

| File | Purpose |
| --- | --- |
| [`../area-owners.yml`](../area-owners.yml) | The area map: id, name, tier, path globs, reviewers |
| `area-reviewers.mjs` | Matches changed files against the map and requests reviewers |

## How matching works

The script lists the PR's changed files, keeps every area whose globs match at least one of
them, and requests those areas' reviewers. Areas are combined, not ranked - a PR spanning
`dxaml/xcp/core/Parser` and `perf` notifies both P1 and T2. The PR author and anyone already
on the PR are dropped, since GitHub rejects those.

Globs support `**` (crosses directories), `*` (does not) and `?`, matched case-insensitively
against repository-relative paths. Files that match no area simply fall through to CODEOWNERS.

## Dry run

The workflow reads the `AREA_REVIEWERS_DRY_RUN` repository variable and **defaults to `true`**.
While it is `true` the job logs the areas and reviewers it would request to the run summary
without touching the PR. Set the variable to `false` to start requesting for real.

## Editing the map

1. Edit `.github/area-owners.yml`.
2. Check your globs before committing - a glob that matches nothing fails **silently**
   at runtime, so the area's reviewers simply never get requested. From the repository root:

   ```
   git ls-files 'controls/dev/YourArea/*'
   ```

   If that prints nothing, the glob is wrong.
3. Reviewers must be repository collaborators, otherwise the request is rejected and the
   reason is logged in the run summary.
