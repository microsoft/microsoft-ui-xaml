// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#define WINRT_LEAN_AND_MEAN

#define NOMINMAX

#pragma warning(disable : 6221) // Disable implicit cast warning for C++/WinRT headers (tracked by Bug 17528784: C++/WinRT headers trigger C6221 comparing e.code() to int-typed things)

// Disable factory caching in CppWinRT as the global COM pointers that are released during dll/process
// unload are not safe. Setting this makes CppWinRT just call get_activation_factory directly every time.
#define WINRT_DISABLE_FACTORY_CACHE 1

#include "targetver.h"

#include "BuildMacros.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

// Basic WinRT ABI things -- without including all of WRL
#include <inspectable.h>
#include <hstring.h>
#include <eventtoken.h>
#include <activation.h>
#include <weakreference.h>
#include <guiddef.h>

#define MUX_ASSERT(X) _ASSERT(X) 
#define MUX_ASSERT_MSG(X, MSG) _ASSERT_EXPR(X, MSG)
#define MUX_ASSERT_NOASSUME(X) _ASSERT(X)

#define MUX_FAIL_FAST() RaiseFailFastException(nullptr, nullptr, 0);
#define MUX_FAIL_FAST_MSG(MSG) RaiseFailFastException(nullptr, nullptr, 0);

#include "gsl/gsl"

// windows.ui.xaml.h accesses LoadLibrary in its inline declaration of CreateXamlUiPresenter
// Accessing LoadLibrary is not always allowed (e.g. on phone), so we need to suppress that.
// We can do so by making this define prior to including the header.
#define CREATE_XAML_UI_PRESENTER_API
#include <Windows.UI.Xaml.Hosting.ReferenceTracker.h>

#include <WindowsNumerics.h>

#include <strsafe.h>
#include <robuffer.h>

// STL
#include <vector>
#include <map>
#include <functional>

#define _USE_MATH_DEFINES
#include <cmath>
#define M_PI       3.14159265358979323846   // pi
#define M_PI_2     1.57079632679489661923   // pi/2

#define WI_IS_FEATURE_PRESENT(FeatureName) 1

#undef GetCurrentTime

#include "CppWinRTIncludes.h"

// This function is a compile time optimization. These GUID calculations are very expensive (each takes about .5-1s to compile)
// and many appear in multiple compilation units so the work is duplicated across invocations of the compiler. I used the
// MSVC compiler option "/d1reportTime" to get a dump of all the work and then searched for "pinterface_guid" invocations to
// determine the set of things to put here.
void constexpr specialize_guids()
{
    winrt::guid_of<struct winrt::Windows::Foundation::AsyncOperationCompletedHandler<bool>>();
    winrt::guid_of<struct winrt::Windows::Foundation::AsyncOperationCompletedHandler<struct winrt::hstring>>();
    winrt::guid_of<struct winrt::Windows::Foundation::AsyncOperationCompletedHandler<struct winrt::Windows::Storage::Streams::IRandomAccessStreamWithContentType>>();
    winrt::guid_of<struct winrt::Windows::Foundation::AsyncOperationCompletedHandler<unsigned int>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterable<struct winrt::hstring>>();

#ifdef REPEATER_INCLUDED
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterable<struct winrt::Microsoft::UI::Xaml::Controls::IndexPath>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::AnimationContext>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVectorView<struct winrt::Microsoft::UI::Xaml::Controls::IndexPath>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterator<struct winrt::Microsoft::UI::Xaml::Controls::IndexPath>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::ElementRealizationOptions>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::FlowLayoutLineAlignment>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::UniformGridLayoutItemsJustification>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::UniformGridLayoutItemsStretch>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::Layout, struct winrt::Windows::Foundation::IInspectable>>();
#endif

#ifdef TREEVIEW_INCLUDED
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterable<struct winrt::Microsoft::UI::Xaml::Controls::TreeViewNode>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterator<struct winrt::Microsoft::UI::Xaml::Controls::TreeViewNode>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IObservableVector<struct winrt::Microsoft::UI::Xaml::Controls::TreeViewNode>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Microsoft::UI::Xaml::Controls::TreeViewNode>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::VectorChangedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::TreeViewNode>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::TreeViewNode, struct winrt::Windows::Foundation::Collections::IVectorChangedEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::TreeViewNode, struct winrt::Windows::Foundation::IInspectable>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::TreeViewNode, struct winrt::Windows::UI::Xaml::DependencyPropertyChangedEventArgs>>();
#endif

#ifdef MENUBAR_INCLUDED
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterable<struct winrt::Microsoft::UI::Xaml::Controls::MenuBarItem>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterator<struct winrt::Microsoft::UI::Xaml::Controls::MenuBarItem>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IObservableVector<struct winrt::Microsoft::UI::Xaml::Controls::MenuBarItem>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Microsoft::UI::Xaml::Controls::MenuBarItem>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::VectorChangedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::MenuBarItem>>();
#endif

#ifdef SCROLLER_INCLUDED
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterable<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::SnapPointBase>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterable<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::ScrollSnapPointBase>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterable<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::ZoomSnapPointBase>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterator<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::SnapPointBase>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterator<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::ScrollSnapPointBase>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterator<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::ZoomSnapPointBase>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IObservableVector<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::SnapPointBase>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IObservableVector<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::ScrollSnapPointBase>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IObservableVector<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::ZoomSnapPointBase>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::SnapPointBase>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::ScrollSnapPointBase>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::ZoomSnapPointBase>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::VectorChangedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::SnapPointBase>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::VectorChangedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::ScrollSnapPointBase>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::VectorChangedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::ZoomSnapPointBase>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::InteractionState>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::Primitives::ScrollSnapPointsAlignment>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::IScrollController, struct winrt::Microsoft::UI::Xaml::Controls::Primitives::ScrollControllerInteractionRequestedEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::IScrollController, struct winrt::Microsoft::UI::Xaml::Controls::Primitives::ScrollControllerScrollToRequestedEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::IScrollController, struct winrt::Microsoft::UI::Xaml::Controls::Primitives::ScrollControllerScrollByRequestedEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::IScrollController, struct winrt::Microsoft::UI::Xaml::Controls::Primitives::ScrollControllerScrollFromRequestedEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::IScrollController, struct winrt::Windows::Foundation::IInspectable>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::Scroller, struct winrt::Microsoft::UI::Private::Controls::ScrollerTestHooksAnchorEvaluatedEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::Scroller, struct winrt::Microsoft::UI::Private::Controls::ScrollerTestHooksInteractionSourcesChangedEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::Scroller, struct winrt::Microsoft::UI::Xaml::Controls::ScrollerAnchorRequestedEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::Scroller, struct winrt::Microsoft::UI::Xaml::Controls::ScrollerBringingIntoViewEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::Scroller, struct winrt::Microsoft::UI::Xaml::Controls::ScrollAnimationStartingEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::Scroller, struct winrt::Microsoft::UI::Xaml::Controls::ZoomAnimationStartingEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::Scroller, struct winrt::Microsoft::UI::Xaml::Controls::ScrollCompletedEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::Scroller, struct winrt::Microsoft::UI::Xaml::Controls::ZoomCompletedEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::Scroller, struct winrt::Windows::Foundation::IInspectable>>();
#endif

#ifdef SWIPECONTROL_INCLUDED
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterable<struct winrt::Microsoft::UI::Xaml::Controls::SwipeItem>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterator<struct winrt::Microsoft::UI::Xaml::Controls::SwipeItem>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IObservableVector<struct winrt::Microsoft::UI::Xaml::Controls::SwipeItem>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Microsoft::UI::Xaml::Controls::SwipeItem>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::VectorChangedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::SwipeItem>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::SwipeBehaviorOnInvoked>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::SwipeMode>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::SwipeControl, struct winrt::Windows::Foundation::IInspectable>>();
#endif

#ifdef COLORPICKER_INCLUDED
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::ColorPickerHsvChannel>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::ColorSpectrumComponents>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::ColorSpectrumShape>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::Primitives::ColorSpectrum, struct winrt::Microsoft::UI::Xaml::Controls::ColorChangedEventArgs>>();
#endif
    
#ifdef NAVIGATIONVIEW_INCLUDED
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::NavigationViewBackButtonVisible>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::NavigationViewDisplayMode>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::NavigationViewOverflowLabelMode>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::NavigationViewPaneDisplayMode>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::NavigationViewSelectionFollowsFocus>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::NavigationViewShoulderNavigationEnabled>>();
#endif

#ifdef PARALLAXVIEW_INCUDED
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::ParallaxSourceOffsetKind>>();
#endif

#ifdef PULLTOREFRESH_INCLUDED
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::RefreshPullDirection>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::RefreshVisualizerOrientation>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::RefreshVisualizerState>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Microsoft::UI::Private::Controls::IRefreshInfoProvider, struct winrt::Microsoft::UI::Xaml::Controls::RefreshInteractionRatioChangedEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Microsoft::UI::Private::Controls::IRefreshInfoProvider, struct winrt::Windows::Foundation::IInspectable>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Microsoft::UI::Xaml::Controls::RefreshVisualizer, struct winrt::Microsoft::UI::Xaml::Controls::RefreshRequestedEventArgs>>();
#endif

#ifdef SCROLLVIEWER_INCLUDED
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::ScrollBarVisibility>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::ChainingMode>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::InputKind>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::RailingMode>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::ScrollMode>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::ZoomMode>>();
#endif

#ifdef TWOPANEVIEW_INCLUDED
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::TwoPaneViewMode>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::TwoPaneViewPriority>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::TwoPaneViewTallModeConfiguration>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Controls::TwoPaneViewWideModeConfiguration>>();
#endif

#ifdef MATERIALS_INCLUDED
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Media::AcrylicBackgroundSource>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Microsoft::UI::Xaml::Media::RevealBrushState>>();
#endif
 
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Windows::Foundation::IInspectable, struct winrt::Microsoft::UI::Private::Controls::MUXControlsTestHooksLoggingMessageEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterable<struct winrt::Windows::Foundation::Collections::IKeyValuePair<struct winrt::hstring,struct winrt::Windows::UI::Xaml::DataTemplate> >>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterable<struct winrt::Windows::Foundation::IInspectable>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterable<struct winrt::Windows::UI::Composition::Interactions::CompositionConditionalValue>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterable<struct winrt::Windows::UI::Composition::Interactions::InteractionTrackerInertiaModifier>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterable<struct winrt::Windows::UI::Xaml::Automation::Peers::AutomationPeer>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterable<struct winrt::Windows::UI::Xaml::Controls::ICommandBarElement>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterable<struct winrt::Windows::UI::Xaml::Controls::MenuFlyoutItemBase>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterable<struct winrt::Windows::UI::Xaml::DependencyObject>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterable<struct winrt::Windows::UI::Xaml::UIElement>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterator<struct winrt::hstring>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterator<struct winrt::Windows::Foundation::Collections::IKeyValuePair<struct winrt::hstring,struct winrt::Windows::UI::Xaml::DataTemplate> >>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterator<struct winrt::Windows::Foundation::IInspectable>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterator<struct winrt::Windows::UI::Composition::Interactions::CompositionConditionalValue>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterator<struct winrt::Windows::UI::Composition::Interactions::InteractionTrackerInertiaModifier>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterator<struct winrt::Windows::UI::Xaml::Automation::Peers::AutomationPeer>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterator<struct winrt::Windows::UI::Xaml::Controls::ICommandBarElement>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterator<struct winrt::Windows::UI::Xaml::Controls::MenuFlyoutItemBase>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterator<struct winrt::Windows::UI::Xaml::DependencyObject>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IIterator<struct winrt::Windows::UI::Xaml::UIElement>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IKeyValuePair<struct winrt::hstring,struct winrt::Windows::UI::Xaml::DataTemplate>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IMap<struct winrt::hstring,struct winrt::hstring>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IMap<struct winrt::hstring,struct winrt::Windows::Foundation::IInspectable>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IMap<struct winrt::hstring,struct winrt::Windows::Graphics::Imaging::BitmapTypedValue>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IMap<struct winrt::hstring,struct winrt::Windows::UI::Composition::ICompositionAnimationBase>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IMap<struct winrt::hstring,struct winrt::Windows::UI::Xaml::DataTemplate>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IMap<struct winrt::Windows::Foundation::IInspectable,struct winrt::Windows::Foundation::IInspectable>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IMapView<struct winrt::hstring,struct winrt::Windows::UI::Xaml::DataTemplate>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IObservableVector<struct winrt::Windows::Foundation::IInspectable>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IObservableVector<struct winrt::Windows::UI::Composition::Interactions::InteractionTrackerInertiaModifier>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IObservableVector<struct winrt::Windows::UI::Xaml::Controls::ICommandBarElement>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IObservableVector<struct winrt::Windows::UI::Xaml::Controls::MenuFlyoutItemBase>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<double>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::hstring>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Windows::Foundation::IInspectable>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Windows::Foundation::Point>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Windows::UI::Composition::Interactions::CompositionConditionalValue>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Windows::UI::Composition::Interactions::InteractionTrackerInertiaModifier>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Windows::UI::Xaml::Automation::Peers::AutomationPeer>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Windows::UI::Xaml::Controls::ICommandBarElement>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Windows::UI::Xaml::Controls::MenuFlyoutItemBase>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Windows::UI::Xaml::Media::Animation::ColorKeyFrame>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Windows::UI::Xaml::Media::Animation::DoubleKeyFrame>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Windows::UI::Xaml::Media::Animation::ObjectKeyFrame>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Windows::UI::Xaml::Media::Animation::PointKeyFrame>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Windows::UI::Xaml::Media::Animation::Timeline>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Windows::UI::Xaml::Media::Animation::Transition>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Windows::UI::Xaml::Media::Brush>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Windows::UI::Xaml::Media::Geometry>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Windows::UI::Xaml::Media::GradientStop>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Windows::UI::Xaml::Media::PathFigure>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Windows::UI::Xaml::Media::PathSegment>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Windows::UI::Xaml::Media::TimelineMarker>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Windows::UI::Xaml::Media::Transform>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Windows::UI::Xaml::TriggerAction>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVector<struct winrt::Windows::UI::Xaml::UIElement>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVectorView<struct winrt::Windows::Foundation::IInspectable>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::IVectorView<struct winrt::Windows::UI::Composition::Interactions::CompositionConditionalValue>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::VectorChangedEventHandler<struct winrt::Windows::Foundation::IInspectable>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::VectorChangedEventHandler<struct winrt::Windows::UI::Xaml::Controls::ICommandBarElement>>();
    winrt::guid_of<struct winrt::Windows::Foundation::Collections::VectorChangedEventHandler<struct winrt::Windows::UI::Xaml::Controls::MenuFlyoutItemBase>>();
    winrt::guid_of<struct winrt::Windows::Foundation::EventHandler<struct winrt::Windows::Foundation::IInspectable>>();
    winrt::guid_of<struct winrt::Windows::Foundation::EventHandler<struct winrt::Windows::UI::Xaml::Controls::ScrollViewerViewChangedEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::EventHandler<struct winrt::Windows::UI::Xaml::Controls::ScrollViewerViewChangingEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<bool>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<winrt::TimeSpan>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<double>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Windows::UI::Text::FontStyle>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Windows::UI::Xaml::ApplicationTheme>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Windows::UI::Xaml::Automation::ExpandCollapseState>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Windows::UI::Xaml::Controls::Orientation>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Windows::UI::Xaml::Controls::Primitives::ScrollingIndicatorMode>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Windows::UI::Xaml::Controls::Symbol>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<enum winrt::Windows::UI::Xaml::Visibility>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<float>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<int>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<struct winrt::hstring>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<struct winrt::Windows::Foundation::Numerics::float2>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<struct winrt::Windows::Foundation::Numerics::float3>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<struct winrt::Windows::Foundation::Numerics::float4>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<struct winrt::Windows::Foundation::Point>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<struct winrt::Windows::Foundation::Rect>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<struct winrt::Windows::UI::Color>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<struct winrt::Windows::UI::Text::FontWeight>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<struct winrt::Windows::UI::Xaml::CornerRadius>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<struct winrt::Windows::UI::Xaml::GridLength>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<struct winrt::Windows::UI::Xaml::Input::KeyEventHandler>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<struct winrt::Windows::UI::Xaml::Input::PointerEventHandler>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<struct winrt::Windows::UI::Xaml::Interop::TypeName>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<struct winrt::Windows::UI::Xaml::Thickness>>();
    winrt::guid_of<struct winrt::Windows::Foundation::IReference<unsigned int>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Windows::ApplicationModel::Core::CoreApplicationViewTitleBar,struct winrt::Windows::Foundation::IInspectable>>();    
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Windows::Foundation::IInspectable,struct winrt::Windows::Foundation::IInspectable>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Windows::Foundation::IInspectable,struct winrt::Windows::UI::Composition::CompositionBatchCompletedEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Windows::Graphics::Display::DisplayInformation,struct winrt::Windows::Foundation::IInspectable>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Windows::UI::Composition::CompositionCapabilities,struct winrt::Windows::Foundation::IInspectable>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Windows::UI::Core::CoreDispatcher,struct winrt::Windows::UI::Core::AcceleratorKeyEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Windows::UI::Core::CoreWindow,struct winrt::Windows::UI::Core::KeyEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Windows::UI::Core::CoreWindow,struct winrt::Windows::UI::Core::PointerEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Windows::UI::Core::CoreWindow,struct winrt::Windows::UI::Core::VisibilityChangedEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Windows::UI::ViewManagement::UISettings,struct winrt::Windows::Foundation::IInspectable>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Windows::UI::Xaml::Controls::Control,struct winrt::Windows::UI::Xaml::Controls::FocusDisengagedEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Windows::UI::Xaml::Controls::Control,struct winrt::Windows::UI::Xaml::Controls::FocusEngagedEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Windows::UI::Xaml::Controls::ListViewBase,struct winrt::Windows::UI::Xaml::Controls::ContainerContentChangingEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Windows::UI::Xaml::Controls::ListViewBase,struct winrt::Windows::UI::Xaml::Controls::DragItemsCompletedEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Windows::UI::Xaml::Controls::Primitives::FlyoutBase,struct winrt::Windows::UI::Xaml::Controls::Primitives::FlyoutBaseClosingEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Windows::UI::Xaml::Controls::SplitView,struct winrt::Windows::Foundation::IInspectable>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Windows::UI::Xaml::Controls::SplitView,struct winrt::Windows::UI::Xaml::Controls::SplitViewPaneClosingEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Windows::UI::Xaml::Controls::TextBox,struct winrt::Windows::UI::Xaml::Controls::TextBoxTextChangingEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Windows::UI::Xaml::FrameworkElement,struct winrt::Windows::UI::Xaml::EffectiveViewportChangedEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Windows::UI::Xaml::Input::XamlUICommand,struct winrt::Windows::UI::Xaml::Input::ExecuteRequestedEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Windows::UI::Xaml::UIElement,struct winrt::Windows::UI::Xaml::BringIntoViewRequestedEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Windows::UI::Xaml::UIElement,struct winrt::Windows::UI::Xaml::Input::AccessKeyInvokedEventArgs>>();
    winrt::guid_of<struct winrt::Windows::Foundation::TypedEventHandler<struct winrt::Windows::UI::Xaml::UIElement,struct winrt::Windows::UI::Xaml::Input::GettingFocusEventArgs>>();
}
