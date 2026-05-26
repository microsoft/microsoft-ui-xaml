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
  Step("Foo")  --->  Step("Bar")
     |                   |
     | source=DataCtx    | source=DataCtx.Foo
     | value=DataCtx.Foo | value=DataCtx.Foo.Bar  <-- this is what TextBlock.Text gets
     |                   |
     | accessor:         | accessor:
     |  m_tpPropertyAccess |  m_tpPropertyAccess
```

Each step has a `PropertyAccess` stored in `m_tpPropertyAccess`.  The accessor
is the object that actually reads the property value and listens for changes.
For `DependencyObject` sources, the accessor is a `DependencyObjectPropertyAccess`;
for CLR/map/custom sources it is one of the other `PropertyAccess` subclasses.

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
  +-- PropertyAccessPathStep -------uses---> +-- DependencyObjectPropertyAccess
  |     m_szProperty: WCHAR*                 +-- PropertyInfoPropertyAccess
  |     m_pDP: const CDependencyProperty*    +-- PropertyProviderPropertyAccess
  |     m_tpPropertyAccess: PropertyAccess   +-- MapPropertyAccess
  |                                          |
  +-- SourceAccessPathStep                   |
  +-- IntIndexerPathStep                     +-- IndexerPropertyAccess (used by Int/StringIndexerPathStep)
  +-- StringIndexerPathStep


```

### PropertyAccessPathStep -- the workhorse

This object handles `"Foo"` and `(Button.Content)` segments.  It resolves the
property on the source object and stores the resulting `PropertyAccess`
subclass in `m_tpPropertyAccess`.  The concrete accessor type depends on the
source:

- **`DependencyObjectPropertyAccess`** -- source is a `DependencyObject`.
- **`PropertyInfoPropertyAccess`** -- source has app metadata (IXamlMetadataProvider).
- **`MapPropertyAccess`** -- source is `IMap<HSTRING, IInspectable*>`.
- **`PropertyProviderPropertyAccess`** -- source is `ICustomPropertyProvider`.

On reconnect, if the source type hasn't changed, the existing accessor's
`TryReconnect` swaps the source pointer -- no teardown/rebuild.

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
  Already using accessor and same source type?  (TryReconnect)
    yes -> swap source pointer, done
    no  |
        v
  Source is IDependencyObject?
    yes -> DependencyObjectPropertyAccess::CreateInstance, done
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
       |  (DO accessor: filtered by DP index;  other accessors: INPC / MapChanged)
       v
  PropertyPathListener::PropertyPathStepChanged()
       |  re-connects downstream steps (Step("Bar") gets new source)
       v
  BindingExpression::SourceChanged()
       |
       v
  TextBlock.Text = new value
```
