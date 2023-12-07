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
#include <winrt\Windows.ApplicationModel.DataTransfer.DragDrop.h>
#include <winrt\Windows.ApplicationModel.Resources.h>
#include <winrt\Windows.ApplicationModel.Resources.Core.h>
#include <winrt\Windows.Globalization.h>
#include <winrt\Windows.Globalization.NumberFormatting.h>
#include <winrt\Windows.Graphics.Imaging.h>
#include <winrt\Windows.Graphics.Display.h>
#include <winrt\Windows.Graphics.Effects.h>
#include <winrt\Windows.Storage.Streams.h>
#include <winrt\Windows.System.h>
#include <winrt\Windows.System.Power.h>
#include <winrt\Windows.System.Profile.h>
#include <winrt\Windows.System.Threading.h>
#include <winrt\Windows.System.UserProfile.h>
#include <winrt\Windows.UI.h>
#include <winrt\Windows.UI.Core.h>
#include <winrt\Windows.UI.Input.h>
#include <winrt\Windows.UI.Text.h>
#include <winrt\Windows.UI.ViewManagement.h>
#include <winrt\Windows.UI.Xaml.Interop.h>

#include <winrt\Microsoft.Windows.ApplicationModel.Resources.h>
#include <winrt\Microsoft.UI.Dispatching.h>
#include <winrt\Microsoft.UI.h>
#include <winrt\Microsoft.UI.Composition.h>
#include <winrt\Microsoft.UI.Composition.Effects.h>
#include <winrt\Microsoft.UI.Composition.Interactions.h>
#include <winrt\Microsoft.UI.Composition.SystemBackdrops.h>
#include <winrt\Microsoft.UI.Private.Composition.Effects.h>
#include <winrt\Microsoft.UI.Content.h>
#include <winrt\Microsoft.UI.Input.h>
#include <winrt\Microsoft.UI.Input.Experimental.h>
#include <winrt\Microsoft.UI.Text.h>
#include <winrt\Microsoft.UI.Xaml.h>
#include <winrt\Microsoft.UI.Xaml.Automation.Peers.h>
#include <winrt\Microsoft.UI.Xaml.Automation.Provider.h>
#include <winrt\Microsoft.UI.Xaml.Controls.h>
#include <winrt\Microsoft.UI.Xaml.Controls.AnimatedVisuals.h>
#include <winrt\Microsoft.UI.Xaml.Controls.Primitives.h>
#include <winrt\Microsoft.UI.Xaml.Data.h>
#include <winrt\Microsoft.UI.Xaml.Documents.h>
#include <winrt\Microsoft.UI.Xaml.Hosting.h>
#include <winrt\Microsoft.UI.Xaml.Input.h>
#include <winrt\Microsoft.UI.Xaml.Interop.h>
#include <winrt\Microsoft.UI.Xaml.Markup.h>
#include <winrt\Microsoft.UI.Xaml.Media.h>
#include <winrt\Microsoft.UI.Xaml.Media.Animation.h>
#include <winrt\Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt\Microsoft.UI.Xaml.Shapes.h>
#include <winrt\Microsoft.UI.Xaml.XamlTypeInfo.h>
#include <winrt\Microsoft.UI.Private.Controls.h>
#include <winrt\Microsoft.UI.Private.Media.h>
#include <winrt\Microsoft.Web.WebView2.core.h>

namespace winrt
{
    using namespace ::winrt::Windows;
    using namespace ::winrt::Windows::ApplicationModel::Activation;
    using namespace ::winrt::Windows::ApplicationModel::Contacts;
    using namespace ::winrt::Windows::ApplicationModel::Core;
    using namespace ::winrt::Windows::ApplicationModel::DataTransfer;
    using namespace ::winrt::Windows::ApplicationModel::DataTransfer::DragDrop;
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
    using namespace ::winrt::Windows::System::Power;
    using namespace ::winrt::Windows::System::Profile;
    using namespace ::winrt::Windows::System::Threading;
    using namespace ::winrt::Windows::System::UserProfile;
    using namespace ::winrt::Windows::UI;
    using namespace ::winrt::Windows::UI::Core;
    using namespace ::winrt::Windows::UI::Text;
    using namespace ::winrt::Windows::UI::ViewManagement;
    using namespace ::winrt::Windows::UI::WindowManagement;
    using namespace ::winrt::Windows::Web;

    using namespace ::winrt::Microsoft::UI::Dispatching;
    using namespace ::winrt::Microsoft::UI::Composition;
    using namespace ::winrt::Microsoft::UI::Private::Composition::Effects;
    using namespace ::winrt::Microsoft::UI::Composition::Interactions;
    using namespace ::winrt::Microsoft::UI::Composition::SystemBackdrops;
    using namespace ::winrt::Microsoft::UI::Content;
    using namespace ::winrt::Microsoft::UI::Input;
    using namespace ::winrt::Microsoft::UI::Input::Experimental;

    using namespace ::winrt::Microsoft::Web::WebView2::Core;

    using namespace ::winrt::Microsoft::UI::Xaml;
    using namespace ::winrt::Microsoft::UI::Xaml::Automation;
    using namespace ::winrt::Microsoft::UI::Xaml::Automation::Provider;
    using namespace ::winrt::Microsoft::UI::Xaml::Data;
    using namespace ::winrt::Microsoft::UI::Xaml::Documents;
    using namespace ::winrt::Microsoft::UI::Xaml::Hosting;
    using namespace ::winrt::Microsoft::UI::Xaml::Input;
    using namespace ::winrt::Microsoft::UI::Xaml::Interop;
    using namespace ::winrt::Microsoft::UI::Xaml::Markup;
    using namespace ::winrt::Microsoft::UI::Xaml::Media::Animation;
    using namespace ::winrt::Microsoft::UI::Xaml::Media::Imaging;
    using namespace ::winrt::Microsoft::UI::Xaml::Shapes;
    using namespace ::winrt::Windows::System::Profile;
    namespace Microsoft::UI::Xaml::XamlTypeInfo {}
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


    namespace Microsoft::Experimental::UI::Xaml::XamlTypeInfo {}
    using namespace ::winrt::Microsoft::Experimental::UI::Xaml::XamlTypeInfo;
    namespace Microsoft::Experimental::UI::Xaml::Controls {}
    using namespace ::winrt::Microsoft::Experimental::UI::Xaml::Controls;
    namespace Microsoft::Experimental::UI::Xaml::Controls::Primitives {}
    using namespace ::winrt::Microsoft::Experimental::UI::Xaml::Controls::Primitives;
    namespace Microsoft::Experimental::UI::Xaml::Controls::AnimatedVisuals {}
    using namespace ::winrt::Microsoft::Experimental::UI::Xaml::Controls::AnimatedVisuals;
    namespace Microsoft::Experimental::UI::Xaml::Media {}
    using namespace ::winrt::Microsoft::Experimental::UI::Xaml::Media;
    namespace Microsoft::Experimental::UI::Xaml::Automation::Peers {}
    using namespace ::winrt::Microsoft::Experimental::UI::Xaml::Automation::Peers;

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

    namespace Microsoft::Experimental::UI::Xaml::Controls::implementation {}
    namespace Microsoft::Experimental::UI::Xaml::Controls::Primitives::implementation {}
    namespace Microsoft::Experimental::UI::Xaml::Controls::AnimatedVisuals::implementation {}
    namespace Microsoft::Experimental::UI::Xaml::Media::implementation {}
    namespace Microsoft::Experimental::UI::Xaml::Automation::Peers::implementation {}
    namespace Microsoft::Experimental::UI::Xaml::XamlTypeInfo::implementation {} 

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

        using namespace ::winrt::Microsoft::Experimental::UI::Xaml::Controls::implementation;
        using namespace ::winrt::Microsoft::Experimental::UI::Xaml::Controls::Primitives::implementation;
        using namespace ::winrt::Microsoft::Experimental::UI::Xaml::Controls::AnimatedVisuals::implementation;
        using namespace ::winrt::Microsoft::Experimental::UI::Xaml::Media::implementation;
        using namespace ::winrt::Microsoft::Experimental::UI::Xaml::Automation::Peers::implementation;
        using namespace ::winrt::Microsoft::Experimental::UI::Xaml::XamlTypeInfo::implementation;
    }

    namespace Microsoft::UI::Private::Controls::factory_implementation {}
    namespace Microsoft::UI::Private::Media::factory_implementation {}
    namespace Microsoft::UI::Xaml::Controls::factory_implementation {}
    namespace Microsoft::UI::Xaml::XamlTypeInfo::factory_implementation {}
    namespace Microsoft::UI::Xaml::Controls::Primitives::factory_implementation {}
    namespace Microsoft::UI::Xaml::Controls::AnimatedVisuals::factory_implementation {}
    namespace Microsoft::UI::Xaml::Media::factory_implementation {}
    namespace Microsoft::UI::Xaml::Automation::Peers::factory_implementation {}

    namespace Microsoft::Experimental::UI::Xaml::Controls::factory_implementation {}
    namespace Microsoft::Experimental::UI::Xaml::XamlTypeInfo::factory_implementation {}
    namespace Microsoft::Experimental::UI::Xaml::Controls::Primitives::factory_implementation {}
    namespace Microsoft::Experimental::UI::Xaml::Controls::AnimatedVisuals::factory_implementation {}
    namespace Microsoft::Experimental::UI::Xaml::Media::factory_implementation {}
    namespace Microsoft::Experimental::UI::Xaml::Automation::Peers::factory_implementation {}

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

        using namespace ::winrt::Microsoft::Experimental::UI::Xaml::Controls::factory_implementation;
        using namespace ::winrt::Microsoft::Experimental::UI::Xaml::XamlTypeInfo::factory_implementation;
        using namespace ::winrt::Microsoft::Experimental::UI::Xaml::Controls::Primitives::factory_implementation;
        using namespace ::winrt::Microsoft::Experimental::UI::Xaml::Controls::AnimatedVisuals::factory_implementation;
        using namespace ::winrt::Microsoft::Experimental::UI::Xaml::Media::factory_implementation;
        using namespace ::winrt::Microsoft::Experimental::UI::Xaml::Automation::Peers::factory_implementation;
    }

    // using namespace ::winrt::Windows::System;
    using VirtualKey = ::winrt::Windows::System::VirtualKey;
    using VirtualKeyModifiers = ::winrt::Windows::System::VirtualKeyModifiers;

    // using namespace ::winrt::Windows::UI::Core;
    using CoreCursorType = ::winrt::Windows::UI::Core::CoreCursorType;

    // using namespace ::winrt::Windows::UI::Input;
    using PointerUpdateKind = ::winrt::Microsoft::UI::Input::PointerUpdateKind;
    using PointerDeviceType = ::winrt::Microsoft::UI::Input::PointerDeviceType;
    using InputSystemCursorShape = ::winrt::Microsoft::UI::Input::InputSystemCursorShape;

    // using namespace ::winrt::Windows::UI::Xaml::Interop;
    using TypeName = ::winrt::Windows::UI::Xaml::Interop::TypeName;
    using TypeKind = ::winrt::Windows::UI::Xaml::Interop::TypeKind;

    // using namespace winrt::Microsoft::UI
    using ColorHelper = winrt::Microsoft::UI::ColorHelper;
    using IColorHelperStatics = winrt::Microsoft::UI::IColorHelperStatics;
    using Colors = winrt::Microsoft::UI::Colors;
    using IColorsStatics = winrt::Microsoft::UI::IColorsStatics;

    // using namespace winrt::Microsoft::UI::Text
    using FontWeights = winrt::Microsoft::UI::Text::FontWeights;
    using FormatEffect = winrt::Microsoft::UI::Text::FormatEffect;
    using IFontWeightsStatics = winrt::Microsoft::UI::Text::IFontWeightsStatics;
    using ITextSelection = winrt::Microsoft::UI::Text::ITextSelection;
    using TextGetOptions = winrt::Microsoft::UI::Text::TextGetOptions;
    using TextRangeUnit = winrt::Microsoft::UI::Text::TextRangeUnit;
    using UnderlineType = winrt::Microsoft::UI::Text::UnderlineType;
    
    // using namespace ::winrt::Microsoft::UI::Xaml::Controls;
    using FxZoomMode = winrt::Microsoft::UI::Xaml::Controls::ZoomMode;
    using FxScrollViewer = winrt::Microsoft::UI::Xaml::Controls::ScrollViewer;
#ifdef REPEATER_INCLUDED
    using ElementFactoryGetArgs = winrt::Microsoft::UI::Xaml::ElementFactoryGetArgs;
    using ElementFactoryRecycleArgs = winrt::Microsoft::UI::Xaml::ElementFactoryRecycleArgs;
    using IElementFactory = winrt::Microsoft::UI::Xaml::IElementFactory;
#endif
    
}
