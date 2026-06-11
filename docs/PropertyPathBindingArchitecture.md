# Property Path Binding Architecture
_This content is mostly AI generated.  AI makes mistakes._

How `{Binding Path=Foo.Bar}` works at runtime: the types involved and how they relate.

All classes live in `DirectUI` namespace under `dxaml/xcp/dxaml/lib/`.

---

## The Cast of Characters

```
BindingExpression          -- "I want Foo.Bar from my DataContext"
  |
  +-- Binding              -- the parsed {Binding} markup, owns the parser
  |     +-- PropertyPathParser          -- turns "Foo.Bar" into descriptors
  |           +-- PropertyPathStepDescriptor[]  -- recipe for each dot-segment
  |
  +-- PropertyPathListener -- the live connection, owns a chain of steps
        +-- PropertyAccessPathStep("Foo")  --> value of Foo
              +-- m_tpNext
                    +-- PropertyAccessPathStep("Bar")  --> value of Bar
```

## Example: `{Binding Path=Foo.Bar}`

Say you have `<TextBlock Text="{Binding Path=Foo.Bar}"/>` and the DataContext
is a `DependencyObject` with properties `Foo` (returns another DO) and `Bar`
(returns a string).

**Parse time** -- `PropertyPathParser` chews up `"Foo.Bar"` and spits out two
`PropertyPathStepDescriptor`s:

```
  Descriptor[0]:  Kind=PropertyAccess,  text="Foo"
  Descriptor[1]:  Kind=PropertyAccess,  text="Bar"
```

**Connect time** -- `PropertyPathListener` creates a step for each descriptor
and links them into a chain:

```
  Step("Foo")  -------->  Step("Bar")
     |                       |
     | source=DataCtx        | source=DataCtx.Foo
     | value=DataCtx.Foo     | value=DataCtx.Foo.Bar  <-- this is what TextBlock.Text gets
     |                       |
     | accessor:             | accessor:
     |  m_inlineDO           |  m_inlineDO
     |  (or fallback         |  (or fallback
     |   m_tpPropertyAccess) |   m_tpPropertyAccess)
```

Each step has a `PropertyAccess` -- either inlined (`m_inlineDO`) for the
common DependencyObject case, or heap-allocated (`m_tpPropertyAccess`) for
CLR/map/custom sources.  The accessor is the object that actually reads the
property value and listens for changes.

In the source code, "DataCtx" is whatever `IInspectable*` gets passed into
`PropertyPathListener::SetSource()`.  Typically that's the `FrameworkElement`'s
effective `DataContext` (resolved by `BindingExpression::GetEffectiveDataContext()`),
which walks up the tree until it finds a non-null `DataContext` property.

**Change notification** -- If `Foo` changes on the DataContext, Step("Foo")
hears it, tells the listener, which re-connects Step("Bar") to the new Foo,
and the final value flows back to `BindingExpression` -> `TextBlock.Text`.

---

## Type Relationships

```
PropertyPathStep (base)                    PropertyAccess (base)
  |                                          |
  +-- PropertyAccessPathStep -------uses---> +-- DependencyObjectPropertyAccess  (*)
  |     m_szProperty: WCHAR*                 +-- PropertyInfoPropertyAccess
  |     m_pDP: const CDependencyProperty*    +-- PropertyProviderPropertyAccess
  |     m_inlineDO: InlineDOAccessor         +-- MapPropertyAccess
  |     m_tpPropertyAccess: PropertyAccess   |
  |                                          +-- IndexerPropertyAccess
  +-- SourceAccessPathStep                       (used by Int/StringIndexerPathStep)
  +-- IntIndexerPathStep
  +-- StringIndexerPathStep

(*) DependencyObjectPropertyAccess exists in the codebase but is now
    replaced by InlineDOAccessor for the common case.
```

### PropertyAccessPathStep -- the workhorse

This object handles `"Foo"` and `(Button.Content)` segments.  It has two ways
to access the property value:

1. **Inline DO path** (fast, ~95% of bindings): If the source is a
   `DependencyObject`, store the accessor state directly inside the step
   as an `InlineDOAccessor` struct.  No heap allocation.  This path is
   enabled unconditionally (see `IsInlineDOAccessEnabled()`).

2. **Fallback path**: Allocate a `PropertyAccess` subclass on the heap
   (`PropertyInfoPropertyAccess`, `MapPropertyAccess`, or
   `PropertyProviderPropertyAccess`).  Used for CLR objects, maps, and
   custom property providers.

### InlineDOAccessor (the optimization)

```cpp
struct InlineDOAccessor {
    TrackerPtr<IInspectable> m_tpSource;       // binding source object
    EventPtr<...>            m_epSyncHandler;  // DP-changed listener
    const CClassInfo*        m_pSourceType;    // cached type (for reconnect)
    const CDependencyProperty* m_pDP;          // resolved DP for this property
};
```

Lives inline in `PropertyAccessPathStep`.  Does the same job as a heap-allocated
`DependencyObjectPropertyAccess`, but without the allocation.  On reconnect,
if the source type hasn't changed, it just swaps the source pointer -- no
teardown/rebuild.

---

## Descriptor Kinds

| Path syntax | Descriptor Kind | Step Type | Example |
|---|---|---|---|
| _(empty)_ | `SourceAccess` | `SourceAccessPathStep` | `{Binding}` |
| `PropertyName` | `PropertyAccess` | `PropertyAccessPathStep` | `Name` |
| `(Type.Prop)` | `DependencyProperty` | `PropertyAccessPathStep` | `(Button.Content)` |
| `[42]` | `IntIndexer` | `IntIndexerPathStep` | `Items[0]` |
| `[key]` | `StringIndexer` | `StringIndexerPathStep` | `Lookup[myKey]` |

---

## How ConnectPropertyAccessForObject Picks an Accessor

When a step connects to a new source object, it goes through this decision tree:

```
  Is source null?
    yes -> disconnect, done
    no  |
        v
  Already using inline DO and same source type?  (reconnect fast path)
    yes -> swap source pointer, done
    no  |
        v
  Already using heap accessor and same source type?  (TryReconnect)
    yes -> swap source pointer, done
    no  |
        v
  Source is IDependencyObject and IsInlineDOAccessEnabled?
    yes -> InlineDOConnect (resolve DP, store inline), done
    no  |
        v
  Try PropertyInfoPropertyAccess (app metadata / IXamlMetadataProvider)
    -> Try MapPropertyAccess (IMap<HSTRING, IInspectable*>)
      -> Try PropertyProviderPropertyAccess (ICustomPropertyProvider)
        -> Give up, not connected
```

---

## Change Notification Flow

```
  Source.Foo changes
       |
       v
  PropertyAccessPathStep("Foo")::SourceChanged()
       |  (inline DO: filtered by DP index;  heap accessor: INPC / MapChanged)
       v
  PropertyPathListener::PropertyPathStepChanged()
       |  re-connects downstream steps (Step("Bar") gets new source)
       v
  BindingExpression::SourceChanged()
       |
       v
  TextBlock.Text = new value
```
