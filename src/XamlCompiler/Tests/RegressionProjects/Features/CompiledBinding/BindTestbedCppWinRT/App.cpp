// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// App.xaml.cpp
// Implementation of the App class.
//

#include "pch.h"
#include "windows.h"
#include "winrt/Windows.ApplicationModel.Activation.h"
#include "winrt/Microsoft.UI.Xaml.Navigation.h"
#include "winrt/Microsoft.UI.Xaml.Media.h"
#include "winrt/Microsoft.UI.Dispatching.h"

#include "App.h"
#include "MainPage.h"
#include "ListAndTemplateTests.h"
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <initializer_list>
#include <chrono>

namespace winrt::BindTestbed::implementation
{
    namespace
    {
        wuxc::TextBlock FindTextBlock(wux::DependencyObject const& root)
        {
            if (auto text = root.try_as<wuxc::TextBlock>())
            {
                return text;
            }
            for (int index = 0; index < wux::Media::VisualTreeHelper::GetChildrenCount(root); index++)
            {
                if (auto text = FindTextBlock(wux::Media::VisualTreeHelper::GetChild(root, index)))
                {
                    return text;
                }
            }
            return nullptr;
        }

        void VerifyPhasedTemplateBindings(wux::ResourceDictionary const& resources)
        {
            auto dataTemplate = resources.Lookup(box_value(L"StaticPhasedBindingTemplate")).as<wux::DataTemplate>();
            auto root = dataTemplate.LoadContent().as<wuxc::StackPanel>();
            auto itemTarget = root.Children().GetAt(0).as<wuxc::TextBlock>();
            auto staticTarget = root.Children().GetAt(1).as<wuxc::TextBlock>();
            auto functionTarget = root.Children().GetAt(2).as<wuxc::TextBlock>();
            auto component = wux::Markup::XamlBindingHelper::GetDataTemplateComponent(root);
            auto settings = BindTestbedModel::FakeSettings::Instance();
            settings.BoolSetting(false);
            int nextPhase = -1;

            // The component, rather than DataContext inheritance, owns the item and phase schedule.
            component.ProcessBindings(settings, 0, 0, nextPhase);
            if (nextPhase != 1 || itemTarget.Tag() || unbox_value<bool>(staticTarget.Tag()))
            {
                throw hresult_error(E_FAIL, L"Phase zero did not defer the item binding to phase one.");
            }
            auto unrelated = BindTestbedModel::FakeSettings();
            unrelated.BoolSetting(true);
            root.DataContext(unrelated);
            component.ProcessBindings(settings, 0, 1, nextPhase);
            if (nextPhase != -1 || unbox_value<bool>(itemTarget.Tag()) ||
                functionTarget.Text() != BindTestbedModel::DataModel::StaticFunctionOnModelNoArgs())
            {
                throw hresult_error(E_FAIL, L"Phase one did not complete the component's item/static function bindings.");
            }

            // Recycling must detach both aliased listener paths, then rebuild independent and new
            // item paths when the same template instance is assigned a different item.
            component.Recycle();
            settings.BoolSetting(true);
            if (unbox_value<bool>(itemTarget.Tag()) || unbox_value<bool>(staticTarget.Tag()))
            {
                throw hresult_error(E_FAIL, L"A recycled component still received source notifications.");
            }
            auto replacement = BindTestbedModel::FakeSettings();
            component.ProcessBindings(replacement, 1, 0, nextPhase);
            component.ProcessBindings(replacement, 1, 1, nextPhase);
            // Check restoration before the singleton returns to the target's old value.
            if (unbox_value<bool>(itemTarget.Tag()) || !unbox_value<bool>(staticTarget.Tag()))
            {
                throw hresult_error(E_FAIL, L"Reusing the component did not restore its static source's current value.");
            }
            replacement.BoolSetting(true);
            settings.BoolSetting(false);
            if (!unbox_value<bool>(itemTarget.Tag()) || unbox_value<bool>(staticTarget.Tag()))
            {
                throw hresult_error(E_FAIL, L"A recycled component did not reconnect independent and replacement-item listeners.");
            }

            component.Recycle();
        }

        void VerifyStaticTemplateBindings(wux::ResourceDictionary const& resources)
        {
            std::puts("Testing observable source notifications.");
            std::fflush(stdout);
            // Use the same observable object as both the item and a static source so notifications
            // after item removal can expose stale item paths without disabling the static paths.
            auto dataTemplate = resources.Lookup(box_value(L"StaticObservableBindingTemplate")).as<wux::DataTemplate>();
            auto root = dataTemplate.LoadContent().as<wuxc::StackPanel>();
            auto itemTarget = root.Children().GetAt(0).as<wuxc::TextBlock>();
            auto staticTarget = root.Children().GetAt(1).as<wuxc::TextBlock>();
            auto settings = BindTestbedModel::FakeSettings::Instance();
            settings.BoolSetting(false);
            root.DataContext(settings);
            settings.BoolSetting(true);
            std::puts("Observable source notifications initialized.");
            std::fflush(stdout);
            if (!unbox_value<bool>(itemTarget.Tag()) || !unbox_value<bool>(staticTarget.Tag()))
            {
                throw hresult_error(E_FAIL, L"The item and static bindings did not receive the initial notification.");
            }

            root.DataContext(nullptr);
            settings.BoolSetting(false);
            std::puts("Observable source null transition completed.");
            std::fflush(stdout);
            if (!unbox_value<bool>(itemTarget.Tag()) || unbox_value<bool>(staticTarget.Tag()))
            {
                throw hresult_error(E_FAIL, L"A static-source notification updated the removed item binding.");
            }

            // Force a target callback while the item is absent, then verify the static reverse path.
            itemTarget.Tag(box_value(false));
            itemTarget.Tag(box_value(true));
            if (settings.BoolSetting())
            {
                throw hresult_error(E_FAIL, L"A TwoWay target wrote to the removed item.");
            }

            staticTarget.Tag(box_value(true));
            if (!settings.BoolSetting())
            {
                throw hresult_error(E_FAIL, L"The independent static TwoWay binding stopped updating its source.");
            }

            // Supplying the item again must rebuild the listeners that removal detached.
            root.DataContext(settings);
            settings.BoolSetting(false);
            if (unbox_value<bool>(itemTarget.Tag()) || unbox_value<bool>(staticTarget.Tag()))
            {
                throw hresult_error(E_FAIL, L"The bindings did not reconnect when the item was restored.");
            }

            // Named sources belong to the template namescope, not its item; their listeners must
            // survive replacement/removal with and without a static binding in the same template.
            for (auto templateKey : { L"StaticNamedSourceTemplate", L"NamedSourceTemplate" })
            {
                std::printf("Testing named source template: %ls\n", templateKey);
                auto namedTemplate = resources.Lookup(box_value(hstring(templateKey))).as<wux::DataTemplate>();
                auto namedRoot = namedTemplate.LoadContent().as<wuxc::StackPanel>();
                auto deferredSource = namedRoot.FindName(L"deferredSource").as<wuxc::TextBlock>();
                auto namedTarget = namedRoot.FindName(L"namedTarget").as<wuxc::TextBlock>();
                namedRoot.DataContext(settings);
                deferredSource.Text(L"connected");
                if (namedTarget.Text() != L"connected")
                {
                    throw hresult_error(E_FAIL, L"The deferred named source did not connect.");
                }
                namedRoot.DataContext(BindTestbedModel::FakeSettings());
                deferredSource.Text(L"after replacement");
                if (namedTarget.Text() != L"after replacement")
                {
                    throw hresult_error(E_FAIL, L"The independent named-source listener was not restored after item replacement.");
                }
                namedRoot.DataContext(nullptr);
                deferredSource.Text(L"after null");
                if (namedTarget.Text() != L"after null")
                {
                    throw hresult_error(E_FAIL, L"The independent named-source listener was not restored after item removal.");
                }
            }

        }

        void VerifyUnaffectedTemplateBindings(wuxc::StackPanel const& host, wux::ResourceDictionary const& resources)
        {
            // A plain item template retains its historical source/listeners on a null context;
            // changing that contract is not part of enabling null-item static bindings.
            auto itemTemplate = resources.Lookup(box_value(L"InstanceOnlyBindingTemplate")).as<wux::DataTemplate>();
            auto itemTarget = itemTemplate.LoadContent().as<wuxc::TextBlock>();
            auto item = BindTestbedModel::FakeSettings();
            itemTarget.DataContext(item);
            item.BoolSetting(true);
            if (!unbox_value<bool>(itemTarget.Tag()))
            {
                throw hresult_error(E_FAIL, L"The ordinary item binding did not initialize.");
            }
            itemTarget.DataContext(nullptr);
            item.BoolSetting(false);
            if (unbox_value<bool>(itemTarget.Tag()))
            {
                throw hresult_error(E_FAIL, L"A non-static template changed its legacy null-context notification behavior.");
            }
            itemTarget.Tag(box_value(true));
            if (!item.BoolSetting())
            {
                throw hresult_error(E_FAIL, L"A non-static template changed its legacy reverse-assignment behavior.");
            }

            // Entering a live tree with no item must not newly initialize a named-only template.
            auto namedTemplate = resources.Lookup(box_value(L"NamedSourceTemplate")).as<wux::DataTemplate>();
            auto namedRoot = namedTemplate.LoadContent().as<wuxc::StackPanel>();
            auto source = namedRoot.FindName(L"deferredSource").as<wuxc::TextBlock>();
            auto target = namedRoot.FindName(L"namedTarget").as<wuxc::TextBlock>();
            host.Children().Append(namedRoot);
            host.UpdateLayout();
            source.Text(L"before an item");
            if (!target.Text().empty())
            {
                throw hresult_error(E_FAIL, L"A named-only template gained initial null-item activation.");
            }
            namedRoot.DataContext(item);
            source.Text(L"after an item");
            if (target.Text() != L"after an item")
            {
                throw hresult_error(E_FAIL, L"The named-only template did not initialize after receiving an item.");
            }
        }

        wf::IAsyncAction WaitForTemplateUpdate(wuxc::StackPanel host, wux::Window window)
        {
            // Observe after live-tree template connections and their dispatcher work complete.
            host.UpdateLayout();
            winrt::apartment_context ui;
            winrt::handle ready{ CreateEventW(nullptr, true, false, nullptr) };
            winrt::check_bool(static_cast<bool>(ready));
            winrt::check_bool(window.DispatcherQueue().TryEnqueue(
                winrt::Microsoft::UI::Dispatching::DispatcherQueuePriority::Low,
                [event = ready.get()] { SetEvent(event); }));
            bool didUpdate = co_await winrt::resume_on_signal(ready.get(), std::chrono::seconds(10));
            co_await ui;
            if (!didUpdate)
            {
                throw hresult_error(E_FAIL, L"The template update observation timed out.");
            }
        }

        wf::IAsyncAction VerifyUntypedTemplateContexts(wuxc::StackPanel host, wux::Window window, wux::ResourceDictionary resources)
        {
            auto dataTemplate = resources.Lookup(box_value(L"StaticUntypedBindingTemplate")).as<wux::DataTemplate>();
            auto expected = BindTestbedModel::DataModel::StaticFunctionOnModelNoArgs();
            wuxc::ContentControl contentControl;
            contentControl.ContentTemplate(dataTemplate);
            contentControl.Content(box_value(L"ordinary item"));
            wuxc::ContentControl inheritedControl;
            inheritedControl.ContentTemplate(dataTemplate);
            wuxc::StackPanel inheritedHost;
            inheritedHost.DataContext(BindTestbedModel::FakeSettings::Instance());
            inheritedHost.Children().Append(inheritedControl);
            host.Children().Append(contentControl);
            host.Children().Append(inheritedHost);
            // Exercise the real presenter's item and inheritance paths: a static binding does
            // not justify skipping an item conversion unless every path is item-independent.
            co_await WaitForTemplateUpdate(host, window);
            auto contentText = FindTextBlock(contentControl);
            if (!contentText || contentText.Text() != expected)
            {
                throw hresult_error(E_FAIL, L"An untyped static template failed with non-null Content.");
            }
            auto inheritedText = FindTextBlock(inheritedControl);
            if (inheritedControl.Content() || !inheritedControl.DataContext() ||
                !inheritedText || inheritedText.Text() != expected)
            {
                throw hresult_error(E_FAIL, L"An untyped static template failed with inherited non-null context.");
            }
            auto transitionControl = host.Children().GetAt(0).as<wuxc::ContentControl>();
            for (auto const& item : { box_value(L"replacement item"), IInspectable{ nullptr } })
            {
                transitionControl.Content(item);
                co_await WaitForTemplateUpdate(host, window);
                auto text = FindTextBlock(transitionControl);
                if (!text || text.Text() != expected)
                {
                    throw hresult_error(E_FAIL, L"An untyped static template failed across null/item/null Content.");
                }
            }
            for (auto const& item : { IInspectable{ nullptr }, BindTestbedModel::FakeSettings().as<IInspectable>(), IInspectable{ nullptr } })
            {
                // Inheritance reuses this visual; changing its text prevents a skipped update
                // from passing merely because the previous function result is still displayed.
                inheritedText.Text(L"not updated");
                inheritedHost.DataContext(item);
                co_await WaitForTemplateUpdate(host, window);
                if (inheritedText.Text() != expected)
                {
                    throw hresult_error(E_FAIL, L"An untyped static template skipped a context transition.");
                }
            }
        }

        void VerifyUntypedTemplateComponents(wux::ResourceDictionary const& resources)
        {
            auto dataTemplate = resources.Lookup(box_value(L"StaticUntypedPhasedBindingTemplate")).as<wux::DataTemplate>();
            for (auto const& item : { IInspectable{ nullptr }, box_value(L"ordinary item"), BindTestbedModel::FakeSettings().as<IInspectable>() })
            {
                auto root = dataTemplate.LoadContent().as<wuxc::TextBlock>();
                auto component = wux::Markup::XamlBindingHelper::GetDataTemplateComponent(root);
                int nextPhase = -1;
                // Call the component directly so a DataContext update cannot hide its cast;
                // phase zero must still defer the static binding until phase one.
                component.ProcessBindings(item, 0, 0, nextPhase);
                if (nextPhase != 1 || !root.Text().empty())
                {
                    throw hresult_error(E_FAIL, L"The untyped component did not defer phase one.");
                }
                component.ProcessBindings(item, 0, 1, nextPhase);
                if (nextPhase != -1 || root.Text() != BindTestbedModel::DataModel::StaticFunctionOnModelNoArgs())
                {
                    throw hresult_error(E_FAIL, L"The untyped component failed to update its independent binding.");
                }
                component.Recycle();
            }
        }

        winrt::fire_and_forget RunStaticTemplateBindingTests(wux::Window testWindow)
        {
            try
            {
                auto owner = winrt::make<ListAndTemplateTests>();
                auto resources = owner.Resources();
                wuxc::StackPanel host;
                for (auto key : { L"StaticUntypedBindingTemplate", L"StaticBindingTemplate" })
                {
                    wuxc::ContentControl control;
                    control.ContentTemplate(resources.Lookup(box_value(hstring(key))).as<wux::DataTemplate>());
                    host.Children().Append(control);
                }

                // Enter a real live tree with no item; synthesizing a data-context notification would
                // miss the original ContentControl activation failure, especially without x:DataType.
                winrt::apartment_context uiThread;
                winrt::handle loaded{ CreateEventW(nullptr, true, false, nullptr) };
                winrt::check_bool(static_cast<bool>(loaded));
                auto token = host.Loaded([event = loaded.get()](IInspectable const&, wux::RoutedEventArgs const&)
                {
                    SetEvent(event);
                });
                testWindow.Content(host);
                testWindow.Activate();
                bool didLoad = co_await winrt::resume_on_signal(loaded.get(), std::chrono::seconds(30));
                co_await uiThread;
                host.Loaded(token);
                if (!didLoad)
                {
                    throw hresult_error(E_FAIL, L"The null-content template test window did not load.");
                }
                host.UpdateLayout();
                for (auto const& child : host.Children())
                {
                    auto control = child.as<wuxc::ContentControl>();
                    auto text = FindTextBlock(control);
                    if (control.Content() || control.DataContext() || !text ||
                        text.Text() != BindTestbedModel::DataModel::StaticFunctionOnModelNoArgs())
                    {
                        throw hresult_error(E_FAIL,
                            L"An initially null-content static template did not display its function result.");
                    }
                }
                std::puts("Initial null-content templates passed.");
                std::fflush(stdout);

                co_await VerifyUntypedTemplateContexts(host, testWindow, resources);
                std::puts("Untyped template contexts and transitions passed.");
                VerifyUntypedTemplateComponents(resources);
                std::puts("Untyped template component entry points passed.");
                VerifyUnaffectedTemplateBindings(host, resources);
                std::puts("Unaffected template behavior passed.");
                VerifyStaticTemplateBindings(resources);
                VerifyPhasedTemplateBindings(resources);
                std::puts("ProcessBindings and recycling passed.");
                std::puts("Static template binding tests passed.");
                std::exit(0);
            }
            catch (hresult_error const& error)
            {
                std::fwprintf(stderr, L"Static template binding test failed: %ls (0x%08X)\n",
                    error.message().c_str(), static_cast<uint32_t>(error.code()));
                std::exit(1);
            }
        }
    }

    // The Blank Application template is documented at http://go.microsoft.com/fwlink/?LinkId=402347&clcid=0x409

    BindTestbedModel::DataModel App::Model = nullptr;
    BindTestbedModel::DOModel App::DOModel = nullptr;
    wux::Window window = nullptr;

    /// <summary>
    /// Initializes the singleton application object.  This is the first line of authored code
    /// executed, and as such is the logical equivalent of main() or WinMain().
    /// </summary>
    App::App() try
    {
        if (std::wcsstr(GetCommandLineW(), L"--test-static-template-bindings") != nullptr)
        {
            UnhandledException([](IInspectable const&, wux::UnhandledExceptionEventArgs const& error)
            {
                std::fwprintf(stderr, L"Unhandled static template binding test failure: %ls\n", error.Message().c_str());
                std::fflush(stderr);
                std::exit(1);
            });
            std::puts("Initializing static template binding test application.");
            std::fflush(stdout);
        }
        InitializeComponent();
        if (std::wcsstr(GetCommandLineW(), L"--test-static-template-bindings") != nullptr)
        {
            std::puts("Static template binding application XAML initialized.");
            std::fflush(stdout);
            // These regressions use resource-local/static sources, not the unrelated demo-page
            // DependencyObject models normally constructed below.
            return;
        }

#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
        UnhandledException([this](IInspectable const&, wux::UnhandledExceptionEventArgs const& e)
            {
                if (IsDebuggerPresent())
                {
                    auto errorMessage = e.Message();
                    __debugbreak();
                }
            });
#endif

        Model = BindTestbedModel::DataModel();
        DOModel = BindTestbedModel::DOModel();
    }
    catch (hresult_error const& error)
    {
        if (std::wcsstr(GetCommandLineW(), L"--test-static-template-bindings") != nullptr)
        {
            std::fwprintf(stderr, L"Static template binding test initialization failed: %ls (0x%08X)\n",
                error.message().c_str(), static_cast<uint32_t>(error.code()));
            std::fflush(stderr);
            std::exit(1);
        }
        throw;
    }

    /// <summary>
    /// Invoked when the application is launched normally by the end user.	Other entry points
    /// will be used such as when the application is launched to open a specific file.
    /// </summary>
    /// <param name="e">Details about the launch request and process.</param>
    void App::OnLaunched(wux::LaunchActivatedEventArgs const& e)
    {
        if (std::wcsstr(GetCommandLineW(), L"--test-static-template-bindings") != nullptr)
        {
            window = wux::Window();
            RunStaticTemplateBindingTests(window);
            return;
        }

        wuxc::Frame rootFrame(nullptr);

        // Desktop (Win32) app model: the window is created by the app, not supplied by
        // the platform as Window::Current() did in the app-container model.
        window = wux::Window::Window();
        auto content = window.Content();
        rootFrame = content.try_as<wuxc::Frame>();
        // Do not repeat app initialization when the Window already has content,
        // just ensure that the window is active
        if (rootFrame == nullptr)
        {
            // Create a Frame to act as the navigation context and associate it with
            // a SuspensionManager key
            rootFrame = wuxc::Frame();
            auto launchedActivatedEventArgs = e.UWPLaunchActivatedEventArgs();

            rootFrame.NavigationFailed({ this, &App::OnNavigationFailed });

            if (launchedActivatedEventArgs.PreviousExecutionState() == wa::Activation::ApplicationExecutionState::Terminated)
            {
                // Restore the saved session state only when appropriate, scheduling the
                // final launch steps after the restore is complete

            }
            window.Content(rootFrame);
        }

        if (rootFrame.Content() == nullptr)
        {
            // When the navigation stack isn't restored navigate to the first page,
            // configuring the new page by passing required information as a navigation
            // parameter
            rootFrame.Navigate(xaml_typename<BindTestbed::MainPage>(), box_value(e.Arguments()));
        }
        // Ensure the current window is active
        window.Activate();

    }

    /// <summary>
    /// Invoked when Navigation to a certain page fails
    /// </summary>
    /// <param name="sender">The Frame which failed navigation</param>
    /// <param name="e">Details about the navigation failure</param>
    void App::OnNavigationFailed(IInspectable const&, wux::Navigation::NavigationFailedEventArgs const& e)
    {
        std::wstring message(L"Failed to load Page ");
        throw hresult_error(E_FAIL, message.append(e.SourcePageType().Name));
    }
}