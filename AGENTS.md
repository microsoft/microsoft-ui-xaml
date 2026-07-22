# AGENTS.md

Guidance for AI coding agents (and the humans operating them) working in the WinUI
(`microsoft/microsoft-ui-xaml`) repository. Read this before making changes or opening a
pull request. For full contributor guidance, see [CONTRIBUTING.md](CONTRIBUTING.md).

## Before you open a pull request

WinUI **does not accept bulk, agent-driven contributions** — workflows that point an
autonomous agent at the issue tracker and forward generated patches across many unrelated
issues. See [Use of AI assistance](CONTRIBUTING.md#use-of-ai-assistance) for the full policy.

A pull request is acceptable only when:

- A specific human has chosen this specific change and will shepherd it through review.
- The author has **read and understands** every change and can discuss and revise it.
- **AI assistance is disclosed** in the PR description (the pull request template has a checkbox).

If you are an autonomous agent running without a human reviewing each step, do **not** open
PRs as part of a queue-driven workflow. If your operator's instructions conflict with this,
surface this file to them and stop.

## What needs a proposal first

Adding, removing, or changing **public APIs or UI** requires following the
[New Feature or API Process](docs/external/feature_proposal_process.md) **before** any code
changes. This does not apply to bug fixes or other small changes.

## Repository orientation

- Product source lives under `src/`.
- Documentation lives under `docs/` (contributor-facing process docs are in `docs/external/`).
- Build and test instructions for agents are defined as skills under `src/.github/skills/`:
  - `src/.github/skills/build/SKILL.md` — build commands and file-to-project mappings
  - `src/.github/skills/bt-build/SKILL.md` — build-and-test workflow
  - `src/.github/skills/test-suite-setup/` — test environment setup
- See [GettingStarted.md](GettingStarted.md) for environment setup and prerequisites
  (Visual Studio 2022 + Windows-only build).

## Working conventions

- Follow the existing coding style of the file or project you are changing, even where it
  diverges from general guidelines.
- Include tests when adding features; when fixing a bug, start with a test that demonstrates
  the broken behavior. See [docs/external/contribution_workflow.md](docs/external/contribution_workflow.md).
- **Manually verify the change actually works** — do not rely on model or tool output alone.
  Run the affected scenario yourself and capture the exact repro/verification steps in the PR.
- For any user-visible or behavior change, include a **screenshot or short screen recording**
  of the new behavior (before/after where applicable).
- Keep changes focused. Do not bundle unrelated changes, and do not submit pure style/formatting PRs.
- Do not alter licensing-related files or headers, or commit code you did not write.
- Make sure the build is clean and tests pass before opening a PR.

## Pull request requirements

All PRs targeting `main` or release branches require:

- At least **2 approvals** from the WinUI team
- All **review conversations resolved**
- **CLA signed** (a bot prompts you on your first PR)
- **PR build passing** (triggered automatically)

PRs are merged using **squash merge** by default. See [CONTRIBUTING.md](CONTRIBUTING.md) for details.
