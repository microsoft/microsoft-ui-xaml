# XAML element identity in ETW events

## Contents

- [Scope and identity](#scope-and-identity)
- [Typed identity fields and correlation](#typed-identity-fields-and-correlation)
- [Layout, roots, and transitions](#layout-roots-and-transitions)
- [Focus, styles, and hit testing](#focus-styles-and-hit-testing)
- [Text selection and input](#text-selection-and-input)
- [Containers, virtualization, navigation, and counting](#containers-virtualization-navigation-and-counting)
- [RefreshTemplateBindings exclusion](#refreshtemplatebindings-exclusion)
- [Focused regression test](#focused-regression-test)

## Scope and identity

This document inventories the element-identity additions in
[PR #12179](https://github.com/microsoft/microsoft-ui-xaml/pull/12179).
It covers **43 distinct emitted event names, 71 emission sites, and 21 `.cpp` files**.
The typed schema below describes the working-tree update. Source links and emission line numbers
identify the earlier `77c3aa1442b37981f883e3da1faca36a48c0e49c` snapshot; those links do not yet
contain this schema update. Start/Stop calls are grouped into one table row.

**The identity fields are not newly allocated IDs.** They are the correctly typed core object's
addresses cast to `uint64_t`. The ambiguous operation field `ElementId` is replaced by
`cuielement`, `objectpointer`, or both:

- In core code, `this` can already point to the relevant core object.
- In the DirectUI/WinRT layer, `GetHandle()` retrieves the underlying `CDependencyObject*` rather than the wrapper's
  address. It is not a window handle or the peer's `IInspectable` identity.
- An address identifies an object within its process and lifetime. It is not a persistent ID across runs, and an address
  can be reused after the object is destroyed.
- In the tables, **`core(x)`** abbreviates the appropriate cast followed by `x->GetHandle()`. Null-guarded container
  call sites write `objectpointer=0`, without `cuielement`, when no subject is available.

These [event definitions][event-definitions] are compiled under `XAMLPROFILER_ENABLED` and written through
`XamlElementTracing` to **`Microsoft-Windows-XAML`** (`{531a35ab-63ce-4bcf-aa98-f88c7a89e455}`), not
`Microsoft-Windows-XAML-Profiler`.

For activity events, the ETL event name is the name in the table, with Start/Stop distinguished by opcode.
`GenerateContainer` is the exception to carrying identity on both edges: its Start has no identity field because the
container has not yet been resolved. Point events are identified in the tables and accompanying notes.

## Typed identity fields and correlation

Classification uses the actual core object's type, not merely the declared `CDependencyObject*`
type returned by `GetHandle()`. A Button reached through `GetHandle()` is still classified as a UI element.
Every identity field is a 64-bit unsigned integer; exact field names are lowercase.

| Actual subject | Fields emitted | Meaning |
|---|---|---|
| UI element, including controls | `cuielement` | Address of the subject's `CUIElement` base. Controls need no separate pointer for visual-tree correlation. |
| Non-UI dependency object with a known UI association | `cuielement`, `objectpointer` | Associated UI element and the distinct dependency-object subject. |
| Non-UI dependency object without a known association | `objectpointer` only | The subject is identified, but no UI ownership is claimed. |
| Null subject | `objectpointer=0` only | No subject is available; even a supplied owner does not create a subject identity. |
| `GenerateContainer` Start | No identity fields | The container has not yet been resolved. |

Known associations are deliberately limited:

- For a `CTextElement` such as Hyperlink, `GetContainingFrameworkElement()` finds the containing
  TextBlock or RichTextBlock. This preserves both the non-UI text-object identity and its UI host.
  Detached text objects have no `cuielement`. The host is not necessarily the RichTextBlockOverflow
  rendering that part of the text.
- `PrepareContainer` supplies the ItemsControl performing preparation. `SetupCCC` supplies the
  ListViewBase-derived control performing setup. `GenerateContainer` Stop supplies its generator
  host when that host supports `IUIElement`. These owners are used only for non-UI subjects.
- A UI/control subject always uses its own `cuielement`, not the containing ItemsControl.
  No arbitrary parent walk is used to infer ownership of generic dependency objects or shared resources.
- Classification and text-owner lookup occur only while the provider enables verbose events.
  Text-owner lookup does not allocate text pointers or introduce focus-helper fail-fast paths.

This follows the existing four-tree relationship pattern: emit both identities when the relationship
is known, rather than label a dependency-object address as a UI-element address.
Join `cuielement` to the Visual tree's `ElementId`, then use
`CompPeerLinked.UIElementId` / `CompNodeId` and WUC `OwnerCompNodeId` for composition correlation.
For a linked non-UI subject, `objectpointer` remains the event's subject while `cuielement` is its
association. Different names do not themselves create a relationship.

The existing four-tree schemas are unchanged. These operation records remain on
`Microsoft-Windows-XAML`; this producer change does not add operation-event ingestion or display
to the profiler viewer. Consumers must inspect each record's fields, because one event name can
have different identity shapes. Identity establishes object/tree association, not causal attribution
across deferred work, and it does not guarantee every non-UI object has a visual-tree node.

## Layout, roots, and transitions

| ETL event | Location - emission lines | Pointer used | What that element represents |
|---|---|---|---|
| `Layout` | [LayoutManager.cpp][layout-manager]: 252, 525 | `pRoot` | Root returned by `m_pVisualTree->GetRootVisual()`. Identifies the **whole layout pass**, not an individual child. |
| `Measure` | [LayoutManager.cpp][layout-manager]: 297, 304 | `pRoot` | Root of the measure pass, including its subtree. |
| `Arrange` | [LayoutManager.cpp][layout-manager]: 337, 344 | `pRoot` | Root of the arrange pass, including its subtree. |
| `FireSizeChanged` | [LayoutManager.cpp][layout-manager]: 415, 422 | `pRoot` | Layout root whose **batch of size-change notifications** is being dispatched. |
| `FireLayoutUpdated` | [LayoutManager.cpp][layout-manager]: 452, 459 | `pRoot` | Layout root associated with the layout-updated notification pass. **Not each subscriber.** |
| `IndividualSizeChanged` | [LayoutManager.cpp][layout-manager]: 660, 677 | `item.m_pElement` | The **specific element whose size changed** and whose `SizeChanged` event is being raised. |
| `RealizeTransition` | [LayoutManager.cpp][layout-manager]: 943 | `element.get()` | Each element in `m_elementsWithDeferredTransitions`. One point event per element visited. |
| `PutRootVisual` | [VisualTree.cpp][visual-tree]: 438, 528 | `pRoot` | New **public root visual** supplied to `SetPublicRootVisual`; may be null when clearing it. |
| `ProcessLayoutForTransition` | [LayoutTransition.cpp][layout-transition]: 655, 812 | `pTarget` | UI element whose layout change is being processed for a transition. **Not the transition object.** |
| `CancelTransitions` | [LayoutTransition.cpp][layout-transition]: 1425, 1482 | `pTarget` | UI element whose transitions are being canceled. |

## Focus, styles, and hit testing

| ETL event(s) | Location - emission lines | Pointer used | What that element represents |
|---|---|---|---|
| `UpdateFocus` | [focusmgr.cpp][focus-manager]: 1852, 2156 | Start: `movement.GetTarget()`; Stop: `pNewFocus` | Requested focus target at entry; possibly redirected target at exit. **The two IDs can differ.** Stop alone does not prove focus succeeded, because cleanup also handles failures/cancellation. |
| `XYFocusEntered` | [focusmgr.cpp][focus-manager]: 2644, 2647 | `currentFocusedElementOrComponent` | **Starting point of focus navigation**: current focused element, or the hosted component passed in, such as WebView. **Not the next element found.** |
| `GetBuiltInStyle` | [Control.cpp][control]: 1123, 1126 | `this` | The `CControl` whose built-in style is being retrieved. **Not the `CStyle`.** |
| `HitTest` | [uielement.cpp][ui-element]: 13376, 13390 | `this` | `CUIElement` on which `BoundsTestEntry` begins: the root of the tested subtree. **Not necessarily the element ultimately hit.** |
| `OuterBoundsStats`, `InnerBoundsStats`, `ContentBoundsStats`, `ChildBoundsStats` | [uielement.cpp][ui-element]: 13400-13403 | `this` | Same hit-test entry element. These point-event counters describe the hit-test operation, not separate identities for every descendant whose bounds were examined. |

## Text selection and input

| ETL event(s) | Location - emission lines | Pointer used | What that element represents |
|---|---|---|---|
| `ChangeSelection` | [TextSelectionManager.cpp][text-selection-manager]: 3027, 3065 | `m_pOwnerUIElement` | Text control owning the selection manager, such as a TextBlock/RichTextBlock. **Not the manager or gripper.** |
| `ExtendSelectionRange` | [TextSelectionManager.cpp][text-selection-manager]: 3088, 3125 | `m_pOwnerUIElement` | Same owning text control whose selection range is being extended. |
| `TouchSelectionGripperShowBegin` | [Gripper.cpp][gripper]: 1000 | `this` | The `CTextSelectionGripper` being shown: the actual selection handle, not its text control. |
| `TouchSelectionGripperShowEnd` | [Gripper.cpp][gripper]: 467, 1036 | Animation callback: `thisTextSelectionGripper.get()`; immediate path: `this` | Gripper whose show operation completed. The callback obtains it from the storyboard's target. **Not the storyboard/sender.** |
| `TouchSelectionGripperHideBegin`, `TouchSelectionGripperHideEnd` | [Gripper.cpp][gripper]: 1066, 1121 | `this` | Gripper being hidden / now hidden. |
| `TouchSelectionGripperReposition` | [Gripper.cpp][gripper]: 870 | `this` | Gripper whose position changed. |
| `SendInputScopeToRichEdit` | [PasswordBox.cpp][password-box]: 734 | `this` | `CPasswordBox` sending its input scope to text services. **Not the RichEdit service object.** |
| `SetIhmKeyboardSkin` | [PasswordBox.cpp][password-box]: 759, 771 | `this` | `CPasswordBox` requesting the BitLocker or default keyboard skin. **Not the keyboard/window.** |

The gripper `Begin`/`End` names above are **individual point-event names**, unlike the activity events that share one
name and use Start/Stop opcodes. `SendInputScopeToRichEdit` and `SetIhmKeyboardSkin` are also point events.

## Containers, virtualization, navigation, and counting

| ETL event | Location - emission lines | Pointer used | What that element represents |
|---|---|---|---|
| `GenerateContainer` | [ItemContainerGenerator_Partial.cpp][item-container-generator]: 1354, 1491 | Start: **no ID**; Stop: `core(spContainer)` | Container generated or reused by `Generator::GenerateNext`. **Not the generator or data item.** For a non-UI container, its UI generator host is supplied separately when available. Stop uses `objectpointer=0` if no container exists. |
| `GenerateMCContainer` | [ModernCollectionBasePanel_IICG2_Partial.cpp][modern-panel-generator]: 397 | `core(spContainer)` | Container realized for `indexInItemCollection`. A point event emitted before ownership is transferred to the caller. |
| `GenerateItems` | [ModernCollectionBasePanel_Partial.cpp][modern-panel]: 1010, 1018 | `GetHandle()` | The **ModernCollectionBasePanel itself**, identifying the overall generation operation, not an individual generated container. |
| `PlaceElement` | [ModernCollectionBasePanel_ContainerMapper_Partial.cpp][container-mapper]: 342, 350 | `core(spElement)` | Realized element being placed into the container manager's valid-elements mapping for `dataIndex`. **Not the container manager.** |
| `PrepareContainer` | [ItemsControl_Partial.cpp][items-control]: 1816, 1851 | `core(pContainer)` | Subject: container being prepared, not the data item. A non-UI container also carries the preparing ItemsControl's `cuielement`. |
| `SetupCCC` | [ListViewBase_Partial_ContainerPhase.cpp][container-phase]: 115, 325 | `core(container)` | Subject: container whose `ContainerContentChanging` processing is being set up. A non-UI container also carries the ListViewBase-derived owner's `cuielement`. |
| `VirtualizationAdd` | [OrientedVirtualizingPanel_Partial.cpp][oriented-panel]: 1486, 1568 | `core(pChild)` | Child container being added from the generator. |
| `VirtualizationCleanup` | [OrientedVirtualizingPanel_Partial.cpp][oriented-panel]: 1992, 2000 | `GetHandle()` | The **OrientedVirtualizingPanel** performing container cleanup. Not each container removed. |
| `UpdateLogicalScrollData` | [OrientedVirtualizingPanel_Partial.cpp][oriented-panel]: 1153, 1161 | `GetHandle()` | The **OrientedVirtualizingPanel** updating its logical scrolling information. |
| `MeasureChild` | [OrientedVirtualizingPanel_Partial.cpp][oriented-panel]: 2694, 2725 | `core(pChild)` | Specific child/container being measured. **Not the panel.** |
| `VirtualizationMeasure` | [WrapGrid_Partial.cpp][wrap-grid]: 277, 345 | `GetHandle()` | The **WrapGrid itself**, whose `MeasureOverride` is running. |
| `VirtualizationIsEnabledByLayout` | [ListViewBase_Partial.cpp][list-view-base]: 815 | `GetHandle()` | The **ListViewBase-derived control**, such as ListView/GridView, whose virtualization state is reported by a point event. Not its items panel or container. |
| `FrameNavigating` | [Frame_Partial.cpp][frame]: 873 | `GetHandle()` | The **Frame** raising the navigating event. **Not the destination Page.** Point event. |
| `FrameNavigated` | [Frame_Partial.cpp][frame]: 835 | `GetHandle()` | The **Frame** raising the navigated event. **Not the new Page.** Point event. |
| `HubSectionCount` | [Hub_Partial.cpp][hub]: 177 | `GetHandle()` | The **Hub** whose section collection changed. Not an individual HubSection. Point event. |
| `GetElementCount` | [DependencyObject.cpp][dependency-object]: 287, 300 | `m_owner->GetHandle()` | DependencyObject owning the memory-info tear-off: the starting object of the descendant-count walk. **Not the tear-off or each descendant counted.** |

## RefreshTemplateBindings exclusion

The new `RefreshTemplateBindings` instrumentation was reverted and is excluded from this inventory.
The [original unconditional code][refresh-template-bindings] remains:
`TraceRefreshTemplateBindingsBegin/End` and the `Control_RefreshTemplateBindings` TraceLogging calls.
The latter already carry `ObjectPointer = reinterpret_cast<uint64_t>(this)`, identifying the `CControl` whose
template bindings are being refreshed.

## Focused regression test

From an initialized repository:

```powershell
.\initrun.ps1 python .\tools\XamlProfiler\tests\test_operation_identities.py
```

The test compiles the production operation emitters, identity resolver, and containing-text-host
resolver against core-object/TraceLogging test doubles. It covers all 43 event names, retained extra
payloads and opcodes, adjusted base-pointer addresses, controls reached as dependency objects,
TextBlock/RichTextBlock associations, detached text, explicit container owners, unlinked/null subjects,
the identity-free Start, and disabled-provider behavior. It does not replace an application ETW capture
for validating transport or joins against a live tree.

[event-definitions]: https://github.com/microsoft/microsoft-ui-xaml/blob/77c3aa1442b37981f883e3da1faca36a48c0e49c/dxaml/xcp/components/base/inc/XamlProfilerTracing.h#L339
[layout-manager]: https://github.com/microsoft/microsoft-ui-xaml/blob/77c3aa1442b37981f883e3da1faca36a48c0e49c/dxaml/xcp/core/layout/LayoutManager.cpp#L252
[visual-tree]: https://github.com/microsoft/microsoft-ui-xaml/blob/77c3aa1442b37981f883e3da1faca36a48c0e49c/dxaml/xcp/core/dll/VisualTree.cpp#L438
[layout-transition]: https://github.com/microsoft/microsoft-ui-xaml/blob/77c3aa1442b37981f883e3da1faca36a48c0e49c/dxaml/xcp/core/core/elements/LayoutTransition.cpp#L655
[focus-manager]: https://github.com/microsoft/microsoft-ui-xaml/blob/77c3aa1442b37981f883e3da1faca36a48c0e49c/dxaml/xcp/core/dll/focusmgr.cpp#L1852
[control]: https://github.com/microsoft/microsoft-ui-xaml/blob/77c3aa1442b37981f883e3da1faca36a48c0e49c/dxaml/xcp/core/core/elements/Control.cpp#L1123
[ui-element]: https://github.com/microsoft/microsoft-ui-xaml/blob/77c3aa1442b37981f883e3da1faca36a48c0e49c/dxaml/xcp/core/core/elements/uielement.cpp#L13376
[text-selection-manager]: https://github.com/microsoft/microsoft-ui-xaml/blob/77c3aa1442b37981f883e3da1faca36a48c0e49c/dxaml/xcp/core/text/common/TextSelectionManager.cpp#L3027
[gripper]: https://github.com/microsoft/microsoft-ui-xaml/blob/77c3aa1442b37981f883e3da1faca36a48c0e49c/dxaml/xcp/core/text/common/Gripper.cpp#L467
[password-box]: https://github.com/microsoft/microsoft-ui-xaml/blob/77c3aa1442b37981f883e3da1faca36a48c0e49c/dxaml/xcp/core/native/text/Controls/PasswordBox.cpp#L734
[item-container-generator]: https://github.com/microsoft/microsoft-ui-xaml/blob/77c3aa1442b37981f883e3da1faca36a48c0e49c/dxaml/xcp/dxaml/lib/ItemContainerGenerator_Partial.cpp#L1354
[modern-panel-generator]: https://github.com/microsoft/microsoft-ui-xaml/blob/77c3aa1442b37981f883e3da1faca36a48c0e49c/dxaml/xcp/dxaml/lib/ModernCollectionBasePanel_IICG2_Partial.cpp#L397
[modern-panel]: https://github.com/microsoft/microsoft-ui-xaml/blob/77c3aa1442b37981f883e3da1faca36a48c0e49c/dxaml/xcp/dxaml/lib/ModernCollectionBasePanel_Partial.cpp#L1010
[container-mapper]: https://github.com/microsoft/microsoft-ui-xaml/blob/77c3aa1442b37981f883e3da1faca36a48c0e49c/dxaml/xcp/dxaml/lib/ModernCollectionBasePanel_ContainerMapper_Partial.cpp#L342
[items-control]: https://github.com/microsoft/microsoft-ui-xaml/blob/77c3aa1442b37981f883e3da1faca36a48c0e49c/dxaml/xcp/dxaml/lib/ItemsControl_Partial.cpp#L1816
[container-phase]: https://github.com/microsoft/microsoft-ui-xaml/blob/77c3aa1442b37981f883e3da1faca36a48c0e49c/dxaml/xcp/dxaml/lib/ListViewBase_Partial_ContainerPhase.cpp#L115
[oriented-panel]: https://github.com/microsoft/microsoft-ui-xaml/blob/77c3aa1442b37981f883e3da1faca36a48c0e49c/dxaml/xcp/dxaml/lib/OrientedVirtualizingPanel_Partial.cpp#L1153
[wrap-grid]: https://github.com/microsoft/microsoft-ui-xaml/blob/77c3aa1442b37981f883e3da1faca36a48c0e49c/dxaml/xcp/dxaml/lib/WrapGrid_Partial.cpp#L277
[list-view-base]: https://github.com/microsoft/microsoft-ui-xaml/blob/77c3aa1442b37981f883e3da1faca36a48c0e49c/dxaml/xcp/dxaml/lib/ListViewBase_Partial.cpp#L815
[frame]: https://github.com/microsoft/microsoft-ui-xaml/blob/77c3aa1442b37981f883e3da1faca36a48c0e49c/dxaml/xcp/dxaml/lib/Frame_Partial.cpp#L835
[hub]: https://github.com/microsoft/microsoft-ui-xaml/blob/77c3aa1442b37981f883e3da1faca36a48c0e49c/dxaml/xcp/dxaml/lib/Hub_Partial.cpp#L177
[dependency-object]: https://github.com/microsoft/microsoft-ui-xaml/blob/77c3aa1442b37981f883e3da1faca36a48c0e49c/dxaml/xcp/dxaml/lib/DependencyObject.cpp#L287
[refresh-template-bindings]: https://github.com/microsoft/microsoft-ui-xaml/blob/77c3aa1442b37981f883e3da1faca36a48c0e49c/dxaml/xcp/core/core/elements/Control.cpp#L949
