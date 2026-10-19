// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"
#include "winrt/Microsoft.UI.Dispatching.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "winrt/Microsoft.UI.Xaml.Media.h"
#include "winrt/Windows.Foundation.Collections.h"
#include "winrt/Windows.UI.h"
#include <chrono>
#include <cstdio>
#include <type_traits>

namespace winrt::Simple::implementation
{
    using namespace winrt::Windows::Foundation;
    using namespace winrt::Microsoft::UI::Xaml;
    using namespace winrt::Microsoft::UI::Xaml::Controls;

    // Catch named-field projections that emit System.String instead of the native hstring type.
    static_assert(std::is_same_v<decltype(std::declval<MainPage>().NamedResourceString()), hstring>);
    static_assert(std::is_same_v<decltype(std::declval<MainPage>().NamedEmptyString()), hstring>);
    static_assert(std::is_same_v<decltype(std::declval<MainPage>().NamedInlineString()), hstring>);

    MainPage::MainPage()
        : MainPageT<MainPage>(hstring(L"This is MainPage"))
    {
        // Connect must actively clear default-constructed strings, not leave stale field values.
        NamedEmptyString(L"Must be cleared by Connect");
        NamedEmptyInlineString(L"Must be cleared by Connect");
        InitializeComponent();
    }

    void MainPage::OnNavigatedTo(Navigation::NavigationEventArgs const& e)
    {
        MainPageBase::OnNavigatedTo(e);
        // Schedule validation after navigation has made the generated fields available.
        RunTestScenario(unbox_value_or<hstring>(e.Parameter(), L""));
    }

    winrt::fire_and_forget MainPage::RunTestScenario(hstring navigationParameter)
    {
        auto lifetime = get_strong();
        auto dispatcher = DispatcherQueue();
        HardAssert(dispatcher != nullptr, L"The page must have a dispatcher queue.");
        co_await winrt::resume_after(std::chrono::seconds(5));
        HardAssert(dispatcher.TryEnqueue([lifetime, navigationParameter]()
        {
            HardAssert(lifetime->MainGrid() != nullptr, L"The generated grid field must be connected.");
            auto color = lifetime->MainGrid().Background().as<Media::SolidColorBrush>().Color();
            if (navigationParameter.empty())
            {
                HardAssert(color == winrt::Windows::UI::Color{ 255, 255, 255, 255 },
                    L"Grid color should be white normally.");
                std::puts("Default background test passed.");
                std::fflush(stdout);
            }
            else if (navigationParameter == L"IncrementalTest")
            {
                HardAssert(color == winrt::Windows::UI::Color{ 255, 0, 0, 0 },
                    L"Grid color should be black after the incremental change.");
                std::puts("Incremental background test passed.");
                std::fflush(stdout);
            }

            // Check compiled fields and namescope lookup before the test process exits.
            lifetime->ValidateNamedStrings();
            ValidateStringNameLookup();
            std::puts("Named string tests passed.");
            std::fflush(stdout);
            Application::Current().Exit();
        }), L"Unable to dispatch runtime validation to the UI thread.");
    }

    void MainPage::ValidateNamedStrings()
    {
        // Compare generated fields, resource values, and namescope lookups for the same strings.
        auto resource = NamedResourceString();
        auto empty = NamedEmptyString();
        auto inlineValue = NamedInlineString();
        // Unnamed strings provide the baseline for default-construction behavior.
        auto unnamed = Markup::XamlReader::Load(LR"(
<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid.Resources><x:String x:Key='emptyString'/></Grid.Resources>
    <TextBlock><TextBlock.Text><x:String/></TextBlock.Text></TextBlock>
</Grid>)").as<Grid>();
        HardAssert(resource == L"Resource value", L"The named resource field must contain its initialized value.");
        // A null ABI string projects as an empty hstring in C++/WinRT.
        HardAssert(empty.empty(), L"The named empty resource must retain its default construction.");
        HardAssert(inlineValue == L"Inline value", L"The named inline field must contain its initialized value.");
        HardAssert(NamedEmptyInlineString().empty(), L"The named inline string must retain its default construction.");
        HardAssert(Resources().Lookup(box_value(hstring(L"NamedResourceString"))).as<hstring>() == resource,
            L"The implicit resource key must preserve the named value.");
        // Default resources can be absent or null; naming must preserve either representation.
        auto key = box_value(hstring(L"emptyString"));
        bool namedHasValue = Resources().HasKey(key);
        bool unnamedHasValue = unnamed.Resources().HasKey(key);
        HardAssert(namedHasValue == unnamedHasValue, L"Naming must not change default resource lookup behavior.");
        if (namedHasValue)
        {
            auto namedValue = Resources().Lookup(key);
            auto unnamedValue = unnamed.Resources().Lookup(key);
            HardAssert((namedValue == nullptr) == (unnamedValue == nullptr),
                L"Naming must preserve null resource values.");
            if (namedValue)
            {
                HardAssert(namedValue.as<hstring>() == unnamedValue.as<hstring>(),
                    L"Naming must preserve resource string values.");
            }
        }
        HardAssert(NamedStringTextBlock().Text() == inlineValue,
            L"TextBlock.Text and its named string field must contain the same value.");
        HardAssert(EmptyStringTextBlock().Text() == unnamed.Children().GetAt(0).as<TextBlock>().Text(),
            L"Naming must not change a default string assigned to TextBlock.Text.");
        HardAssert(FindName(L"NamedResourceString").as<hstring>() == resource,
            L"The resource name must resolve to the initialized value.");
        auto namedEmpty = FindName(L"NamedEmptyString");
        auto unnamedEmpty = Markup::XamlReader::Load(
            LR"(<x:String xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'/>)");
        HardAssert((namedEmpty == nullptr) == (unnamedEmpty == nullptr) &&
            namedEmpty.as<hstring>() == unnamedEmpty.as<hstring>(),
            L"Named and unnamed default strings must have equivalent boxed values.");
        HardAssert(FindName(L"NamedInlineString").as<hstring>() == inlineValue,
            L"The inline name must resolve to the initialized value.");
        auto emptyInline = FindName(L"NamedEmptyInlineString");
        HardAssert(emptyInline != nullptr && emptyInline.as<hstring>().empty(),
            L"The empty inline name must resolve to an empty string.");
        HardAssert(FindName(L"NamedStringTextBlock").as<TextBlock>() == NamedStringTextBlock(),
            L"Control lookup must preserve object identity.");
        HardAssert(FindName(L"propertyBagObject").as<winrt::Simple::PropBag>() == propertyBagObject(),
            L"Custom object lookup must preserve object identity.");
        HardAssert(FindName(L"MissingName") == nullptr, L"An absent name must still return null.");
    }

    void MainPage::ValidateStringNameLookup()
    {
        // A classless compiled fixture exercises FindName without generated fields or callbacks.
        Grid compiledRoot;
        Application::LoadComponent(compiledRoot, Uri(L"ms-appx:///NamedStrings.xaml"),
            Controls::Primitives::ComponentResourceLocation::Application);
        std::puts("Checking named string lookups.");
        std::fflush(stdout);
        HardAssert(compiledRoot.FindName(L"text").as<hstring>() == L"Compiled string",
            L"FindName must resolve a compiled resource string.");
        auto compiledEmpty = compiledRoot.FindName(L"empty");
        HardAssert(compiledEmpty != nullptr && compiledEmpty.as<hstring>().empty(),
            L"FindName must resolve a compiled empty string.");
        HardAssert(compiledRoot.FindName(L"button").as<Button>() == compiledRoot.Children().GetAt(0).as<Button>(),
            L"Compiled control lookup must preserve object identity.");

        // Loose markup also checks that saved names preserve language handling and custom peers.
        auto root = Markup::XamlReader::Load(LR"(
<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
      xmlns:local='using:Simple'>
    <Grid.Resources>
        <x:String x:Name='text'>Loose string</x:String>
        <x:String x:Name='empty'></x:String>
        <local:PropBag x:Name='custom' StringProp='Custom value' />
    </Grid.Resources>
    <Button x:Name='button' xml:lang='fr-FR' />
    <Button xml:lang='fr-FR' />
</Grid>)").as<Grid>();
        HardAssert(root.FindName(L"text").as<hstring>() == L"Loose string", L"FindName must return a string.");
        auto emptyValue = root.FindName(L"empty");
        HardAssert(emptyValue != nullptr && emptyValue.as<hstring>().empty(), L"FindName must return an empty string, not null.");
        auto button = root.FindName(L"button").as<Button>();
        HardAssert(button == root.Children().GetAt(0).as<Button>(), L"Loose-markup control lookup must preserve object identity.");
        HardAssert(button.Language() == root.Children().GetAt(1).as<Button>().Language(),
            L"A saved name must not change language handling.");
        auto custom = root.FindName(L"custom").as<winrt::Simple::PropBag>();
        HardAssert(custom.StringProp() == L"Custom value" &&
            custom == root.Resources().Lookup(box_value(hstring(L"custom"))).as<winrt::Simple::PropBag>(),
            L"Saved names must preserve custom C++/WinRT object identity.");
        HardAssert(root.FindName(L"MissingName") == nullptr, L"An absent loose-markup name must return null.");

        // Reusing names in another root must not cross namescope boundaries.
        auto otherRoot = Markup::XamlReader::Load(LR"(
<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid.Resources><x:String x:Name='text'>Other string</x:String></Grid.Resources>
    <Button x:Name='button' />
</Grid>)").as<Grid>();
        HardAssert(otherRoot.FindName(L"text").as<hstring>() == L"Other string",
            L"String names must resolve within their own namescope.");
        HardAssert(root.FindName(L"text").as<hstring>() == L"Loose string",
            L"A second namescope must not replace the first scope's value.");
        HardAssert(otherRoot.FindName(L"button").as<Button>() == otherRoot.Children().GetAt(0).as<Button>(),
            L"Control names must remain isolated between roots.");
    }

    void MainPage::HardAssert(bool condition, wchar_t const* message)
    {
        if (!condition)
        {
            std::fwprintf(stderr, L"%ls\n", message);
            throw hresult_error(E_FAIL, message);
        }
    }

    void MainPage::ClickHandler(IInspectable const&, ::winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        Button1().Content(::winrt::box_value(::winrt::hstring(L"Bad things will happen now")));
    }

    void MainPage::TappedHandler(IInspectable const&, ::winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const&)
    {}
}
