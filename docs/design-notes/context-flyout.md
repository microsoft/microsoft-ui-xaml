# ContextFlyout Property

**This document is mostly AI-generated.  AI makes mistakes.**

## Overview

`UIElement.ContextFlyout` is a sparse dependency property. When the user
right-clicks (or otherwise requests a context menu), `OnContextRequestedCore`
calls `ShowAt()` on the flyout, which opens a `Popup` in the visual tree.

## Default Value for TextBlock and RichTextBlock

For most UIElement types the default is `null`. `CTextBlock` and
`CRichTextBlock` resolve a `TextCommandBarFlyout` from the theme resource
dictionary under the key `"TextControlCommandBarContextFlyout"` (added in May 2018. 
*Microsoft contributors: ADO PR 1818094*).

This default is **not stored** in sparse storage. `GetValueByIndex` computes
it on-the-fly via `GetDefaultValue`:

```
GetValueByIndex(UIElement_ContextFlyout)
  -> GetDefaultValue
    -> GetDefaultTextControlContextFlyout
      -> GetTextControlFlyoutResource("TextControlCommandBarContextFlyout")
        -> applicationResourceDictionary->GetKeyNoRef() or LookupThemeResource()
```

Because `GetTextControlFlyoutResource` checks `Application.Resources` first,
an app can override the default by defining its own resource with that key.

## Enter/Leave for ContextFlyout

ContextFlyout is a sparse property with `IsVisualTreeProperty() == true`, so
there are two standard Enter paths for locally-set values:

1. **SetValue path**: `SetEffectiveValueInSparseStorage` -> `EnterEffectiveValue`
   -> `flyout->Enter` (if the owner is already live).

2. **Tree entry path**: `CDependencyObject::EnterImpl` -> `EnterSparseProperties`
   -> iterates sparse storage -> `flyout->Enter`.

Neither path triggers for default values, since defaults are not in sparse
storage. The system-provided `TextControlCommandBarFlyout` does not carry
user-defined keyboard accelerators and does not need an explicit Enter.

### GetContextFlyout() in EnterImpl (July 2017)

Flyouts are not part of the visual tree, so keyboard accelerators on flyout
menu items were treated as zombies.
We fixed this by adding an explicit `GetContextFlyout()` + `Enter` in
`CUIElement::EnterImpl`:

```cpp
CFlyoutBase* pFlyoutBase = GetContextFlyout();
if (pFlyoutBase)
{
    EnterParams newParams(params);
    newParams.visualTree = nullptr;
    IFC_RETURN(pFlyoutBase->Enter(pNamescopeOwner, newParams));
}
```

This Enter uses `visualTree = nullptr` because the flyout can be shared across
ContentRoots. `CUIElement::LeaveImpl` had a matching `GetContextFlyout()` + `Leave`.

Note: `CButton::EnterImpl` does the same thing for `Button.Flyout` but passes
the **real** `EnterParams` with a valid `visualTree`.

*For Microsoft contributors, see ADO PR 655767, ADO Bug 19548424*

## Keyboard Accelerators on Default ContextFlyout

If an app overrides `"TextControlCommandBarContextFlyout"` in
`Application.Resources` with a custom flyout that has keyboard accelerators,
those accelerators have **never worked** in WinUI3
([microsoft-ui-xaml#11025](https://github.com/microsoft/microsoft-ui-xaml/issues/11025)).

Locally-set ContextFlyouts do not have this problem: they are entered via
`EnterSparseProperties` with valid `EnterParams` (including `visualTree`), so
their accelerators register on the correct ContentRoot.

### Root cause

The default ContextFlyout is not in sparse storage, so it is only entered by
the explicit `GetContextFlyout()` + Enter in `CUIElement::EnterImpl`, which
uses `visualTree = nullptr`. This means
`CKeyboardAcceleratorCollection::EnterImpl` cannot find the correct ContentRoot
to register accelerators on:

1. `GetContentRootForElement(this)` reads `element->GetVisualTree()` -- **null**.
2. Parent walk fails (flyout is not a visual child).
3. Fallback `Unsafe_IslandsIncompatible_CoreWindowContentRoot()` returns the
   vestigial CoreWindow ContentRoot -- not the island's ContentRoot.

The accelerator IS registered, but on the wrong ContentRoot.
`KeyboardInputProcessor::ProcessKeyEvent` dispatches accelerators via
`contentRoot->GetAllLiveKeyboardAccelerators()` on the **island's** ContentRoot,
which has an empty list.

By contrast, `CButton::EnterImpl` passes a valid `visualTree`, so
`Button.Flyout` accelerators register on the correct ContentRoot and work.

### When accelerators appear to work

Opening the context menu via `OnContextRequestedCore` -> `ShowAt()` creates a
`Popup` in the live visual tree. If the accelerator is on the **focused** menu
item (typically the first item), `ProcessLocalAccelerators` finds it directly
on the focused element during the KeyDown event bubble -- this bypasses the
global accelerator collection entirely. This can make it appear that the global
registration is working, but it's actually the local path.

Accelerators on non-focused items (e.g. the second `MenuFlyoutItem`) are NOT
found by `ProcessLocalAccelerators` and would need the global collection. But
the global registration is on the wrong ContentRoot, so they don't fire.

Additionally, if the flyout was previously entered by a `ResourceDictionary`
(e.g. `Application.Resources->Insert`), its children are already marked as
active. When the Popup later does a live Enter, `CDependencyObject::Enter`
sees they're already active and skips re-entering -- so the accelerators never
get re-registered on the island's ContentRoot.

## Performance Optimization

The `GetContextFlyout()` call in `EnterImpl`/`LeaveImpl` became a performance
problem after we gave TextBlock/RichTextBlock a default ContextFlyout
(*Microsoft contributors, see ADO PR 1818094*).
`GetContextFlyout()` calls `GetValueByIndex`, which triggers the expensive
`GetTextControlFlyoutResource` lookup on every tree entry -- even when no
ContextFlyout was explicitly set. This showed up as a measurable cost on pages
with many CTextBlock elements.

The explicit Enter was also **redundant** for locally-set ContextFlyouts, which
are already entered by `EnterSparseProperties`. And as described above, the
Enter uses `visualTree = nullptr` so the keyboard-accelerator registration
doesn't work anyway.

We skip the `GetContextFlyout()` + Enter/Leave in `EnterImpl`/`LeaveImpl`
behind `IsPerfOptInEnabled()`. This removes the expensive default-value lookup
with no observable behavior change.

### Edge case: Application.Resources override

If an app overrides `"TextControlCommandBarContextFlyout"` in
`Application.Resources`, that custom flyout becomes the default for all
TextBlock/RichTextBlock elements. Even with the old code, keyboard accelerators
on that flyout did not work (the `visualTree = nullptr` problem means they
register on the wrong ContentRoot). Our optimization does not change the
observable behavior.

### Tests

| Test | PerfOptIn | Verifies |
|------|-----------|----------|
| `VerifyAppResourceOverrideContextFlyoutAcceleratorsPerfOptInOn` (native) | On | Accel on non-focused item does not fire (before open or while open) |
| `VerifyAppResourceOverrideContextFlyoutAcceleratorsPerfOptInOff` (native) | Off | Same behavior |

