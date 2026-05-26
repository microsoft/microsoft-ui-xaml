Layout manager overview
===

# Background

A key part of the XAML framework is its layout manager, which is responsible for arranging the UI on the screen.
The UI, made up of UIElement elements, is typically defined in XAML markup which imposes layout relationships 
and constraints.

Layout is performed in two fundamental passes: a measure pass and an arrange pass. This first determines the 
sizes of the various elements, while the second arranges them based on those sizes.
There is also a pass for computing the effective viewports, typically introduced by scrollers in the UI.


# Checklist
- Scope of the area  
Large.

- Traffic/inflow in this area  
Minimal, as this old feature has not seen much churn in recent years.

- Dependencies  
None.

- Active Work / Bugs  
  * [Watson Failure] caused by ACCESS_VIOLATION_c0000005_Microsoft.UI.Xaml.dll!event_base_event_winrt::Windows::Foundation::TypedEventHandler_winrt::Microsoft::UI::Xaml::Controls::Layout,winrt::Windows::Foundation::IInspectable___,winrt::Windows::Foundation::
  * [Watson Failure] caused by ACCESS_VIOLATION_c0000005_Microsoft.UI.Xaml.dll!event_base_event_winrt::Windows::Foundation::TypedEventHandler_winrt::Microsoft::UI::Xaml::Controls::Layout,winrt::Windows::Foundation::IInspectable___,winrt::Windows::Foundation::
  * [Watson Failure] caused by ACCESS_VIOLATION_c0000005_Microsoft.UI.Xaml.dll!event_base_event_winrt::Windows::Foundation::TypedEventHandler_winrt::Microsoft::UI::Xaml::Controls::Layout,winrt::Windows::Foundation::IInspectable___,winrt::Windows::Foundation::
  * [Watson Failure] caused by ACCESS_VIOLATION_1007_Microsoft.UI.Xaml.dll!event_base_event_winrt::Windows::Foundation::TypedEventHandler_winrt::Microsoft::UI::Xaml::Controls::Layout,winrt::Windows::Foundation::IInspectable___,winrt::Windows::Foundation::Type

- Active Backlog / Enhancement / Committed work  
  * RTL : Support layoutoverride scenario and upgrade scenario
  * Layout feature request: element that doesn't contribute during Measure and lives with the constraints during Arrange
  * Permanently fix layout cycle issues in ScrollBar, Slider, and ProgressBar by no longer using SizeChanged

- Best practices  
  * Avoid the use of UIElement.UpdateLayout as it can lead to layout cycles.
  * Avoid changing layout characteristics within a layout pass as this can lead to layout cycles as well.


# Layout constraint types

This section lists the various constraints fed into the layout manager so it can compute the
final deterministic arrangement of the UI elements.


## Containment

The most fundamental layout constraint is imposed by the concept of containment. Each UIElement has a
parent and optional children. That results in a visual tree (obviously the root element has no parent).

The parent is exposed through the FrameworkElement.Parent property. And panels, which derive from the 
Panel base class, for example, have a Children property which is a collection of UIElement. Other 
examples of children are: Border.Child, ScrollViewer.Content, ScrollViewer.LeftHeader, ContentControl.Content.

An element can only have one parent. Attempting to add it as a child of some other parent will raise an exception.


## Visibility

The UIElement.Visibility property is tracked by CUIElement::m_fVisibility (1 means Collapsed and 0 means Visible). 


## Alignment

Alignment characteristics help in placing a child within its parent in a predictable way. Alignments are
for a particular direction: horizontal or vertical.
Horizontal alignments: Left, Center, Right, Stretch
Vertical alignments: Top, Center, Bottom, Stretch

Properties involved are:
- FrameworkElement.HorizontalAlignment,
- FrameworkElement.VerticalAlignment,
- Control.HorizontalContentAlignment,
- Control.VerticalContentAlignment

The child's alignment is often template-bound to the parent's content alignment:
HorizontalAlignment="{TemplateBinding HorizontalContentAlignment}"


## Min size, size and max size

Element sizing can be imposed through the FrameworkElement.Width and FrameworkElement.Height properties.
When FrameworkElement.Width is NaN, the width is not specifically constrained. Same for the Height property.

They can also be restricted to a range via the FrameworkElement MinWidth, MaxWidth, MinHeight & MaxHeight
properties.

Those properties are 'coerced', so that MinWidth <= Width <= MaxWidth and 
MinHeight <= Height <= MaxHeight. Two properties cannot contradict each other.


## Relative size

The Grid panel allows to set relative sizes through the use of the * character:
  <Grid RowDefinitions="2*, 3*">
The second row will be 50% taller than the first one.


## Margin and Padding

The FrameworkElement.Margin property allows to surround an element with empty space,
while Control.Padding allows to pad the control content with empty space. Both are 
of type Thickness, and can use negative numbers.


## Absolute positioning

The Canvas panel, thanks to its LeftProperty and TopProperty attached properties, can position its children
using explicit offsets.


## Scroll offset and zoom factor

The old ScrollViewer (with its ScrollContentPresenter) and new ScrollView (with its ScrollPresenter) controls
can offset and scale their content. Properties reflecting that are HorizontalOffset, VerticalOffset and ZoomFactor.


## Clipping

Talking about scrolling controls, they also make use of clipping, which restricts the rendering of an element
to a sub-area. The UIElement.Clip property, of type RectangleGeometry, allows to do that in a general manner.


## Rounding

The UIElement.UseLayoutRounding boolean property is used to optionally align an element's
rendering, and its subtree, to whole screen pixels. This affects the crispness vs. blurriness,
and aliasing of the rendered content (https://learn.microsoft.com/en-us/uwp/api/windows.ui.xaml.uielement.uselayoutrounding?view=winrt-26100).

In the 'Scale & layout' section of the Windows Settings app, if the Scale is for example set to 125%,
then a FrameworkElement.Width set to 100.1 is rounded to 125 pixels instead of 125.125.


## Flow direction

For right-to-left languages, the layout can be flipped horizontally by using FrameworkElement.FlowDirection == FlowDirection.RightToLeft.


## Render transform

The UIElement.RenderTransform property allows to further alter the orientation, size & position of elements with the use 
of CompositeTransform, TransformGroup, MatrixTransform, RotateTransform, SkewTransform, TranslateTransform and ScaleTransform.

Note: The FrameworkElement.LayoutTransform property, which is available in WPF, does not exist in WinUI. In WPF,
it allows to apply transformations like RotateTransform, ScaleTransform, SkewTransform, and MatrixTransform to any FrameworkElement.


# Measure pass

Changing most of the aforementioned characteristics triggers a new measure pass to re-evaluate sizes, resulting in a new layout.
The measure pass' job is to determine the final size of all elements involved in the rendered UI. 
Note that changing an alignment for example for not trigger a measure pass - the layout manager goes straight to the arrange pass.

UI elements may be created and inserted into the visual tree as part of the measure pass. They may also be removed from the tree
and discarded (for example when scrolling through a UI-virtualized list).

When measured, an element is given an available size and returns a desired size (often based on the available size).
A parent typically measures its children, applies some layout logic to their desired sizes, and returns its own resulting desired size.
For perf reasons, only elements deemed 'dirty' from a measurement perspective are visited during a measure pass. Elements not marked dirty
would return the same desired size (normally :-)).

Dirtying an element does not synchronously trigger a new measure pass. It triggers the request for a new UI-thread tick during which
the measure pass will be performed.
You can bypass that delay though by calling the UIElement.UpdateLayout method which starts a synchronous re-layout. That method is taboo
though because it increases the risk of layout cycles.

The measure pass involves traversing the visual tree, recursively. The depth of those measure calls, 
CLayoutManager::m_cMeasuresOnStack, cannot exceed MaxLayoutDepth = 250 or an E_FAIL error is returned.

The key methods for the measure pass are:

Core control code:

    CFrameworkElement::MeasureCore(XSIZEF availableSize, XSIZEF& desiredSize)

DirectUI code:

    IFACEMETHODIMP SomeFrameworkElement::MeasureOverride(wf::Size availableSize, _Out_ wf::Size* pDesired)

MUXC code:

    winrt::Size SomeFrameworkElement::MeasureOverride(winrt::Size const& availableSize)


# Arrange pass

Once element sizes have been established, the arrange pass' job is to position the elements in relation 
to each other.

The arrange pass involves traversing the visual tree, recursively. The depth of those arrange calls, 
CLayoutManager::m_cArrangesOnStack, cannot exceed MaxLayoutDepth = 250 or an E_FAIL error is returned.

The key methods for the arrange pass are:

Core control code:

    CFrameworkElement::ArrangeCore(XRECTF finalRect)

DirectUI code:

    IFACEMETHODIMP SomeFrameworkElement::ArrangeOverride(        
        wf::Size finalSize, // The computed size that is used to arrange the content.
        _Out_ wf::Size* pReturnValue  // The size of the control.)

MUXC code:

    winrt::Size SomeFrameworkElement::ArrangeOverride(winrt::Size const& finalSize)


# Invalidations

An element can be marked invalid from a measure and/or arrange perspective. This will trigger the 
layout passes seen above.

A core property can be marked as dirtying measurement. 
Example from `dxaml/xcp/tools/XCPTypesAutoGen/XamlOM/Model/Microsoft.UI.Xaml.Controls.cs`:

    public class DatePicker : Microsoft.UI.Xaml.Controls.Control
    {
      [PropertyFlags(AffectsMeasure = true)]
      public Windows.Foundation.Object Header { get; set; }
    }

This gets translated into 

    // Microsoft.UI.Xaml.Controls.DatePicker.Header
    {
        KnownPropertyIndex::DatePicker_Header, // Index name
        KnownTypeIndex::Object, // Property type
        KnownTypeIndex::DatePicker, // Declaring type
        KnownTypeIndex::DatePicker, // Target type
        static_cast<MetaDataPropertyInfoFlags>(static_cast<UINT>(MetaDataPropertyInfoFlags::IsSparse) | static_cast<UINT>(MetaDataPropertyInfoFlags::AffectMeasure) | static_cast<UINT>(MetaDataPropertyInfoFlags::IsPublic)), // Property flags
    }
in `dxaml/xcp/components/metadata/StaticMetadata.g.cpp`.

So whenever that property changes, it triggers a measure pass for the owning element.

Likewise, a core property can be marked as dirtying arrangement.
Examples from `dxaml/xcp/tools/XCPTypesAutoGen/XamlOM/Model/Microsoft.UI.Xaml.Controls.cs`:

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Main")]
    [ControlPattern]
    [NativeName("CControl")]
    [Guids(ClassGuid = "7ef20697-b515-4ddc-903d-5605d6dfae81")]
    public abstract class Control : Microsoft.UI.Xaml.FrameworkElement
    {
        ...
        [PropertyFlags(AffectsArrange = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_horizontalContentAlignment")]
        public Microsoft.UI.Xaml.HorizontalAlignment HorizontalContentAlignment
        {
            get;
            set;
        }

        [PropertyFlags(AffectsArrange = true)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_verticalContentAlignment")]
        public Microsoft.UI.Xaml.VerticalAlignment VerticalContentAlignment
        {
            get;
            set;
        }
        ...
    }

This gets translated into

    // Microsoft.UI.Xaml.Controls.Control.HorizontalContentAlignment
    {
        KnownPropertyIndex::Control_HorizontalContentAlignment, // Index name
        KnownTypeIndex::HorizontalAlignment, // Property type
        KnownTypeIndex::Control, // Declaring type
        KnownTypeIndex::Control, // Target type
        static_cast<MetaDataPropertyInfoFlags>(static_cast<UINT>(MetaDataPropertyInfoFlags::AffectArrange) | static_cast<UINT>(MetaDataPropertyInfoFlags::IsPublic)), // Property flags
    },
    // Microsoft.UI.Xaml.Controls.Control.VerticalContentAlignment
    {
        KnownPropertyIndex::Control_VerticalContentAlignment, // Index name
        KnownTypeIndex::VerticalAlignment, // Property type
        KnownTypeIndex::Control, // Declaring type
        KnownTypeIndex::Control, // Target type
        static_cast<MetaDataPropertyInfoFlags>(static_cast<UINT>(MetaDataPropertyInfoFlags::AffectArrange) | static_cast<UINT>(MetaDataPropertyInfoFlags::IsPublic)), // Property flags
    }
in `dxaml/xcp/components/metadata/StaticMetadata.g.cpp`.

So whenever those properties change, it triggers an arrange pass for the owning element.


Non-core controls, 3rd party controls, and app code in general, can invalidate elements by invoking the public
UIElement.InvalidateMeasure and UIElement.InvalidateArrange methods.


Core control code:

Examples of low level code marking elements dirty:  

    // Mark this element's rendering content as dirty.
    CUIElement::NWSetContentDirty(this, DirtyFlags::Render | DirtyFlags::Bounds);

    // Mark this element's transform as dirty.
    CUIElement::NWSetTransformDirty(this, DirtyFlags::Render | DirtyFlags::Bounds);


DirectUI code:

    _Check_return_ HRESULT ScrollContentPresenter::put_LeftHeader(
      _In_opt_ const ctl::ComPtr<IUIElement>& spLeftHeader,
      _In_ ScrollViewer* pOwningScrollViewer)
    {
      ...
      IFC_RETURN(RemoveLeftHeader(pOwningScrollViewer, true /*removeFromChildrenCollection*/));
      SetPtrValue(m_trLeftHeader, spLeftHeader);
      IFC_RETURN(InvalidateMeasure());
      ...
    }


MUXC code:

    void ScrollPresenter::OnPropertyChanged(
      const winrt::DependencyPropertyChangedEventArgs& args)
    {
      ...
      if (args.Property() == s_ContentOrientationProperty)
      {
        m_contentOrientation = ContentOrientation();

        InvalidateMeasure();
      }
      ...
    }


# Effective viewport

Feature added some years ago for perf optimizations. It helps virtualizing collection controls
generate UI elements only for the visible areas on screen, and their immediate surrounding areas
(for possible scrolling).

UI elements that contribute to the effective viewport evaluations are the ones registered with
the framework through the UIElement.RegisterAsScrollPort(UIElement) method. The MUX ScrollViewer
and MUXC ScrollPresenter are our scrollers which call that method.

Public APIs:  
- event Windows.Foundation.TypedEventHandler<Microsoft.UI.Xaml.FrameworkElement,Microsoft.UI.Xaml.EffectiveViewportChangedEventArgs> EffectiveViewportChanged;
- runtimeclass EffectiveViewportChangedEventArgs
- static void UIElement.RegisterAsScrollPort(Microsoft.UI.Xaml.UIElement element);

Key methods:  
- CUIElement::EffectiveViewportWalk, CUIElement::EffectiveViewportWalkCore,
- CUIElement::ComputeEffectiveViewportChangedEventArgsAndNotifyLayoutManager,
- CLayoutManager::RaiseEffectiveViewportChangedEvents,
- CUIElement::RegisterAsScroller


# Layout cycles

In CLayoutManager::UpdateLayout(XUINT32 controlWidth, XUINT32 controlHeight), the layout manager will iterate 
up to MaxLayoutIterations = 250 times through measure, arrange and effective viewport passes. If it cannot 
settle with a particular layout because of constant dirtying, an AG_E_LAYOUT_CYCLE error is thrown.

Cycles are typically caused by app code dirtying some element during the layout passes, float rounding or 
rounding in general which dirty the visual tree.

See
  - int  CLayoutManager::m_layoutCycleWarningContextsCountdown{ -1 };
  - bool CLayoutManager::m_isLayoutCycleLoggingSuspended{ false };
which are used for debugging purposes.

For information on debugging layout cycles, see [DebugSettings for Layout Cycle Debugging](debug-settings-layoutCycle-spec.md).


# Key source code locations

Core code:
- dxaml/xcp/core/layout/LayoutManager.cpp
- dxaml/xcp/core/core/elements/framework.cpp
- dxaml/xcp/core/core/elements/uielement.cpp
- dxaml/xcp/core/core/elements/RootVisual.cpp
- dxaml/xcp/core/dll/VisualTree.cpp
- dxaml/xcp/core/inc/framework.h
- dxaml/xcp/core/inc/uielement.h
- dxaml/xcp/core/inc/RootVisual.h
- dxaml/xcp/core/inc/VisualTree.h
- dxaml/xcp/core/inc/EffectiveViewportChangedEventArgs.h
- dxaml/xcp/components/elements/FrameworkElement.cpp
- dxaml/xcp/components/elements/UIElementLayout.cpp

DirectUI code:
- dxaml/xcp/dxaml/lib/FrameworkElement_partial.h / .cpp

Test code:
- EffectiveViewport: 
  * dxaml/test/native/external/foundation/hosting/XamlIslandTests.cpp
  * dxaml/test/native/external/framework/layout/LayoutManagerIntegrationTests.cpp
  * dxaml/xcp/components/elements/unittests/UIElementUnitTests.cpp
- Layout:
  * dxaml/test/native/external/framework/layout/LayoutInformationIntegrationTests.cpp
  * dxaml/test/native/external/framework/layout/LayoutTests.cpp


# Additional key methods when debugging

You may want to set breakpoints in these methods:
- SetLayoutFlags calls in `dxaml/xcp/core/inc/uielement.h`.
- CLayoutManager::UpdateLayout(XUINT32 controlWidth, XUINT32 controlHeight)


# Layout storage & key fields when debugging

You may want to keep track of these fields:
- LayoutFlags enum values defined in `dxaml/xcp/core/inc/uielement.h`:
LF_MEASURE_DIRTY, LF_ON_MEASURE_DIRTY_PATH, LF_ARRANGE_DIRTY, LF_ON_ARRANGE_DIRTY_PATH, 
LF_VIEWPORT_DIRTY, LF_ON_VIEWPORT_DIRTY_PATH, LF_WANTS_VIEWPORT, LF_CONTRIBUTES_TO_VIEWPORT, etc 
accessed and set with GetLayoutFlagsAnd / SetLayoutFlags calls, the storage being in CUIElement::m_layoutFlags.
- XFLOATs CFrameworkElement::m_eWidth / m_eHeight.
- struct FrameworkElementGroupStorage CFrameworkElement::m_pLayoutProperties which includes:
  * XFLOAT m_eMinWidth;
  * XFLOAT m_eMaxWidth;
  * XFLOAT m_eMinHeight;
  * XFLOAT m_eMaxHeight;
  * DirectUI::HorizontalAlignment m_horizontalAlignment;
  * DirectUI::VerticalAlignment m_verticalAlignment;
  * XTHICKNESS m_margin;
  * XUINT32 m_nGridRow;
  * XUINT32 m_nGridColumn;
  * XUINT32 m_nGridRowSpan;
  * XUINT32 m_nGridColumnSpan;
- CLayoutStorage CUIElement::m_layoutStorage which includes:
  * XSIZEF  m_previousAvailableSize;
  * XSIZEF  m_desiredSize;
  * XRECTF  m_finalRect;
  * XPOINTF m_offset;
  * XSIZEF  m_unclippedDesiredSize;
  * XSIZEF  m_size;
  * CRectangleGeometry* m_pLayoutClipGeometry;
- XRECTF_RB CUIElement::m_contentInnerBounds;   // The bounds of this element's content only (e.g. the background of a Panel). Relative to this element.
- XRECTF_RB CUIElement::m_childBounds;          // The unioned outer bounds of all of this element's children. Relative to this element.
- XRECTF_RB CUIElement::m_combinedInnerBounds;  // The union of m_contentInnerBounds and m_childBounds. Relative to this element.
- XRECTF_RB CUIElement::m_outerBounds;          // 2D: m_combinedInnerBounds transformed through this element's properties (the same as its parent's inner space)
- unsigned int CUIElement::m_isRightToLeft : 1; // Coerced value of FrameworkElement.FlowDirection
- unsigned int CUIElement::m_fVisibility : 1;   // UIElement.Visibility
- unsigned int CUIElement::m_fUseLayoutRounding : 1;
- unsigned int CUIElement::m_isScroller : 1;

- Some more CUIElement fields involved in layout: m_fNWVisibilityDirty, m_fNWTransformDirty, m_fNWClipDirty, m_fNWContentDirty, 
m_fNWSubgraphDirty, m_fNWHadUserClip, m_fNWHadLayoutClip, m_fNWLayoutClipDirty, etc...

- VisualTree* CRootVisual::m_pVisualTree = nullptr;
- Fields in VisualTree class, which include the various visual roots.
