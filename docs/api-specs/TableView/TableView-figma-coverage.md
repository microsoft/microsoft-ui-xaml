# TableView Figma review coverage

Snapshot date: September 28, 2026. Source: *Fluent Windows Visual Library - IDC*,
page `72491:280393` (Lists & collections).

This ledger accompanies the [requirements draft](./TableView-figma-requirements.md).
**Inventory is not visual review.** The two TableView sheets were exported to PDF.
The 53 supplemental roots below were identified from captured page metadata; their
detailed visual and component-property review is still outstanding. Figma MCP
reached its View-seat call limit, and a subsequent browser export could not finish
when desktop input became unavailable.

The scope includes the TableView area shown in the supplied canvas overview:
the two sheets inside section `183665:50944` and the other top-level roots to its
left (metadata x < -32000). This is a spatial inventory, not an assertion that all
unnamed neighboring frames define TableView requirements. The unrelated ListView,
TreeView, GridView, Expander, and FlipView sheets are not substitute specifications.

## Evidence levels

- **Sheet evidence:** exported text and artwork; does not establish design approval.
- **Note evidence:** a standalone metadata name corroborated by the supplied text.
- **Inventory only:** identity/geometry captured; contents or variant properties
  have not been sufficiently inspected to establish requirements.

Source artwork remains local and is not redistributed in this change.

## TableView sheets

| Source | Node | Evidence |
| --- | --- | --- |
| A | `182972:12908` | TableView sheet, 1392 x 19706; exported text and artwork. |
| B | `183907:68060` | Adjacent TableView sheet, 1392 x 19981; exported text and artwork. |

Neither relative position nor node numbering establishes which sheet supersedes
the other. Differences must be retained as design decisions until an owner
identifies the authoritative version.

### Source-panel crosswalk

The panels below were matched to the exported text and artwork. Both sheets'
visible sections were reviewed, including selection/checkbox variants and their
light/dark examples. This does not inspect hidden properties, prototypes, comments,
or external component definitions.

| Section | A panel / annotation | B panel / annotation |
| --- | --- | --- |
| General | `183090:24568`; compositions `182972:19833` | `183907:68067`; compositions `183907:68217` |
| Header | `183092:26314`; options note `183096:38757` | `183907:68220`; options note `183907:68226` |
| Row types | `183096:50766`; default note `183096:51935` | `183907:68252`; default note `183907:68259` |
| Single/multiple row selection | `183356:29499` | `183907:68456` |
| Checkbox selection | `183356:49361` | `183907:68708` |
| Sections | `183096:63662` | `183907:68971` |
| Columns | `183096:63665`; sizing note `183097:64795` | `183907:69240`; sizing note `183907:69244` |
| Cell state/density/selection | `183613:35424`; design note `183613:35432` | `183907:69491`; design note `183907:69504` |
| Regular / Compact minima | `183624:35678` / `183624:35681` | `183907:69501` / `183907:69503` |
| Cell overflow | `183613:35266`; design note `183613:35273` | `183907:69526`; design note `183907:69533` |
| Cell alignment | `183613:35143` | `183907:69565` |

## Supplemental-root ledger

Every row still needs detailed visual review. The three standalone text roots
have note/name evidence; generic frame names and collapsed instances do not expose
their complete contents.

| # | Node | Captured name / type | Current evidence |
| --- | --- | --- | --- |
| 1 | `183096:52911` | Section / frame | Inventory only |
| 2 | `183008:25011` | Lists & Collections / frame | Inventory only |
| 3 | `183033:14740` | Table / Table / instance | Inventory only; variant unknown |
| 4 | `183015:137873` | Table / frame | Inventory only |
| 5 | `183015:138623` | Frame 3465216 | Inventory only |
| 6 | `183012:124266` | Table / Table / instance | Inventory only; variant unknown |
| 7 | `183012:124333` | Frame 3465210 | Inventory only |
| 8 | `183014:124787` | Row first / section | Inventory only |
| 9 | `183014:134464` | Frame 3465214 | Inventory only |
| 10 | `183014:126561` | Frame 3465213 | Inventory only |
| 11 | `183014:135369` | Frame 3465215 | Inventory only |
| 12 | `183014:135964` | Variant / error / freeze note | Note evidence; exploratory, incomplete |
| 13 | `183027:14533` | Header / loading note | Note evidence |
| 14 | `183019:138639` | Tables style-guide reference / text | Reference title only; destination not reviewed |
| 15 | `183032:14534` | Frame | Inventory only |
| 16 | `183042:138421` | Content | Inventory only |
| 17 | `183045:144305` | Content | Inventory only |
| 18 | `183045:148337` | Content | Inventory only |
| 19 | `183044:142112` | Content | Inventory only |
| 20 | `183101:71855` | Content | Inventory only |
| 21 | `183045:144426` | Content | Inventory only |
| 22 | `183080:43493` | Content | Inventory only |
| 23 | `183042:140242` | Content | Inventory only |
| 24 | `183045:144567` | Content | Inventory only |
| 25 | `183080:45233` | Content | Inventory only |
| 26 | `183044:142233` | Content | Inventory only |
| 27 | `183042:138947` | Content | Inventory only |
| 28 | `183042:139521` | Content | Inventory only |
| 29 | `183042:140363` | Content | Inventory only |
| 30 | `183045:144951` | Content | Inventory only |
| 31 | `183080:45289` | Content | Inventory only |
| 32 | `183044:142410` | Content | Inventory only |
| 33 | `183042:136297` | ROW / frame | Inventory only |
| 34 | `183042:136307` | ROW / frame | Inventory only |
| 35 | `183042:136315` | ROW / frame | Inventory only |
| 36 | `183042:136324` | Table / Table / instance | Inventory only; variant unknown |
| 37 | `183042:136325` | Table / Table / instance | Inventory only; variant unknown |
| 38 | `183042:136326` | Table / Table / instance | Inventory only; variant unknown |
| 39 | `183042:136327` | Table / Table / instance | Inventory only; variant unknown |
| 40 | `183042:136331` | Table / Table / instance | Inventory only; variant unknown |
| 41 | `183042:136332` | Table / Table / instance | Inventory only; variant unknown |
| 42 | `183042:136333` | Table / Table / instance | Inventory only; variant unknown |
| 43 | `183042:129499` | Frame | Inventory only |
| 44 | `183042:138407` | Section/Subtle / symbol | Inventory only; related variants unknown |
| 45 | `183012:124498` | _Cell_archive / frame | Inventory only; explicitly archive-named |
| 46 | `183101:69175` | File Explorer / frame | Application example; contents not reviewed |
| 47 | `183101:89134` | 132 / frame | Inventory only; scenario unknown |
| 48 | `183101:91463` | 133 / frame | Inventory only; scenario unknown |
| 49 | `183132:28108` | image 1 / rectangle | Image pixels not reviewed |
| 50 | `183248:29563` | Frame 3465241 | Inventory only |
| 51 | `183248:30589` | Frame | Inventory only |
| 52 | `183778:87131` | Frame 3465242 | Inventory only |
| 53 | `183778:86942` | Frame 3465226 | Inventory only |

The inventory totals 38 frames, nine instances, one section, three text roots,
one symbol, and one image rectangle. Its subtree counts represent 3,098 nodes;
that number is **not** a count of reviewed visuals or enumerated component variants.

## Completing the review

Inspect each outstanding root's artwork, text, component/variant properties,
and relationship to A/B. Classify it as a requirement source, corroborating example,
alternative, archive, unrelated content, or unresolved design work. Record any
new requirements or conflicts in the main draft without treating proximity or
an example's dimensions as an approved control contract.

Only after this coverage is resolved should a complete implementation-conformance
assessment be claimed. The later gap analysis must distinguish missing behavior,
visual mismatch, intentional deviation, and an unresolved design decision.
