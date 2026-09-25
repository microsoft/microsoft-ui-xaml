# Goal

Design, implement, and test Window Placement Persistence in the WinUI repository.
Keep product and documentation diffs small and easy for people to review.

# Loop contract

1. Read `inbox.md` and take the first unchecked task. Work on only that task.
2. Treat the bracketed value as a task label, not a persona. Do not invent a role or
   search for unrelated work.
3. Use the task's acceptance criteria as the definition of done. Do not expand scope
   unless a directly coupled fix is required for correctness.
4. Preserve existing work. Never stash, reset, discard, or overwrite uncommitted changes.
   If they belong to the task, continue them. If unrelated changes prevent safe work,
   mark the task blocked and exit.
5. Make the smallest complete change and run the narrowest existing validation that
   proves the acceptance criteria.
6. Re-read `inbox.md` before editing it. Mark the task `[x]` only when its acceptance
   criteria are met. Mark it `[!]` with the blocker and exact next action when it cannot
   proceed.
7. Add follow-up work to the end of the queue as a new unchecked task with explicit
   acceptance criteria. Do not add acknowledgments or status-only handoffs.
8. Commit only meaningful product, test, documentation, or tooling changes. Do not make
   a commit solely to update the queue, acknowledge another task, or record that a review
   found no issues.
9. Add to `case-studies.md` only for a reproducible failure, an important rejected
   approach, or a durable diagnostic result.
10. Exit after completing or blocking one task. The runner will start the next iteration.

# Task labels

- `[Code]`: Product implementation and focused tests.
- `[Test]`: Test review, execution, and coverage gaps.
- `[Review]`: Correctness review of a named change. Fix high-confidence issues directly.
- `[Docs]`: A specific decision or documentation correction.
- `[Infra]`: A reproducible build, test, deployment, or loop failure.

# Safety

- Keep environment changes process-local.
- Do not repair machine-wide tools, change system settings, elevate privileges, or
  disrupt other agents, builds, installers, or VMs.
- Do not hide failures by skipping work or weakening acceptance criteria.
