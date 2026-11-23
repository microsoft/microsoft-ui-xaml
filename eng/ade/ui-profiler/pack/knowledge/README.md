# Knowledge

Put your team's knowledge here as Markdown files. The runtime selects files for each task and puts
them in the coding agent's instructions. The agent cites the files that it used, and the report shows
the citations.

## How the runtime selects a file

- A file with `always: true` in its front matter is selected for every task.
- Other files are selected when the task text shares words with the file's `keywords`, `title` or
  file name. The runtime selects at most 6 files and at most `limits.maxKnowledgeBytes` of text.
- This `README.md` is never selected.

## File format

```markdown
---
title: Query parsing rules
keywords: query, parser, tokenize, case, normalization
---

# Query parsing rules

- Normalize case in the tokenizer, not in callers.
- ...
```

## What to write

Write short, specific rules that a new engineer would need. Good sources are code review comments that
repeat, incident reviews and design decisions. One topic in each file.

`team-conventions.md` is the starter file. Replace its content with your own rules.
