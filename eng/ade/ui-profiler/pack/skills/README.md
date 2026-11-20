# Skills

Put Copilot skills for the inner coding agent here. Each skill is a folder with a `SKILL.md` file.
The runtime passes this folder to the coding agent, which loads a skill when its description matches
the work.

Use a skill for a procedure, for example "how to add a new ranking signal". Use `knowledge\` for rules.

```
skills\
  add-ranking-signal\
    SKILL.md
```

```markdown
---
name: add-ranking-signal
description: Use when adding a new ranking signal to the scorer.
---

1. Add the signal to ...
2. Register it in ...
3. Add a unit test in ...
```

The folder is empty at the start. That is valid.
