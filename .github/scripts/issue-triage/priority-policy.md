# WinUI triage priority policy

This table is the single source of truth for model impact classification and
deterministic publisher priority selection. Rows are ordered by impact
precedence: select the first applicable impact when multiple impacts apply.

P0 means Shipblocker; P1 means Must Fix in this release; P2 means Should Fix.
`None` means defer priority for maintainer review.

| Impact | Issue kinds | Report evidence | Without reported workaround | With reported workaround |
|---|---|---|---|---|
| CRASH | BUG | Actual process crash, termination or fail-fast; not a caught exception, hang, comparison or requested crash-prevention feature. | P0 | P1 |
| DATA_LOSS | BUG | Concrete loss or corruption of user data; not a cosmetic display problem or a data-protection feature request. | P0 | P1 |
| SECURITY | BUG | Concrete security impact; not a keyword, generic concern or security-enhancement request. | P0 | P1 |
| PUBLIC_API_BREAK | BUG | Concrete compatibility break in a previously supported public API contract; not simply a missing desired API. | P0 | P1 |
| REGRESSION | BUG | Evidence that previously working behavior stopped working, in any release or main-branch build; broken behavior or a regression label alone is insufficient. | P1 | P1 |
| ADOPTION_BLOCKER | BUG, FEATURE | Concrete evidence that the issue prevents adoption or migration; a general feature request is insufficient. | P1 | P1 |
| BROKEN_FUNCTIONALITY | BUG | A capability fails or behaves incorrectly without meeting a higher-impact row. | P1 | P2 |
| NICE_TO_HAVE | BUG, FEATURE | Clearly classified lower-impact bug, feature or enhancement that does not meet a higher-impact row. | P2 | P2 |
| OTHER | BUG, FEATURE, OTHER | Questions or unsupported/unclear impact. | None | None |

A workaround is a reported possible bypass of the same failure, not a proposed
product fix, unrelated sample, failed attempt or negated workaround. No mentioned
workaround means no known workaround for assessment, not proof that none exists.
Attribute unverified impact and workarounds to their authors. Concrete
report-supported impact is sufficient without maintainer confirmation; keywords,
requested priorities or claims of importance alone are not evidence.

Existing Type/Priority values remain authoritative. Conflicting issue Type,
bug assessment contradicting feature intake, missing rationale and truncated
context defer priority. The assessment does not promise a release commitment
or claim that a project update succeeded.
