# Team work types

A work type is one kind of work that the ADE can do. The runtime ships four built-in work types:
`bug`, `feature`, `change` and `research`. Add a file here to add a kind of work for your team.
A file with the same `name` as a built-in work type replaces it for your team.

Each work type is a Markdown file with front matter:

| Field | Required | Meaning |
|---|---|---|
| `name` | Yes | Lowercase name. Engineers pass it to `--type`. |
| `description` | Yes | One sentence. The agent uses it to pick the work type. |
| `done` | Yes | `fail-to-pass`, `checks-pass` or `deliverable`. |
| `deliverable` | For `deliverable` | The file name of the document, for example `triage.md`. |
| `sections` | No | Comma-separated headings that the document must contain. |

The body holds the instructions for the agent. Write numbered steps.

Done rules:

- `fail-to-pass`: a check fails before the work and passes after it. Use it for bugs and features.
- `checks-pass`: the team checks pass and a file changed. Use it for refactors and migrations.
- `deliverable`: the agent writes a document in `ade-output\`. No source file may change. Use it for research, triage and design notes.

Knowledge files can target work types with `appliesTo` in their front matter. Example: `appliesTo: [triage, bug]`.

See `triage.md` for an example. Run `ade work types` or `ade doctor` to validate your files.
