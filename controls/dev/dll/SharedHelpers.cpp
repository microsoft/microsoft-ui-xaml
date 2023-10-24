// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "MUXControlsFactory.h"
#include "SharedHelpers.h"

#include <appmodel.h>
#include <roapi.h>

// Package constants copied from AppModel.h. These values were only defined for Vb and later;
// when passed as flags to ::GetCurrentPackageInfo() on pre-Vb, they are simply ignored.
#define PACKAGE_PROPERTY_STATIC             0x00080000
#define PACKAGE_FILTER_STATIC               PACKAGE_PROPERTY_STATIC
#define PACKAGE_PROPERTY_DYNAMIC            0x00100000
#define PACKAGE_FILTER_DYNAMIC              PACKAGE_PROPERTY_DYNAMIC

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
    static bool s_isInDesignModeV2 = winrt::ApiInformation::IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 5) && winrt::Windows::ApplicationModel::DesignMode::DesignMode2Enabled();
    return s_isInDesignModeV2;
}

bool SharedHelpers::IsCoreWindowActivationModeAvailable()
{
    // In RS3 we got CoreWindow.ActivationMode API which can be queried for window activation state,
    // and particularly in a Component UI host appto check if the compoenent is active (while host isn't).
    static bool s_isAvailable = winrt::ApiInformation::IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 5);
    return s_isAvailable;
}

bool SharedHelpers::IsApplicationViewGetDisplayRegionsAvailable()
{
    static bool s_isApplicationViewGetDisplayRegionsAvailable =
        winrt::ApiInformation::IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 8) ||
        winrt::ApiInformation::IsMethodPresent(L"Windows.UI.ViewManagement.ApplicationView", L"GetDisplayRegions");
    return s_isApplicationViewGetDisplayRegionsAvailable;
}

namespace
{
    constexpr wchar_t c_reunionPackageNamePrefix[] = L"Microsoft.WindowsAppRuntime";
    constexpr int c_reunionPackageNamePrefixLength = ARRAYSIZE(c_reunionPackageNamePrefix) - 1;

    // Tries to retrieve the current package graph. Will return true if a package graph exists
    // and is non-empty, else false.
    bool TryGetCurrentPackageGraph(
        const std::uint32_t flags,
        std::uint32_t& packageCount,
        const PACKAGE_INFO*& packageGraph,
        std::unique_ptr<BYTE[]>& buffer)
    {
        std::uint32_t bufferLength{};
        LONG rc{ ::GetCurrentPackageInfo(flags, &bufferLength, nullptr, nullptr) };
        if ((rc == APPMODEL_ERROR_NO_PACKAGE) || (rc == ERROR_SUCCESS))
        {
            // No/empty package graph
            return false;
        }
        else if (rc != ERROR_INSUFFICIENT_BUFFER)
        {
            if (FAILED(HRESULT_FROM_WIN32(rc)))
            {
                return false;
            }
        }

        buffer = std::make_unique<BYTE[]>(bufferLength);
        if (SUCCEEDED(HRESULT_FROM_WIN32(::GetCurrentPackageInfo(flags, &bufferLength, buffer.get(), &packageCount))))
        {
            packageGraph = reinterpret_cast<PACKAGE_INFO*>(buffer.get());
            return true;
        }

        return false;
    }

    bool IsInFrameworkPackageImpl(winrt::hstring& frameworkInstallLocation)
    {
        // We could be running in an app that is using WinUI "In-App" (i.e. the WinUI dlls are deployed as part of the app). 
        // Or we could be running in the Project Reunion Framework Package that an app takes a dependency on.
        // We determine if we are in a Framework Package by looking at the App's package dependencies. If the app has a Project Reunion dependency
        // then we must be running in a Framework Package.
        const UINT32 c_filter{ PACKAGE_FILTER_HEAD | PACKAGE_FILTER_DIRECT | PACKAGE_FILTER_STATIC | PACKAGE_FILTER_DYNAMIC | PACKAGE_INFORMATION_BASIC };
        std::uint32_t packageCount{};
        const PACKAGE_INFO* packageGraph{};
        std::unique_ptr<BYTE[]> packageBuffer;
        if (TryGetCurrentPackageGraph(c_filter, packageCount, packageGraph, packageBuffer))
        {
            if (packageGraph)
            {
                for (std::uint32_t index=0; index < packageCount; index++)
                {
                    const PACKAGE_INFO& packageInfo{ packageGraph[index] };

                    // We use FamilyName here and not DisplayName because DisplayName is a localized string, but FamilyName is a stable identifier.
                    auto packageFamilyName = packageInfo.packageFamilyName;
                    int nameLength{ static_cast<int>(wcslen(packageFamilyName)) };

                    if (nameLength < c_reunionPackageNamePrefixLength)
                    {
                        continue;
                    }

                    if (
                       CompareStringOrdinal(
                        packageFamilyName,
                        c_reunionPackageNamePrefixLength, 
                        c_reunionPackageNamePrefix, 
                        c_reunionPackageNamePrefixLength, 
                        TRUE) == CSTR_EQUAL)
                    {
                        frameworkInstallLocation = winrt::hstring(packageInfo.path);
                        return true;
                    }
                }
            }
        }
        return false;
    }
}

bool SharedHelpers::IsInFrameworkPackage()
{
    //unused l-value
    static winrt::hstring s;
    return IsInFrameworkPackageImpl(s);
}

bool SharedHelpers::IsInFrameworkPackage(winrt::hstring& frameworkPackageInstallLocation)
{
    static winrt::hstring s_frameworkInstallLocation;
    static bool isInFrameworkPackage = IsInFrameworkPackageImpl(s_frameworkInstallLocation);
    frameworkPackageInstallLocation = s_frameworkInstallLocation;

    return isInFrameworkPackage;
}

// Platform scale helpers
winrt::Rect SharedHelpers::ConvertDipsToPhysical(winrt::UIElement const& xamlRootReference, const winrt::Rect& dipsRect)
{
    try
    {
        const auto scaleFactor = static_cast<float>(xamlRootReference.XamlRoot().RasterizationScale());
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

bool SharedHelpers::IsMouseModeEnabled()
{
    if (!s_isMouseModeEnabledInitialized)
    {
        s_isMouseModeEnabled = winrt::Application::Current().RequiresPointerMode() == winrt::ApplicationRequiresPointerMode::Auto;
        s_isMouseModeEnabledInitialized = true;
    }
    return s_isMouseModeEnabled;
}

void SharedHelpers::ScheduleActionAfterWait(
    std::function<void()> const& action,
    unsigned int millisecondWait)
{
    if(auto dispatcherQueue = winrt::DispatcherQueue::GetForCurrentThread())
    {
        // The callback that is given to CreateTimer is called off of the UI thread.
        // In order to make this useful by making it so we can interact with XAML objects,
        // we'll use the dispatcher to first post our work to the UI thread before executing it.
        auto timer = winrt::ThreadPoolTimer::CreateTimer(winrt::TimerElapsedHandler(
            [action, dispatcherQueue](auto const&)
            {
                dispatcherQueue.TryEnqueue(winrt::DispatcherQueueHandler(action));
            }),
            std::chrono::milliseconds{ millisecondWait });
    }
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
        *renderingEventToken = winrt::Microsoft::UI::Xaml::Media::CompositionTarget::Rendering([renderingEventToken, callback](auto&, auto&) {

            // Detach event or Rendering will keep calling us back.
            winrt::Microsoft::UI::Xaml::Media::CompositionTarget::Rendering(*renderingEventToken);

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

#if defined(TITLEBAR_INCLUDED) || defined(SWIPECONTROL_INCLUDED) || defined(TEACHINGTIP_INCLUDED) || defined(TABVIEW_INCLUDED)

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
#if defined(IMAGEICON_INCLUDED)
    // Note: this check must be done before BitmapIconSource
    // since ImageIconSource uses BitmapIconSource as a composable interface,
    // so a ImageIconSource will also register as a BitmapIconSource.
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
    else if (auto bitmapIconSource = iconSource.try_as<winrt::BitmapIconSource>())
    {
        winrt::BitmapIcon bitmapIcon;

        if (bitmapIconSource.UriSource())
        {
            bitmapIcon.UriSource(bitmapIconSource.UriSource());
        }

        bitmapIcon.ShowAsMonochrome(bitmapIconSource.ShowAsMonochrome());

        if (const auto newForeground = bitmapIconSource.Foreground())
        {
            bitmapIcon.Foreground(newForeground);
        }
        
        return bitmapIcon;
    }
#if defined(ANIMATEDICON_INCLUDED)
    // Note: this check must be done before PathIconSource
    // since AnimatedIconSource uses PathIconSource as a composable interface,
    // so a AnimatedIconSource will also register as a PathIconSource.
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


// To avoid having to call a potentially throwing cppwinrt api, we call directly through the abi.
// We don't want to include the entirety of the abi headers, so we just reproduce the single interface
// that we need.
namespace ABI::Windows::ApplicationModel::Core
{
    MIDL_INTERFACE("0AACF7A4-5E1D-49DF-8034-FB6A68BC5ED1")
        ICoreApplication : public IInspectable
    {
        virtual HRESULT STDMETHODCALLTYPE get_Id(HSTRING * value) = 0;
        virtual HRESULT STDMETHODCALLTYPE add_Suspending(void* handler, EventRegistrationToken* token) = 0;
        virtual HRESULT STDMETHODCALLTYPE remove_Suspending(EventRegistrationToken token) = 0;
        virtual HRESULT STDMETHODCALLTYPE add_Resuming(void* handler, EventRegistrationToken* token) = 0;
        virtual HRESULT STDMETHODCALLTYPE remove_Resuming(EventRegistrationToken token) = 0;
        virtual HRESULT STDMETHODCALLTYPE get_Properties(void** value) = 0;
        virtual HRESULT STDMETHODCALLTYPE GetCurrentView(void** value) = 0;
        virtual HRESULT STDMETHODCALLTYPE Run(void* viewSource) = 0;
        virtual HRESULT STDMETHODCALLTYPE RunWithActivationFactories(void* activationFactoryCallback) = 0;
    };
}


winrt::CoreApplicationView SharedHelpers::TryGetCurrentCoreApplicationView()
{
    winrt::CoreApplicationView view{ nullptr };

    // We could call winrt::CoreApplication::GetCurrentView() here, but that can throw in some cases. Even if we catch, it will still
    // generate exception noise in the debugger.
    // Check if we have a CoreWindow to avoid throwing and catching an exception which can be annoying during debugging.
    if (winrt::CoreWindow::GetForCurrentThread())
    {
        auto coreApplication = winrt::get_activation_factory<winrt::CoreApplication, ABI::Windows::ApplicationModel::Core::ICoreApplication>();
        auto ignorehr = coreApplication->GetCurrentView(winrt::put_abi(view));
    }

    return view;
}
