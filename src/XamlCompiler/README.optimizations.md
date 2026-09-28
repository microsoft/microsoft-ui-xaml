# Experimental XAML source optimizations

This experimental feature adds a **default-off, managed compiler pass** before
connection-ID rewriting and native XBF generation. It does not change GenXbf,
the XBF format, or the runtime. GenXbf already optimizes instructions and packs
supported constants; this pass supplies a simpler source representation.

## Opt in

Use compiler binaries **and targets** built from this change:

```xml
<PropertyGroup>
  <EnableXamlCompilerOptimizations>true</EnableXamlCompilerOptimizations>
</PropertyGroup>
```

The property becomes an existing `XamlFeatureControlFlags` item and travels through
the normal task/executable inputs. Changing the flag participates in the compiler's
saved-state invalidation. Removing it or setting it to `false` restores the original
path. Released Windows App SDK targets do not know this experimental property.
The last successful normal pass-two selection is persisted separately: pass one
must not consume the change and allow pass two to restore stale generated files.

Only normal **pass-two, classed page/control XAML** is eligible. Pass one,
design-time builds, Application XAML, and classless XAML remain unchanged.
Classless and Application exclusions are explicit, not a promise of optimization
coverage for resource dictionaries or every way of supplying XAML.

## First rule: `InlineThicknessPadding`

Inside an eligible classed page:

```xml
<Border>
    <Border.Padding><Thickness>1,2,3,4</Thickness></Border.Padding>
</Border>
```

Generated XAML:

```xml
<Border>
    <Border.Padding>           1,2,3,4            </Border.Padding>
</Border>
```

The spaces are intentional: only the opening and closing `Thickness` tags are
erased. The value text, property-element syntax, setter order, other objects, and
all subsequent line/column positions stay in place. Original source checksums
remain the debugger's source identity. The removed temporary value-object
instructions no longer have distinct XBF instruction locations; this is not a
promise of identical instruction-level stepping or diagnostic object inventories.

GenXbf can emit one packed `SetValueConstant` for `Border.Padding`, instead of
creating an inline Thickness wrapper, ending its initialization, and assigning it.
The public property is still a value-type Thickness with the same four components;
no UI element, public reference identity, or shared resource is removed.

### Conservative eligibility

The owner, value type, and declaring property must resolve to the built-in
`Border`, `Thickness`, and `Border.Padding`, not similarly spelled custom types.
The built-in Thickness identity is checked independently of whether its managed
projection needs code generation; that classification alone does not mean custom.
Inherited namespace aliases are supported. The Thickness must contain exactly one
initialization value and no attributes, directives, or local namespace declarations.

Accepted text is one, two, or four comma-separated, invariant-culture finite
nonnegative numbers in the native float range. Decimal and exponent notation and
XML whitespace are supported. The text is **not** evaluated and rewritten into
different numbers. Negative values, overflow, NaN, infinity, unsupported separators,
comments, CDATA, entities, processing instructions, and uncertain source spans
are left to the original compilation path.

Names/UIDs and other unmodeled directives, events, bindings/markup extensions,
attached properties, conditional members/objects, custom-type ancestry, dictionaries, and template
scopes on the candidate or its ancestors cause a decline. Unrelated siblings
can still have connection IDs and compiled event handlers. These restrictions
are intentional proof boundaries, not optimization targets to remove casually.

The existing source-position helper cannot reliably bound every XML spelling.
For example, a multiline closing tag or a preceding self-closing sibling on the
same line is conservatively declined rather than changing that shared helper.

## Architecture and observability

`Optimization\XamlOptimization.cs` defines a read-only source context, rule
interface, decisions, and immutable erasure spans. Rules analyze the same validated
DOM in a fixed order without serializing or reparsing it. The pipeline validates
all spans for bounds and overlap **before** any edit is applied. Invalid proposals
fail compilation through the compiler's existing exception reporting; unsupported
source shapes produce no edits.

`InlineThicknessPaddingRule` separates schema/scope/literal eligibility from
source-shape proof. Scope decisions are cached, so a scene with many sibling
candidates does not repeatedly traverse the same ancestor collections.
`XamlConnectionIdRewriter` applies the spans before its existing directive,
event, and connection-ID edits. With the feature off, no optimization DOM is
retained and the original editor path is used.

Diagnostic compiler logs contain `perfXC_XamlOptimization` markers, such as
`InlineThicknessPadding:Lowered=1`, grouped by rule and reason per file.
They contain no literal source contents. Declines distinguish unsupported
types/properties, decorated values, scopes, literals, and source shapes.
The in-memory decisions also retain the original candidate line and column.

## Building the compiler

Build the compiler from the repository root using its existing wrapper:

```powershell
.\initrun.ps1 -Flavor amd64fre -Command @(
    'msb', '/q', 'src\XamlCompiler\Microsoft.UI.Xaml.Markup.Compiler.csproj',
    '/m:2', '/nr:false'
)
```

## Focused compiler coverage

The focused test class is `UnitTests.XamlOptimizationTests`. Build the compiler
and test project with the repository's existing prerequisites, then stage
`UnitTests.dll`, its proxy assembly, the matching net472 compiler assemblies and
executable dependencies, and the public WinMDs together. Use the compiler
executable's generated binding redirects as `UnitTests.dll.config` and retain
`Tests\UnitTests\test.runsettings` (`DeploymentEnabled=false`). Run the existing
Visual Studio runner directly from that directory:

```cmd
vstest.console.exe UnitTests.dll /Settings:test.runsettings /TestCaseFilter:"FullyQualifiedName~XamlOptimizationTests"
```

Do not use this checkout's legacy `runtests.cmd` for this focused run: it also
patches and reverts fixture projects. These tests need no `LibManaged*` fixtures.
Coverage includes managed/native schemas, literals, namespace aliases, source
positions, connection IDs, excluded forms/scopes, deterministic and idempotent
lowering, projection codegen classification, the default/pass/design-time gate,
and saved-state flag transitions across pass one, pass two, and save/reload.

Broader adoption requires workload evidence, broader language
and SDK coverage, diagnostic/tooling compatibility review, and quantified benefit
that exceeds measurement noise. The intentionally unsupported scopes must acquire
their own semantic proof and coverage before the rule is widened.
