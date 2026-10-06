<!--See comments in Markdown for how to use this spec template-> 

<!-- The purpose of this spec is to describe a new feature and
its APIs that make up a new feature in WinUI. -->

<!-- There are two audiences for the spec. The first are people
that want to evaluate and give feedback on the API, as part of
the submission process.  When it's complete
it will be incorporated into the public documentation at
docs.microsoft.com (http://docs.microsoft.com/uwp/toolkits/winui/).
Hopefully we'll be able to copy it mostly verbatim.
So the second audience is everyone that reads there to learn how
and why to use this API. -->

# Background
<!-- Use this section to provide background context for the new API(s) 
in this spec. -->

<!-- This section and the appendix are the only sections that likely
do not get copied to docs.microsoft.com; they're just an aid to reading this spec. -->

<!-- If you're modifying an existing API, included a link here to the
existing page(s) -->

<!-- For example, this section is a place to explain why you're adding this API rather than
modifying an existing API. -->

<!-- For example, this is a place to provide a brief explanation of some dependent
area, just explanation enough to understand this new API, rather than telling
the reader "go read 100 pages of background information posted at ...". -->

>See proposal documents for:
>
> [determinate mode for ProgressRing](https://github.com/microsoft/microsoft-ui-xaml/issues/688) 
>
> [ProgressRing style](https://github.com/microsoft/microsoft-ui-xaml/issues/837)
>
> [Guidance for Progress controls](https://github.com/microsoft/microsoft-ui-xaml/issues/880)

Progress controls indicate to a user that an operation is occuring. This includes ProgressBar and ProgressRing controls. ProgressBar has both a determinate and indeterminate mode. ProgressRing only has an indeterminate mode. 

Important APIs: [ProgressBar](https://docs.microsoft.com/en-us/uwp/api/windows.ui.xaml.controls.progressbar) and [ProgressRing](https://docs.microsoft.com/en-us/uwp/api/windows.ui.xaml.controls.progressring)
# Description
<!-- Use this section to provide a brief description of the feature.
For an example, see the introduction to the PasswordBox control 
(http://docs.microsoft.com/windows/uwp/design/controls-and-patterns/password-box). -->
 Represents a control that indicates that an operation is ongoing. The typical visual appearance is a ring-shaped "spinner" that animates a filled area as progress continues.

# Examples
<!-- Use this section to explain the features of the API, showing
example code with each description. The general format is: 
  feature explanation,
  example code
  feature explanation,
  example code
  etc.-->
  
<!-- Code samples should be in C# and/or C++/WinRT -->

<!-- As an example of this section, see the Examples section for the PasswordBox control 
(https://docs.microsoft.com/windows/uwp/design/controls-and-patterns/password-box#examples). -->
[Please see this table of current ProgressBar and ProgressRing modes and states.](https://github.com/microsoft/microsoft-ui-xaml-specs/blob/user/kathyang/progress-styles/active/progress-styles/progress-styles-tables.md)

The following examples show how to use the IsIndeterminate property to change the mode of the ProgressRing and the Value property to change the proportionate amount indicated. 

## Indeterminate ProgressRing

```xml
<ProgressRing IsActive="True" Height="100" Width="100"/>
```

![](images/ProgressRing-indeterminate.PNG)
## Determinate ProgressRing

```xml
<ProgressRing IsActive="True" Height="100" Width="100" Value="75" IsIndeterminate="False"/>
```
![](images/ProgressRing-determinate.PNG)

Below are tables showing how ProgressRing is affected by the IsActive, IsIndeterminate, ShowPaused, ShowError and Value properties.

## Inactive ProgressRing
| Note | Image |
|:--|:-:|
| When IsActive is False, ProgressRing will always appear blank, regardless of other properties | ![](images/ProgressRing-determinate-not-active.PNG) |

## Indeterminate ProgressRing
IsIndeterminate is True

| ShowPaused | ShowError| Value | Image | 
|:--|:-:| :-:| :-:|
| False | False |    | ![](images/ProgressRing-indeterminate.PNG) |
| False | False | 75 | ![](images/ProgressRing-indeterminate.PNG) |
| False | True  |    | ![](images/ProgressRing-determinate-empty.PNG) |
| False | True  | 75 | ![](images/ProgressRing-determinate-empty.PNG) |
| True  | False |    | ![](images/ProgressRing-determinate-empty.PNG) |
| True  | False | 75 | ![](images/ProgressRing-determinate-empty.PNG) |
| True  | True  |    | ![](images/ProgressRing-determinate-empty.PNG) | 
| True  | True  | 75 | ![](images/ProgressRing-determinate-empty.PNG) | 

Setting a Value on an Indeterminate ProgressRing should display as though there is no Value.

## Determinate ProgressRing
IsIndeterminate is False

| ShowPaused | ShowError| Value | Image | 
|:--|:-:| :-:| :-:|
| False | False |    | ![](images/ProgressRing-determinate-empty.PNG) |
| False | False | 0  | ![](images/ProgressRing-determinate-empty.PNG) |
| False | False | 75 | ![](images/ProgressRing-determinate.PNG) |
| False | True  |    | ![](images/ProgressRing-determinate-empty.PNG) | 
| False | True  | 75 | ![](images/ProgressRing-determinate-empty.PNG) |
| True  | False |    | ![](images/ProgressRing-determinate-empty.PNG) |
| True  | False | 75 | ![](images/ProgressRing-determinate-paused.PNG) |
| True  | True  |    | ![](images/ProgressRing-determinate-empty.PNG) |
| True  | True  | 75 | ![](images/ProgressRing-determinate-empty.PNG) | 

### Minimum and Maximum

ProgressRing should indicate progress based on the Value, relative to the Minimum and Maximum.

| Properties | Image |
|:--|:-:|
| Minimum=10, Maximum=18, and Value=16| ![](images/ProgressRing-determinate.PNG) |

*Note that the designs for ProgressRing are not finalized. See [here](https://github.com/microsoft/microsoft-ui-xaml-specs/blob/user/chigy/ControlUpdates/active/ControlUpdates/images/Progress.png) for early designs. 

# Remarks
<!-- Explanation and guidance that doesn't fit into the Examples section. -->

<!-- APIs should only throw exceptions in exceptional conditions; basically,
only when there's a bug in the caller, such as argument exception.  But if for some
reason it's necessary for a caller to catch an exception from an API, call that
out with an explanation either here or in the Examples -->

With the addition of a determinate mode of ProgressRing, the ShowPaused and ShowError properties that ProgressBar already has should be aligned in ProgressRing. ProgressRing can be used in scenarios where progress is paused, or an error occured during the process. With this additional capability, the determinate ProgressRing does not represent a "hung" state where the user cannot interact with the app. Previously, guidance recommended that ProgressRing only be used when the user cannot continue to interact with the app, but this is no longer the only use case and ProgressRing can be used in scenarios where user interaction can continue while the ring is spinning.

ProgressRing will continue to inherit from Control, and will not inherit from RangeBase. Rangebase has functionality that is not used in ProgressBar and would not be used in ProgressRing. There is an ongoing investigation into adding content (Text or icons) in the center of the ProgressRing to potentially align with Xbox and other Microsoft teams.

# API Notes
<!-- Option 1: Give a one or two line description of each API (type
and member), or at least the ones that aren't obvious
from their name.  These descriptions are what show up
in IntelliSense. For properties, specify the default value of the property if it
isn't the type's default (for example an int-typed property that doesn't default to zero.) -->

<!-- Option 2: Put these descriptions in the below API Details section,
with a "///" comment above the member or type. -->
Below are properties being added to ProgressRing. The IsIndeterminate property of ProgressRing will default to True for back compatability.

|Name | Type | Description | 
|:--|:-:|:-:|
| IsIndeterminate | boolean | Defaults to True. Gets or sets a value that indicates whether the progress ring reports generic progress with a repeating pattern (indeterminate progress) or reports progress based on the Value property (determinate progress) |
| Value | double | Gets or sets the current setting of the range control, which may be coerced. | 
| ShowPaused | boolean | Defaults to False. Gets or sets a value that indicates whether the progress bar should use visual states that communicate a Paused state to the user.|
| ShowError | boolean | Defaults to False. Gets or sets a value that indicates whether the progress bar should use visual states that communicate an Error state to the user. |
| Maximum | double | Defaults to 100. Gets or sets the highest possible Value of the range element. |
| Minimum | double | Defaults to 0. Gets or sets the Minimum possible Value of the range element. |
| ValueChanged | event | Event that occurs when the range value changes. |

# API Details
<!-- The exact API, in MIDL3 format (https://docs.microsoft.com/en-us/uwp/midl-3/) -->

# Appendix
<!-- Anything else that you want to write down for posterity, but 
that isn't necessary to understand the purpose and usage of the API.
For example, implementation details. -->

# Open Questions
* How should the edge cases of 0%, 1%, 98%, 99% be visualized? We are currently discussing with other teams at Microsoft how to visualize this with the rounded interior corners.  
