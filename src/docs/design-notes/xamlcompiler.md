# XAML Compiler Development and Architecture

## Table of Contents

- [Versions](#versions)
- [XAML compiler invocation](#xaml-compiler-invocation)
- [Interfacing with the Build System](#interfacing-with-the-build-system)
  - [MSBuild Task](#msbuild-task)
  - [As a Standalone Executable](#as-a-standalone-executable)
  - [Host Process and Lifetime](#host-process-and-lifetime)
  - [Parameters](#parameters)
- [Development](#development)
  - [Sources](#sources)
  - [Dependencies](#dependencies)
  - [Building](#building)
  - [Deploying and Debugging](#deploying-and-debugging)
  - [Regression and Unit Tests](#regression-and-unit-tests)
- [Components](#components)
  - [CompileXaml/CompileXamlInternal](#compilexamlcompilexamlinternal)
  - [Type Universe, Schema Context, and Type Resolver](#type-universe-schema-context-and-type-resolver)
  - [Xaml Dom Validation](#xaml-dom-validation)
  - [Xaml Rewriting](#xaml-rewriting)
  - [XBF](#xbf)
  - [Xaml Parsing](#xaml-parsing)
    - [System.Xaml](#systemxaml)
    - [Antlr Parsing](#antlr-parsing)
    - [Parsing strings into objects](#parsing-strings-into-objects)
  - [Code Generation](#code-generation)
    - [TT Files](#tt-files)
    - [How do they work](#how-do-they-work)
  - [Code-gen Models](#code-gen-models)
  - [TT Base class](#tt-base-class)
  - [Model](#model)
  - [x:Bind's Models](#xbinds-models)
  - [Two Pass Code Generation](#two-pass-code-generation)
  - [Xaml Pages](#xaml-pages)
  - [Xaml Type Info](#xaml-type-info)
  - [Incremental Xaml Type Info](#incremental-xaml-type-info)
  - [Homework](#homework)
  - [Questions](#questions)

## Versions

Across the company, there are currently multiple components that compile XAML
into various languages. There's the Xamarin XAML Compiler, the WPF XAML
Compiler, the Visual Studio XAML Compiler, and then there's our WinUI XAML
Compiler. We are based almost entirely on the OS Windows.UI.Xaml XAML Compiler,
Microsoft.Windows.UI.Xaml.Build.Tasks.dll, which ships in the Windows SDK.

## XAML compiler invocation

We ship an MSBuild targets file, Microsoft.UI.Xaml.Markup.Compiler.interop.targets,
as part of the WinUI NuGet package.  This .targets file is automatically imported by
MSBuild projects referencing the WinUI NuGet, and configures the project to invoke the
Xaml compiler as part of the project's build.


## Interfacing with the Build System

We ship the Xaml compiler in two forms, as an MSBuild task in a dll, and as a standalone executable.

### MSBuild Task

The primary form of the XAML Compiler is a .NET Framework MSBuild Task.
We ship with the WinUI NuGet package with these relevant files:

* Microsoft.UI.Xaml.Markup.Compiler.dll - the .NET Framework MS Build Task dll, a managed,
strong-name-signed assembly that contains all the XAML Compiler code.

* Microsoft.UI.Xaml.Markup.Compiler.interop.targets - the MS Build targets file, an XML
that describes and is used to invoke our tasks. This file is the glue between
MS Build and the XAML Compiler. If you open it, you will see that we receive
and mangle with a variety of MS Build variables that are being collected as
part of the project build’s setup phase, or by other previously executed tasks.

### As a Standalone Executable

We also ship the Xaml compiler as a .NET Framework executable.  The executable's
main purpose is to allow the compiler to be used in .NET 5 builds,
which use a .NET Core version of MSBuild instead of a .NET Framework version.
Because the MSBuild task version of the compiler is built on .NET Framework,
it can't be loaded in-process and used in the .NET Core MSBuild.  Dual-building
the Xaml compiler for .NET Framework and .NET Core would require
a significant amount of refactoring in the Xaml compiler.  So instead, we use thin
.NET Standard wrappers to convert MSBuild input to the JSON format,
run the executable compiler with the input JSON, and then convert the JSON outputted
by the compiler back into MSBuild outputs (like generated files, errors, and warnings).
The relevant files are:

 * XamlCompiler.exe - the standalone executable compiler.  Accepts two arguments via
the command line, the path to the compiler inputs in a JSON file, and the path to an
output JSON file where the compiler should write its outputs to.

 * Microsoft.UI.Xaml.Markup.Compiler.IO.dll - a .NET Standard MSBuild task dll,
containing the MSBuild tasks to convert MSBuild inputs to JSON compiler inputs, and from
JSON compiler outputs to MSBuild outputs.

### Host Process and Lifetime

In Visual Studio, the XAML Compiler MS Build task is hosted in MSBuild.exe for
regular builds, and in devenv.exe for designer builds. VS reuses MSBuild.exe
processes and thus the XAML Compiler is optimized to cache various objects
obtained through heavy operations, such as assembly reflection. While this
greatly improves perf, it can also lead to bugs, of course, which are often
resolved by restarting VS.

Talking about perf, a typical XAML compilation takes under a couple seconds for smaller
projects, but for larger projects can take upwards of a minute.  We are investigating
improvements to speed up compile times for large projects with many .xaml files.

### Parameters

The CompileXaml object exposed by the Xaml Compiler task takes multiple
parameters. To see them all, open the Common.targets file referenced above.
Among them, some of the important ones are:

* *Language*: the language for which we're generating code. The 4 supported
languages are C#, C++\CX, C++\WinRT, and Visual Basic.
* *RootNamespace*: the component root namespace.
* *XamlPages*: a collection of all files added as a *Page* in the project file.
* *PriIndexName*: the pri index is used in generating the call to LoadComponent,
a common cause of failures when internal projects convert from building in VS
vs. razzle.
* *IsPass1*: the Xaml Compiler runs in 2 phases during a project build - more on
that later. This is how you tell which phase we're in.
* *DisableXbfGeneration*: this flag is useful for turning off XBF generation, for
debugging or other reasons.
* *LocalAssembly*: local asembly identifies the dll or winmd that contains local
types. Local asembly types get special treatment since they don't exist in
Pass1 (more on that later, but as an example, for C++\WinRT they may get a
different Xaml Metadata Provider activation call).
* *ReferenceAssemblies*: this is a collection of all assemblies that the Xaml
Compiler should know about, including the Windows SDK. If a type is not in any
of these assemblies, the type *does not exist*.
* *SuppressWarnings*: XAML warnings may be suppressed using this parameter.
* *TargetPlatformMinVersion*: very important! This parameter tells the XAML
Compiler which is our *Target Min Platform*. With this information we can
detect and disable features that cannot run on those older SDKs.

There are other parameters that are not covered above. Browse through the
targets file and code to see what they're used for.

The executable compiler accepts two arguments via the command line, the path
to the compiler inputs in a JSON file, and the path to an output JSON file where
the compiler should write its outputs to.


## Development

### Sources

The source code and tests for the Xaml compiler are located under:

    src\XamlCompiler

With the bulk of the compiler code under:
    src\XamlCompiler\BuildTasks

There are several files and folders of interest here, but the most important
ones are:

* *XamlCompiler.sln*: the Xaml Compiler sollution that contains all the dev
code.
* *XamlCompilerTests.sln*: contains the 59 and counting projects that make all
regression and unit tests that we subject the compiler to in our daily work.

*Note: Yes, it all builds in VS, with full intellisense and code-browsing.*

***(Note: XamlCompiler unit test projects are currently broken)***

### Dependencies

The Xaml Compiler builds both from the command line and from VS.

### Building

To build the Xaml Compiler, load the XamlCompiler.sln in VS and build.
Alternatively, build from the command line like you would WinUI product code.
Incremental builds work fine.  Note that the MSBuild processes used to build
WinUI will keep a handle on compiler dlls with MSBuild tasks.
If you see file access errors because of this, you can run this command to kill
any existing MSBuild processes keeping a handle on the dlls:

taskkill /IM "msbuild.exe" /f

Then rebuild.

### Deploying and Debugging

After a successful build, projects that have imported usexamlcompiler.props
will automatically pick up the newly built compiler.

Note that:
* If building the compiler from VS, you **don't** need to shut down the VS instance where you build the Xaml
Compiler.
* You **do** need to shut down VS/MSBuild instances that use the newly built compiler.

After you're done, open a test project, and do a clean build once to have
MSBuild.exe start again. Then, in the original instance of Visual Studio where
you have the Xaml Compiler project loaded, attach to MSBuild.exe (you may have
multiple instanced running - attach to all). Set a breakpoint on
genxbf!OnFailureEncountered. Do another clean build of the project in VS. The
breakpoint should get hit at the point of failure.

### Regression and Unit Tests

The XamlCompilerTests.sln contains 50 some projects that test various features
of the Xaml Compiler in 3 distinct ways:

* *Unit Tests*: we have some 300 unit tests that ensure a limited number of
featrures work in isolation. We always keep those clean and passing. These are
useful for testing negative scenarios, too, like error cases.

* *Testbed-type Tests*: Most of these are just complex projects for regression
testing purposes. We test them manually when changing large chunks of code, or
when adding new features.

* *Code-comparison-type Tests*: Under the test-references folder we have a copy of
the generated code for all the above regression testbed projects. When we do
any bug fix, no matter how small, Unit Tests will detect a missmatch in
generated code and trigger a failure. If the change is intended, the developer
updates the reference files using the reference-update script located in the root Xaml
Compiler folder, then checks in the test references with their main fix. That way,
reviewers can validate code changes in all 4 languages.

We require all Xaml Compiler fixes to be accompanied by a regression test.

However, our regression tests do not consume from a NuGet package, instead importing
the usexamlcompiler.props file to consume the Xaml compiler and WinUI binaries directly
from the build for quick inner loop development.  Before checking in a change, you should also validate
the real-world NuGet scenario works by building perf/scenarios/MeasureMUX-set.sln.  To do this:

1. Build the mock Microsoft.WindowsAppSdk NuGet package by running **pack.cmd**.
2. Clear the WindowsAppSdk and WinUI packages used by NuGet cache used by perf/scenarios/MeasureMUX-set.sln,
located under perf\scenarios\packages.  
    "Microsoft.WindowsAppSDK.999.0.0-mock-3.0.0-dev.nupkg"
    "Microsoft.WinAppSDK.WinUI.3.0.0-dev.nupkg"
If in doubt, you can delete the entire perf\scenarios\packages folder.
3. Run **nuget restore** on MeasureMUX-set.sln to restore the newly built WinUI packages.
4. Build MeasureMUX-set.sln.  If you get errors about "Debug|Win32" being invalid,
**set Platform=x86** from your command line.

## Components

### CompileXaml/CompileXamlInternal

CompileXaml is the outer most layer of the Xaml Compiler. It derives from MS
Build's Task and receives all the parameters discussed above from the targets
file. The equivalent executable, xamlcompiler.exe, has a similar outer layer
which in turn receives parameters from the command line. They both pass these
parameters to the inner layer, CompileXamlInternal, which is shared.

You will notice that this class, CompileXamlInternal, is not very well
organized, a consequence of the many years of development and the many hands
that this code has been altered by. With every change we're making to this file
we're taking small steps to refactor it and make it better. But until then, it
is what it is.

The main entrypoint is:

    CompileXamlInternal.DoExecute

It's execution could be described as doing this:

* Checking that task arguments are valid,
* Determine if any work is needed,
* Create the Type Universe, Schema Context, and Type Resolver, all objects that
work together in a tangled way to resolve type information.
* Create a Xaml Metadata Provider to be used by XBF Gen.
* Gather information about the types seen in project's Xaml files, including
any types marked as [Bindable] or markup extensions.
* Generate code and xaml:

  * Validate Xaml pages
  * Generate pages' code-behind
  * *Re-write* xaml pages (xaml that goes to genxbf)
  * Generate XBF (by calling genxbf.dll)
  * Generate binding info (for x:bind)
  * Generate Xaml type info (IXamlMetadataProvider)

* Return list of generated headers, sources, xaml, and xbf files

We'll examine each of these steps next.

### Type Universe, Schema Context, and Type Resolver

As mentioned above, these are all objects that work together in a tangled way to
resolve type information. They use the LMR (Light Metadata Reader, a precursor
to .Net reflection) to read the assemblies passed in (dll and winmd) and
extract all types that we care about. These objects will stay cached so they
can be reused in subsequent invocations because reading metadata is an
expensive operation.

To learn more about what they do, see the following source files:

* XamlTypeUniverse.cs
* TypeResolver.cs
* DUI.cs
* DirectUISystem.cs
* DirectUISchemaContext.cs

### Xaml Dom Validation

*The Validator* runs after reference metadata (non-local types) is gathered and
only looks at xaml. It checks:

* namespaces
* elements
* members
* directives
* x:Bind usage
* types present in min SDK
* and lots and lots of other stuff

Ideally, as the first line of defence, the Validator should raise all errors
that the Xaml Compiler raises, but it's not perfect and since it can't look at
local types, it's limited in scope. See the folowing file for the exact details
on the types of checks that the validator does:

    XamlDomValidator.cs

It is the responsability of components further down the pipe to check for
conditions that the validator cannot check, keeping in mind that the earlier
the validation, the better.

To raise failures, see an example of using one of the following error types and
how to associate resource strings to error text:

* ErrorCode
* XamlCompileErrror
* XamlCompileWarning
* XamlCompilerResources.resx

Any uncaught exceptions will bubble up and be raised as a Xaml Compiler
Internal Error WMC9999. Those are unhandled exceptions and are all bugs. We
should never fail with that error code. See CompileXamlInternal.cs for usage of
LogError_XamlInternalError.

### Xaml Rewriting

Xaml files, the way they're written by the developer, don't make it all the way
to genxbf before we re-touch them slightly, a process we call re-writing. See:

    XamlConnectionIdRewriter.cs

We remove members such as:

* x:Name
* Events (such as Click, Tapped, etc)
* x:Bind
* x:DataType

And we add new ones like:

* x:ConnectionId
* x:Load="false"
* etc

The elements we remove, we replace with spaces and the ones we add, we add at
the end of the line, in order to preserve line/column information for debugging.

### XBF

Every Windows SDK ships with a copy of the Xaml Compiler and genxbf.dll.

Both Xaml Compiler and genxbf.dll will be selected based on Target Platform Version (not Min),
because it knows how to generate code that is portable downlevel, and select
the right XBF for the binary xaml (as previously mentioned).

genxbf.dll ship for both x86 and x64, and the Xaml Compiler makes the selection
based on Environment.Is64BitProcess.

We send the folwoing params in:

* an array of streams to the xaml files to be processed,
* their checksums,
* a metadata provider that XBF gen uses to query about type information it
encounters while parsing the xaml,
* the target OS version and some other flags

 See the main source file for details:

    XbfGenerator.cs, specifically GenerateXbfFromStreams

Errors from genxbf are bubbled back as WMC0605 XbfGeneration_GeneralFailure.
There is a planned effort to make reporting errors from genxbf.dll more
descriptive as the Xaml Compiler cannot always prevent all mallformed markup
from reaching genxbf, and errors returned by genxbf happen to be crypic as they
are now.

### Xaml Parsing

#### System.Xaml

We do most of our parsing using System.Xaml. See DirectUIXamlType and other
DirectUI* types, which derive from System.Xaml types. The System.Xaml parser,
based on an Xml Lite parser does most of the work for us.

Note: In the OS Windows.UI.Xaml compiler from version 10.0.15063.0, we introduced
conditional namespaces.  System.Xaml could not
handle some of the constructs, so we could not wait for them to implement the
support for it, so we ended up taking a copy of System.Xaml and shipping it
with the Xaml Compiler. We still do today.

#### Antlr Parsing

System.Xaml cannot help with x:Bind and some API Information parsing, so we're
using Antlr for that. Antlr makes things very easy for us, however, there's a
learning curve to it.

See how x:Bind parsing is implementing by looking at these files:

* *BindingPath.g4*: the Antlr x:Bind syntax
* *BindPathParser.cs*: the C# callbacks for the Antlr parsing engine
* Then continue looking at the Code-gen Models section below, to explore the
model objects we create from those parsing callbacks.

It takes a little getting used to and exploring around the code to understand
Antlr, but once you get familiar, making changes is not too hard.

The folowing projects that are part of the Xaml Compiler solution are Antlr
generated helper classes. There are instructions at the top of BindingPath.g4
on how to run antlr to opdate these projects:

* *Antlr4.runtime*: the antlr 4 runtime, ships as part of the Xaml Compiler dll
* *BindingPath*: the x:Bind Antlr parsing generated helper classes
* *ConditionalNamespace*: the conditional namespaces feature Antlr parsing
generated helper classes.

*Note: Antlr is open source and is used elsewhere in the code base, too.*

#### Parsing strings into objects

When we say this in markup:
```<TextBlock Margin="40"/> ```
How does the XAML Compiler know how to convert from the string "40" to the numeric value 40?

If you look at the [`Margin`](https://docs.microsoft.com/en-us/uwp/api/windows.ui.xaml.frameworkelement.margin)
property, it is not of type `Double`, it's of type
[`Thickness`](https://docs.microsoft.com/en-us/uwp/api/windows.ui.xaml.thickness). And the margin can be
specified in a number of ways:
*	a single value for a uniform thickness, like `"20"`
*	left, top, right, bottom, like `"20, 40, 20, 40"`
*	left & right, top & bottom, like `"20, 40"`

The Xaml compiler does a couple of things to try to make sense of how to convert a string to a Xaml type:
*	if it's a string, just use that
*	if it's a user-defined type, that has a
[`CreateFromString(…)`](https://docs.microsoft.com/en-us/uwp/api/windows.foundation.metadata.createfromstringattribute)
attribute and a
[`CreateFromString`](https://docs.microsoft.com/en-us/uwp/api/windows.ui.xaml.markup.ixamltype.createfromstring) method,
it calls that
*	otherwise, it tries using the `XamlBindingHelper`
[`ConvertValue`](https://docs.microsoft.com/en-us/uwp/api/windows.ui.xaml.markup.xamlbindinghelper.convertvalue) method

In the case of "Core" types (things that the Xaml property system understands, like numbers, Thickness, GridLength,
Uris…), XBH's `ConvertValue` eventually ends up in the property system's `CValue` code, which will look up the Xaml
metadata tables (those things that get codegen'd), and
look for
a "Core constructor" to call.

For example, for `Thickness`, it ends up in `CThickness::Create` which
calls `ThicknessFromString`.
`ThicknessFromString` does the parsing.

So, if we ever need to add a new syntax for a type that isn't a string, that’s how it works.

### Code Generation

We use [T4 Text Templating](https://msdn.microsoft.com/en-us/library/bb126445.aspx)
(a.k.a. tt) to generate code.

Note: *Text Templating* is not installed by default with VS. To install it, open
the *Visual Studio Installer*, and select the Text Template Transformation from
the Individual Components tab.

#### TT Files

To see examples of such tt files, look under

    Xaml/Xaml Compiler/CodeGenerators

At a first glance, tt files look uggly, but after spending some time with them,
and after going out of our way to keep them organized and minimal, they're
actually not that bad. Here are a few examples:

Expression:

    auto targetElement = target.as<<#=element.Type#>>();

Prints:

    auto targetElement = target.as<::winrt::hstring>();

You can use C# between <# and #> to control flow:

    <# if (element.HasFieldDefinition) { #>
        // generate some code
    <# } #>

You can declare and call functions:

    <#+ private void Output_ConnectionId_Case(ConnectionIdElement element) #>
    <#+ { #>
    <#+     // generate connection id code here #>
    <#+ } #>

And so on, see doc referenced above for full syntax. The easiest, though, is to
just learn by observing examples in the code-base. We have plenty of usage
examples there.

#### How do they work

Whenever you save a tt file, Visual Studio runs the text templating tool. You
can also invoke the tool manually by right clicking a tt file and selecting
*Run Custom Tool* from the menu. This generates a C# file that contains one
class for your tt file.

Ex:

    internal partial class CppWinRT_PagePass2

We get such classes for all tt files. They all contain a *TransformText()*
function, which we invoke from *XamlCodeGenerator.cs*. The actual selection
of tt files for each types of code that we generate for each of the languages
we support is in Language.cs, and looks like this:

```csharp
    private Language(
        string name,
        string pass1Extension,
        string pass2Extension,
        bool isManaged,
        bool isStringNullable,
        bool isExperimental,
        CodeGeneratorDelegate appPass1CodeGenerator,
        CodeGeneratorDelegate appPass2CodeGenerator,
        CodeGeneratorDelegate pagePass1CodeGenerator,
        CodeGeneratorDelegate pagePass2CodeGenerator,
        CodeGeneratorDelegate typeInfoPass1CodeGenerator,
        CodeGeneratorDelegate typeInfoPass1bCodeGenerator,
        CodeGeneratorDelegate typeInfoPass1ImplCodeGenerator,
        CodeGeneratorDelegate typeInfoPass2CodeGenerator,
        CodeGeneratorDelegate bindingInfoPass1CodeGenerator,
        CodeGeneratorDelegate bindingInfoPass2CodeGenerator)
```

Where *CodeGeneratorDelegate* is of type T4Base, the base class for all of the
tt generated classes.

Have a look at *T4Base.cs*, but not before reading the following section, which
describes the model that's driving these tt files.

### Code-gen Models

If you open a tt file, you will see this directive at the very top:

```
inherits="CppWinRT_CodeGenerator<PageDefinition>"
```

This tells the text templating tool that this file should derive from
CppWinRT_CodeGenerator\<PageDefinition\>. This allows us to do two important
things:

1. Define a base class that helps with language specific constructs, like
namespace separator (. vs ::), etc: *CppWinRT_CodeGenerator\<T\>*
2. Define a model for this file: *PageDefinition*

### TT Base class

The tt base class, in my example above CppWinRT_CodeGenerator, has a deep
hierarchy of base classes.

* T4Base
  * T4Base\<T\>
    * CodeGenerator\<T\>
      * NativeCodeGenerator\<T\>
        * CppWinRT_CodeGenerator\<T\>
        * CppCX_CodeGenerator\<T\>
    * ManagedCodeGenerator\<T\>
      * CSharp_CodeGenerator\<T\>
      * VB_CodeGenerator\<T\>

Go ahead and look at some of them, see how they're structured. Here are just
two of the functions that are used in the generated code that perform
differently based on which language we're generating code for:

* *Colonize(namespace)*: writes Windows.UI.Xaml for C# and ::Windows::UI::Xaml
for C++\CX.

* *LanguageSpecificString*: this class was designed to abstract language
concerns and keep the tt file simple. In one of the previous examples there's
code like this:

    auto targetElement = target.as<<#=element.Type#>>();

But how does it work when element.Type is of type XamlType? Here's how:

* T4Base has an abstract method

```csharp
string ToStringWithCulture (XamlType);
```
* CppWinRT_CodeGenerator implements it

```csharp
public override string ToStringWithCulture(XamlType type)
{
    return type.CppWinRTName();
}
```

* XamlType has an extension method:

```csharp
public static string CppWinRTName(this XamlType type)
{
    return GetTypeForCodeGen(type).FullName.CppWinRTName();
}
```

* The TransformText method calls ToStringWithCulture(), and the call gets
dispatched to the right function that will ensure the right language syntax is
being returned.

With that, the tt file stays small and simple.

### Model

As we've seen, the *inherits* attribute of the tt file defines the base class.
In our implementation, that's a generic class that takes another type as its
template argument. That type is its *model type* that becomes available all
throughout the tt file as property *Model*.

Ex:

    partial class <#=Model.CodeInfo.ClassName.ShortName#>

In this example from *CSharpPagePass1.tt* Model is of type PageDefinition.

Similar approaches are done with opther model objects like BindUniverse,
FieldDefinition, etc, and the benefits are obvious when dealing with complex
codepaths in x:Bind.

### x:Bind's Models

x:Bind's models start from BindUniverse, which is the object governing a
binding scope, such as a page or data template.

```
BindUniverse.cs
```

Within a BindUniverse there's:

* *BindAssignments*: objects that tye a *binding* to an *element's property*
(refered to as member).
* *BindPath and BindPathStep*: an object that represents a *step* in the
*binding path*, and which defines a bunch of properties that are used in the
tt file, such as:

  * UniqueName
  * ValueType
  * CodeName
  * MemberAccessOperator
  * and so on... see BindPathStep.cs for all properties

Also, see BindPathStep.cs to understand how we use polimorphism to make the tt
easier to maintain, by overriding methods and properties in derived classes.

### Two Pass Code Generation

We generate code in two passes:

* In Pass1, we don't know about local types such
as MainPage, MyUserControl, etc, because they were not built yet. We know
something about them from the xaml files we parse, but not enough to understand
and generate complex x:Bind code.
* So, in *Pass 1* we generate enough code-behind partial classes to allow the
project to compile.
* The language compiler (C#, C++, VB) then builds the project and generates a
winmd, which we then use in *Pass 2* to generate full classes.

The most accurate way to understand what we generate in each pass is to look at
Language.cs, but for convenience, tt files include the pass name in their names.

### Xaml Pages

Not much to tell about Xaml Pages besides to invite you to go look at the tt
files themselves. Search for *Page\*.tt*

### Xaml Type Info

Xaml Type info generation happens in 2 passes, too. In *Pass 1* we generate as
much as we can that does not require knowing specifics about types. In *Pass 2*
we generate type tables, because we now know everything about types from the
metadata we got (winmd, dll). Search for *\*TypeInfo\*.tt*

### Incremental Xaml Type Info

The Xaml Metadata Provider we generate has seen its share of complains from
C++ developers with large code-bases. It's becase the *Pass 2* file,
XamlTypeInfo.g.cpp, #includes all headers and implementation files for
which we generate type info, causing this file to need to be recompiled every
time a Xaml header changes (say, developer adds an x:Name). For that reason, in
RS3, we introduced an "Incremental Xaml Type Info" mode. This mode is not
perfect but it aleviates some of the pain points around incremental builds by
writing page specific metadata in the page files themselves.

To see how this mode differs from the normal mode (C++\CX and C++\WinRT only),
search for *GenerateIncrementalTypeInfo*

### Homework

After reading this document, why not create a new project, C# or C++\CX, add an
x:Bind or two, and look at the generated files.

### Questions

Contact the WinUI team and let us know if we can help.