ScrollView/ScrollPresenter/IScrollController overviews
===

# Background

The ScrollView is a new control which allows scrolling and zooming operations like the old [ScrollViewer](ScrollViewer-overview.md) control, but it uses the new Composition 
InteractionTracker component behind the scene instead of DirectManipulation. This allows a tighter integration with the Composition layer and 
provides more flexibility to devs as far as programmatically controlling the curves over time of the position and scale of the manipulated content.

The ScrollView is comparable to the ScrollViewer, while the ScrollPresenter is comparable to the old ScrollContentPresenter. The ScrollPresenter 
just hosts the manipulated content, while the ScrollView brings ScrollBars into the picture.

The IScrollController interface is a standard contract used for the communication between the ScrollPresenter and the stock ScrollBars. But more 
importantly, it allows plugging in other scrolling widgets than your typical scrollbar. One of those alternatives is our own AnnotatedScrollBar control.

ScrollView with shy (mouse over content) ScrollBar and with expanded (mouse over scrollbar) ScrollBar:  
![ScrollView with ScrollBars](Images/ScrollView-with-scrollbars.png)

ScrollView with alternative IScrollController implementation:  
![ScrollView with AnnotatedScrollBar](Images/ScrollView-with-custom-scrollcontroller.png)

The InteractionTracker produces translation and scale transforms that get applied to the manipulated content independently of the UI thread, 
for smooth visual changes.

The ScrollViewer and ScrollView feature sets overlap but each has missing capabilities compared to the other.

Examples: 
- The ScrollView does not support header elements.
- The ScrollViewer does not support customization of its inertia curve.


# Checklist
- Scope of the area  
Large. The overall codebase involved in anything ScrollView-related is about 3 MB of code.

- Traffic/inflow in this area  
Minimal. Not many controls make use of the ScrollView yet.

- Dependencies  
InteractionTracker

- Active Work / Bugs
  * [GitHub] Blurry text on ScrollView

- Active Enhancement / Committed work  
  * [GitHub] ScrollView bugs/missing functionality compared to the platform ScrollViewer

- Active Backlog  
  * [GitHub] ScrollView and ScrollViewer have different scroll speeds (I'm guessing By Design)
  * [GitHub] RefreshContainer doesn't work with ScrollView
  * ScrollView: Shows one blank frame when jumping to disconnected offset
  * ScrollView: ScrollView spec document updates/feedback from API review

- Best practices  
Do make use of the sprinkled debug outputs in most ScrollView-related components.
A ton of debug outputs can be turned on by recompiling Xaml controls with s_IsDebugOutputEnabled (for non-verbose) 
and s_IsVerboseDebugOutputEnabled (for verbose) flags set to true.

  Examples:
    * In ScrollPresenter.cpp:  
        // Change to 'true' to turn on debugging outputs in Output window  
        bool ScrollPresenterTrace::s_IsDebugOutputEnabled{ false };  
        bool ScrollPresenterTrace::s_IsVerboseDebugOutputEnabled{ false };  

    * In ScrollView.cpp:  
        // Change to 'true' to turn on debugging outputs in Output window  
        bool ScrollViewTrace::s_IsDebugOutputEnabled{ false };  
        bool ScrollViewTrace::s_IsVerboseDebugOutputEnabled{ false };  

  ScrollPresenter::***Dbg are methods dedicated to debugging only. It's a pattern that should be followed generally.

  Do use the new scrolling/zooming controls when developping new features or controls.


# Key control parts

Inside the ScrollView control template, we have four key elements:
- the ScrollPresenter which is a FrameworkElement showing the ScrollPresenter.Content == ScrollView.Content.
- the horizontal ScrollBar
- the vertical ScrollBar
- a little square separator between the scrollbars, which is a Border.

Typical layout:  
![Layout approximation](Images/ScrollViewTypicalParts.png)


# Key source code locations

These are paths for Microsoft.UI.Xaml.Controls.

## IScrollController

- Product: \controls\dev\ScrollPresenter\ScrollPresenterPrimitives.idl
- Test: \controls\dev\ScrollPresenter\TestUI\*Controller*


## ScrollPresenter

- Product: \controls\dev\ScrollPresenter
- Test: \controls\dev\ScrollPresenter\APITest & InteractionTests


## ScrollView

- Product: \controls\dev\ScrollView
- Test: \controls\dev\ScrollView\APITest & InteractionTests


# ScrollView XAML markup resources

Style resources, including the control template, are located under \controls\dev\ScrollView:
- \controls\dev\ScrollView\ScrollView_themeresources.xaml
- \controls\dev\ScrollView\ScrollView.xaml


# MuxControlsTestApp test pages

- ScrollView

Examples:  
![Top level test page](images\ScrollViewTestPage1.png)  
![APIs test page](images\ScrollViewTestPage2.png)

- ScrollPresenter

Examples:  
![Top level test page](images\ScrollPresenterTestPage1.png)  
![APIs test page](images\ScrollPresenterTestPage2.png)

- IScrollController pages are within the ScrollView/ScrollPresenter sections.


# Key methods when debugging

You may want to set breakpoints in these methods:
- ScrollPresenter::ArrangeOverride
- ScrollPresenter::***StateEntered
- ScrollPresenter::ValuesChanged
- ScrollPresenter::OnPointerPressed
- ScrollPresenter::ProcessOffsetsChange
- ScrollPresenter::ProcessZoomFactorChange


# Key fields when debugging

You may want to keep track of these important fields:
- float ScrollPresenter::m_zoomFactor{ 1.0f };
- double ScrollPresenter::m_zoomedHorizontalOffset{ 0.0 };
- double ScrollPresenter::m_zoomedVerticalOffset{ 0.0 };
- double ScrollPresenter::m_unzoomedExtentWidth{ 0.0 };
- double ScrollPresenter::m_unzoomedExtentHeight{ 0.0 };
- double ScrollPresenter::m_viewportWidth{ 0.0 };
- double ScrollPresenter::m_viewportHeight{ 0.0 };
- std::list<std::shared_ptr<InteractionTrackerAsyncOperation>> ScrollPresenter::m_interactionTrackerAsyncOperations;
- winrt::ScrollingInteractionState ScrollPresenter::m_state{ winrt::ScrollingInteractionState::Idle };



# Factoids about the ScrollPresenter

- It is UI-less & the 'heart' of the wrapping ScrollView.
- It uses a ScrollPresenterAutomationPeer automation peer which supports the IScrollProvider interface.
- It supports scroll and zoom snap points but not the IScrollSnapPointsInfo interface (yet). Snap points have to be manually fed to the control.
- It supports optional and mandatory snap points like the ScrollViewer.
- It implements the IScrollAnchorProvider interface to support anchoring:
  * https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.iscrollanchorprovider?view=windows-app-sdk-1.6
  * https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.primitives.scrollpresenter.horizontalanchorratio?view=windows-app-sdk-1.6
  * https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.primitives.scrollpresenter.verticalanchorratio?view=windows-app-sdk-1.6
- It exposes an ExpressionAnimationSources property of type CompositionPropertySet: https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.primitives.scrollpresenter.expressionanimationsources?view=windows-app-sdk-1.6.
It's a Composition property bag that includes animated composition values that can be used as sources of Composition animations. Properties include viewport / extent sizes, 
position & scale.
- The underlying InteractionTracker handles mouse wheel input for smooth scrolling.
- Thanks to the IScrollController contract between the ScrollPresenter and its scrolling widgets, those widgets can have animated UI pieces, like ScrollBar thumbs, that are
kept in sync with the primary ScrollPresenter.Content, off the UI thread.  So when the user pans the Content, the linked thumbs are animated smoothly using constant velocity ratios.
- Unlike with DirectManipulation, the ScrollPresenter consumer can provide a Composition animation to animate the Position or Scale of the Content.
- So the developer can control the animation curves entirely, or simply accelerate / decelerate the stock animations.
- The use of public inertia decay rates easily allows constant velocity scrolling and zooming.
- All developer-requested Position / Scale changes are asynchronous. They are queued up and handed off to the InteractionTracker a few ticks later to allow possible
size changes to settle down.
- Scroll requests coming from an IScrollController implementation are also fulfilled asynchronously, but executed as soon as possible for a more responsive user experience.
- The ScrollPresenter implements an internal interface called IRepeaterScrollingSurface. This interface can be deleted in the future,
along with the ViewportManagerDownLevel class.

# Factoids about the ScrollView

- It uses a ScrollBarController class defined in controls/dev/ScrollView/ScrollBarController.cpp, that wraps the ScrollBar control and turns it into
an IScrollController implementation. That implementation is handed off to the ScrollView's ScrollPresenter, for the HorizontalScrollController
and VerticalScrollController properties. 
https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.primitives.scrollpresenter.horizontalscrollcontroller?view=windows-app-sdk-1.6
- The object model surfaced by the ScrollView control is pretty much the same as for its inner ScrollPresenter. It typically just delegates the API calls to its ScrollPresenter.
- ScrollView does not implement IScrollAnchorProvider, but exposes public CurrentAnchor, RegisterAnchorCandidate & UnregisterAnchorCandidate APIs which delegate to the ScrollPresenter's
IScrollAnchorProvider implementation.
- The ScrollView handles keyboard-driven scrolling and zooming.


# Features that make use of InteractionTracker-driven scrolling

## Bring-into-view operations

Source code involved (in MUX):
- dxaml/xcp/core/input/InputPaneHandler.cpp
- dxaml/xcp/core/input/BringIntoViewHandler.cpp
- dxaml/xcp/core/inc/BringIntoViewRequestedEventArgs.h
- dxaml/xcp/core/core/elements/BringIntoViewRequestedEventArgs.cpp
- dxaml/xcp/core/core/elements/uielement.cpp's CUIElement::BringIntoView


## Parallaxing with the ParallaxView control

The ParallaxView control supports both the ScrollViewer and ScrollPresenter.


# Features that also make use of the InteractionTracker

The SwipeControl and PullToRefresh controls.


# Controls that use the ScrollView
- ItemsView


# Controls that use the ScrollPresenter
- ScrollView
- ParallaxView


# Related docs

The [ScrollView](ScrollView-spec.md) & [ScrollPresenter](ScrollPresenter-spec) (for IScrollController too) documents provide lots of information 
about the expected behaviors.
