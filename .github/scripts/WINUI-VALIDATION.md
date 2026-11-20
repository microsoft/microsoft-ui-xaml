# WinUI Quick validation

## Activation hold: prove the configured trust boundary first

The command workflow is explicitly disabled in code, even when
`WINUI_VALIDATION_ENABLED=true`. Normal Azure PR validation is unchanged.

The gate uses a dedicated GitHub App with **Checks: write**, not the shared
GitHub Actions identity. The ruleset must require that exact App ID; an ordinary
workflow token publishing the same name must not satisfy it. The App token is
used only for gate checks. Ref publication, comments and artifact reads retain
the ordinary job token, which has no checks/statuses write permission.

This boundary also depends on administrator configuration: the App private key
must be an environment secret available only to reviewed default-branch code.
Do not remove the existing Azure requirement or enable production validation-ref
CI triggers until that configuration and adversarial end-to-end proof have been
reviewed. Local tests do not establish live isolation or merge eligibility.

### Authentication precedent

| Mechanism | Evidence in WinUI |
| --- | --- |
| Immutable action SHAs | Merged `.github/workflows/docs.yml`, `pr-build.yml` and issue-triage automation |
| Job-scoped `GITHUB_TOKEN`, trusted automation, no persisted checkout credentials | Merged `.github/workflows/area-reviewers.yml` and `area-owners-sync.yml` |
| Comment-command permission checks and reviewed validation refs | Unmerged [PR #11852](https://github.com/microsoft/microsoft-ui-xaml/pull/11852) |
| Azure bearer token via `az account get-access-token` | The same unmerged proposal, using an existing manual Azure login |
| GitHub OIDC through `azure/login` | No merged WinUI workflow precedent established by this review |
| Dedicated checks-only publisher | Standard [GitHub App token action](https://github.com/actions/create-github-app-token) and [Checks API](https://docs.github.com/en/rest/checks/runs); no merged WinUI precedent established |

These are platform-supported mechanisms, not a claim of existing merged WinUI
adoption. Environment restrictions, federation claims, Azure permissions and
protected validation-ref namespaces require configuration verification. The
remaining setup sections are not approval to activate production.

## Basis and scope

This uses the Actions command/validation-ref pattern in
[Galina's PR #11852](https://github.com/microsoft/microsoft-ui-xaml/pull/11852)
and the reduced matrix in
[Jesse's PR #11909](https://github.com/microsoft/microsoft-ui-xaml/pull/11909).
It does not require either PR to be merged first.

There is no hosted controller, database, webhook endpoint or PAT. A dedicated
GitHub App supplies short-lived, repository-scoped check tokens inside GitHub
Actions; it does not need a webhook subscription or separate service. The existing
GitHub-to-ADO connection starts builds from validation-branch pushes. A federated
Entra identity is used only to **read** ADO build metadata and timelines.

Galina's PR reports a successful manual queue/status-association proof, but its
UI-defined CI-trigger path and lifecycle cleanup were not fully proven. Her
source also explicitly disables conflicting native fork runs. We reuse the
pattern, not a claim that every part is already production-validated.

This first version supports custom commands on **in-repository PR branches**.
It does not promote fork code or change fork-secret policies; that remains the
separate `/validate` proposal. Native Full results for forks can still be
evaluated if the existing pipeline supports them. Merge queues are not supported.
Enabling Galina's fork promotion alongside this required gate needs a separate
integration: her head-SHA CI runs are not automatically treated as native
merge-ref Full runs by this helper.

## Developer commands

| Action | Result |
| --- | --- |
| Open/update a PR | Full remains the default |
| `/azp run` | Azure starts matching pipelines; this gate selects native Full |
| `/azp run <registered pipeline name>` | Select native Full for this pipeline only |
| `/winui quick` | Publish a Quick validation ref for the current merge revision |
| `/winui quick <head-sha>` | Same, but refuse if the reviewed head no longer matches |
| `/winui full [<head-sha>]` | Publish a Full validation ref and select that run |

Commands must occupy the entire comment. Edited comments are ignored. Like
Galina's flow, actual repository write/maintain/admin permission is checked,
not `author_association`. An optional Quick approver list narrows that permission.
Prefer a pinned SHA when the PR is actively changing.

Quick compiles **x86 Debug**, retains the full developer test suite in **20
Windows 11 25H2 shards**, and omits scenario apps/tests, other build architectures,
older test OS jobs, the system-composition WPF stage, the static-test stage and
the extra final-release build.

Full preserves the upstream system-composition stage condition: it runs for
non-fork PR builds, but is skipped for CI/manual builds. The gate requires that
stage and its WPF shards when the upstream condition applies.

Within the helper's reconciliation logic, only the selected run can satisfy
**WinUI Validation Gate**. A late Full result cannot overwrite a selected Quick
result, and vice versa. Requiring the dedicated App identity separates this gate
from ordinary workflows; the App credential must not be shared with them. Quick
is never selected merely because it passed. A changed head/base/merge revision resets
selection to Full. Other required checks and reviews still apply.

The normal Azure pipeline context remains diagnostic after the ruleset migration.
The ordinary `WinUI validation` Actions job is **not** the required check: command
events are not necessarily eligible PR-check workflow events. The helper creates
and updates an App-owned check on the tested merge SHA. An uncertain check write
blocks further transactions rather than retrying past a potentially late result.

## How it runs without a server

1. A trusted default-branch workflow checks the command and current PR.
2. It publishes the exact GitHub merge commit, without checking out or executing
   PR code, to `validation/quick-pr/<pr>/c<comment-id>` or
   `validation/full-pr/<pr>/c<comment-id>`. Request refs never move.
3. A pipeline **UI-defined CI trigger** starts the existing pipeline. The Quick
   prefix selects Quick coverage; ordinary PR refs still select Full.
4. The helper matches the exact branch, SHA, pipeline definition, parameters and
   trigger reason, then remembers **one build ID**. It checks the actual stage
   and runtime-shard results before accepting success.
5. A five-minute scheduled sweep discovers PR changes, build completion and
   closed-PR cleanup. Commands also reconcile immediately when processed. Jobs
   do not wait for hours while a build runs. Actions scheduling delays can add
   latency beyond five minutes; use the default-branch manual run to reconcile.

Only `issue_comment`, `schedule` and default-branch `workflow_dispatch` run the
privileged controller. Both the workflow condition and helper require the exact
default-branch workflow/ref. Checkout is pinned to `github.workflow_sha`, which
must equal `github.sha`. PR/status/check-run events are deliberately not triggers:
their run metadata can identify a PR revision rather than the automation commit.
Polling replaces these notifications, not the native Full pipeline triggers.

State is checkpointed in an Actions artifact, not in an external store. Restore
requires the exact workflow, allowed event, default branch and trusted ancestry.
The checkpoint binds its automation SHA and run/attempt to GitHub's run metadata.
Wrong-branch/PR-event artifacts are never downloaded. An expired or invalid
trusted checkpoint fails closed rather than rolling selection back. Legacy
unbound state is not migrated automatically. Start with a fresh approved proof;
existing deployments need operator recovery under the Full requirement.
Before changing any gate, the helper also compares the restored checkpoint with
the latest App-owned checks for current PR merge revisions. Each check records
the selected request's digest as its external ID. Deleting the latest artifact
cannot silently restore an older selection, even if the remaining artifact has
valid provenance. Missing or stale state requires recovery, not a fresh selection.
Unchanged state is not uploaded again. A single workflow-level concurrency group
serializes runs; `queue: max` prevents pending-run replacement.

A short-lived `winui-gate/transactions/<run-id>-<attempt>` ref guards the interval
between publishing checks and saving state. It is deleted only after a safe
checkpoint (or when state did not change). If the runner is interrupted or a
check write has an uncertain outcome, later jobs stop rather than letting a
new result overtake that write. See recovery below.

Quick is refused for shared head commits. If another PR acquires the same head
after acceptance, Quick is blocked; when merge SHAs coincide, all selected
attempts must succeed before that commit can be green.

Quick commands also refuse to run unless the target branch already requires
`WinUI Validation Gate` with exactly `WINUI_PUBLISHER_APP_ID`, not any non-null
App or the shared GitHub Actions App. This is an important rollout guard:
a Quick CI run uses the same Azure definition and can itself satisfy the old
Azure status context. The old context alone is not a reliable Full-only barrier
once Quick runs are available.

## Enablement: administrator steps

### 1. Land the code, with the old gate still required

Keep the existing full-validation policy. The new workflow is disabled by the
activation hold; setting `WINUI_VALIDATION_ENABLED` cannot activate it. Review
the configured publisher boundary before removing that hold in a separate,
approved activation change. The workflow must exist on the default branch for
command/schedule events to work.

Do not restore or deploy the earlier standalone-controller prototype.

### 2. Configure CI on the existing Azure pipeline

For an approved isolated proof only, in the test pipeline's
**Edit > Triggers > Continuous integration** settings,
enable **Override the YAML continuous integration trigger from here**, adding:

```text
validation/quick-pr/*
validation/full-pr/*
```

Preserve unrelated existing filters, the normal PR trigger and fork settings.
Do not add the `winui-gate/*` metadata namespace to CI. Do not depend on a
YAML-only trigger: the file used by a branch push comes from the candidate
commit, which might predate this change.

The candidate commit must contain the new mode-routing YAML. Rebase/update PRs
created before it lands. If a Quick ref builds old Full YAML, the coverage
contract deliberately rejects it rather than treating it as Quick.

### 3. Configure the trusted publisher environment

Register a private GitHub App, installed only on this repository, with
**Checks: read and write** and the mandatory Metadata read permission. Do not
grant it Contents, Workflows, Administration, Actions, or commit-status write
permissions. Disable its webhook; no service is required.

Create the `winui-validation` environment with **selected deployment branches**
allowing only the exact default branch. Do not choose all protected branches.
Store `WINUI_PUBLISHER_PRIVATE_KEY` only as a secret in that environment, never a
repository/organization secret accessible to other branches. Set the numeric
App ID as `WINUI_PUBLISHER_APP_ID`. The pinned token action defaults to the current
repository, explicitly requests only Checks write, and revokes its token on exit.

Protect the default branch and require review for this workflow and its scripts.
Any workflow merged to that branch is inside this trust boundary: the environment
does not identify a single workflow. If that is not an acceptable trust boundary,
do not deploy this design. Do not expose the private key to PR-authored workflows,
reusable workflows with untrusted code inputs, or arbitrary script execution.
Administrators who can change these protections or credentials remain trusted.

### 4. Configure standard workload identity federation

Reuse an approved Entra service principal or create one for this integration.
Add it to the Azure DevOps organization/project with permission to **view the
pipeline, builds and build timelines**. This workflow does not need permission to
queue/cancel builds, edit pipelines or change ADO artifacts.

Using the same default-branch-only `winui-validation` environment, add a
federated credential to the identity:

```text
Issuer:   https://token.actions.githubusercontent.com
Subject:  repo:<owner>/<repository>:environment:winui-validation
Audience: api://AzureADTokenExchange
```

The environment restriction is important: PR-authored workflows must not obtain
the identity. `azure/login` exchanges the GitHub OIDC token; the helper requests
an Azure DevOps bearer token for read-only API calls. No Azure subscription is
required by the workflow's login step.

Set these repository variables:

| Variable | Value |
| --- | --- |
| `WINUI_PUBLISHER_APP_ID` | Numeric dedicated GitHub App ID, also required by the target ruleset |
| `WINUI_AZURE_CLIENT_ID` | Federated identity application/client ID |
| `WINUI_AZURE_TENANT_ID` | Entra tenant ID |
| `WINUI_ADO_ORGANIZATION` | ADO organization name, not a URL |
| `WINUI_ADO_PROJECT` | ADO project name |
| `WINUI_ADO_PIPELINE_ID` | Existing GitHub PR pipeline definition ID |
| `WINUI_ADO_PIPELINE_NAME` | Its exact registered name, including suffixes |
| `WINUI_VALIDATION_TARGETS` | Exact targets; initially a non-production `feature/*` proof branch |
| `WINUI_QUICK_APPROVERS` | Optional comma-separated GitHub logins; all must also have write access |
| `WINUI_VALIDATION_ENABLED` | Leave unset until the above setup is ready |

The App private key is the only new GitHub environment secret. `GITHUB_TOKEN`
supplies non-publisher GitHub permissions. Ensure applicable rules allow it to create/delete its
reserved validation and transaction refs without granting it a bypass for the
protected product branches. Environment/Actions policies must allow the
first-party checkout, Node setup, App token, Azure login and artifact actions.

### 5. Exercise an approved test PR

Only after resolving the security hold, scope `WINUI_VALIDATION_TARGETS` to a
non-production `feature/*` branch covered by
the existing PR trigger. Set `WINUI_VALIDATION_ENABLED=true` and let native Full
validation publish the new gate once. On that test branch, require **both** the
existing Azure context and the new gate, pinned to its approved isolated
publisher identity, before issuing Quick commands.
Confirm:

- The App owns the gate check and the ruleset names its exact ID. A separate
  ordinary workflow token cannot satisfy the requirement with a same-name check
  or status. A feature-branch manual run cannot enter the environment or obtain
  the private key. Use an isolated test target and non-secret probe, not a test
  that prints or exports credentials.
- `/winui quick <sha>` creates the intended ref, the UI CI trigger actually
  starts a run, and the run builds x86 Debug plus all 25H2 runtime shards.
  No build ID within 15 minutes is reported as a trigger/configuration failure.
- The reported ADO timeline identifiers match the contract in
  `winui-validation-policy.mjs`. The local tests cannot establish the live
  OneBranch timeline layout.
- The gate is reported on the PR merge SHA and links to the exact selected run.
  A native Full run completing later, whether successful or failed, does not
  change a Quick selection.
- `/winui full` and `/azp run` restore Full selection; an old Quick success cannot
  satisfy it. New commits, wrong SHA pins, unauthorized or edited commands, and
  failed/canceled/skipped-test runs cannot grant Quick success.
- Command-to-scheduled/manual checkpoint restoration preserves the selected
  build even when a newer matching run exists. Feature-branch/PR-event artifacts
  are rejected, and damaged or expired trusted state stops the controller.
- Interrupted-workflow recovery, shared-head PRs and cleanup on close behave as documented.
- Use a test branch with the new required context to verify **actual merge
  eligibility**, not just green Actions/ADO pages. Keep CLA/reviews required.

PRs changing workflow files may encounter GitHub token restrictions during ref
publication. Treat that as a refusal, not permission to introduce a broader
credential. Prove such changes separately under the existing Full gate.

### 6. Production gate migration remains gated on proof

Do not replace the existing Azure requirement until the isolated test target
proves that the dedicated App alone satisfies the gate, including adversarial
same-name checks and statuses. Never pin the requirement to GitHub Actions or
allow any source. On the test target, migrate to the new gate and demonstrate
actual merge eligibility from selected Quick success without Full success, while
preserving CLA/reviews. Only then approve the same migration for production.

## Operations and rollback

- Expect normal Actions scheduling latency; a command is accepted when the
  workflow acknowledges it. Bare `/azp run` can still start other matching
  Azure pipelines. A canceled/unprocessed command is not an accepted selection.
- Native Full can be waiting on an existing approval or queue. The helper does
  not bypass it. Superseded builds are not automatically canceled.
- Changing the base can create a new merge revision without a native build;
  request `/winui full` if no matching Full run starts.
- Closing a PR removes reserved validation refs unless another open PR uses one
  as its source. Deleting a branch does not cancel an already-running build.
- State artifacts retain 30 days of checkpoints. Do not delete/replace them
  while relying on Quick-selected merge protection.
- If an interrupted transaction blocks the workflow, inspect its named Actions
  run and checkpoint. First restore the old **Full** required check. Resolve any
  outstanding check write before removing that one transaction ref. Reconcile
  under the Full requirement, then repeat the gate proof. Do not auto-delete
  transaction refs or blindly restore older state.
- Rollback: restore the old Full requirement first, set
  `WINUI_VALIDATION_ENABLED=false`, then retire the identity/CI filters if no
  longer used. The original Azure pipeline still reports independently.

## Local checks

From the repository root:

```powershell
npm ci --prefix .github\scripts --ignore-scripts
node --test .github\scripts\tests\winui-validation*.test.mjs
```

Tests cover command authorization, immutable revision binding, exact selected
builds, superseded outcomes, artifact provenance, cleanup and Full/Quick YAML
selection. They use API fakes and do not publish refs or queue live builds.
