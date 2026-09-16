# Experimental XAML source optimizations

This hackathon prototype adds a **default-off, managed compiler pass** before
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
`InlineThicknessPadding:Lowered=200`, grouped by rule and reason per file.
They contain no literal source contents. Declines distinguish unsupported
types/properties, decorated values, scopes, literals, and source shapes.
The in-memory decisions also retain the original candidate line and column.

## Measured compiler-integrated demonstration

This is a **synthetic framework demonstration**, not a real-app optimization claim:
the earlier repository corpus search found no unadorned inline Thickness instances.
The following measurements use actual generated XAML/XBF from the changed compiler,
not manually rewritten inputs or the earlier direct-source experiment.

The same classed Grid scene contains 200 unnamed Borders, each with the literal
`1,2,3,4`. Seven fresh processes ran randomized/interleaved warm construction
comparisons; the statistical summaries are at process level, not individual scenes.
Each process performed 100 warmups per label, then 24 blocks of 20 scenes per
label (off-A1, off-A2, on, and code). A process summary is the median of its batch
means; off averages its two label medians. The table averages the seven process
summaries. Paired differences and percentages are computed per process, with
two-sided Student-t 95% intervals using six degrees of freedom. Dispatcher and
GC drains are outside the timed regions.

The host uses the stable self-contained x64 Windows App SDK `1.8.260416003`,
WinUI package `1.8.260415005`, XAML runtime file version `3.1.8.2604`, and
.NET `8.0.31`, not the earlier native resource prototype.

| Variant | Mean process-summary construction time |
| --- | ---: |
| Compiler optimization off | 1.052 ms |
| Compiler optimization on | 0.768 ms |
| Equivalent code construction control | 1.079 ms |

The paired saving is **0.284 ms per scene (27.02%)**, with a reported 95% interval
of **26.48%-27.57%**. The no-op A/A comparison is **-0.14%**, with an interval of
**-1.17% to +0.89%**. These are local warm-construction results, not disk-cold
launch, on-screen first-frame, retained-memory, leak, or real-app benefit claims.

Compiler artifacts show 200 lowered candidates in the scene and 20 in the
21-case value fixture. Each eligible Padding uses a packed setter instead of
the temporary Thickness construction sequence. Scene XBF size decreases from
**6,255 to 5,455 bytes**. Off/on/unchanged-on/off builds use the same intermediates
without touching source between transitions; unchanged output timestamps stay
stable, and feature-off output matches the pre-change compiler.

All 21 actual off/on value cases agree, including zero/default local-value state,
signed-zero component bits, ClearValue, isolation, and layout. The pure scene has
201 tree nodes. Separate untimed rendering adds a 20-column/10-row arrangement,
backgrounds, and one labeled TextBlock per Border after loading, giving 401 nodes.
Off/on layout, nonblank 480x400 pixels, and 200 static TextBlock peer snapshots
agree. Three code-versus-XBF signed-zero
differences exist in the baseline: compiled XBF normalizes signs that code can
preserve. They are recorded separately, not hidden or attributed to the optimizer.
This finite matrix is not exhaustive production compatibility.

### Reproduction artifacts

Build the compiler from the repository root using its existing wrapper:

```powershell
.\initrun.ps1 -Flavor amd64fre -Command @(
    'msb', '/q', 'src\XamlCompiler\Microsoft.UI.Xaml.Markup.Compiler.csproj',
    '/m:2', '/nr:false'
)
```

The local demo project, build/measurement scripts, compiler/runtime fingerprints,
generated XAML/XBF dumps, and raw samples are preserved under:

```text
%USERPROFILE%\.copilot\session-state\5be5573a-9111-461f-bdb6-b30e554ade37\files\FallbackCompilerProbe\
    Run-CompilerTests.ps1
    Demo\README.md
    Demo\REPORT.md
    Demo\artifacts\final-summary.json
    Demo\results\measured-20260915-124211\
```

Follow the demo README for its project-local compiler staging and SDK adapter.
The measured net472 compiler SHA256 is
`71161FF0D76CDD3D510FC28C40894978250C101751725357B74A2344DAABB457`.
These generated binaries and local measurement artifacts are not source changes.

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

Promotion beyond a hackathon requires real workload evidence, broader language
and SDK coverage, diagnostic/tooling compatibility review, and quantified benefit
that exceeds measurement noise. The intentionally unsupported scopes must acquire
their own semantic proof and coverage before the rule is widened.
