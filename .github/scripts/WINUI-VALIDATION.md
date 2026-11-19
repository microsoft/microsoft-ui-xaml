# WinUI Quick validation

## Security hold: do not activate the merge gate

The command workflow is explicitly disabled in code, even when
`WINUI_VALIDATION_ENABLED=true`. Normal Azure PR validation is unchanged.

Security review identified a publisher-isolation gap: requiring the **GitHub
Actions** App does not identify this particular workflow. Another repository
workflow with status-write permission can impersonate the same gate. Serialized
execution and selected-run tracking do not prevent that impersonation.

Do not remove the existing Azure validation requirement or enable the production
validation-ref CI triggers based on this prototype. Removing this hold requires
an isolated, trusted publisher (or another independently enforced trust boundary)
and an adversarial end-to-end proof. A dedicated publisher can run in GitHub
Actions without a hosted service, but its authentication integration needs
explicit review; it is not established by the current WinUI precedents.

### Authentication precedent

| Mechanism | Evidence in WinUI |
| --- | --- |
| Immutable action SHAs | Merged `.github/workflows/docs.yml`, `pr-build.yml` and issue-triage automation |
| Job-scoped `GITHUB_TOKEN`, trusted automation, no persisted checkout credentials | Merged `.github/workflows/area-reviewers.yml` and `area-owners-sync.yml` |
| Comment-command permission checks and reviewed validation refs | Unmerged [PR #11852](https://github.com/microsoft/microsoft-ui-xaml/pull/11852) |
| Azure bearer token via `az account get-access-token` | The same unmerged proposal, using an existing manual Azure login |
| GitHub OIDC through `azure/login` | No merged WinUI workflow precedent established by this review |
| Isolated status-publisher identity | No merged implementation precedent established by this review |

Action pinning is fixed, but it does not resolve publisher isolation. Environment
restrictions, federation claims, Azure permissions and protected validation-ref
namespaces also require configuration verification. The remaining setup sections
are design/reference material, not approval to activate production.

## Basis and scope

This uses the Actions command/validation-ref pattern in
[Galina's PR #11852](https://github.com/microsoft/microsoft-ui-xaml/pull/11852)
and the reduced matrix in
[Jesse's PR #11909](https://github.com/microsoft/microsoft-ui-xaml/pull/11909).
It does not require either PR to be merged first.

The current prototype has no hosted controller, database, custom GitHub App,
webhook endpoint or PAT. This does not provide an isolated publisher. Its design
uses GitHub Actions for commands and short reconciliation jobs. The existing
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
result, and vice versa. This is not a security boundary against other workflows
using the same App identity. Quick is never
selected merely because it passed. A changed head/base/merge revision resets
selection to Full. Other required checks and reviews still apply.

The normal Azure pipeline context remains diagnostic after the ruleset migration.
The ordinary `WinUI validation` Actions job is **not** the required check: command
and completion events are not necessarily eligible PR-check workflow events.
The helper explicitly posts the gate commit status on the tested merge SHA.

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
5. Native Azure status/check events and a five-minute scheduled sweep reconcile
   the result. Jobs do not wait for hours while a build runs.

State is checkpointed in an Actions artifact, not in an external store. Only
artifacts from this workflow with a trusted default-branch ancestry are restored.
Unchanged state is not uploaded again. A single workflow-level concurrency group
serializes commands and completion events; `queue: max` prevents the normal
single-pending-run replacement behavior.

A short-lived `winui-gate/transactions/<run-id>-<attempt>` ref guards the interval
between publishing statuses and saving state. It is deleted only after a safe
checkpoint (or when state did not change). If the runner is interrupted or a
status write has an uncertain outcome, later jobs stop rather than letting a
new result overtake that write. See recovery below.

Quick is refused for shared head commits. If another PR acquires the same head
after acceptance, Quick is blocked; when merge SHAs coincide, all selected
attempts must succeed before that commit can be green.

Quick commands also refuse to run unless the target branch already requires
`WinUI Validation Gate` with an expected App. This is an important rollout guard:
a Quick CI run uses the same Azure definition and can itself satisfy the old
Azure status context. The old context alone is not a reliable Full-only barrier
once Quick runs are available.

## Enablement: administrator steps

### 1. Land the code, with the old gate still required

Keep the existing full-validation policy. The new workflow is disabled by the
security hold; setting `WINUI_VALIDATION_ENABLED` cannot activate it. Publisher
isolation must be resolved and reviewed before removing that hold. The workflow
must then exist on the default branch for command/status/schedule events to work.

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

### 3. Configure standard workload identity federation

Reuse an approved Entra service principal or create one for this integration.
Add it to the Azure DevOps organization/project with permission to **view the
pipeline, builds and build timelines**. This workflow does not need permission to
queue/cancel builds, edit pipelines or change ADO artifacts.

Create a GitHub environment named `winui-validation`, restricted to the trusted
default branch. Add a federated credential to the identity:

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
| `WINUI_AZURE_CLIENT_ID` | Federated identity application/client ID |
| `WINUI_AZURE_TENANT_ID` | Entra tenant ID |
| `WINUI_ADO_ORGANIZATION` | ADO organization name, not a URL |
| `WINUI_ADO_PROJECT` | ADO project name |
| `WINUI_ADO_PIPELINE_ID` | Existing GitHub PR pipeline definition ID |
| `WINUI_ADO_PIPELINE_NAME` | Its exact registered name, including suffixes |
| `WINUI_VALIDATION_TARGETS` | Exact targets; initially a non-production `feature/*` proof branch |
| `WINUI_QUICK_APPROVERS` | Optional comma-separated GitHub logins; all must also have write access |
| `WINUI_VALIDATION_ENABLED` | Leave unset until the above setup is ready |

No additional repository secrets are used. `GITHUB_TOKEN` supplies GitHub
permissions. Ensure applicable rules allow the workflow to create/delete its
reserved validation and transaction refs without granting it a bypass for the
protected product branches. Environment/Actions policies must allow the
first-party checkout, Node setup, Azure login and artifact actions used here.

### 4. Exercise an approved test PR

Only after resolving the security hold, scope `WINUI_VALIDATION_TARGETS` to a
non-production `feature/*` branch covered by
the existing PR trigger. Set `WINUI_VALIDATION_ENABLED=true` and let native Full
validation publish the new gate once. On that test branch, require **both** the
existing Azure context and the new gate, pinned to its approved isolated
publisher identity, before issuing Quick commands. This requires the publisher
implementation changes; the current shared-token prototype is not sufficient.
Confirm:

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
- Artifact restoration, interrupted-workflow recovery, shared-head PRs and
  cleanup on close behave as documented.
- Use a test branch with the new required context to verify **actual merge
  eligibility**, not just green Actions/ADO pages. Keep CLA/reviews required.

PRs changing workflow files may encounter GitHub token restrictions during ref
publication. Treat that as a refusal, not permission to introduce a broader
credential. Prove such changes separately under the existing Full gate.

### 5. Production gate migration is blocked

Do not replace the existing Azure validation requirement with the shared-App
status. First establish a trusted publisher unavailable to PR-authored workflows,
require its exact identity, and prove that an ordinary workflow token cannot
satisfy the gate even for the same context and SHA. Preserve CLA, approvals and
other requirements. A safe migration plan must be reviewed with that design;
the previous recommendation to pin the gate to GitHub Actions is withdrawn.

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
  outstanding status write before removing that one transaction ref. Reconcile
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
