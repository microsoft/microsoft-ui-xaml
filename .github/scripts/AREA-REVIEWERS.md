# Area-based reviewer suggestions

`.github/workflows/area-reviewers.yml` requests **optional** reviewers on a PR: when a file is
added or changed, whoever owns that area gets added. Nothing here blocks a merge - the
`* @microsoft/tm-winui` entry in [`CODEOWNERS`](../CODEOWNERS) remains the authoritative
reviewer requirement.

## Files

| File | Purpose |
| --- | --- |
| [`../area-owners.yml`](../area-owners.yml) | The area map: id, name, tier, path globs, reviewers. The source of truth for area ownership. |
| [`../workflows/area-reviewers.yml`](../workflows/area-reviewers.yml) | Requests area reviewers on PRs (`area-reviewers.mjs`). |
| [`../workflows/area-owners-check.yml`](../workflows/area-owners-check.yml) | Validates the map on every PR (`validate-area-owners.mjs`). |
| [`../workflows/area-owners-sync.yml`](../workflows/area-owners-sync.yml) | Weekly PR that fixes drift between the map and the tree (`sync-area-owners.mjs`). |
| `area-owners-lib.mjs` | Shared loading, glob matching and validation, so all three agree on what a glob matches. |

## How matching works

The script lists the PR's changed files (for a move, both the old and the new path), keeps
every area whose globs match at least one of them, and requests those areas' reviewers.
Areas are combined, not ranked - a PR spanning `dxaml/xcp/core/Parser` and `perf` notifies
both P1 and T2. Files that match no area simply fall through to CODEOWNERS.

A glob is a path pattern with wildcards ([background](https://en.wikipedia.org/wiki/Glob_(programming))):
`**` matches across directories, `*` and `?` match within a single path segment, everything
else is literal. Matching is case-insensitive against repository-relative paths, so
`controls/dev/ScrollView/**` covers everything under that folder and `*.cmd` only covers
`.cmd` files at the repository root.

## When reviewers are requested

The workflow runs when a PR is opened, reopened or marked ready for review, and on every push.
Each person is requested **at most once per PR**. Someone is skipped when they:

- authored the PR (GitHub rejects self-requests),
- are already requested,
- have already reviewed, or
- were requested earlier - including people the author later removed, so a removal sticks.

So a push that touches a new area only adds that area's owners. Existing reviewers are never
removed.

Reviewers are requested individually, on top of the `tm-winui` team request from CODEOWNERS.
An individual request shows up in the reviewer's own queue - filter pull requests with
`is:open is:pr user-review-requested:@me` (individual requests only, unlike
`review-requested:@me`, which includes team requests) and bookmark the result.

If GitHub refuses a handle (for example, it is not a repository collaborator), everyone else
is still requested and the refused handle is reported as a warning on the run. API failures
fail the run so they do not go unnoticed; the job is not a required check, so a failure never
blocks a merge.

## Dry run

Both the reviewer workflow and the weekly sync ship disabled:

| Variable | Default | Effect while `true` |
| --- | --- | --- |
| `AREA_REVIEWERS_DRY_RUN` | `true` | Logs the areas and reviewers it would request to the run summary without touching the PR. |
| `AREA_OWNERS_SYNC_DRY_RUN` | `true` | Logs the proposed map changes to the run summary without opening a PR. |

Set a repository variable to `false` to turn the behaviour on.

## Keeping the map in sync with the tree

The map lives in one place, so it can drift as code is added, moved or deleted. Two
mechanisms keep it honest:

1. **Every PR** - `area-owners-check.yml` fails if the PR adds files no area covers, leaves a
   glob matching nothing (for example by moving or deleting a folder), or breaks the map's
   format. Drift that already exists on `main` is reported but not blamed on the PR.
2. **Every week** - `area-owners-sync.yml` scans `main` and opens (or updates) a single PR from
   the `bot/area-owners-sync` branch. It only edits *paths*, never reviewers:

   | Finding | Proposed fix |
   | --- | --- |
   | Glob matches nothing, folder of the same name appeared elsewhere | Rewrite the glob (folder moved) |
   | Glob matches nothing | Remove the glob |
   | New folder whose name or commit history points at one area | Add it to that area |
   | New folder with no clear owner | Park it in `UNASSIGNED` and ask a human |
   | `UNASSIGNED` path now covered by a real area | Remove it from `UNASSIGNED` |

   For a new folder, "points at one area" means its name extends a sibling's (`dll-tabular`
   next to `dll/**`) or at least 60% of the sibling areas changed in the same commits as the
   folder belong to one area. The PR description lists the evidence for each change. The sync
   never overwrites the bot branch once a person has pushed to it, and it closes its own PR
   when there is nothing left to sync.

   Opening the PR needs **Allow GitHub Actions to create and approve pull requests** enabled
   in the repository settings. PRs opened with `GITHUB_TOKEN` do not trigger other workflows;
   set the optional `AREA_OWNERS_SYNC_TOKEN` secret (a GitHub App or bot token) if the sync PR
   should run the usual checks. The sync validates the proposed map itself either way.

### The `UNASSIGNED` area

`UNASSIGNED` is a holding area for paths nobody owns yet. It requests no reviewers (CODEOWNERS
applies) and counts as covered, so the PR check stays green, but the validator keeps reporting
how many files are parked there. To assign them, move the path into the right area.

## Editing the map

1. Edit `.github/area-owners.yml`.
2. Validate from the repository root (one-time `npm ci` in `.github/scripts`):

   ```
   cd .github/scripts && npm ci --ignore-scripts && cd ../..
   node .github/scripts/validate-area-owners.mjs
   ```

   This checks the format, reports globs that match nothing, and lists files no area
   covers. Files are read from the git index, so `git add` new files first.
3. Optionally preview what the weekly sync would change, or apply it to your working copy:

   ```
   node .github/scripts/sync-area-owners.mjs           # print the proposal
   node .github/scripts/sync-area-owners.mjs --write   # apply it to area-owners.yml
   ```

4. Reviewers must be repository collaborators, otherwise GitHub refuses the request and the run
   reports a warning naming the handle.
