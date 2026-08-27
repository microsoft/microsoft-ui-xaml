## Fixes

<!-- Link the issue this PR addresses. Use "Fixes #xxx" or "Closes #xxx" so GitHub auto-closes it on merge. -->
<!-- PRs without a linked issue may be closed unless they are minor documentation/typo fixes. -->

Fixes #

## PR Type

<!-- Check the type of change. Limit each PR to one type when possible. -->

- [ ] Bugfix
- [ ] Feature
- [ ] Code style update (formatting, renaming)
- [ ] Refactoring (no functional changes, no API changes)
- [ ] Build related changes
- [ ] Documentation content changes
- [ ] Other (please describe):

## Description

<!-- Describe your changes in detail. -->

### Current Behavior
<!-- How does this work today? -->

### New Behavior
<!-- How will it work after this PR? -->

## Customer Impact

<!-- How does this change affect end users? Is it user-facing or infra changes? -->

## Regression Potential

<!-- Could this change cause regressions? What areas might be affected? -->

- [ ] Low risk — isolated change, limited scope
- [ ] Medium risk — touches shared components or public APIs
- [ ] High risk — architectural or breaking API change

## How Has This Been Tested?

<!-- Describe how you validated your changes. -->

- [ ] I have performed a self-review of my own code
- [ ] I have added tests to cover my changes
- [ ] Existing tests pass locally

## Screenshots (if appropriate)

<!-- If you are making visual changes, include before/after screenshots. -->

<!--
================================================================================
FORK PR VALIDATION (for maintainers) — required checks won't build on their own.

PRs opened from a fork get no pipeline secrets, so the internal
"WinUI-GitHub-PR (OneBranch)" build fails at "Install Pipeline Tools" (401).

To validate a fork PR:
  1. Review the exact head commit (code AND any pipeline/YAML changes).
  2. Comment `/validate` on the PR (or `/validate <sha>` to pin the reviewed
     commit). Only OWNER/MEMBER/COLLABORATOR comments are honored.

This promotes the fork's identical head commit to an internal validation branch
and runs the trusted build; because both share the head SHA, the check also
lands back on this PR. See .github/workflows/fork-pr-validation.yml.

SECURITY: `/validate` runs the fork's code in a credentialed pipeline, so only
comment it after reviewing that exact SHA. Every new commit invalidates the
previous validation — a maintainer must re-review and comment `/validate` again.
New commits are never validated automatically.
================================================================================
-->
