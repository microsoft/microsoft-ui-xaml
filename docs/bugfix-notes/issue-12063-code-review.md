# Code review: #12063 (backdrop XamlCompositionBrushBase crash with a mask or nine grid)

Branch: `user/copilot/issue-12063`

## Round 1 (reviewing 55cd75d08)

| Reviewer | Model | Kind | Result |
|---|---|---|---|
| r1-normal-a | gpt-6.1-sol | Normal | No significant issues |
| r1-normal-b | claude-sonnet-5.5 | Normal | 3 findings (low) |
| r1-adv-a | gpt-5.6-terra | Adversarial | 1 finding (medium) |
| r1-adv-b | gemini-4-argon | Adversarial | 4 findings (medium/low) |

### Findings

| # | From | Severity | Finding | Disposition |
|---|---|---|---|---|
| 1 | r1-adv-a | Medium | Reuse check matches any effect brush with a source parameter of the same name. An app effect brush that used the same name could be mistaken for our wrapper. | **Fixed.** Wrappers now carry a fixed `Comment`. Only brushes with that comment are treated as ours. |
| 2 | r1-adv-b | Low | Reuse check calls `GetSourceParameter` on any effect brush in `Source` (for example from an `AcrylicBrush`). That can log a spurious first-chance error. | **Fixed** by #1. Brushes without our comment aren't probed. |
| 3 | r1-adv-b | Low | When the XCBB hands Xaml a new backdrop instance, a whole new wrapper is created instead of updating the old one. | **Fixed.** The existing wrapper's source parameter is updated in place. New test step `ReplaceBackdrop` covers it. |
| 4 | r1-normal-b, r1-adv-b | Low | Tests don't cover wrapper reuse directly. | **Partly addressed.** Tests now replace the backdrop instance. On a real-DComp run I added a temporary check that the wrapper's `Comment` reads back, so reuse really runs. A permanent assert on reuse would need comp tree inspection, which the mock doesn't expose for this. |
| 5 | r1-normal-b | Low | Add a `TextBlock.Foreground` scenario. Text also goes through the mask path. | **Fixed.** Added `BackdropXcbbTextBlockForeground`. It crashed (0xC000027B) without the fix and passes with it. |
| 6 | r1-normal-b | Low | WUC shapes path (`CShape::PopulateCompositionSpriteShape`) sets the XCBB brush directly and isn't wrapped. | **Out of scope.** Only used when the `WUCShapes` feature is on, and it's off. Noted in the notes. |
| 7 | r1-normal-b, r1-adv-b | Low/Medium | Questioned committing `docs/bugfix-notes` files. | **Rejected.** This is the repo's convention for these branches. |

Adversarial angles tried with no defect:
- Rounded Grid, StackPanel, ContentPresenter, Rectangle, and Path. All reach `GetMaskBrush`.
- Backdrop A to backdrop B, backdrop to non-backdrop, and backdrop nested inside an app effect brush.
- Rendering fidelity (SOURCE_OVER with transparent is the identity), COM identity, host backdrop brush, mock DComp, factory lifetime and device loss, leaks, and other call sites.
- `CompositeEffect` source order (source 0 is `Destination`), and `MuxGetActivationFactory` use.

### Validation after round 1

- Rebuilt MUX and Foundation tests.
- VM, mock DComp: `*BorderTests::*` 19/19 passed.
- VM, real DComp (temporary guard change): `*BorderTests::BackdropXcbb*` 5/5 passed.
- VM, fix disabled: `BackdropXcbbTextBlockForeground` failed (host crash, 0xC000027B).

## Round 2 (reviewing 2c444addd)

| Reviewer | Model | Kind | Result |
|---|---|---|---|
| r2-normal-a | gpt-6-sol | Normal | No significant issues |
| r2-normal-b | claude-opus-5.5 | Normal | No significant issues. No overlapping open PR. |
| r2-adv-a | grok-4.7 | Adversarial | No significant issues |
| r2-adv-b | gpt-6-astra | Adversarial (other XCBB `Source` sites, visual fidelity, false-passing tests) | No significant issues |

No findings, so no code changes.

Adversarial angles tried with no defect:
- Wrapper shared across elements. Sprite visuals come from each element's own render data, and the reuse check only reads this visual's own mask or nine-grid `Source`.
- A wrapper later read as an app brush. No other render path reads the mask or nine-grid `Source`.
- Device loss. The factory is cleared with the compositor, and the brush manager rebinds to the new compositor.
- Islands. One factory per brush manager, which is one per compositor host.
- `existingBrush` of the wrong type (not a mask, not a nine grid). The QI fails, a new brush is created, and a new wrapper is made.
- `ICompositionObject2` missing, or a null or empty comment. Reuse is skipped, which is still correct.
- Switching from a backdrop to Acrylic, solid, gradient, or image. The wrapper is replaced and released.
- Pass-through sampling and source order (source 0 is `Destination`).

After round 2, the notes gained "Why this fixes the issue" and "Why it's safe" sections (3b1f98ac4).
