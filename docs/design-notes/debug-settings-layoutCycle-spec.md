DebugSettings Layout Cycle Debugging API spec
===

# Background

Xaml layout is the process where elements are given a size and position. For example in a vertical
`StackPanel` layout, each child is given a position such that they align horizontally, and appear
sequentialy vertically.

Laying out all of the elements in the tree is an iterative process, where the process of layout can
cause a change that triggers another layout pass. This process is expected to converge, with a final
layout pass producing a result without triggering any new layout passes. If the process fails to
converge (every layout pass triggers another layout pass), the process is terminated and the app
crashes with a layout cycle exception.

Currently XAML apps which experience a layout cycle crash have minimal information to help find and
fix the problem:
* The [Application.UnhandledException](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.Application.UnhandledException)
event is raised and will inform that the crash was a layout cycle issue.
* Some information is saved in [stowed exceptions](https://learn.microsoft.com/en-us/windows/apps/windows-app-sdk/migrate-to-windows-app-sdk/guides/threading#stowed-exceptions)
if the developer knows how to look for stowed exceptions.

This spec proposes an extension to
[DebugSettings](https://docs.microsoft.com/windows/windows-app-sdk/api/winrt/Microsoft.UI.Xaml.DebugSettings)
to enable tracing to debug output (rather than just stowed exceptions), increasing the verbosity of
layout cycle logging, and enabling the debugger to break in on interesting events.


# Conceptual pages (How To)

An app may set `DebugSettings.LayoutCycleTracingLevel` either just for better native debugger output
during a layout cycle crash, or to raise the level/verbosity of logging also included in crash dumps.
The app may also set `DebugSettings.LayoutCycleDebugBreakLevel` to trigger an attached native debugger
to break in during interesting events leading up to a potential layout cycle crash.

C#
```cs
namespace MUX7952
{
    public partial class App : Application
    {
        public App()
        {
            this.InitializeComponent();

            // Add more tracing at High, but DebugBreak only at Low.
            this.DebugSettings.LayoutCycleTracingLevel = LayoutCycleTracingLevel.High;
            this.DebugSettings.LayoutCycleDebugBreakLevel = LayoutCycleDebugBreakLevel.Low;
        }

        // other methods snipped
    }
}
```

Here is example `LayoutCycleTracingLevel.Low` output for a layout cycle crash which Slider used to hit:
```
[LayoutCycleTracing] Layout Iteration Countdown: 7. Raising SizeChanged Events.
[LayoutCycleTracing] "SetValue(Column)","LayoutCycleCountdown: 7","ClassName: Microsoft.UI.Xaml.Shapes.Rectangle","Name: HorizontalDecreaseRect","Instance: 0x000001C67FB0EE70","TypeIndex: 701"
[LayoutCycleTracing] "SetValue(Width=866.400024)","LayoutCycleCountdown: 7","ClassName: Microsoft.UI.Xaml.Shapes.Rectangle","Name: HorizontalDecreaseRect","Instance: 0x000001C67FB0EE70","TypeIndex: 701"
[LayoutCycleTracing] "LayoutCycleCountdown: 7","ClassName: Microsoft.UI.Xaml.Shapes.Rectangle","Name: HorizontalDecreaseRect","Instance: 0x000001C67FB0EE70","TypeIndex: 701"
[LayoutCycleTracing] "LayoutCycleCountdown: 7","ClassName: Microsoft.UI.Xaml.Controls.Primitives.TickBar","Name: TopTickBar","Instance: 0x000001C677687F40","TypeIndex: 718"
[LayoutCycleTracing] "LayoutCycleCountdown: 7","ClassName: Microsoft.UI.Xaml.Controls.Primitives.TickBar","Name: HorizontalInlineTickBar","Instance: 0x000001C67FB349F0","TypeIndex: 718"
[LayoutCycleTracing] "LayoutCycleCountdown: 7","ClassName: Microsoft.UI.Xaml.Controls.Primitives.TickBar","Name: BottomTickBar","Instance: 0x000001C67FB33F40","TypeIndex: 718"
[LayoutCycleTracing] Layout Iteration Countdown: 6. Launching Measure Pass.
[LayoutCycleTracing] "DesiredSize changed, old: 865.599976x0.000000 new: 866.400024x0.000000","LayoutCycleCountdown: 6","ClassName: Microsoft.UI.Xaml.Shapes.Rectangle","Name: HorizontalDecreaseRect","Instance: 0x000001C67FB0EE70","TypeIndex: 701"
[LayoutCycleTracing] "LayoutCycleCountdown: 6","ClassName: Microsoft.UI.Xaml.Controls.Primitives.TickBar","Name: TopTickBar","Instance: 0x000001C677687F40","TypeIndex: 718"
[LayoutCycleTracing] "LayoutCycleCountdown: 6","ClassName: Microsoft.UI.Xaml.Controls.Primitives.TickBar","Name: BottomTickBar","Instance: 0x000001C67FB33F40","TypeIndex: 718"
[LayoutCycleTracing] Layout Iteration Countdown: 5. Launching Arrange Pass.
[LayoutCycleTracing] "oldSize: 885.599976x33.599998","LayoutCycleCountdown: 5","ClassName: Microsoft.UI.Xaml.Controls.Grid","Name: SliderContainer","Instance: 0x000001C6777B3FE0","TypeIndex: 672"
[LayoutCycleTracing] Layout Iteration Countdown: 4. Raising SizeChanged Events.
[LayoutCycleTracing] "SetValue(Column)","LayoutCycleCountdown: 4","ClassName: Microsoft.UI.Xaml.Shapes.Rectangle","Name: HorizontalDecreaseRect","Instance: 0x000001C67FB0EE70","TypeIndex: 701"
[LayoutCycleTracing] "SetValue(Width=865.599976)","LayoutCycleCountdown: 4","ClassName: Microsoft.UI.Xaml.Shapes.Rectangle","Name: HorizontalDecreaseRect","Instance: 0x000001C67FB0EE70","TypeIndex: 701"
[LayoutCycleTracing] "LayoutCycleCountdown: 4","ClassName: Microsoft.UI.Xaml.Shapes.Rectangle","Name: HorizontalDecreaseRect","Instance: 0x000001C67FB0EE70","TypeIndex: 701"
[LayoutCycleTracing] "LayoutCycleCountdown: 4","ClassName: Microsoft.UI.Xaml.Controls.Primitives.TickBar","Name: TopTickBar","Instance: 0x000001C677687F40","TypeIndex: 718"
[LayoutCycleTracing] "LayoutCycleCountdown: 4","ClassName: Microsoft.UI.Xaml.Controls.Primitives.TickBar","Name: HorizontalInlineTickBar","Instance: 0x000001C67FB349F0","TypeIndex: 718"
[LayoutCycleTracing] "LayoutCycleCountdown: 4","ClassName: Microsoft.UI.Xaml.Controls.Primitives.TickBar","Name: BottomTickBar","Instance: 0x000001C67FB33F40","TypeIndex: 718"
[LayoutCycleTracing] Layout Iteration Countdown: 3. Launching Measure Pass.
[LayoutCycleTracing] "DesiredSize changed, old: 866.400024x0.000000 new: 865.599976x0.000000","LayoutCycleCountdown: 3","ClassName: Microsoft.UI.Xaml.Shapes.Rectangle","Name: HorizontalDecreaseRect","Instance: 0x000001C67FB0EE70","TypeIndex: 701"
[LayoutCycleTracing] "LayoutCycleCountdown: 3","ClassName: Microsoft.UI.Xaml.Controls.Primitives.TickBar","Name: TopTickBar","Instance: 0x000001C677687F40","TypeIndex: 718"
[LayoutCycleTracing] "LayoutCycleCountdown: 3","ClassName: Microsoft.UI.Xaml.Controls.Primitives.TickBar","Name: BottomTickBar","Instance: 0x000001C67FB33F40","TypeIndex: 718"
[LayoutCycleTracing] Layout Iteration Countdown: 2. Launching Arrange Pass.
[LayoutCycleTracing] "oldSize: 884.799988x33.599998","LayoutCycleCountdown: 2","ClassName: Microsoft.UI.Xaml.Controls.Grid","Name: SliderContainer","Instance: 0x000001C6777B3FE0","TypeIndex: 672"
[LayoutCycleTracing] Layout Iteration Countdown: 1. Raising SizeChanged Events.
[LayoutCycleTracing] "SetValue(Column)","LayoutCycleCountdown: 1","ClassName: Microsoft.UI.Xaml.Shapes.Rectangle","Name: HorizontalDecreaseRect","Instance: 0x000001C67FB0EE70","TypeIndex: 701"
[LayoutCycleTracing] "SetValue(Width=866.400024)","LayoutCycleCountdown: 1","ClassName: Microsoft.UI.Xaml.Shapes.Rectangle","Name: HorizontalDecreaseRect","Instance: 0x000001C67FB0EE70","TypeIndex: 701"
[LayoutCycleTracing] "LayoutCycleCountdown: 1","ClassName: Microsoft.UI.Xaml.Shapes.Rectangle","Name: HorizontalDecreaseRect","Instance: 0x000001C67FB0EE70","TypeIndex: 701"
[LayoutCycleTracing] "LayoutCycleCountdown: 1","ClassName: Microsoft.UI.Xaml.Controls.Primitives.TickBar","Name: TopTickBar","Instance: 0x000001C677687F40","TypeIndex: 718"
[LayoutCycleTracing] "LayoutCycleCountdown: 1","ClassName: Microsoft.UI.Xaml.Controls.Primitives.TickBar","Name: HorizontalInlineTickBar","Instance: 0x000001C67FB349F0","TypeIndex: 718"
[LayoutCycleTracing] "LayoutCycleCountdown: 1","ClassName: Microsoft.UI.Xaml.Controls.Primitives.TickBar","Name: BottomTickBar","Instance: 0x000001C67FB33F40","TypeIndex: 718"
[LayoutCycleTracing] Layout Iteration Countdown: 0. Launching Measure Pass.
[LayoutCycleTracing] "DesiredSize changed, old: 865.599976x0.000000 new: 866.400024x0.000000","LayoutCycleCountdown: 0","ClassName: Microsoft.UI.Xaml.Shapes.Rectangle","Name: HorizontalDecreaseRect","Instance: 0x000001C67FB0EE70","TypeIndex: 701"
[LayoutCycleTracing] "LayoutCycleCountdown: 0","ClassName: Microsoft.UI.Xaml.Controls.Primitives.TickBar","Name: TopTickBar","Instance: 0x000001C677687F40","TypeIndex: 718"
[LayoutCycleTracing] "LayoutCycleCountdown: 0","ClassName: Microsoft.UI.Xaml.Controls.Primitives.TickBar","Name: BottomTickBar","Instance: 0x000001C67FB33F40","TypeIndex: 718"
```

These are some of the common properties listed in the tracing output:
* **LayoutCycleCountdown:** Specifies the current layout pass count. The layout pass count starts at the maximum allowed passes and counts down to 0. If the layout process doesn't converge by the end of pass 0, then a layout cycle exception occurs.
* **ClassName:** The class name of the `FrameworkElement` subclass associated with the message.
* **Name:** The `FrameworkElement.Name` of the `FrameworkElement` associated with the message, if any.
* **Instance:** The memory address of the internal representation of the `FrameworkElement` associated with the message. This is particularly useful to filter to trace messages for individual instances.
* **TypeIndex:** This can usually be ignored. This represents the internal index of the `FrameworkElement` class type.
* **SetValue:** When a property which affects layout gets set, this lists the name of the property and also lists the new value for some value types.
* **DesiredSize:** When the result of measuring a `FrameworkElement` is a different size than when it was previously measured, this shows the change in desired size.

Looking at the log output above, the most interesting information is that the `Width` (and
resulting desired size) of the `HorizontalDecreaseRect` element was toggling between 866.400024
and 865.599976. Here is the relevant subset of lines from the full output above:
```
[LayoutCycleTracing] "SetValue(Width=866.400024)","LayoutCycleCountdown: 7","ClassName: Microsoft.UI.Xaml.Shapes.Rectangle","Name: HorizontalDecreaseRect","Instance: 0x000001C67FB0EE70","TypeIndex: 701"
[LayoutCycleTracing] "DesiredSize changed, old: 865.599976x0.000000 new: 866.400024x0.000000","LayoutCycleCountdown: 6","ClassName: Microsoft.UI.Xaml.Shapes.Rectangle","Name: HorizontalDecreaseRect","Instance: 0x000001C67FB0EE70","TypeIndex: 701"
[LayoutCycleTracing] "SetValue(Width=865.599976)","LayoutCycleCountdown: 4","ClassName: Microsoft.UI.Xaml.Shapes.Rectangle","Name: HorizontalDecreaseRect","Instance: 0x000001C67FB0EE70","TypeIndex: 701"
[LayoutCycleTracing] "DesiredSize changed, old: 866.400024x0.000000 new: 865.599976x0.000000","LayoutCycleCountdown: 3","ClassName: Microsoft.UI.Xaml.Shapes.Rectangle","Name: HorizontalDecreaseRect","Instance: 0x000001C67FB0EE70","TypeIndex: 701"
[LayoutCycleTracing] "SetValue(Width=866.400024)","LayoutCycleCountdown: 1","ClassName: Microsoft.UI.Xaml.Shapes.Rectangle","Name: HorizontalDecreaseRect","Instance: 0x000001C67FB0EE70","TypeIndex: 701"
[LayoutCycleTracing] "DesiredSize changed, old: 865.599976x0.000000 new: 866.400024x0.000000","LayoutCycleCountdown: 0","ClassName: Microsoft.UI.Xaml.Shapes.Rectangle","Name: HorizontalDecreaseRect","Instance: 0x000001C67FB0EE70","TypeIndex: 701"
```

Investigating this further, including using `DebugSettings.LayoutCycleDebugBreakLevel` to break
into the debugger to investigate variables, helped to find an issue in layout rounding code.

# API pages

## LayoutCycleTracingLevel enum

Defines constants for the debug output tracing level to use when a layout cycle crash appears
imminent.

Namespace: Microsoft.UI.Xaml

| **Name** | **Value** | **Description** |
|----------|-----------|-----------------|
| None     | 0         | There will be no debug output. Minimal layout cycle information will be preserved in [stowed exceptions](https://learn.microsoft.com/en-us/windows/apps/windows-app-sdk/migrate-to-windows-app-sdk/guides/threading#stowed-exceptions) in crash dumps. |
| Low      | 1         | Write to the native debugger output just the minimal information preserved in crash dumps from the `None` level. |
| High     | 2         | In addition to the `Low` output, add in more verbose output including all `Measure` and `Arrange` calls with their `availableSize` and `finalSize` values. The extra output will also be preserved in [stowed exceptions](https://learn.microsoft.com/en-us/windows/apps/windows-app-sdk/migrate-to-windows-app-sdk/guides/threading#stowed-exceptions) in crash dumps. |

**Internal detail:** Normally the number of layout contexts (or any WarningContext type) is
limited to 50 (MAX_WARNING_CONTEXTS). To handle the higher verbosity of `High`, this limit
should be increased when that mode is in use, currently implemented as 3x.

## DebugSettings.LayoutCycleTracingLevel

Gets or sets the layout cycle tracing level. When a layout cycle crash appears imminent, this
tracing level is used to determine how much debugging information to preserve in crash dumps
and whether to output any information to the native debugger output. The default is **None**.

```cs
public LayoutCycleTracingLevel LayoutCycleTracingLevel { get; set; }
```


## LayoutCycleDebugBreakLevel enum

Defines constants for which level of layout cycle tracing events should trigger a debugger breakpoint
when a layout cycle crash appears imminent. These levels currently match the levels in `LayoutCycleTracingLevel`.
If layout cycle tracing is also enabled for an event, the debugger breakpoint is triggered after outputting
the trace message.

Namespace: Microsoft.UI.Xaml

| **Name** | **Value** | **Description** |
|----------|-----------|-----------------|
| None     | 0         | Never trigger a debugger breakpoint. |
| Low      | 1         | Trigger a debugger breakpoint only at the minimal level, matching `LayoutCycleTracingLevel.Low`. |
| High     | 2         | In addition to the `Low` level, also trigger a debugger breakpoint on more events including all `Measure` and `Arrange` calls, matching `LayoutCycleTracingLevel.High`. |

## DebugSettings.LayoutCycleDebugBreakLevel

Gets or sets the layout cycle debug break level. When a layout cycle crash appears imminent, this
debug break level is used to determine when a native debugger breakpoint will be triggered for
layout-related operations. The default is **None**.

A debugger breakpoint will only be triggered if a native debugger is attached to the process
to avoid crashing the process when not run under a debugger or when only attached with a
managed debugger.

This debug break level is independent of the `DebugSettings.LayoutCycleTracingLevel` level to enable
a common scenario of wanting a higher tracing level than debug break level.

```cs
public LayoutCycleDebugBreakLevel LayoutCycleDebugBreakLevel { get; set; }
```




# API Details

```cs
namespace Microsoft.UI.Xaml
{
    enum LayoutCycleTracingLevel
    {
        None = 0,
        Low = 1,
        High = 2,
    }
}
```

```cs
namespace Microsoft.UI.Xaml
{
    enum LayoutCycleDebugBreakLevel
    {
        None = 0,
        Low = 1,
        High = 2,
    }
}
```

```cs
namespace Microsoft.UI.Xaml
{
    runtimeclass DebugSettings
    {
        // existing APIs snipped

        LayoutCycleTracingLevel LayoutCycleTracingLevel;
        LayoutCycleDebugBreakLevel LayoutCycleDebugBreakLevel;
    }
}
```
