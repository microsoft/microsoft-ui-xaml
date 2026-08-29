# ThemeResource API

{ThemeResource} is currently only exposed through markup. This document talks about what an API for it would look like.

[[_TOC_]]

## Exploration Questions

### What ThemeResource-related objects are already available in the core and DXaml layers?

Several internal types exist today, but **none are projected**. There is no IDL for any ThemeResource type, the
`ThemeResource` type is registered without `IsPublic`, and its `ResourceKey` property is non-public. So code cannot
currently construct or reference any of them.

**Core layer (`dxaml/xcp`)**

- **`CThemeResourceExtension`** (`core/inc/ThemeResourceExtension.h`, `core/core/elements/ThemeResourceExtension.cpp`) -
  the markup extension.
  - Derives from `CMarkupExtensionBase`
  - Created when the parser encounters `{ThemeResource ResourceKey=...}`.
  - Implements `ProvideValue`, `LookupResource`, `ResolveInitialValueAndTargetDictionary`, and theme-change
    notification.
  - Is in the type system (`KnownTypeIndex::ThemeResource`), registered in `StaticMetadata.g.cpp` with flags
    `IsMarkupExtension | IsConstructible | ExecutedClassConstructor` (notably **not** `IsPublic`).
- **`CThemeResource`** (`components/theming/inc/ThemeResource.h`, `components/theming/ThemeResource.cpp`) - the
  lightweight ref-counted runtime binding object.
  - Not a `CDependencyObject` and not in the type system / metadata.
  - Holds the resource key, a weakref to the target dictionary, the last-resolved value, and the theme-walk cache.
  - This is what actually lives on a target DO (tracked via `ThemeResourceMap = vector_map<KnownPropertyIndex,
    xref_ptr<CThemeResource>>` in `CDOAssociative.h`) and re-resolves on theme change.
- **`ThemeWalkResourceCache`** (`components/theming`)
  - Caches resolved values per `(dictionary, key)` during a theme walk so multiple bindings to the same key share one
    lookup/object.

**DXaml / framework layer (`dxaml/lib`)**

- **`ThemeResourceExpression`** (`dxaml/lib/ThemeResourceExpression.{h,cpp}`) - the managed-side expression (peer of
  internal COM interface `IThemeResourceExpression`, derived from `BindingExpressionBase`).
  - Wraps a `CThemeResource*`.
  - This is the `EffectiveValueEntry` expression stored on a `DependencyObject` when a theme-resource binding is live.
- **`ThemeResourceExtension`** (proxy) (`src/XamlCompiler/.../TypeProxyMetadata.cs`) - a build-time XAML-compiler proxy,
  not a runtime object.

The natural candidate to expose to code is `CThemeResourceExtension` (precedent: `MarkupExtension` is already a public,
unsealed, code-subclassable runtime class).

But, this thought experiment hits problems a little further on, because...

### How would ThemeResource attach to the element tree? Via SetValue?

Not through the existing public SetValue API. Markup extensions are a *parser* concept, not a property-system value.

Internally `SetValue` has a branch that calls `TryProcessingThemeResourcePropertyValue` to resolve `ThemeResource`
bindings, but the public `SetValue` API always marks the `ValueOperationFromSetValue` flag, which prevents WinUI from
going down the branch with `TryProcessingThemeResourcePropertyValue`.

The naive thing is to allow the public SetValue to not set that flag, to enable this functionality through SetValue, but
that raises other issues:

* `ThemeResource` and related types will need to be made public, so they can be passed into `SetValue`.
* The public `GetValue` will need updated behavior for properties with a `ThemeResource` bound on it. This is a
  potentially breaking change.
* `TryProcessingThemeResourcePropertyValue` expects a `ThemeResource` that has already been resolved. The public
  `SetValue` method would also need to do resource resolution, or we need a way to let apps manually trigger resource
  resolution.

Doing things this way would also involve more objects (or at least more DXaml peers of existing objects).

The realistic public API is therefore a `SetBinding`-style method, e.g.
`element.SetThemeResourceBinding(DependencyProperty, key)`, that internally resolves the key against the element,
creates the `CThemeResourceExtension`/`CThemeResource`, and calls `SetThemeResourceBinding(element, dp)`.

### How much code is shared between the markup code path and the proposed API?

Lots. Please see the [ThemeResource mechanics](#themeresource-mechanics) section of this document first.

The reapplication code is completely shared. For initial setup, there's a separate mechanism to collect the ambient
dictionaries, but the resolution of the ThemeResource key against that list is shared with the diagnostics code.

**Benefits of wiring into the existing mechanism:**

1. **Theme-change behavior for free.** `CThemeResource`/`ThemeResourceExpression` are responsible for live re-resolution
   on theme/high-contrast switch via `RefreshValue`, the `NotifyThemeChanged` walk, and `ThemeWalkResourceCache`. Reuse
   inherits all of it with zero new code.
1. **Behavioral parity by construction.** Application.Resources override ordering, system-color handling,
   scoped-resource overrides (`ScopedResources::TryCreateOverrideForContext`), and `DictionaryForThemeReference`
   semantics behave identically to markup because it is the same object.
1. **Tooling works automatically.** Hot Reload / Live Visual Tree already read theme bindings as
   `ThemeResourceExpression` via `GetThemeResourceNoRef`. Code-set bindings show up, are editable, and replaceable.
1. **Clean markup & code interop.** Markup-set and code-set values share the same effective-value/expression slot and
   `BaseValueSource` precedence, so override/clear semantics are well-defined.
1. **Minimal, thin surface.** The primitives already exist; the API is an adapter. Less code and less test surface, with
   a single place to evolve.

**The one honest caveat:** the resolution front-door can't fully collapse to one path. A code API has no
`XamlServiceProviderContext`, so `ResolveThemeResourceForElement` walks the live tree from the element itself
(`GetParentFollowPopups`, mirroring the live re-resolution walk) to reconstruct the ambient scope — a deliberate
*reconstruction* of parse-time lexical scope. It
resolves against **where the element currently lives**, which can differ from parse time (e.g. element not yet parented,
resources added later). That is inherent to any code-time API, not a duplication being introduced. (The diagnostics
resolver, `ResolveResourceRuntime`/`GetAmbientValuesRuntime`, is a separate front-end with the same challenge and adds
UserControl/template heuristics and "match the parse time resolution behavior" comments; the code API does not go
through it.)

### Can the parser switch to a tree-walk instead?

No. Two reasons, and the second is the important one:

1. The tree isn't connected yet at parse time. The ObjectWriter builds top‑down: when it hits  {ThemeResource}  on a
   property, the object being constructed has not yet been assigned to its parent (the parent's end‑member hasn't
   fired). Its parent pointer is not established, so a walk up via  GetParent...  would return nothing usable. The
   context stack exists precisely because the tree links don't exist yet. It is the parser's substitute for the
   not-yet-built tree.

2. Context stack = lexical scope; tree walk = positional scope. These are not the same thing. The context stack encodes
   where the markup was authored. The tree encodes where the element ended up. For a plain element tree these coincide,
   but they diverge for templates,  ResourceDictionary.Source , and UserControls. That's why  GetAmbientValuesRuntime 
   is full of  templatedParent / UserControl heuristics with comments literally saying it's "trying to match the parse
   time resolution behavior", and admitting it can get corner cases wrong. The tree walk is a best-effort reconstruction
   of lexical scope, not an equivalent of it.

So even if you solved problem #1, a tree walk would give different, sometimes wrong answers under templates. The
parse-time context stack is the source of truth; the runtime walk only approximates it. This is the real reason the two
resolution front-doors can't merge, not just plumbing.


## API Behavior Summary

### 1. API shape & markup symmetry

- **Existing markup behavior:** `{ThemeResource}` is markup-only; there is **no** public code entry point (no IDL; the
  `ThemeResource` type lacks `IsPublic`; `ResourceKey` is non-public).
- **API _changes_ behavior:** Add a `SetBinding`-style method, e.g.
  `FrameworkElement.SetThemeResourceBinding(DependencyProperty, string key)` (FrameworkElement is the natural anchor
  since resolution needs an element for scope). Potential additions include:
  - Adding a getter (the internal `GetThemeResourceNoRef` exists)
  - Adding a batch update overload that takes an array/collection of properties/resource keys
  - Adding a way to add ThemeResource to `Style` & `Setter` objects
  - Adding a more lenient `TrySetThemeResourceBinding` method that doesn't throw if the resource key can't be found
  - Publicly projecting `ThemeResource : MarkupExtension`.

### 2. Resolution scope & timing

- **Existing markup behavior:** The parser initially resolves the key **at parse time**, against the element's **lexical
  scope** (ambient dictionary stack from `XamlServiceProviderContext`). (The key can be re-resolved after reparenting,
  see next section.)
- **API _changes_ behavior:** Code has no parser context, so it resolves via
  `ResourceResolver::ResolveThemeResourceForElement` (`ResourceResolver.cpp:376`) — an inline ancestor walk from the
  element that follows popups (`GetParentFollowPopups`, matching `ScopedResources::TraverseVisualTreeResources`) —
  **eagerly at call time**, against **where the element currently lives**. (This is distinct
  from the diagnostics-only `ResolveResourceRuntime` path.) Contract: the caller must set the binding after the element
  is in its intended scope. Optionally add an explicit-`ResourceDictionary` overload (the resolver already supports it).

### 3. Lifetime & re-resolution

- **Existing markup behavior:** On **theme / high-contrast change**, the binding updates its value to match the new
  theme, without a full re-resolution. On **reparent into a new live scope**, a live element **does** fully re-resolve.
  If the new scope doesn't define the key, it falls back to the dictionary captured at install time. Merely adding a
  matching resource to an already-in-scope dictionary does **not** trigger re-resolution.
- **API _keeps_ behavior:** Inherits all of the above unchanged, because it is the same `CThemeResource` object. Value
  refresh on theme change, full re-resolution on reparent (with captured-dictionary fallback when the key is absent in
  the new scope), and no re-resolution for late-added resources.

### 4. Target property kinds

- **Existing markup behavior:** The mechanism is keyed on `CDependencyProperty` / `KnownPropertyIndex` — dependency
  properties only.
- **API _keeps_ behavior:** Accept dependency properties (including attached DPs) only; reject plain CLR properties;
  reject read-only DPs.

### 5. Clearing & overriding

- **Existing markup behavior:** Theme bindings have `GetCanSetValue == false`, so **any new local value automatically
  removes the binding** (`ThemeResourceExpression.cpp:47-50`); `ClearValue` also removes it
  (`DependencyObject.cpp:1469`). Same as `Binding`/`TemplateBinding`.
- **API _keeps_ behavior:** No new clear API needed. `SetValue(dp, …)` and `ClearValue(dp)` already tear down the
  binding correctly. Document this as a guaranteed contract.

### 6. Missing / unresolvable key

- **Existing markup behavior:** The parser raises `AG_E_PARSER_FAILED_RESOURCE_FIND` (fails the parse) when a key isn't
  found (`ThemeResource.cpp`).
- **API _keeps_ behavior:** Throw when the key can't be resolved, matching markup. Do not adopt the diagnostics-only
  behavior of registering a deferred dependency to resolve later (`ResourceDependency.cpp:125-131`). That is a tooling
  affordance, not a product contract.

### 7. Type mismatch (value not assignable to property)

- **Existing markup behavior:** The value type must be assignable to the target property; general-purpose type
  conversion is **not** applied except for `Setter.Value` ("Other properties would be invalid",
  `ResourceDependency.cpp:163-164`). `NullKeyedResource` → null.
- **API _keeps_ behavior:** Require the resolved value be assignable to the target property; no type-converter coercion
  for arbitrary properties; an `{x:Null}`-keyed resource clears to null.

### 8. Value precedence (BaseValueSource)

- **Existing markup behavior:** A `{ThemeResource}` in markup installs as a local property value
  (`SetThemeResourceBinding` takes a `baseValueSource`; ordering `Default < BuiltInStyle < Style < Local < Inherited`,
  `xamlOM.h:127`).
- **API _keeps_ behavior:** Install at **`BaseValueSourceLocal`**, so a code-set theme binding behaves exactly like
  directly setting the property in code (overrides Style, overridden by animations).

### 9. Threading & re-entrancy

- **Existing markup behavior:** All DO property access is single-threaded on the owning UI thread; internal theme-walk
  re-entrancy is guarded (`IsProcessingThemeWalk()`, `Theming.cpp:416`).
- **API _keeps_ behavior:** Require the call on the target's UI/dispatcher thread; treat calling during a theme walk as
  unsupported/guarded (not a public scenario).

### 10. Object identity / sharing

- **Existing markup behavior:** Multiple `{ThemeResource}` references to the same key resolve to the **same shared
  object instance** (no clone), because resolution bottoms out in `GetKeyNoRef`. WinUI has no `x:Shared`.
- **API _keeps_ behavior:** Same sharing semantics. But this becomes **directly observable from code** (callers can
  compare or mutate the returned object), so explicitly document "value is shared; do not mutate in place," and add the
  integration test identified as a current coverage gap.

## ThemeResource mechanics

A ThemeResource has two separate mechanisms:
1. Work done once on the initial setup, and
1. Work done on every reapplication.

### Reapplication

Reapplication is implemented in `CDependencyObject::UpdateThemeReference(CThemeResource*)`. It's triggered by multiple things:
* A theme change, or
* A live enter/reparent, or
* Diagnostics/hot reload `UpdateThemeResourceValue` (Hot Reload editing the value of a key in a ResourceDictionary)

Reapplication does a tree walk up the parent chain for live elements, looking for a matching resource key. If none are found
(or if the tree isn't live), it falls back (via `CThemeResource::RefreshValue`) to the original ResourceDictionary captured
during initial setup.

Note that this means a theme change triggers a tree walk. If a ResourceDictionary closer to the ThemeResource got a key/value
that matches the ThemeResource, a theme change would cause the ThemeResource to pick up the new value.

### Initial setup

Initial setup has multiple code paths. The three main ones are:
* Markup (`{ThemeResource}`)
* Code (public `FrameworkElement::SetThemeResourceBinding`)
* Diagnostics/hot reload API (`DiagnosticsInterop::SetThemeResourceBinding`)

These code paths gather up ambient ResourceDictionaries in a list. They're considered "Ambient" because they're picked up based
on where they are in the tree relative to the ThemeResource binding, and are not explicitly identified by the binding.
* Markup uses `ResourceResolver::GetAmbientValues`, which uses the parser to get the ambient ResourceDictionaries from a lexical
  scope (i.e. in which files they are defined).
* Code does a walk up the parent chain and calls `CFrameworkElement::GetResourcesNoCreateNoRef` on each element, starting with
  the element with the ThemeResource itself.
* Diagnostics uses `ResourceResolver::GetAmbientValuesRuntime`, which walks `Diagnostics::GetParentForElementStateChanged` and
  tries to match the parse time resolution behavior without having the parser available.

These code paths then all try to resolve the key against the ambient ResourceDictionary list collected using
`CResourceDictionary::GetKeyForResourceResolutionNoRef`, falling back to the global `ResourceDictionary` or to `Application.Resources`
via `ResourceResolver::FallbackGetKeyForResourceResolutionNoRef` if the key can't be resolved.

These code paths all end with an explicit call to `CDependencyObject::UpdateThemeReference`, running the reapplication code
path above. Note that this explicit call always happens at the end of initial setup, and does not depend on a condition identified
above in the [Reapplication](#reapplication) section.

There are two more uncommon cases that set up a ThemeResource binding:
* MUX controls calling internal APIs (`CThemeResourceExtension::SetThemeResourceBinding`)
* Scoped-resource override cloning

These two paths gather no ambient list and have no fallback chain:
* **MUX controls** (AppBar, CalendarView, Popup) look the key up **directly in the global theme
  resources dictionary** — `core->GetThemeResources()->GetKeyNoRef(...)` / `core->LookupThemeResource(...)` — and
  reuse that same dictionary as the captured target (`AppBar_Partial.cpp:1979-1995`, `CalendarView.cpp:216-230`).
  These are hard-coded framework theme brushes, so there is nothing ambient to walk and no fallback to perform.
* **Scoped-resource override cloning** reuses the value and dictionary already found by the override search
  (`foundOverride.value` / `foundOverride.dictionary`) and resolves nothing itself
  (`ScopedResources_Cloning.cpp:277-281`).

These two code paths also end in an explicit call to `CDependencyObject::UpdateThemeReference`, kicking off a reapplication.
One exception: the extension entry `CThemeResourceExtension::SetThemeResourceBinding` defers for a **Style** `Setter.Value`
with `PreserveThemeResourceExtension()` — it stores the extension via `SetValue` and reapplication happens later, when
the setter is applied to a control; `ThemeResourceExtension.cpp:394-399`.
