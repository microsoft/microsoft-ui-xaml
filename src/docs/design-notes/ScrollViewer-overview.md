ScrollViewer overview
===

# Background

The ScrollViewer is a XAML framework control that can handle scrolling and zooming.

In OS XAML, it ships in the \Windows\System32\Windows.UI.Xaml.dll (WUX dll) and in WinUI 3, it ships in the Microsoft.UI.Xaml.dll (MUX dll).

Internally, it uses the [DirectManipulation](https://learn.microsoft.com/en-us/windows/win32/api/_directmanipulation/) technology to perform the scrolling and zooming operations. 
DirectManipulation was orgininally a Win32 API set, shipped in \Windows\System32\directmanipulation.dll. 
The code was lifted alongside Xaml and now lives in Microsoft.DirectManipulation.dll for WinAppSdk applications.

The use of DirectManipulation allows manipulations to be performed independently of the UI thread, after they get initiated on the UI thread.
So for example, the manipulated element can follow a finger, or move with inertia while the UI thread is blocked for some reason.

DirectManipulation is involved behind the scene for all input kinds: touch, pen, trakpad, mouse pointer with ScrollBar.

Both DirectManipulation and the ScrollViewer control have been in maintenance mode for years. No further investments are planned for those components.
They have been replaced with newer offerings, the Composition InteractionTracker and Xaml ScrollView respectively, to take better advantage of the Composition layer.


# Checklist
- Scope of the area
Huge. The overall codebase involved in anything ScrollViewer-related is very large. At least 300000 lines of code?

- Traffic/inflow in this area
Very minimal as this feature has been in maintenance mode for years.

- Dependencies
DirectManipulation

- Active Work / Bugs
[GitHub] ComboBox in a ScrollViewer in Xaml Islands can be scrolled off the window
Some ScrollViewer anchoring scenarios are broken
[GitHub] ScrollViewer children are silently losing pointer events after PointerExited with pen & touch
[GitHub] AutoSuggestBox does not correctly clip in ScrollViewer
[GitHub] when the zoom is set to 200%. ScrollViewer has the wrong center point when zooming using Ctrl+Wheel.
[GitHub] TabFocusNavigation Once does not propagate down through ScrollViewer

- Active Backlog
[GitHub] Scrolling is terrible when ScrollViewer IsScrollInertiaEnabled is False (I'm guessing By Design)

- Active Enhancement / Committed work
None

- Best practices
Do make use of the sprinkled debug outputs in most components of the ScrollViewer.
A ton of debug outputs can be turned on by recompiling Xaml 

    * with DM_DEBUG #defined being uncommented:

    Example:
    In ScrollViewer_Partial.h:
    // Uncomment for DManip debug outputs.
    //#define DM_DEBUG

    * with DM**** #defines being set to 1 instead of 0, for non-verbose and verbose outputs.

    Example:
    In DirectManipulationService.cpp:
    // Define as 1 (i.e. XCP_TRACE_OUTPUT_MSG) to get debug outputs, and 0 otherwise
    #define DMS_DBG 0

    // Define as 1 (i.e. XCP_TRACE_OUTPUT_MSG) to get verbose debug outputs, and 0 otherwise
    #define DMSv_DBG 0


# Key control parts

Inside the ScrollViewer control template, we have four key elements:
- the ScrollContentPresenter which is a ControlPresenter showing the ScrollViewer.Content, and potentially three header elements.
- the horizontal ScrollBar
- the vertical ScrollBar
- a little square separator between the scrollbars

Typical layout:  
![Layout approximation without headers](Images/ScrollViewerTypicalParts.png)

Rare usage with headers:  
![Layout approximation with headers](Images/ScrollViewerControlTemplate.png)


# Key source code locations

PAL means Platform Abstraction Layer. This originated during the Silverlight years when we were targetting both Windows and Apple devices.
- dxaml/xcp/plat/win/browserdesktop/DirectManipulation*
- dxaml/xcp/pal/inc/PalDirectManipulation*.h

Define the layer that directly interacts with the DirectManipulation (DM / DManip) APIs, and were originally designed to be an abstract wrapper for those DM APIs.
The key file is dxaml/xcp/plat/win/browserdesktop/DirectManipulationService.cpp.

Input Manager code:
- dxaml/xcp/core/input/InputManagerNotifyUpdate.h
- dxaml/xcp/core/input/InputManagerDMViewportEventHandler.cpp
- dxaml/xcp/core/input/DMContent.cpp
- dxaml/xcp/core/input/DMDeferredRelease.cpp
- dxaml/xcp/core/input/DMViewport.cpp
- dxaml/xcp/core/input/InputServices.cpp
- dxaml/xcp/core/inc/InputServices.h
- dxaml/xcp/core/inc/DirectManipulationContainer.h / DirectManipulationContainerHandler.h / DirectManipulationControllers.h / DirectManipulationServiceSharedState.h
- dxaml/xcp/core/inc/DMContent.h / DMDeferredRelease.h / DMViewport.h
- dxaml/xcp/core/inc/UIDMContainer.h / UIDMContainerHandler.h
- dxaml/xcp/core/optional/elements/touch/UIDMContainer.cpp
- dxaml/xcp/core/optional/elements/touch/UIDMContainerHandler.cpp

Core control code:
- dxaml/xcp/core/inc/ScrollViewer.h
- dxaml/xcp/core/core/elements/ScrollViewer.cpp
- dxaml/xcp/core/core/elements/ScrollContentControl.cpp

Compositor code:
- dxaml/xcp/core/input/DirectManipulationServiceSharedState.cpp 
- dxaml/xcp/components/comptree/HWCompNodeWinRT.cpp
- dxaml/xcp/components/graphics/ExpressionHelper.cpp
- dxaml/xcp/core/hw/DManipData.cpp
- dxaml/xcp/core/hw/hwcompnode.cpp
- dxaml/xcp/core/hw/ManipulationTransform.cpp

DirectUI code:
- dxaml/xcp/dxaml/lib/ScrollContentPresenter_Partial.h / .cpp
- dxaml/xcp/dxaml/lib/ScrollViewer_Partial.h / .cpp
- dxaml/xcp/dxaml/lib/ScrollViewerAutomationPeer_Partial.h / .cpp
- dxaml/xcp/dxaml/lib/ScrollData.h / .cpp
- dxaml/xcp/dxaml/lib/SecondaryContentRelationship_Partial.h / .cpp
- dxaml/xcp/dxaml/lib/UIElement_Partial.cpp
- dxaml/xcp/dxaml/lib/InputSiteAdapter.cpp

Test code:
- dxaml/test/managed/enterprise/ScrollViewer
- dxaml/test/managed/controls/flipview  combobox  textbox  richeditbox
- dxaml/test/native/external/controls/scrollviewer  flipview  semanticzoom  textbox  richeditbox  contentdialog  combobox  listbox  hub  pivot  
- dxaml/test/native/external/foundation/input/DManip
- dxaml/test/native/external/enterprise/CalendarView  GridView  ListView  SemanticZoom  StickyHeaders

- controls/dev/CommonStyles/TestUI/ScrollViewerPage.xaml
- controls/dev/CommonStyles/TestUI/ScrollViewerPage.xaml.cs


# ScrollViewer XAML markup resources

Style resources, including the control template, are located under controls/dev/CommonStyles/ScrollViewer_themeresources.xaml for WinUI 3.


# Key methods when debugging

You may want to set breakpoints in these methods:
- CInputServices::ProcessUIThreadTick
- CDirectManipulationService::SetContact
- CDirectManipulationService::GetPrimaryContentTransform
- ScrollViewer::NotifyManipulationProgress
- ScrollViewer::ChangeViewInternal
- ScrollViewer::InvalidateScrollInfoImpl
- ScrollViewer::MeasureOverride
- ScrollContentPresenter::VerifyScrollData
- ScrollContentPresenter::MeasureOverride


# Key fields when debugging

You may want to keep track of these fields:
- CInputServices::m_pViewports
- CInputServices::m_pDMServices

- CDMViewport::m_pManipulatedElement
- CDMViewport::m_state
- CDMViewport::m_touchConfiguration
- CDMViewport::m_nonTouchConfiguration
- CDMViewport::m_bringIntoViewportConfiguration
- CDMViewport::m_statuses

- CDirectManipulationService::m_mapViewports (in practice there is only one viewport per CDirectManipulationService instance)

- ScrollViewer::m_dmanipState


# Factoids about the ScrollViewer

- DirectManipulation-driven touch manipulations (i.e. scrolling and zooming) are enabled when the touched UIElement's ManipulationMode property includes ManipulationModes.System.
Every UIElement in its parent path to ScrollViewer.Content must also include the System flag, or else no manipulation is triggered.

- The MuxControlsTestApp found under WinUI 3's \BuildOutput\bin\amd64chk\Test\UnpackagedApps\MUXControlsTestApp folder has a basic test page dedicated to the ScrollViewer, called !["ScrollViewer"](Images/ScrollViewerTestPage.jpg).

- There is a special ScrollViewer at the root of the UI tree called 'RootScrollViewer' which involves software keyboards: the app's UI is shrunk vertically when the soft keyboard appears and expanded when it disappears.

- The ScrollViewer supports header elements that do not scroll in certain direction:
    * ScrollViewer.TopLeftHeader: does not scroll at all.
    * ScrollViewer.LeftHeader: does not scroll horizontally.
    * ScrollViewer.TopHeader: does not scroll vertically.

- Each ScrollViewer makes use of:
    * A DManip Manager
    * A DManip Viewport
    * A DManip Content

- DManip generates a matrix transform which gets applied to the UIElement being manipulated on the compositor thread, and the UI thread.
The transform contributes to the global placement evaluation of an element in CUIElement::GetLocalTransformHelper.

- The ScrollViewer.Content can implement the IScrollSnapPointsInfo interface to support scrolling snap points:
https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.primitives.iscrollsnappointsinfo?view=windows-app-sdk-1.6
But snap points are not applied when scrolling with a ScrollBar's Thumb + mouse pointer.
Snap points can be optional or mandatory.

- The built-in StackPanel, CarouselPanel, ModernCollectionBasePanel, OrientedVirtualizingPanel panels implement that IScrollSnapPointsInfo interface.

- The ScrollViewer also supports zooming snap points (https://learn.microsoft.com/en-us/uwp/api/windows.ui.xaml.controls.scrollviewer.zoomsnappoints?view=winrt-26100).
No interface is involved for those.

- The ScrollViewer and ScrollBar communicate through the IScrollInfo interface.

- The ScrollViewer implements the IScrollAnchorProvider interface to support anchoring:
  * https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.iscrollanchorprovider?view=windows-app-sdk-1.6
  * https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.scrollviewer.horizontalanchorratio?view=windows-app-sdk-1.6
  * https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.scrollviewer.verticalanchorratio?view=windows-app-sdk-1.6

- DManip is still handed off a HWND - it's one of the rare remaining component that still operates with HWND.

- Typically a ScrollViewer content and viewport size are expressed in pixels.
But some old school ItemsControl panels use unit sizes, as in how many items are there in the content, and how many items fit in the viewport.
In those rare cases, the ScrollViewer.Content implements IManipulationDataProvider.

- The ScrollViewer exposes a Composition PropertySet containing manipulated transform matrices
This is done through the ElementCompositionPreview.GetScrollViewerManipulationPropertySet method: https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.hosting.elementcompositionpreview.getscrollviewermanipulationpropertyset?view=windows-app-sdk-1.6
It lets developers drive a Composition animation with the DManip output.
The PropertySet includes these properties:
  * Translation Vector3,
  * Pan Vector3,
  * Scale Vector3,
  * CenterPoint Vector3,
  * Matrix Matrix4x4

- A UIElement inside ScrollViewer.Content can be manipulated via UIElement.ManipulationXXX events when using ManipulationModes values other than System.


# Features that make use of DirectManipulation-driven scrolling

## Bring-into-view operations

Source code involved (in MUX):
- dxaml/xcp/core/input/InputPaneHandler.cpp
- dxaml/xcp/core/input/BringIntoViewHandler.cpp
- dxaml/xcp/core/inc/BringIntoViewRequestedEventArgs.h
- dxaml/xcp/core/core/elements/BringIntoViewRequestedEventArgs.cpp
- dxaml/xcp/core/core/elements/uielement.cpp's CUIElement::BringIntoView


## Edge-scrolling in ListView/GridView controls

Edge-scrolling, also known as constant velocity scrolling and auto-scrolling, is triggered when the user drags an item close to the edge of a ListView/GridView during an item re-ordering operation or drag-and-drop operation.
Code involved:
- CDirectManipulationService::ActivateAutoScroll
- CDirectManipulationService::StopAutoScroll


## Sticky headers in ListView control

ListView supports sticky headers for grouped items. That is supported in particular by:
- dxaml\xcpdxaml/lib/StickyHeaderWrapper.h / .cpp
- dxaml\xcpdxaml/lib/SecondaryContentRelationship_Partial.h / .cpp


## Parallaxing with the ParallaxView control

The ParallaxView control supports both the ScrollViewer and ScrollPresenter.


## Pull-to-refresh with the RefreshContainer / RefreshVisualizer controls

Pull-to-refresh only works with the ScrollViewer and not the new ScrollPresenter.


# Controls that use the ScrollViewer
- CalendarView
- FlipView
- ListView
- GridView
- ComboBox
- TextBox
- RichEditBox
- PaswordBox
- ContentDialog
- SemanticZoom
- FlyoutPresenter
- MenuFlyoutPresenter
- CommandBarOverflowPresenter
- Hub
- Pivot
- LoopingSelector
- ListBox

- ParallaxView
- PullToRefresh


# Somewhat related doc

The [ScrollView](ScrollView-spec.md) document compares the new ScrollView usages with the old ScrollViewer ones.
