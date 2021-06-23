// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <winrt\base.h>
#include <winrt\Windows.Foundation.h>
#include <winrt\Windows.Foundation.Collections.h>
#include <winrt\Windows.Foundation.Metadata.h>
#include <winrt\Windows.ApplicationModel.Activation.h>
#include <winrt\Windows.ApplicationModel.Contacts.h>
#include <winrt\Windows.ApplicationModel.Core.h>
#include <winrt\Windows.ApplicationModel.DataTransfer.h>
#include <winrt\Windows.ApplicationModel.Resources.h>
#include <winrt\Windows.ApplicationModel.Resources.Core.h>
#include <winrt\Windows.Devices.Input.h>
#include <winrt\Windows.Globalization.h>
#include <winrt\Windows.Globalization.NumberFormatting.h>
#include <winrt\Windows.Graphics.Imaging.h>
#include <winrt\Windows.Graphics.Display.h>
#include <winrt\Windows.Graphics.Effects.h>
#include <winrt\Windows.Storage.Streams.h>
#include <winrt\Windows.System.Power.h>
#include <winrt\Windows.System.Profile.h>
#include <winrt\Windows.System.h>
#include <winrt\Windows.System.Threading.h>
#include <winrt\Windows.System.UserProfile.h>
#include <winrt\Windows.UI.h>
#include <winrt\Windows.UI.Composition.h>
#include <winrt\Windows.UI.Composition.Effects.h>
#include <winrt\Windows.UI.Composition.Interactions.h>
#include <winrt\Windows.UI.Core.h>
#include <winrt\Windows.UI.Input.h>
#include <winrt\Windows.UI.Text.h>
#include <winrt\Windows.UI.ViewManagement.h>
#include <winrt\Windows.UI.WindowManagement.h>
#include <winrt\Windows.UI.Xaml.h>
#include <winrt\Windows.UI.Xaml.Automation.Peers.h>
#include <winrt\Windows.UI.Xaml.Automation.Provider.h>
#include <winrt\Windows.UI.Xaml.Controls.h>
#include <winrt\Windows.UI.Xaml.Controls.Primitives.h>
#include <winrt\Windows.UI.Xaml.Data.h>
#include <winrt\Windows.UI.Xaml.Documents.h>
#include <winrt\Windows.UI.Xaml.Hosting.h>
#include <winrt\Windows.UI.Xaml.Input.h>
#include <winrt\Windows.UI.Xaml.Interop.h>
#include <winrt\Windows.UI.Xaml.Markup.h>
#include <winrt\Windows.UI.Xaml.Media.h>
#include <winrt\Windows.UI.Xaml.Media.Animation.h>
#include <winrt\Windows.UI.Xaml.Media.Imaging.h>
#include <winrt\Windows.UI.Xaml.Shapes.h>
#include <winrt\Microsoft.UI.Private.Controls.h>
#if __has_include("winrt\Microsoft.UI.Private.Media.h")
#include <winrt\Microsoft.UI.Private.Media.h>
#endif

#include <winrt\Microsoft.UI.Xaml.Controls.h>
#include <winrt\Microsoft.UI.Xaml.XamlTypeInfo.h>
#if __has_include("winrt\Microsoft.UI.Xaml.Controls.Primitives.h")
#include <winrt\Microsoft.UI.Xaml.Controls.Primitives.h>
#endif

#if __has_include("winrt\Microsoft.UI.Xaml.Controls.AnimatedVisuals.h")
#include <winrt\Microsoft.UI.Xaml.Controls.AnimatedVisuals.h>
#endif

#if __has_include("winrt\Microsoft.UI.Xaml.Media.h")
#include <winrt\Microsoft.UI.Xaml.Media.h>
#endif

#if __has_include("winrt\Microsoft.UI.Xaml.Automation.Peers.h")
#include <winrt\Microsoft.UI.Xaml.Automation.Peers.h>
#endif

#if __has_include("winrt\Microsoft.UI.Private.Composition.Effects.h")
#include <winrt\Microsoft.UI.Private.Composition.Effects.h>
#endif

namespace winrt
{
    using namespace ::winrt::Windows;
    using namespace ::winrt::Windows::ApplicationModel::Activation;
    using namespace ::winrt::Windows::ApplicationModel::Contacts;
    using namespace ::winrt::Windows::ApplicationModel::Core;
    using namespace ::winrt::Windows::ApplicationModel::DataTransfer;
    using namespace ::winrt::Windows::ApplicationModel::Resources;
    using namespace ::winrt::Windows::ApplicationModel::Resources::Core;
    using namespace ::winrt::Windows::Devices::Input;
    using namespace ::winrt::Windows::Foundation;
    using namespace ::winrt::Windows::Foundation::Collections;
    using namespace ::winrt::Windows::Foundation::Metadata;
    using namespace ::winrt::Windows::Foundation::Numerics;
    using namespace ::winrt::Windows::Globalization;
    using namespace ::winrt::Windows::Globalization::NumberFormatting;
    using namespace ::winrt::Windows::Graphics::Display;
    using namespace ::winrt::Windows::Graphics::Imaging;
    using namespace ::winrt::Windows::Graphics::Effects;
    using namespace ::winrt::Windows::Storage::Streams;
    using namespace ::winrt::Windows::System;
    using namespace ::winrt::Windows::System::Power;
    using namespace ::winrt::Windows::System::Threading;
    using namespace ::winrt::Windows::System::UserProfile;
    using namespace ::winrt::Windows::UI;
    using namespace ::winrt::Windows::UI::Composition;
    using namespace ::winrt::Windows::UI::Composition::Effects;
    using namespace ::winrt::Windows::UI::Composition::Interactions;
    using namespace ::winrt::Windows::UI::Core;
    using namespace ::winrt::Windows::UI::Input;
    using namespace ::winrt::Windows::UI::Text;
    using namespace ::winrt::Windows::UI::ViewManagement;
    using namespace ::winrt::Windows::UI::WindowManagement;
    using namespace ::winrt::Windows::UI::Xaml;
    using namespace ::winrt::Windows::UI::Xaml::Automation;
    using namespace ::winrt::Windows::UI::Xaml::Automation::Provider;
    using namespace ::winrt::Windows::UI::Xaml::Data;
    using namespace ::winrt::Windows::UI::Xaml::Documents;
    using namespace ::winrt::Windows::UI::Xaml::Hosting;
    using namespace ::winrt::Windows::UI::Xaml::Input;
    using namespace ::winrt::Windows::UI::Xaml::Interop;
    using namespace ::winrt::Windows::UI::Xaml::Markup;
    using namespace ::winrt::Windows::UI::Xaml::Media::Animation;
    using namespace ::winrt::Windows::UI::Xaml::Media::Imaging;
    using namespace ::winrt::Windows::UI::Xaml::Shapes;
    using namespace ::winrt::Windows::System::Profile;
    using namespace ::winrt::Microsoft::UI::Xaml::XamlTypeInfo;
    namespace Microsoft::UI::Private::Controls {}
    using namespace ::winrt::Microsoft::UI::Private::Controls;
    namespace Microsoft::UI::Private::Media {}
    using namespace ::winrt::Microsoft::UI::Private::Media;
    namespace Microsoft::UI::Xaml::Controls {}
    using namespace ::winrt::Microsoft::UI::Xaml::Controls;
    namespace Microsoft::UI::Xaml::Controls::Primitives {}
    using namespace ::winrt::Microsoft::UI::Xaml::Controls::Primitives;
    namespace Microsoft::UI::Xaml::Controls::AnimatedVisuals {}
    using namespace ::winrt::Microsoft::UI::Xaml::Controls::AnimatedVisuals;
    namespace Microsoft::UI::Xaml::Media {}
    using namespace ::winrt::Microsoft::UI::Xaml::Media;
    namespace Microsoft::UI::Xaml::Automation::Peers {}
    using namespace ::winrt::Microsoft::UI::Xaml::Automation::Peers;

    // using namespace will affect headers included later as well, so crack these namespaces now for convenience
    // but in order to avoid "namespace not defined" errors we have to define the namespaces here too.
    namespace Microsoft::UI::Private::Controls::implementation {}
    namespace Microsoft::UI::Private::Media::implementation {}
    namespace Microsoft::UI::Xaml::Controls::implementation {}
    namespace Microsoft::UI::Xaml::XamlTypeInfo::implementation {}
    namespace Microsoft::UI::Xaml::Controls::Primitives::implementation {}
    namespace Microsoft::UI::Xaml::Media::implementation {}
    namespace Microsoft::UI::Xaml::Automation::Peers::implementation {}
    namespace Microsoft::UI::Xaml::Controls::AnimatedVisuals::implementation {}
    namespace implementation
    {
        using namespace ::winrt::Microsoft::UI::Private::Controls::implementation;
        using namespace ::winrt::Microsoft::UI::Private::Media::implementation;
        using namespace ::winrt::Microsoft::UI::Xaml::Controls::implementation;
        using namespace ::winrt::Microsoft::UI::Xaml::XamlTypeInfo::implementation;
        using namespace ::winrt::Microsoft::UI::Xaml::Controls::Primitives::implementation;
        using namespace ::winrt::Microsoft::UI::Xaml::Controls::AnimatedVisuals::implementation;
        using namespace ::winrt::Microsoft::UI::Xaml::Media::implementation;
        using namespace ::winrt::Microsoft::UI::Xaml::Automation::Peers::implementation;

    }

    namespace Microsoft::UI::Private::Controls::factory_implementation {}
    namespace Microsoft::UI::Private::Media::factory_implementation {}
    namespace Microsoft::UI::Xaml::Controls::factory_implementation {}
    namespace Microsoft::UI::Xaml::XamlTypeInfo::factory_implementation {}
    namespace Microsoft::UI::Xaml::Controls::Primitives::factory_implementation {}
    namespace Microsoft::UI::Xaml::Controls::AnimatedVisuals::factory_implementation {}
    namespace Microsoft::UI::Xaml::Media::factory_implementation {}
    namespace Microsoft::UI::Xaml::Automation::Peers::factory_implementation {}
    namespace factory_implementation
    {
        using namespace ::winrt::Microsoft::UI::Private::Controls::factory_implementation;
        using namespace ::winrt::Microsoft::UI::Private::Media::factory_implementation;
        using namespace ::winrt::Microsoft::UI::Xaml::Controls::factory_implementation;
        using namespace ::winrt::Microsoft::UI::Xaml::XamlTypeInfo::factory_implementation;
        using namespace ::winrt::Microsoft::UI::Xaml::Controls::Primitives::factory_implementation;
        using namespace ::winrt::Microsoft::UI::Xaml::Controls::AnimatedVisuals::factory_implementation;
        using namespace ::winrt::Microsoft::UI::Xaml::Media::factory_implementation;
        using namespace ::winrt::Microsoft::UI::Xaml::Automation::Peers::factory_implementation;
    }

#ifdef EFFECTS_INCLUDED
    using namespace ::winrt::Microsoft::UI::Private::Composition::Effects;
#endif

    // using namespace ::winrt::Windows::UI::Xaml::Controls;
    using AppBar = winrt::Windows::UI::Xaml::Controls::AppBar;
    using AppBarButton = winrt::Windows::UI::Xaml::Controls::AppBarButton;
    using AppBarSeparator = winrt::Windows::UI::Xaml::Controls::AppBarSeparator;
    using AppBarToggleButton = winrt::Windows::UI::Xaml::Controls::AppBarToggleButton;
    using AutoSuggestBox = winrt::Windows::UI::Xaml::Controls::AutoSuggestBox;
    using BitmapIcon = winrt::Windows::UI::Xaml::Controls::BitmapIcon;
    using Border = winrt::Windows::UI::Xaml::Controls::Border;
    using Button = ::winrt::Windows::UI::Xaml::Controls::Button;
    using Canvas = winrt::Windows::UI::Xaml::Controls::Canvas;
    using CheckBox = ::winrt::Windows::UI::Xaml::Controls::CheckBox;
    using ColumnDefinition = winrt::Windows::UI::Xaml::Controls::ColumnDefinition;
    using CommandBar = ::winrt::Windows::UI::Xaml::Controls::CommandBar;
    using ComboBox = ::winrt::Windows::UI::Xaml::Controls::ComboBox;
    using ComboBoxItem = ::winrt::Windows::UI::Xaml::Controls::ComboBoxItem;
    using ContainerContentChangingEventArgs = winrt::Windows::UI::Xaml::Controls::ContainerContentChangingEventArgs;
    using ContentControl = winrt::Windows::UI::Xaml::Controls::ContentControl;
    using ContentPresenter = winrt::Windows::UI::Xaml::Controls::ContentPresenter;
    using Control = ::winrt::Windows::UI::Xaml::Controls::Control;
    using IControl = ::winrt::Windows::UI::Xaml::Controls::IControl;
    using DataTemplateSelector = winrt::Windows::UI::Xaml::Controls::DataTemplateSelector;
    using DisabledFormattingAccelerators = winrt::Windows::UI::Xaml::Controls::DisabledFormattingAccelerators;
    using DragItemsCompletedEventArgs = winrt::Windows::UI::Xaml::Controls::DragItemsCompletedEventArgs;
    using DragItemsStartingEventArgs = winrt::Windows::UI::Xaml::Controls::DragItemsStartingEventArgs;
    using DragItemsStartingEventHandler = winrt::Windows::UI::Xaml::Controls::DragItemsStartingEventHandler;
    using Flyout = winrt::Windows::UI::Xaml::Controls::Flyout;
    using FlyoutPresenter = winrt::Windows::UI::Xaml::Controls::FlyoutPresenter;
    using IFlyoutPresenter2 = winrt::Windows::UI::Xaml::Controls::IFlyoutPresenter2;
    using FocusDisengagedEventArgs = winrt::Windows::UI::Xaml::Controls::FocusDisengagedEventArgs;
    using FocusEngagedEventArgs = winrt::Windows::UI::Xaml::Controls::FocusEngagedEventArgs;
    using FontIcon = winrt::Windows::UI::Xaml::Controls::FontIcon;
    using Grid = ::winrt::Windows::UI::Xaml::Controls::Grid;
    using ICommandBarElement = ::winrt::Windows::UI::Xaml::Controls::ICommandBarElement;
    using IconElement = winrt::Windows::UI::Xaml::Controls::IconElement;
    using IconSourceElement = winrt::Windows::UI::Xaml::Controls::IconSourceElement;
    using IContentControlFactory = winrt::Windows::UI::Xaml::Controls::IContentControlFactory;
    using IControl5 = ::winrt::Windows::UI::Xaml::Controls::IControl5;
    using IControl7 = ::winrt::Windows::UI::Xaml::Controls::IControl7;
    using IControlFactory = winrt::Windows::UI::Xaml::Controls::IControlFactory;
    using IControlProtected = ::winrt::Windows::UI::Xaml::Controls::IControlProtected;
    using IDataTemplateSelectorFactory = winrt::Windows::UI::Xaml::Controls::IDataTemplateSelectorFactory;
    using IInsertionPanel = winrt::Windows::UI::Xaml::Controls::IInsertionPanel;
    using IListViewFactory = winrt::Windows::UI::Xaml::Controls::IListViewFactory;
    using IListViewItemFactory = winrt::Windows::UI::Xaml::Controls::IListViewItemFactory;
    using Image = winrt::Windows::UI::Xaml::Controls::Image;
    using IPanelFactory = winrt::Windows::UI::Xaml::Controls::IPanelFactory;
    using ISliderFactory = winrt::Windows::UI::Xaml::Controls::ISliderFactory;
    using IStyleSelectorFactory = winrt::Windows::UI::Xaml::Controls::IStyleSelectorFactory;
    using IPasswordBox5 = ::winrt::Windows::UI::Xaml::Controls::IPasswordBox5;
    using IRichEditBox6 = ::winrt::Windows::UI::Xaml::Controls::IRichEditBox6;
    using IRichEditBox8 = ::winrt::Windows::UI::Xaml::Controls::IRichEditBox8;
    using IRichTextBlock6 = ::winrt::Windows::UI::Xaml::Controls::IRichTextBlock6;
    using ITextBlock7 = ::winrt::Windows::UI::Xaml::Controls::ITextBlock7;
    using ITextBox8 = ::winrt::Windows::UI::Xaml::Controls::ITextBox8;
    using ItemClickEventArgs = winrt::Windows::UI::Xaml::Controls::ItemClickEventArgs;
    using ItemClickEventHandler = winrt::Windows::UI::Xaml::Controls::ItemClickEventHandler;
    using ItemsControl = winrt::Windows::UI::Xaml::Controls::ItemsControl;
    using ItemsPanelTemplate = winrt::Windows::UI::Xaml::Controls::ItemsPanelTemplate;
    using ItemsPresenter = winrt::Windows::UI::Xaml::Controls::ItemsPresenter;
    using ItemsWrapGrid = winrt::Windows::UI::Xaml::Controls::ItemsWrapGrid;
    using ListView = winrt::Windows::UI::Xaml::Controls::ListView;
    using ListViewBase = winrt::Windows::UI::Xaml::Controls::ListViewBase;
    using ListViewItem = winrt::Windows::UI::Xaml::Controls::ListViewItem;
    using ListViewReorderMode = winrt::Windows::UI::Xaml::Controls::ListViewReorderMode;
    using ListViewSelectionMode = winrt::Windows::UI::Xaml::Controls::ListViewSelectionMode;
    using MenuFlyout = winrt::Windows::UI::Xaml::Controls::MenuFlyout;
    using MenuFlyoutItemBase = winrt::Windows::UI::Xaml::Controls::MenuFlyoutItemBase;
    using MenuFlyoutItem = winrt::Windows::UI::Xaml::Controls::MenuFlyoutItem;
    using MenuFlyoutSubItem = winrt::Windows::UI::Xaml::Controls::MenuFlyoutSubItem;
    using Orientation = winrt::Windows::UI::Xaml::Controls::Orientation;
    using Panel = winrt::Windows::UI::Xaml::Controls::Panel;
    using PasswordBox = winrt::Windows::UI::Xaml::Controls::PasswordBox;
    using PathIcon = winrt::Windows::UI::Xaml::Controls::PathIcon;
    using RadioButton = winrt::Windows::UI::Xaml::Controls::RadioButton;
    using RichEditBox = winrt::Windows::UI::Xaml::Controls::RichEditBox;
    using RichTextBlock = winrt::Windows::UI::Xaml::Controls::RichTextBlock;
    using RichTextBlockOverflow = winrt::Windows::UI::Xaml::Controls::RichTextBlockOverflow;
    using RowDefinition = winrt::Windows::UI::Xaml::Controls::RowDefinition;
    using ScrollContentPresenter = winrt::Windows::UI::Xaml::Controls::ScrollContentPresenter;
    using ScrollIntoViewAlignment = winrt::Windows::UI::Xaml::Controls::ScrollIntoViewAlignment;
    using FxZoomMode = winrt::Windows::UI::Xaml::Controls::ZoomMode;
    using FxScrollViewer = winrt::Windows::UI::Xaml::Controls::ScrollViewer;
    using ScrollViewerViewChangedEventArgs = winrt::Windows::UI::Xaml::Controls::ScrollViewerViewChangedEventArgs;
    using ScrollViewerViewChangingEventArgs = winrt::Windows::UI::Xaml::Controls::ScrollViewerViewChangingEventArgs;
    using SelectionChangedEventArgs = winrt::Windows::UI::Xaml::Controls::SelectionChangedEventArgs;
    using SelectionChangedEventHandler = winrt::Windows::UI::Xaml::Controls::SelectionChangedEventHandler;
    using Slider = winrt::Windows::UI::Xaml::Controls::Slider;
    using SplitView = winrt::Windows::UI::Xaml::Controls::SplitView;
    using SplitViewDisplayMode = winrt::Windows::UI::Xaml::Controls::SplitViewDisplayMode;
    using SplitViewPaneClosingEventArgs = winrt::Windows::UI::Xaml::Controls::SplitViewPaneClosingEventArgs;
    using StackPanel = winrt::Windows::UI::Xaml::Controls::StackPanel;
    using StyleSelector = winrt::Windows::UI::Xaml::Controls::StyleSelector;
    using Symbol = winrt::Windows::UI::Xaml::Controls::Symbol;
    using SymbolIcon = winrt::Windows::UI::Xaml::Controls::SymbolIcon;
    using TextBlock = ::winrt::Windows::UI::Xaml::Controls::TextBlock;
    using TextBox = ::winrt::Windows::UI::Xaml::Controls::TextBox;
    using TextBoxTextChangingEventArgs = winrt::Windows::UI::Xaml::Controls::TextBoxTextChangingEventArgs;
    using TextChangedEventArgs = winrt::Windows::UI::Xaml::Controls::TextChangedEventArgs;
    using TextChangedEventHandler = winrt::Windows::UI::Xaml::Controls::TextChangedEventHandler;
    using ToggleMenuFlyoutItem = winrt::Windows::UI::Xaml::Controls::ToggleMenuFlyoutItem;
    using ToolTip = winrt::Windows::UI::Xaml::Controls::ToolTip;
    using ToolTipService = winrt::Windows::UI::Xaml::Controls::ToolTipService;
    using VirtualizingStackPanel = winrt::Windows::UI::Xaml::Controls::VirtualizingStackPanel;
    using WebView = winrt::Windows::UI::Xaml::Controls::WebView;

    // using namespace ::winrt::Windows::UI::Xaml::Media;
    using ArcSegment = winrt::Windows::UI::Xaml::Media::ArcSegment;
    using Brush = winrt::Windows::UI::Xaml::Media::Brush;
    using BrushMappingMode = winrt::Windows::UI::Xaml::Media::BrushMappingMode;
    using ElementCompositeMode = winrt::Windows::UI::Xaml::Media::ElementCompositeMode;
    using FontFamily = winrt::Windows::UI::Xaml::Media::FontFamily;
    using GeneralTransform = winrt::Windows::UI::Xaml::Media::GeneralTransform;
    using Geometry = winrt::Windows::UI::Xaml::Media::Geometry;
    using GradientSpreadMethod = winrt::Windows::UI::Xaml::Media::GradientSpreadMethod;
    using GradientStop = ::winrt::Windows::UI::Xaml::Media::GradientStop;
    using ImageBrush = ::winrt::Windows::UI::Xaml::Media::ImageBrush;
    using ImageSource = ::winrt::Windows::UI::Xaml::Media::ImageSource;
    using IVisualTreeHelperStatics = winrt::Windows::UI::Xaml::Media::IVisualTreeHelperStatics;
    using IXamlCompositionBrushBaseFactory = winrt::Windows::UI::Xaml::Media::IXamlCompositionBrushBaseFactory;
    using IXamlCompositionBrushBaseProtected = winrt::Windows::UI::Xaml::Media::IXamlCompositionBrushBaseProtected;
    using IXamlLightFactory = winrt::Windows::UI::Xaml::Media::IXamlLightFactory;
    using IXamlLightProtected = winrt::Windows::UI::Xaml::Media::IXamlLightProtected;
    using LinearGradientBrush = ::winrt::Windows::UI::Xaml::Media::LinearGradientBrush;
    using LoadedImageSurface = ::winrt::Windows::UI::Xaml::Media::LoadedImageSurface;
    using PathFigure = winrt::Windows::UI::Xaml::Media::PathFigure;
    using RectangleGeometry = winrt::Windows::UI::Xaml::Media::RectangleGeometry;
    using SolidColorBrush = ::winrt::Windows::UI::Xaml::Media::SolidColorBrush;
    using Stretch = winrt::Windows::UI::Xaml::Media::Stretch;
    using ThemeShadow = winrt::Windows::UI::Xaml::Media::ThemeShadow;
    using TranslateTransform = winrt::Windows::UI::Xaml::Media::TranslateTransform;
    using VisualTreeHelper = winrt::Windows::UI::Xaml::Media::VisualTreeHelper;
    using XamlCompositionBrushBase = ::winrt::Windows::UI::Xaml::Media::XamlCompositionBrushBase;
    using XamlLight = winrt::Windows::UI::Xaml::Media::XamlLight;    

    // using namespace ::winrt::Windows::UI::Xaml::Media.Animation;
    using IStoryboard = ::winrt::Windows::UI::Xaml::Media::Animation::IStoryboard;

    // using namespace ::winrt::Windows::UI::Xaml::Controls::Primitives;
    using ButtonBase = winrt::Windows::UI::Xaml::Controls::Primitives::ButtonBase;
    using CarouselPanel = winrt::Windows::UI::Xaml::Controls::Primitives::CarouselPanel;
    using ComponentResourceLocation = winrt::Windows::UI::Xaml::Controls::Primitives::ComponentResourceLocation;
    using FlyoutBase = winrt::Windows::UI::Xaml::Controls::Primitives::FlyoutBase;
    using FlyoutBaseClosingEventArgs = winrt::Windows::UI::Xaml::Controls::Primitives::FlyoutBaseClosingEventArgs;
    using FlyoutPlacementMode = winrt::Windows::UI::Xaml::Controls::Primitives::FlyoutPlacementMode;
    using FlyoutShowMode = winrt::Windows::UI::Xaml::Controls::Primitives::FlyoutShowMode;
    using FlyoutShowOptions = winrt::Windows::UI::Xaml::Controls::Primitives::FlyoutShowOptions;
    using IFlyoutBase3 = winrt::Windows::UI::Xaml::Controls::Primitives::IFlyoutBase3;
    using IFlyoutBase5 = winrt::Windows::UI::Xaml::Controls::Primitives::IFlyoutBase5;
    using IFlyoutBase6 = winrt::Windows::UI::Xaml::Controls::Primitives::IFlyoutBase6;
    using ILayoutInformationStatics = winrt::Windows::UI::Xaml::Controls::Primitives::ILayoutInformationStatics;
    using IListViewItemPresenterFactory = winrt::Windows::UI::Xaml::Controls::Primitives::IListViewItemPresenterFactory;
    using IScrollEventArgs = winrt::Windows::UI::Xaml::Controls::Primitives::IScrollEventArgs;
    using LayoutInformation = winrt::Windows::UI::Xaml::Controls::Primitives::LayoutInformation;
    using ListViewItemPresenter = winrt::Windows::UI::Xaml::Controls::Primitives::ListViewItemPresenter;
    using OrientedVirtualizingPanel = winrt::Windows::UI::Xaml::Controls::Primitives::OrientedVirtualizingPanel;
    using Popup = winrt::Windows::UI::Xaml::Controls::Primitives::Popup;
    using IPopup3 = winrt::Windows::UI::Xaml::Controls::Primitives::IPopup3;
    using RangeBase = winrt::Windows::UI::Xaml::Controls::Primitives::RangeBase;
    using RangeBaseValueChangedEventArgs = winrt::Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs;
    using RepeatButton = winrt::Windows::UI::Xaml::Controls::Primitives::RepeatButton;
    using ScrollBar = winrt::Windows::UI::Xaml::Controls::Primitives::ScrollBar;
    using ScrollEventArgs = winrt::Windows::UI::Xaml::Controls::Primitives::ScrollEventArgs;
    using ScrollEventType = winrt::Windows::UI::Xaml::Controls::Primitives::ScrollEventType;
    using ScrollingIndicatorMode = winrt::Windows::UI::Xaml::Controls::Primitives::ScrollingIndicatorMode;
    using Selector = winrt::Windows::UI::Xaml::Controls::Primitives::Selector;
    using SelectorItem = winrt::Windows::UI::Xaml::Controls::Primitives::SelectorItem;
    using ToggleButton = winrt::Windows::UI::Xaml::Controls::Primitives::ToggleButton;
    using IToggleButton = winrt::Windows::UI::Xaml::Controls::Primitives::IToggleButton;

    // using namespace winrt::Windows::UI::Xaml::Automation::Peers;
    using AccessibilityView = winrt::Windows::UI::Xaml::Automation::Peers::AccessibilityView;
    using AutomationControlType = winrt::Windows::UI::Xaml::Automation::Peers::AutomationControlType;
    using AutomationControlType = winrt::Windows::UI::Xaml::Automation::Peers::AutomationControlType;
    using AutomationEvents = winrt::Windows::UI::Xaml::Automation::Peers::AutomationEvents;
    using AutomationLiveSetting = winrt::Windows::UI::Xaml::Automation::Peers::AutomationLiveSetting;
    using AutomationNavigationDirection = winrt::Windows::UI::Xaml::Automation::Peers::AutomationNavigationDirection;
    using AutomationOrientation = winrt::Windows::UI::Xaml::Automation::Peers::AutomationOrientation;
    using AutomationPeer = winrt::Windows::UI::Xaml::Automation::Peers::AutomationPeer;
    using ButtonAutomationPeer = winrt::Windows::UI::Xaml::Automation::Peers::ButtonAutomationPeer;
    using FrameworkElementAutomationPeer = winrt::Windows::UI::Xaml::Automation::Peers::FrameworkElementAutomationPeer;
    using IAutomationPeer = winrt::Windows::UI::Xaml::Automation::Peers::IAutomationPeer;
    using IAutomationPeer7 = winrt::Windows::UI::Xaml::Automation::Peers::IAutomationPeer7;
    using IAutomationPeerOverrides = winrt::Windows::UI::Xaml::Automation::Peers::IAutomationPeerOverrides;
    using IFrameworkElementAutomationPeer = winrt::Windows::UI::Xaml::Automation::Peers::IFrameworkElementAutomationPeer;
    using IFrameworkElementAutomationPeerFactory = winrt::Windows::UI::Xaml::Automation::Peers::IFrameworkElementAutomationPeerFactory;
    using IFrameworkElementAutomationPeerStatics = winrt::Windows::UI::Xaml::Automation::Peers::IFrameworkElementAutomationPeerStatics;
    using IListViewItemAutomationPeerFactory = winrt::Windows::UI::Xaml::Automation::Peers::IListViewItemAutomationPeerFactory;
    using ISelectorAutomationPeer = winrt::Windows::UI::Xaml::Automation::Peers::ISelectorAutomationPeer;
    using ISelectorAutomationPeerFactory = winrt::Windows::UI::Xaml::Automation::Peers::ISelectorAutomationPeerFactory;
    using ISliderAutomationPeer = winrt::Windows::UI::Xaml::Automation::Peers::ISliderAutomationPeer;
    using ISliderAutomationPeerFactory = winrt::Windows::UI::Xaml::Automation::Peers::ISliderAutomationPeerFactory;
    using ItemAutomationPeer = winrt::Windows::UI::Xaml::Automation::Peers::ItemAutomationPeer;
    using ListViewItemAutomationPeer = winrt::Windows::UI::Xaml::Automation::Peers::ListViewItemAutomationPeer;
    using PatternInterface = winrt::Windows::UI::Xaml::Automation::Peers::PatternInterface;
    using ItemsControlAutomationPeer = winrt::Windows::UI::Xaml::Automation::Peers::ItemsControlAutomationPeer;
    using SelectorAutomationPeer = winrt::Windows::UI::Xaml::Automation::Peers::SelectorAutomationPeer;
    using SliderAutomationPeer = winrt::Windows::UI::Xaml::Automation::Peers::SliderAutomationPeer;

    // using namespace winrt::Windows::UI::Xaml
    using IElementFactoryGetArgs = winrt::Windows::UI::Xaml::IElementFactoryGetArgs;
    using IElementFactoryRecycleArgs = winrt::Windows::UI::Xaml::IElementFactoryRecycleArgs;
    

#ifdef REPEATER_INCLUDED
    using ElementFactoryGetArgs = winrt::Microsoft::UI::Xaml::Controls::ElementFactoryGetArgs;
    using ElementFactoryRecycleArgs = winrt::Microsoft::UI::Xaml::Controls::ElementFactoryRecycleArgs;
    using IElementFactory = winrt::IInspectable;
#endif
}
