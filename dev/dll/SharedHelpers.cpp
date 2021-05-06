// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "MUXControlsFactory.h"
#include "SharedHelpers.h"
#include <roapi.h>

bool SharedHelpers::s_isOnXboxInitialized{ false };
bool SharedHelpers::s_isOnXbox{ false };
bool SharedHelpers::s_isMouseModeEnabledInitialized{ false };
bool SharedHelpers::s_isMouseModeEnabled{ false };

bool SharedHelpers::IsAnimationsEnabled()
{
    winrt::UISettings uiSettings = winrt::UISettings();
    return uiSettings.AnimationsEnabled();
}

bool SharedHelpers::IsInDesignMode()
{
    // Design mode V2 can do many things that V1 could not do. Codepaths that were checking for design mode should generally be
    // let through if V2 is enabled.
    return IsInDesignModeV1();
}

bool SharedHelpers::IsInDesignModeV1()
{
    static bool s_isInDesignModeV1 = winrt::Windows::ApplicationModel::DesignMode::DesignModeEnabled() && !IsInDesignModeV2();
    return s_isInDesignModeV1;
}

bool SharedHelpers::IsInDesignModeV2()
{
    static bool s_isInDesignModeV2 = IsRS3OrHigher() && winrt::Windows::ApplicationModel::DesignMode::DesignMode2Enabled();
    return s_isInDesignModeV2;
}

// logical helpers
bool SharedHelpers::Is21H1OrHigher()
{
    return IsAPIContractV14Available();
}

bool SharedHelpers::IsVanadiumOrHigher()
{
    return IsAPIContractV9Available();
}

bool SharedHelpers::Is19H1OrHigher()
{
    return IsAPIContractV8Available();
}

bool SharedHelpers::IsRS5OrHigher()
{
    return IsAPIContractV7Available();
}

bool SharedHelpers::IsRS4OrHigher()
{
    return IsAPIContractV6Available();
}

bool SharedHelpers::IsRS3OrHigher()
{
    return IsAPIContractV5Available();
}

bool SharedHelpers::IsRS2OrHigher()
{
    return IsAPIContractV4Available();
}

bool SharedHelpers::IsRS1OrHigher()
{
    return IsAPIContractV3Available();
}

bool SharedHelpers::IsRS1()
{
    return IsAPIContractV3Available() && !IsAPIContractV4Available();
}

bool SharedHelpers::IsTH2OrLower()
{
    return !IsAPIContractV3Available();
}

bool SharedHelpers::IsXamlCompositionBrushBaseAvailable()
{
    // On RS3 we know XamlCompositionBrushBase was always present, so short circuit the check there.
    static bool s_isAvailable = IsRS3OrHigher() || winrt::ApiInformation::IsTypePresent(L"Windows.UI.Xaml.Media.XamlCompositionBrushBase");
    return s_isAvailable;
}

bool SharedHelpers::DoesXamlMoveRSVLightToRootVisual()
{
    // In RS3 we made a change where WUX internally sets lights on the RootScrollViewer on the RootVisual instead. If that
    // happens, then we don't need to attach lights to the other roots.
    static bool s_movesLightFromRSVToRootVisual = IsRS3OrHigher();
    return s_movesLightFromRSVToRootVisual;
}

bool SharedHelpers::DoesListViewItemPresenterVSMWork()
{
    // The fix to make ListViewItemPresenter's VSM work was done at the same time as when the RevealListViewItemPresenter type was *removed* from windows.
    // Also check if RevealBorderBrush was present because RS3 was around for a bit before we added Reveal in.
    static bool s_isAvailable = IsRS3OrHigher();
    return s_isAvailable;
}

bool SharedHelpers::IsCoreWindowActivationModeAvailable()
{
    // In RS3 we got CoreWindow.ActivationMode API which can be queried for window activation state,
    // and particularly in a Component UI host appto check if the compoenent is active (while host isn't).
    static bool s_isAvailable = IsRS3OrHigher();
    return s_isAvailable;
}

bool SharedHelpers::IsFlyoutShowOptionsAvailable()
{
    static bool s_isFlyoutShowOptionsAvailable =
        Is19H1OrHigher() ||
        winrt::ApiInformation::IsTypePresent(L"Windows.UI.Xaml.Primitives.FlyoutShowOptions");
    return s_isFlyoutShowOptionsAvailable;
}

bool SharedHelpers::IsScrollViewerReduceViewportForCoreInputViewOcclusionsAvailable()
{
    static bool s_isScrollViewerReduceViewportForCoreInputViewOcclusionsAvailable =
        Is19H1OrHigher() ||
        winrt::ApiInformation::IsPropertyPresent(L"Windows.UI.Xaml.Controls.ScrollViewer", L"ReduceViewportForCoreInputViewOcclusions");
    return s_isScrollViewerReduceViewportForCoreInputViewOcclusionsAvailable;
}

bool SharedHelpers::IsScrollContentPresenterSizesContentToTemplatedParentAvailable()
{
    static bool s_isScrollContentPresenterSizesContentToTemplatedParentAvailable =
        Is19H1OrHigher() ||
        winrt::ApiInformation::IsPropertyPresent(L"Windows.UI.Xaml.Controls.ScrollContentPresenter", L"SizesContentToTemplatedParent");
    return s_isScrollContentPresenterSizesContentToTemplatedParentAvailable;
}

bool SharedHelpers::IsBringIntoViewOptionsVerticalAlignmentRatioAvailable()
{
    static bool s_isBringIntoViewOptionsVerticalAlignmentRatioAvailable =
        IsRS4OrHigher() ||
        winrt::ApiInformation::IsPropertyPresent(L"Windows.UI.Xaml.BringIntoViewOptions", L"VerticalAlignmentRatio");
    return s_isBringIntoViewOptionsVerticalAlignmentRatioAvailable;
}

bool SharedHelpers::IsFrameworkElementInvalidateViewportAvailable()
{
    static bool s_isFrameworkElementInvalidateViewportAvailable = IsRS5OrHigher();
    return s_isFrameworkElementInvalidateViewportAvailable;
}

bool SharedHelpers::IsControlCornerRadiusAvailable()
{
    static bool s_isControlCornerRadiusAvailable =
        Is19H1OrHigher() ||
        (IsRS5OrHigher() && winrt::ApiInformation::IsPropertyPresent(L"Windows.UI.Xaml.Controls.Control", L"CornerRadius"));
    return s_isControlCornerRadiusAvailable;
}

bool SharedHelpers::IsTranslationFacadeAvailable(const winrt::UIElement& element)
{
    static bool s_areFacadesAvailable = (element.try_as<winrt::Windows::UI::Xaml::IUIElement9>() != nullptr);
    return s_areFacadesAvailable;
}

bool SharedHelpers::IsIconSourceElementAvailable()
{
    static bool s_isAvailable =
        Is19H1OrHigher() ||
        winrt::ApiInformation::IsTypePresent(L"Windows.UI.Xaml.Controls.IconSourceElement");
    return s_isAvailable;
}

bool SharedHelpers::IsStandardUICommandAvailable()
{
    static bool s_isAvailable =
        Is19H1OrHigher() ||
        (winrt::ApiInformation::IsTypePresent(L"Windows.UI.Xaml.Input.XamlUICommand") &&
            winrt::ApiInformation::IsTypePresent(L"Windows.UI.Xaml.Input.StandardUICommand"));
    return s_isAvailable;
}

bool SharedHelpers::IsDispatcherQueueAvailable()
{
    static bool s_isAvailable =
        IsRS4OrHigher() ||
        winrt::ApiInformation::IsTypePresent(L"Windows.System.DispatcherQueue");
    return s_isAvailable;
}

bool SharedHelpers::IsThemeShadowAvailable()
{
    static bool s_isThemeShadowAvailable =
        IsVanadiumOrHigher() ||
        winrt::ApiInformation::IsTypePresent(L"Windows.UI.Xaml.Media.ThemeShadow");
    return s_isThemeShadowAvailable;
}

bool SharedHelpers::IsIsLoadedAvailable()
{
    static bool s_isAvailable =
        IsRS5OrHigher() ||
        winrt::ApiInformation::IsPropertyPresent(L"Windows.UI.Xaml.FrameworkElement", L"IsLoaded");
    return s_isAvailable;
}

bool SharedHelpers::IsCompositionRadialGradientBrushAvailable()
{
    static bool s_isAvailable =
        Is21H1OrHigher() ||
        winrt::ApiInformation::IsTypePresent(L"Windows.UI.Composition.CompositionRadialGradientBrush");
    return s_isAvailable;
}

bool SharedHelpers::IsSelectionIndicatorModeAvailable()
{
    static bool s_isSelectionIndicatorModeAvailable = winrt::ApiInformation::IsTypePresent(L"Windows.UI.Xaml.Controls.Primitives.ListViewItemPresenterSelectionIndicatorMode");
    return s_isSelectionIndicatorModeAvailable;
}

template <uint16_t APIVersion> bool SharedHelpers::IsAPIContractVxAvailable()
{
    static bool isAPIContractVxAvailableInitialized = false;
    static bool isAPIContractVxAvailable = false;
    if (!isAPIContractVxAvailableInitialized)
    {
        isAPIContractVxAvailableInitialized = true;
        isAPIContractVxAvailable = winrt::ApiInformation::IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", APIVersion);
    }

    return isAPIContractVxAvailable;
}

// base helpers
bool SharedHelpers::IsAPIContractV14Available()
{
    return IsAPIContractVxAvailable<14>();
}

bool SharedHelpers::IsAPIContractV9Available()
{
    return IsAPIContractVxAvailable<9>();
}

bool SharedHelpers::IsAPIContractV8Available()
{
    return IsAPIContractVxAvailable<8>();
}

bool SharedHelpers::IsAPIContractV7Available()
{
    return IsAPIContractVxAvailable<7>();
}

bool SharedHelpers::IsAPIContractV6Available()
{
    return IsAPIContractVxAvailable<6>();
}

bool SharedHelpers::IsAPIContractV5Available()
{
    return IsAPIContractVxAvailable<5>();
}


bool SharedHelpers::IsAPIContractV4Available()
{
    return IsAPIContractVxAvailable<4>();
}

bool SharedHelpers::IsAPIContractV3Available()
{
    return IsAPIContractVxAvailable<3>();
}

void* __stdcall winrt_get_activation_factory(std::wstring_view const& name);

bool IsInPackage(std::wstring_view detectorName)
{
    // Special type that we manually list here which is not part of the Nuget dll distribution package. 
    // This is our breadcrumb that we leave to be able to detect at runtime that we're using the framework package.
    // It's listed only in the Framework packages' AppxManifest.xml as an activatable type but only so
    // that RoGetActivationFactory will change behavior and call our DllGetActivationFactory. It doesn't
    // matter what comes back for the activationfactory. If it succeeds it means we're running against
    // the framework package.

    winrt::hstring typeName{ detectorName };
    winrt::IActivationFactory activationFactory;

    if (SUCCEEDED(RoGetActivationFactory(static_cast<HSTRING>(winrt::get_abi(typeName)), winrt::guid_of<IActivationFactory>(), winrt::put_abi(activationFactory))))
    {
        return true;
    }

    return false;
}

bool SharedHelpers::IsInFrameworkPackage()
{
    static bool isInFrameworkPackage = IsInPackage(L"Microsoft.UI.Private.Controls.FrameworkPackageDetector"sv);
    return isInFrameworkPackage;
}

bool SharedHelpers::IsInCBSPackage()
{
    static bool isInCBSPackage = IsInPackage(L"Microsoft.UI.Private.Controls.CBSPackageDetector"sv);
    return isInCBSPackage;
}

// Platform scale helpers
winrt::Rect SharedHelpers::ConvertDipsToPhysical(winrt::UIElement const& xamlRootReference, const winrt::Rect& dipsRect)
{
    try
    {
        const auto scaleFactor = Is19H1OrHigher()?
                                    static_cast<float>(xamlRootReference.XamlRoot().RasterizationScale()):
                                    static_cast<float>(winrt::DisplayInformation::GetForCurrentView().RawPixelsPerViewPixel());
        return winrt::Rect
        {
            dipsRect.X * scaleFactor,
            dipsRect.Y * scaleFactor,
            dipsRect.Width * scaleFactor,
            dipsRect.Height * scaleFactor
        };
    }
    catch (winrt::hresult_error)
    {
        // Calling GetForCurrentView on threads without a CoreWindow throws an error. This comes up in places like LogonUI.
        // In this circumstance, we'll just always expand down, since we can't get bounds information.
    }

    return dipsRect;
}

winrt::Rect SharedHelpers::ConvertPhysicalToDips(winrt::UIElement const& xamlRootReference, const winrt::Rect& physicalRect)
{
    try
    {
        const auto scaleFactor = static_cast<float>(winrt::DisplayInformation::GetForCurrentView().RawPixelsPerViewPixel());
        return winrt::Rect
        {
            physicalRect.X / scaleFactor,
            physicalRect.Y / scaleFactor,
            physicalRect.Width / scaleFactor,
            physicalRect.Height / scaleFactor
        };
    }
    catch (winrt::hresult_error)
    {
        // Calling GetForCurrentView on threads without a CoreWindow throws an error. This comes up in places like LogonUI.
        // In this circumstance, we'll just always expand down, since we can't get bounds information.
    }
    
    return physicalRect;
}

bool SharedHelpers::IsOnXbox()
{
    if (!s_isOnXboxInitialized)
    {
        auto deviceFamily = winrt::AnalyticsInfo::VersionInfo().DeviceFamily();
        s_isOnXbox = (deviceFamily == L"Windows.Xbox")
            || (deviceFamily == L"Windows.XBoxSRA")
            || (deviceFamily == L"Windows.XBoxERA");
        s_isOnXboxInitialized = true;
    }
    return s_isOnXbox;
}

// Be careful to use this API.
// If IsTH2OrLower(), it return false;
bool SharedHelpers::IsMouseModeEnabled()
{
    if (!s_isMouseModeEnabledInitialized)
    {
        if (IsRS1OrHigher())
        {
            // RequiresPointerMode is in Windows.Foundation.UniversalApiContract (introduced v3)
            s_isMouseModeEnabled = winrt::Application::Current().RequiresPointerMode()
                == winrt::ApplicationRequiresPointerMode::Auto;
        }
        else
        {
            s_isMouseModeEnabled = false;
        }
        s_isMouseModeEnabledInitialized = true;
    }
    return s_isMouseModeEnabled;
}

void SharedHelpers::ScheduleActionAfterWait(
    std::function<void()> const& action,
    unsigned int millisecondWait)
{
    
    DispatcherHelper dispatcherHelper;

    // The callback that is given to CreateTimer is called off of the UI thread.
    // In order to make this useful by making it so we can interact with XAML objects,
    // we'll use the dispatcher to first post our work to the UI thread before executing it.
    auto timer = winrt::ThreadPoolTimer::CreateTimer(winrt::TimerElapsedHandler(
        [action, dispatcherHelper](auto const&)
        {
            dispatcherHelper.RunAsync(action);
        }),
        std::chrono::milliseconds{ millisecondWait });
}


// Stream helpers
winrt::InMemoryRandomAccessStream SharedHelpers::CreateStreamFromBytes(const winrt::array_view<const byte>& bytes)
{
    winrt::InMemoryRandomAccessStream stream;
    winrt::DataWriter writer(stream);

    writer.WriteBytes(winrt::array_view<const byte>(bytes));
    SyncWait(writer.StoreAsync());
    SyncWait(writer.FlushAsync());
    auto detachedStream = writer.DetachStream();
    writer.Close();

    stream.Seek(0);

    return stream;
}

void SharedHelpers::QueueCallbackForCompositionRendering(std::function<void()> callback)
{
    try
    {
        auto renderingEventToken = std::make_shared<winrt::event_token>();
        *renderingEventToken = winrt::Xaml::Media::CompositionTarget::Rendering([renderingEventToken, callback](auto&, auto&) {

            // Detach event or Rendering will keep calling us back.
            winrt::Xaml::Media::CompositionTarget::Rendering(*renderingEventToken);

            callback();
        });
    }
    catch (const winrt::hresult_error &e)
    {
        // DirectUI::CompositionTarget::add_Rendering can fail with RPC_E_WRONG_THREAD if called while the Xaml Core is being shutdown,
        // and there is evidence from Watson that such calls are made in real apps (see Bug 13554197).
        // Since the core is being shutdown, we no longer care about whatever work we wanted to defer to CT.Rendering, so ignore this error.
        if (e.to_abi() != RPC_E_WRONG_THREAD) { throw; }
    }
}

// Rect helpers

// Returns TRUE if either rect is empty or the rects
// have an empty intersection.
bool SharedHelpers::DoRectsIntersect(
    const winrt::Rect& rect1,
    const winrt::Rect& rect2)
{
    const auto doIntersect =
        !(rect1.Width <= 0 || rect1.Height <= 0 || rect2.Width <= 0 || rect2.Height <= 0) &&
        (rect2.X <= rect1.X + rect1.Width) &&
        (rect2.X + rect2.Width >= rect1.X) &&
        (rect2.Y <= rect1.Y + rect1.Height) &&
        (rect2.Y + rect2.Height >= rect1.Y);
    return doIntersect;
}

winrt::IInspectable SharedHelpers::FindResource(const std::wstring_view& resource, const winrt::ResourceDictionary& resources, const winrt::IInspectable& defaultValue)
{
    auto boxedResource = box_value(resource);
    return resources.HasKey(boxedResource) ? resources.Lookup(boxedResource) : defaultValue;
}

winrt::IInspectable SharedHelpers::FindInApplicationResources(const std::wstring_view& resource, const winrt::IInspectable& defaultValue)
{
    return SharedHelpers::FindResource(resource, winrt::Application::Current().Resources(), defaultValue);
}

// When checkVisibility is True, IsAncestor additionally checks if any UIElement from the 'child'
// to the 'parent' chain is Collapsed. It returns False when that is the case.
bool SharedHelpers::IsAncestor(
    const winrt::DependencyObject& child,
    const winrt::DependencyObject& parent,
    bool checkVisibility)
{
    if (!child || !parent || child == parent)
    {
        return false;
    }

    if (checkVisibility)
    {
        winrt::IUIElement parentAsUIE = parent.as<winrt::IUIElement>();

        if (parentAsUIE && parentAsUIE.Visibility() == winrt::Visibility::Collapsed)
        {
            return false;
        }

        winrt::IUIElement childAsUIE = child.as<winrt::IUIElement>();

        if (childAsUIE && childAsUIE.Visibility() == winrt::Visibility::Collapsed)
        {
            return false;
        }
    }

    winrt::DependencyObject parentTemp = winrt::VisualTreeHelper::GetParent(child);
    while (parentTemp)
    {
        if (checkVisibility)
        {
            winrt::IUIElement parentTempAsUIE = parentTemp.as<winrt::IUIElement>();

            if (parentTempAsUIE && parentTempAsUIE.Visibility() == winrt::Visibility::Collapsed)
            {
                return false;
            }
        }

        if (parentTemp == parent)
        {
            return true;
        }

        parentTemp = winrt::VisualTreeHelper::GetParent(parentTemp);
    }

    return false;
}

#ifdef ICONSOURCE_INCLUDED

winrt::IconElement SharedHelpers::MakeIconElementFrom(winrt::IconSource const& iconSource)
{
    if (auto fontIconSource = iconSource.try_as<winrt::FontIconSource>())
    {
        winrt::FontIcon fontIcon;

        fontIcon.Glyph(fontIconSource.Glyph());
        fontIcon.FontSize(fontIconSource.FontSize());
        if (const auto newForeground = fontIconSource.Foreground())
        {
            fontIcon.Foreground(newForeground);
        }

        if (fontIconSource.FontFamily())
        {
            fontIcon.FontFamily(fontIconSource.FontFamily());
        }

        fontIcon.FontWeight(fontIconSource.FontWeight());
        fontIcon.FontStyle(fontIconSource.FontStyle());
        fontIcon.IsTextScaleFactorEnabled(fontIconSource.IsTextScaleFactorEnabled());
        fontIcon.MirroredWhenRightToLeft(fontIconSource.MirroredWhenRightToLeft());

        return fontIcon;
    }
    else if (auto symbolIconSource = iconSource.try_as<winrt::SymbolIconSource>())
    {
        winrt::SymbolIcon symbolIcon;
        symbolIcon.Symbol(symbolIconSource.Symbol());
        if (const auto newForeground = symbolIconSource.Foreground())
        {
            symbolIcon.Foreground(newForeground);
        }
        return symbolIcon;
    }
    else if (auto bitmapIconSource = iconSource.try_as<winrt::BitmapIconSource>())
    {
        winrt::BitmapIcon bitmapIcon;

        if (bitmapIconSource.UriSource())
        {
            bitmapIcon.UriSource(bitmapIconSource.UriSource());
        }

        if (winrt::ApiInformation::IsPropertyPresent(L"Windows.UI.Xaml.Controls.BitmapIcon", L"ShowAsMonochrome"))
        {
            bitmapIcon.ShowAsMonochrome(bitmapIconSource.ShowAsMonochrome());
        }
        if (const auto newForeground = bitmapIconSource.Foreground())
        {
            bitmapIcon.Foreground(newForeground);
        }
        return bitmapIcon;
    }
#ifdef IMAGEICON_INCLUDED
    else if (auto imageIconSource = iconSource.try_as<winrt::ImageIconSource>())
    {
        winrt::ImageIcon imageIcon;
        if (const auto imageSource = imageIconSource.ImageSource())
        {
            imageIcon.Source(imageSource);
        }
        if (const auto newForeground = imageIconSource.Foreground())
        {
            imageIcon.Foreground(newForeground);
        }
        return imageIcon;
    }
#endif
    else if (auto pathIconSource = iconSource.try_as<winrt::PathIconSource>())
    {
        winrt::PathIcon pathIcon;

        if (pathIconSource.Data())
        {
            pathIcon.Data(pathIconSource.Data());
        }
        if (const auto newForeground = pathIconSource.Foreground())
        {
            pathIcon.Foreground(newForeground);
        }
        return pathIcon;
    }
#ifdef ANIMATEDICON_INCLUDED
    else if (auto animatedIconSource = iconSource.try_as<winrt::AnimatedIconSource>())
    {
        winrt::AnimatedIcon animatedIcon;
        if (auto const source = animatedIconSource.Source())
        {
            animatedIcon.Source(source);
        }
        if (auto const fallbackIconSource = animatedIconSource.FallbackIconSource())
        {
            animatedIcon.FallbackIconSource(fallbackIconSource);
        }
        if (const auto newForeground = animatedIconSource.Foreground())
        {
            animatedIcon.Foreground(newForeground);
        }
        return animatedIcon;
    }
#endif

    return nullptr;
}

#endif

void SharedHelpers::SetBinding(
    std::wstring_view const& pathString,
    winrt::DependencyObject const& target,
    winrt::DependencyProperty const& targetProperty)
{
    winrt::Binding binding;
    winrt::RelativeSource relativeSource;
    relativeSource.Mode(winrt::RelativeSourceMode::TemplatedParent);
    binding.RelativeSource(relativeSource);

    binding.Path(winrt::PropertyPath(pathString));

    winrt::BindingOperations::SetBinding(target, targetProperty, binding);
}

// Be cautious: this function may introduce memory leak because Source holds strong reference to target too
// There’s an intermediary object – the BindingExpression when BindingOperations::SetBinding
// For example, if source is NavigationView and target is content control,
// and there is strong reference: NavigationView -> ContentControl
// BindingExpression.Source also make a strong reference to NavigationView
// and it introduces the cycle: ContentControl -> BindingExpression -> NavigationView -> ContentControl
// Prefer to use RelativeSource version of SetBinding if possible.
void SharedHelpers::SetBinding(
    winrt::IInspectable const& source,
    std::wstring_view const& pathString,
    winrt::DependencyObject const& target,
    winrt::DependencyProperty const& targetProperty,
    winrt::IValueConverter const& converter,
    winrt::BindingMode mode)
{
    winrt::Binding binding;
    binding.Source(source);

    binding.Path(winrt::PropertyPath(pathString));
    binding.Mode(mode);

    if (converter)
    {
        binding.Converter(converter);
    }

    winrt::BindingOperations::SetBinding(target, targetProperty, binding);
}

winrt::ITextSelection SharedHelpers::GetRichTextSelection(winrt::RichEditBox const& richEditBox)
{
    return richEditBox.Document() ? richEditBox.Document().Selection() : nullptr;
}

winrt::VirtualKey SharedHelpers::GetVirtualKeyFromChar(WCHAR c)
{
    switch (c)
    {
    case L'A':
    case L'a':
        return winrt::VirtualKey::A;
    case L'B':
    case L'b':
        return winrt::VirtualKey::B;
    case L'C':
    case L'c':
        return winrt::VirtualKey::C;
    case L'D':
    case L'd':
        return winrt::VirtualKey::D;
    case L'E':
    case L'e':
        return winrt::VirtualKey::E;
    case L'F':
    case L'f':
        return winrt::VirtualKey::F;
    case L'G':
    case L'g':
        return winrt::VirtualKey::G;
    case L'H':
    case L'h':
        return winrt::VirtualKey::H;
    case L'I':
    case L'i':
        return winrt::VirtualKey::I;
    case L'J':
    case L'j':
        return winrt::VirtualKey::J;
    case L'K':
    case L'k':
        return winrt::VirtualKey::K;
    case L'L':
    case L'l':
        return winrt::VirtualKey::L;
    case L'M':
    case L'm':
        return winrt::VirtualKey::M;
    case L'N':
    case L'n':
        return winrt::VirtualKey::N;
    case L'O':
    case L'o':
        return winrt::VirtualKey::O;
    case L'P':
    case L'p':
        return winrt::VirtualKey::P;
    case L'Q':
    case L'q':
        return winrt::VirtualKey::Q;
    case L'R':
    case L'r':
        return winrt::VirtualKey::R;
    case L'S':
    case L's':
        return winrt::VirtualKey::S;
    case L'T':
    case L't':
        return winrt::VirtualKey::T;
    case L'U':
    case L'u':
        return winrt::VirtualKey::U;
    case L'V':
    case L'v':
        return winrt::VirtualKey::V;
    case L'W':
    case L'w':
        return winrt::VirtualKey::W;
    case L'X':
    case L'x':
        return winrt::VirtualKey::X;
    case L'Y':
    case L'y':
        return winrt::VirtualKey::Y;
    case L'Z':
    case L'z':
        return winrt::VirtualKey::Z;
    default:
        return winrt::VirtualKey::None;
    }
}

// Sometimes we want to get a string representation from an arbitrary object. E.g. for constructing a UIA Name
// from an automation peer. There is no guarantee that an arbitrary object is convertable to a string, so
// this function may return an empty string.
winrt::hstring SharedHelpers::TryGetStringRepresentationFromObject(winrt::IInspectable obj)
{
    winrt::hstring returnHString;

    if(obj)
    {
        if (auto stringable = obj.try_as<winrt::IStringable>())
        {
            returnHString = stringable.ToString();
        }
        if(returnHString.empty())
        {
            returnHString = winrt::unbox_value_or<winrt::hstring>(obj, returnHString);
        }
    }
    
    return returnHString;
}

/* static */
winrt::float4 SharedHelpers::RgbaColor(const winrt::Color& color)
{
    return { static_cast<float>(color.R), static_cast<float>(color.G), static_cast<float>(color.B), static_cast<float>(color.A) };
}
