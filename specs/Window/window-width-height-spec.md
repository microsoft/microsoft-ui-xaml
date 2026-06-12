<!--
    Before submitting, delete all "<!-- TEMPLATE marked" comments in this file,
    and the following quote banner:
-->
> See comments in Markdown for how to use this spec template

<!-- TEMPLATE
    The purpose of this spec is to describe new APIs, in a way
    that will transfer to docs.microsoft.com (DMC).

    There are two audiences for the spec. The first are people that want to evaluate and
    give feedback on the API, as part of the submission process.
    When it's complete it will be incorporated into the public documentation at
    http://docs.microsoft.com (DMC).
    Hopefully we'll be able to copy it mostly verbatim. So the second audience is
    everyone that reads there to learn how and why to use this API.
    Some of this text also shows up in Visual Studio Intellisense.

    For example, much of the examples and descriptions in the `RadialGradientBrush` API spec
    (https://github.com/microsoft/microsoft-ui-xaml-specs/blob/master/active/RadialGradientBrush/RadialGradientBrush.md)
    were carried over to the public API page on DMC
    (https://docs.microsoft.com/windows/winui/api/microsoft.ui.xaml.media.radialgradientbrush?view=winui-2.5)

    Once the API is on DMC, that becomes the official copy, and this spec becomes an archive.
    For example if the description is updated, that only needs to happen on DMC and needn't
    be duplicated here.

    Examples:
    * New class (RadialGradientBrush):
      https://github.com/microsoft/microsoft-ui-xaml-specs/blob/master/active/RadialGradientBrush/RadialGradientBrush.md
    * New member on an existing class (UIElement.ProtectedCursor):
      https://github.com/microsoft/microsoft-ui-xaml-specs/blob/master/active/UIElement/ElementCursor.md

    Style guide:
    * Use second person; speak to the developer who will be learning/using this API.
    (For example "you use this to..." rather than "the developer uses this to...")
    * Use hard returns to keep the page width within ~100 columns.
    (Otherwise it's more difficult to leave comments in a GitHub PR.)
    * Talk about an API's behavior, not its implementation.
    (Speak to the developer using this API, not to the team implementing it.)
    * A picture is worth a thousand words.
    * An example is worth a million words.
    * Keep examples realistic but simple; don't add unrelated complications.
    (An example that passes a stream needn't show the process of launching the File-Open dialog.)

-->

Title
===

# Background

<!-- TEMPLATE
    Use this section to provide background context for the new API(s) 
    in this spec. Try to briefly provide enough information to be able to read
    the rest of the document.

    This section and the appendix are the only sections that likely
    do not get copied to DMC; they're just an aid to reading this spec.

    For example this is a place to provide a brief explanation of some dependent
    area, just explanation enough to understand this new API, rather than telling
    the reader "go read 100 pages of background information posted at ...".

    For example this section is a place to explain why you're adding this new API rather than
    using an existing related API.

    For a simple example see the spec for the UIElement.ProtectedCursor property
    (https://github.com/microsoft/microsoft-ui-xaml-specs/blob/master/active/UIElement/ElementCursor.md)
    which has some of the thinking about how this Xaml API relates to existing
    Composition and WPF APIs. This is interesting background both for the current reader
    and the future reader trying to understand why we designed it this way,
    but not the kind of information
    that would land on DMC.
-->

# Conceptual pages (How To)

_(This is conceptual documentation that will go to docs.microsoft.com "how to" page)_

<!-- TEMPLATE
    (Optional)

    All APIs have a page on DMC, some APIs or groups of APIs have an additional high level,
    conceptual page (called a "how-to" page). This section can be used for that content.

    For example, there are several Xaml controls for different forms of text input,
    each with an API page, and then there's also a conceptual page that
    discusses them collectively
    (https://docs.microsoft.com/windows/uwp/design/controls-and-patterns/text-controls).

    Another way to use this section is as a draft of a blog post that introduces the new feature.

    Sometimes it's difficult to decide if text belongs on a how-to page or an API page.
    It's not important to make a final decision on that in this spec; we can always
    adjust it when copying to DMC.
-->

# API Pages

_(Each of the following L2 sections correspond to a page that will be on docs.microsoft.com)_

<!-- TEMPLATE

  Each of the L2 sections in this "API Pages" section corresponds to a page on DMC.

  It's not necessary to have a section for every class member though:
  * If its purpose and usage is obvious from it's name/type, it's not necessary to
    create a section for it.
  * If its purpose and usage is fully explained by brief description, either
      put it in a table in the "Other [class] members" section
      put it with /// comments in the IDL section

  Create an L2 section here for each API that needs more description or examples.
  For a new class with members, the members should go in their own L2 section.

  Example layout
    ## MyClass
    ## MyClass.Member1
    ## MyClass.Member2
    ## Other MyClass members
    ## MyOtherClass
    ## ...

  Notes:
  * The first line of each of these sections should become that first line on the DMC page,
    which then becomes the description you see in Intellisense.
  * Each page can have description, examples, and remarks.
    Remarks are where the documentation calls out special considerations that the developer
    should be aware of.
  * It can be helpful at the top of an API page (or after the Intellisense text) to add the
    API signature in C#
  * Add a "_Spec note: ..._" to add a note that's useful in this spec but shouldn't go to DMC.
  * Show _examples_, not _samples_; an example is a snippet, a sample is a full working app.

-->

## MyExample class

Brief description of this class.

Introduction to one or more example usages of a MyExample class:

```c#
void SampleMethod() 
{
  var show = new MyExample();
  show.SomeMembers = AndWhyItMight(be, interesting)
}
```
Remarks about the MyExample class. For example,
APIs should only throw exceptions in exceptional conditions; basically,
only when there's a bug in the caller, such as argument exception.  But if for some
reason it's necessary for a caller to catch an exception from an API, call that
out with an explanation either here or in the Examples

## MyExample.PropertyOne property

Brief description of the MyExample.PropertyOne property.

Paragraph with more detail about the property.

_Spec note: internal comment about this property that won't go into the public docs._

Introduction to one or more usages of the MyExample.PropertyOne property.

```c#
...
```

## Other MyExample members

| Name | Description |
|-|-|
| PropertyTwo | Brief description of the PropertyTwo property (defaults to ...) |
| MethodOne | Brief description of the MethodOne method |

# API Details

```c# (but really MIDL3)
namespace Microsoft.Name.Space
{
  runtimeclass MyExample
  {
      Int32 PropertyOne;
      String PropertyTwo { get; }
      void MethodOne();

       /// Brief description of the MethodTwo method
      void MethodTwo();
  }
}
```

# Appendix

<!-- TEMPLATE
  Anything else that you want to write down about implementation notes and for posterity,
  but that isn't necessary to understand the purpose and usage of the API.

  This or the Background section are a good place to describe alternative designs
  and why they were rejected.
-->