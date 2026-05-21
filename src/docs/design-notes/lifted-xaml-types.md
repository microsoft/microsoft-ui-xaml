*This is originally from 7/2019, but was in a now-defunct specs repo, so copied here*

# List of Lifted WinUI Xaml Types

## Table of Contents

- [Windows.UI](#windowsui)
  - [Color](#color)
  - [Colors](#colors)
  - [ColorHelper](#colorhelper)
- [Windows.UI.Text](#windowsuitext)
- [Other Notes](#other-notes)

Lifted WinUI Xaml will include all of the types in `Windows.UI.Xaml`, minus some types and members that aren't lifting (at 
least not lifting in V1). (This list **WIP** and **TBD**)

There are also some types that are part of "Xaml" that aren't in the `Windows.UI.Xaml` namespace, in `Windows.UI` and 
`Windows.UI.Text`. For example, `Window.UI.Colors`. This document describes exactly which will be part of the lifted set.

Notes:
*  All classes and interfaces lifted from `Windows.` to `Microsoft.` will have a new IID.
*  We could leave all structs in `Windows.` namespace, but apps would then be required to have mixed namespace 
  references (mixed `using`s).

## Windows.UI

Xaml has three types in the top level `Windows.UI` namespace:
* [Color](https://docs.microsoft.com/uwp/api/Windows.UI.Color)
* [Colors](https://docs.microsoft.com/uwp/api/Windows.UI.Colors)
* [ColorHelper](https://docs.microsoft.com/uwp/api/Windows.UI.ColorHelper)

`Colors` and `ColorHelper` will be part of the lifted set, `Color` will stay in `Windows.UI`. More details below.

### Color

`Color` is special in that it's a struct. So while it may be in the Xaml source code, it doesn't live in a DLL; 
it's just a definition in .winmd and header files.

And also since `Color` is a struct, it can never compatibly change. It's extremely unlikely that we would ever want to 
modify `Color`, any changes would likely go into a new type name. `Color` is vary stable.

There are dozens of APIs outside of Xaml that use the `Color` struct (which is why it was moved out of 
`Windows.UI.Xaml`).

`Color` will stay in `Windows.UI`, there will be no `Color` type in the lifted set. This means, for example, that Xaml properties 
like [SolidColorBrush.Color](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Media.SolidColorBrush.Color) will be of 
type `Windows.UI.Color`, not Microsoft.UI.Color.

### Colors

[Colors](https://docs.microsoft.com/uwp/api/Windows.UI.Colors) is a static helper class with `Color`-typed properties 
like `Red` and `CadetBlue`. The names and values are 
[copied from HTML](https://www.w3schools.com/colors/colors_names.asp).

`Colors`, unlike `Color`, actually has an implementation, which is in Windows.UI.Xaml.dll.

`Colors` will be copied to lifted WinUI. This gives us the opportunity to change it over time, even though it has been 
extremely stable (HTML spec doesn't appear to have changed in decades?).

> Note that the return type of `Microsoft.UI.Colors` will be `Windows.UI.Color`, which could lead to app code with a 
> `using` for both `Microsoft.UI` and `Windows.UI`, which will cause a type ambiguity over the `Colors` and 
> `ColorHelper` type. Similarly, assuming we lift `UIContext`, that will also cause apps to be referencing duplicate 
> types in the two namespaces.

### ColorHelper

[ColorHelper](https://docs.microsoft.com/uwp/api/Windows.UI.ColorHelper) was originally created to provide a static 
`FromArgb()` method for `Color`. For example:

```cs
Color redColor = ColorHelper.FromArgb(255,0,0,0);
```

There's some special-casing in the projection of `ColorHelper`. The C# and C++/CX projections of the `Color` struct get 
the static `FromArgb()` method. 
(Struct methods are supported in C# and C++ but not in WinRT.) So for example in C# you can also type:

```cs
Color redColor = Color.FromArgb(255,0,0,0);
```

C++/WinRT does not have this special projection. 

This got more complicated in RS2 when 
[ColorHelper.ToDisplayName()](https://docs.microsoft.com/uwp/api/Windows.UI.ColorHelper.ToDisplayName) was added. That 
only shows up on `ColorHelper`, not on `Color`.

`ColorHelper` is the least stable of the 3 Color classes and most likely to change over time, so will be copied to lifted WinUI.

## Windows.UI.Text

Xaml has Text Object Model (TOM) types in the `Windows.UI.Text` namespace:
* CaretType
* FindOptions
* FontStretch (enum)
* FontStyle (enum)
* FontWeight (struct)
* FormatEffect
* HorizontalCharacterAlignment
* IRichEditTextRange
* ITextCharacterFormat
* ITextDocument
* ITextParagraphFormat
* ITextRange
* ITextSelection
* LetterCase
* LineSpacingRule
* LinkType
* MarkerAlignment
* MarkerStyle
* MarkerType
* ParagraphAlignment
* ParagraphStyle
* PointOptions
* RangeGravity
* RichEditTextDocument
* RichEditTextRange
* SelectionOptions
* SelectionType
* TabAlignment
* TabLeader
* TextConstants
* TextDecorations
* TextGetOptions
* TextRangeUnit
* TextScript
* TextSetOptions
* UnderlineType (enum)
* VerticalCharacterAlignment
 
Most of these are only used by Xaml (only referenced by types in `Windows.UI.Xaml`). The two exceptions:
* `FontStretch`, `FontStyle`, and `FontWeight` are used by 
  [LanguageFont](https://docs.microsoft.com/uwp/api/Windows.Globalization.Fonts.LanguageFont)
* `UnderlineType` is used by 
  [CoreTextEditContext.FormatUpdating](https://docs.microsoft.com/uwp/api/Windows.UI.Text.Core.CoreTextEditContext.FormatUpdating)

Not everything in `Windows.UI.Text` is part of Xaml, for example 
[CoreTextServicesManager](https://docs.microsoft.com/uwp/api/Windows.UI.Text.Core.CoreTextServicesManager). 
Today, `Windows.UI.Text` itself is almost entirely Xaml, with the non-Xaml types in `Windows.UI.Text.Core`. 
We could separate out those that are only used by Xaml into a new Microsoft.UI.Text.Xaml or Microsoft.UI.Xaml.Text 
namespace. But we'll leave them where they are for consistency and compatibility.

The Xaml types in `Windows.UI.Text` have churned over time, for example 
[RichEditTextDocument](https://docs.microsoft.com/uwp/api/Windows.UI.Text.RichEditTextDocument) and 
[TextDecorations](https://docs.microsoft.com/uwp/api/Windows.UI.Text.TextDecorations)
were added in RS2, [RichEditTextRange](https://docs.microsoft.com/uwp/api/Windows.UI.Text.RichEditTextRange) in RS4, 
[RichEditTextDocument.ClearUndoRedoHistory](https://docs.microsoft.com/uwp/api/Windows.UI.Text.RichEditTextDocument.ClearUndoRedoHistory) 
in RS5.

All of the above listed types will be lifted. This means that `FontStretch`, `FontStyle`, `FontWeight`, and 
`UnderlineType` could create a conflict in code that has an implicit `using` of both `Windows.UI.Text` and 
`Microsoft.UI.Text`. This overlap is small enough to be acceptable.

One additional change we will make is to change the type of the 
[RichEditBox.Document](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.RichEditBox.Document) property from 
[ITextDocument](https://docs.microsoft.com/uwp/api/Windows.UI.Text.ITextDocument) to 
[RichEditTextDocument](https://docs.microsoft.com/uwp/api/Windows.UI.Text.RichEditTextDocument). 
The latter implements the former, and was created to fix the flaw that the former was an interface, which can't be 
versioned. Today `RichEditBox` exposes two properties of these two types, `RichEditBox.Document` (`ITextDocument`) and 
`RichEditBox.TextDocument` (`RichEditTextDocument`). We'll fix the `Document` property to be a `RichEditTextDocument`, 
and leave the duplicate `TextDocument` property for compat. Note that this is a source compatible change.

## Other Notes

The foundation [Rect](https://docs.microsoft.com/uwp/api/Windows.Foundation.Rect) has a 
[RectHelper](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.RectHelper) class that has nothing to do with Xaml, but 
is in the Xaml namespace. Similarly for [PointHelper](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.PointHelper) 
and [SizeHelper](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.SizeHelper). These helper classes will all stay in 
the Xaml namespace (`Microsoft.UI.Xaml`).

We should consider **not** lifting the 
[Windows.UI.Xaml.TypeName](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Interop.TypeName) struct. This is 
specially projected into C# and somewhat into C++/WinRT. If we can't make those projections work for that struct in the 
`Microsoft.` namespace, it might be best to leave it in the `Windows.` namespace. It's a struct, so it won't change 
over time, and it won't cause the app to load the WUX DLL.
