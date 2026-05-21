// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ContentPresenterIntegrationTests.h"
#include "DataContextEventRecorder.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>

#include <ControlHelper.h>
#include <TreeHelper.h>
#include <WUCRenderingScopeGuard.h>

using namespace Concurrency;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace ::Windows::Storage::Streams;
using namespace ::Windows::UI;


namespace Local {

    [wf::Metadata::WebHostHidden]
    public ref class TestDataTemplateSelector : public xaml_controls::DataTemplateSelector
    {
    public:
        void AddEntry(Platform::Type^ type, xaml::DataTemplate^ dataTemplate)
        {
            m_dataTemplateMap[type->FullName] = dataTemplate;
        }

        xaml::DataTemplate^ SelectTemplateCore(Platform::Object^ item) override
        {
            return item ? m_dataTemplateMap[item->GetType()->FullName] : nullptr;
        }

        xaml::DataTemplate^ SelectTemplateCore(Platform::Object^ item, xaml::DependencyObject^ container) override
        {
            return SelectTemplateCore(item);
        }

    private:
        std::map<Platform::String^, xaml::DataTemplate^> m_dataTemplateMap;

    }; // public ref class TestDataTemplateSelector;

} // namespace Local

namespace
{
    void PrintRecordedChanges(
        Microsoft::UI::Xaml::Tests::Controls::ContentPresenter::DataContextEventRecorder^ recorder,
        xaml_controls::ContentPresenter^ outerCP,
        xaml_controls::ContentPresenter^ innerCP)
    {
        for (const auto& change : recorder->GetRecordedChanges())
        {
            if (change.Presenter == outerCP) { LOG_OUTPUT(L"OuterCP"); }
            else                             { LOG_OUTPUT(L"InnerCP"); }
            LOG_OUTPUT(L"Change: %s", change.Type.ToString()->Data());
            LOG_OUTPUT(L"Old value: %s", change.OldValue->Data());
            LOG_OUTPUT(L"New value: %s\n", change.NewValue->Data());
        }
    }
}

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ContentPresenter {

    bool ContentPresenterIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ContentPresenterIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ContentPresenterIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void ContentPresenterIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::ContentPresenter>::CanInstantiate();
    }

    void ContentPresenterIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::ContentPresenter>::CanEnterAndLeaveLiveTree();
    }

    void ContentPresenterIntegrationTests::ValidateDefaultPropertyValues()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([]()
        {
            auto contentPresenter = ref new xaml_controls::ContentPresenter();

            VERIFY_IS_NULL(contentPresenter->Content);
            VERIFY_IS_NULL(contentPresenter->ContentTemplate);
            VERIFY_IS_NULL(contentPresenter->ContentTemplateSelector);

            VERIFY_ARE_EQUAL(contentPresenter->FontSize, 14.0);
            VERIFY_ARE_EQUAL(contentPresenter->FontFamily->Source, ref new Platform::String(L"Segoe UI"));
            VERIFY_ARE_EQUAL(contentPresenter->FontWeight.Weight, 400);
            VERIFY_ARE_EQUAL(contentPresenter->FontStyle, wut::FontStyle::Normal);
            VERIFY_ARE_EQUAL(contentPresenter->FontStretch, wut::FontStretch::Normal);
            VERIFY_ARE_EQUAL(contentPresenter->CharacterSpacing, 0);
            VERIFY_ARE_EQUAL(contentPresenter->Foreground, safe_cast<xaml_media::Brush^>(xaml::Application::Current->Resources->Lookup(L"DefaultTextForegroundThemeBrush")));
        });
    }

    void ContentPresenterIntegrationTests::CanSetAndGetContentProperty()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([]()
        {
            auto contentPresenter = ref new xaml_controls::ContentPresenter();
            auto rectangle = ref new xaml_shapes::Rectangle();
            Platform::String^ stringContent = L"Content!";

            contentPresenter->Content = rectangle;
            VERIFY_IS_TRUE(contentPresenter->Content->Equals(rectangle));

            contentPresenter->Content = stringContent;
            VERIFY_IS_TRUE(contentPresenter->Content->Equals(stringContent));
        });
    }

    void ContentPresenterIntegrationTests::CanSetAndGetContentTemplateProperty()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ContentPresenter^ contentPresenter = nullptr;
        Platform::String^ stringContent = L"I'm some good looking content";

        RunOnUIThread([&]()
        {
            contentPresenter = ref new xaml_controls::ContentPresenter;

            contentPresenter->Content = stringContent;
            contentPresenter->ContentTemplate = safe_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                L"    <TextBox Text='{Binding}'/>"
                L"</DataTemplate>"
                ));

            TestServices::WindowHelper->WindowContent = contentPresenter;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto textbox = TreeHelper::GetVisualChildByType<xaml_controls::TextBox>(contentPresenter);
            VERIFY_ARE_EQUAL(textbox->Text, stringContent);
        });
    }

    void ContentPresenterIntegrationTests::CanChangeContentTemplateProperty()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ContentPresenter^ contentPresenter = nullptr;

        RunOnUIThread([&]()
        {
            contentPresenter = ref new xaml_controls::ContentPresenter;

            contentPresenter->ContentTemplate = safe_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                L"    <TextBox/>"
                L"</DataTemplate>"
            ));

            TestServices::WindowHelper->WindowContent = contentPresenter;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            contentPresenter->ContentTemplate = safe_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                L"    <Rectangle/>"
                L"</DataTemplate>"
            ));
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void ContentPresenterIntegrationTests::DoesContentTemplateSelectorChooseTemplateBasedOnContent()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ContentPresenter^ contentPresenter = nullptr;
        Local::TestDataTemplateSelector^ testDataTemplateSelector = nullptr;

        Platform::String^ expectedStringText = L"STRING DataTemplate";
        Platform::String^ expectedIntText = L"INT DataTemplate";
        Platform::String^ expectedDoubleText = L"DOUBLE DataTemplate";

        RunOnUIThread([&]()
        {
            // Setup our custom data template selector with some data templates.
            testDataTemplateSelector = ref new Local::TestDataTemplateSelector();
            testDataTemplateSelector->AddEntry(
                Platform::String::typeid,
                safe_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                    L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                    L"    <TextBox Text='" + expectedStringText + L"'/>"
                    L"</DataTemplate>"
                )));

            testDataTemplateSelector->AddEntry(
                int::typeid,
                safe_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                    L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                    L"    <TextBox Text='" + expectedIntText + L"'/>"
                    L"</DataTemplate>"
                )));

            testDataTemplateSelector->AddEntry(
                double::typeid,
                safe_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                    L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                    L"    <TextBox Text='" + expectedDoubleText + L"'/>"
                    L"</DataTemplate>"
                )));

            contentPresenter = ref new xaml_controls::ContentPresenter;
            TestServices::WindowHelper->WindowContent = contentPresenter;

            // Setup first test case: string content
            contentPresenter->Content = ref new Platform::String(L"some string");

            // ContentPresenter differs from ContentControl in that it doesn't re-select
            // the content template whenever content changes.  It only does it when
            // the content template is set to null and the data template selector changes.
            // So we force a change in the selected template by nulling out the selector
            // and then resetting it.
            contentPresenter->ContentTemplateSelector = nullptr;
            contentPresenter->ContentTemplateSelector = testDataTemplateSelector;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validate content template selector with string content.");
        RunOnUIThread([&]()
        {
            auto textbox = TreeHelper::GetVisualChildByType<xaml_controls::TextBox>(contentPresenter);
            VERIFY_ARE_EQUAL(textbox->Text, expectedStringText);

            // Setup next test case: int content
            contentPresenter->Content = 123;

            // Force a change in the selected template.
            contentPresenter->ContentTemplateSelector = nullptr;
            contentPresenter->ContentTemplateSelector = testDataTemplateSelector;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validate content template selector with int content.");
        RunOnUIThread([&]()
        {
            auto textbox = TreeHelper::GetVisualChildByType<xaml_controls::TextBox>(contentPresenter);
            VERIFY_ARE_EQUAL(textbox->Text, expectedIntText);

            // Setup next test case: double content
            contentPresenter->Content = 123.456;

            // Force a change in the selected template.
            contentPresenter->ContentTemplateSelector = nullptr;
            contentPresenter->ContentTemplateSelector = testDataTemplateSelector;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validate content template selector with double content.");
        RunOnUIThread([&]()
        {
            auto textbox = TreeHelper::GetVisualChildByType<xaml_controls::TextBox>(contentPresenter);
            VERIFY_ARE_EQUAL(textbox->Text, expectedDoubleText);
        });
    }

    void ContentPresenterIntegrationTests::CanSetAndGetContentTransitionsProperty()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([]()
        {
            auto contentPresenter = ref new xaml_controls::ContentPresenter();
            contentPresenter->ContentTransitions = safe_cast<xaml_animation::TransitionCollection^>(
                xaml_markup::XamlReader::Load(
                        L"<TransitionCollection xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                        L"    <ContentThemeTransition/>"
                        L"</TransitionCollection>"
                    ));

            auto transition = contentPresenter->ContentTransitions->GetAt(0);
            VERIFY_IS_NOT_NULL(transition);
            VERIFY_ARE_EQUAL(transition->GetType()->FullName, ref new Platform::String(L"Microsoft.UI.Xaml.Media.Animation.ContentThemeTransition"));
        });
    }

    void ContentPresenterIntegrationTests::DoesKerningTakeEffect()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ContentPresenter^ contentPresenter1 = nullptr;
        xaml_controls::ContentPresenter^ contentPresenter2 = nullptr;
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);
        auto loadedEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            auto stackPanel = ref new xaml_controls::StackPanel();

            contentPresenter1 = ref new xaml_controls::ContentPresenter();
            contentPresenter1->Content = L"Test content";
            contentPresenter1->HorizontalAlignment = HorizontalAlignment::Left;
            xaml_docs::Typography::SetKerning(contentPresenter1, true);

            contentPresenter2 = ref new xaml_controls::ContentPresenter();
            contentPresenter2->Content = L"Test content";
            contentPresenter2->HorizontalAlignment = HorizontalAlignment::Left;
            xaml_docs::Typography::SetKerning(contentPresenter2, false);

            stackPanel->Children->Append(contentPresenter1);
            stackPanel->Children->Append(contentPresenter2);

            loadedRegistration.Attach(stackPanel, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                loadedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = stackPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ContentPresenter width with kerning: %.2f; without: %.2f", contentPresenter1->ActualWidth, contentPresenter2->ActualWidth);
            VERIFY_IS_LESS_THAN(contentPresenter1->ActualWidth, contentPresenter2->ActualWidth);
        });
    }

    void ContentPresenterIntegrationTests::ValidateTextProperties()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ContentPresenter^ cp1 = nullptr;
        xaml_controls::ContentPresenter^ cp2 = nullptr;
        xaml_controls::ContentPresenter^ cp3 = nullptr;
        xaml_controls::TextBlock^ textBlock = nullptr;
        xaml_controls::Button^ button1 = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel "
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"  x:Name='LayoutRoot' Background='SlateBlue'> "
                L"  <ContentPresenter x:Name='contentPresenter1'> "
                L"  </ContentPresenter> "
                L"  <ContentPresenter Content='Hello' x:Name='contentPresenter2' OpticalMarginAlignment='TrimSideBearings' "
                L"                    TextLineBounds='TrimToBaseline' TextWrapping='Wrap' LineStackingStrategy='MaxHeight' MaxLines='2' LineHeight='37.0'> "
                L"  </ContentPresenter> "
                L"  <Button x:Name='button1' Width='300' FontSize='50' Content='This is a CONTENT 0123456789.'> "
                L"  </Button> "
                L"</StackPanel>"));

            cp1 = safe_cast<xaml_controls::ContentPresenter^>(rootPanel->FindName("contentPresenter1"));
            VERIFY_IS_NOT_NULL(cp1);

            cp2 = safe_cast<xaml_controls::ContentPresenter^>(rootPanel->FindName("contentPresenter2"));
            VERIFY_IS_NOT_NULL(cp2);

            button1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName("button1"));
            VERIFY_IS_NOT_NULL(button1);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            cp3 = safe_cast<xaml_controls::ContentPresenter^>(TreeHelper::GetVisualChildByName(button1, L"ContentPresenter"));
            VERIFY_IS_NOT_NULL(cp3);

            cp3->OpticalMarginAlignment = Microsoft::UI::Xaml::OpticalMarginAlignment::TrimSideBearings;
            cp3->TextLineBounds = Microsoft::UI::Xaml::TextLineBounds::TrimToBaseline;
            cp3->TextWrapping = Microsoft::UI::Xaml::TextWrapping::Wrap;
            cp3->LineStackingStrategy = Microsoft::UI::Xaml::LineStackingStrategy::MaxHeight;
            cp3->MaxLines = 2;

            auto textBlock = TreeHelper::GetVisualChildByType<xaml_controls::TextBlock>(cp3);
            auto textBlock2 = TreeHelper::GetVisualChildByType<xaml_controls::TextBlock>(cp2);
            VERIFY_IS_NOT_NULL(textBlock);
            VERIFY_IS_NOT_NULL(textBlock2);

            LOG_OUTPUT(L"CP1 OpticalMarginAlignment=%d TextLineBounds=%d TextWrapping=%d LineStackingStrategy=%d MaxLines=%d IsTextScaleFactorEnabled=%d LineHeight=%lf",
                cp1->OpticalMarginAlignment, cp1->TextLineBounds, cp1->TextWrapping, cp1->LineStackingStrategy, cp1->MaxLines, cp1->IsTextScaleFactorEnabled, cp1->LineHeight);
            LOG_OUTPUT(L"CP2 OpticalMarginAlignment=%d TextLineBounds=%d TextWrapping=%d LineStackingStrategy=%d MaxLines=%d IsTextScaleFactorEnabled=%d LineHeight=%lf",
                cp2->OpticalMarginAlignment, cp2->TextLineBounds, cp2->TextWrapping, cp2->LineStackingStrategy, cp2->MaxLines, cp2->IsTextScaleFactorEnabled, cp2->LineHeight);
            LOG_OUTPUT(L"CP3 OpticalMarginAlignment=%d TextLineBounds=%d TextWrapping=%d LineStackingStrategy=%d MaxLines=%d IsTextScaleFactorEnabled=%d LineHeight=%lf",
                cp3->OpticalMarginAlignment, cp3->TextLineBounds, cp3->TextWrapping, cp3->LineStackingStrategy, cp3->MaxLines, cp3->IsTextScaleFactorEnabled, cp3->LineHeight);
            LOG_OUTPUT(L"TextBlock OpticalMarginAlignment=%d TextLineBounds=%d TextWrapping=%d LineStackingStrategy=%d MaxLines=%d IsTextScaleFactorEnabled=%d LineHeight=%lf",
                textBlock->OpticalMarginAlignment, textBlock->TextLineBounds, textBlock->TextWrapping, textBlock->LineStackingStrategy, textBlock->MaxLines, textBlock->IsTextScaleFactorEnabled, textBlock->LineHeight);

            VERIFY_ARE_EQUAL(cp1->OpticalMarginAlignment, Microsoft::UI::Xaml::OpticalMarginAlignment::None);
            VERIFY_ARE_EQUAL(cp1->TextLineBounds, Microsoft::UI::Xaml::TextLineBounds::Full);
            VERIFY_ARE_EQUAL(cp1->TextWrapping, Microsoft::UI::Xaml::TextWrapping::NoWrap);
            VERIFY_ARE_EQUAL(cp1->LineStackingStrategy, Microsoft::UI::Xaml::LineStackingStrategy::MaxHeight);
            VERIFY_ARE_EQUAL(cp1->MaxLines, 0);
            VERIFY_ARE_EQUAL(cp1->LineHeight, 0.0);

            VERIFY_ARE_EQUAL(cp2->OpticalMarginAlignment, Microsoft::UI::Xaml::OpticalMarginAlignment::TrimSideBearings);
            VERIFY_ARE_EQUAL(cp2->TextLineBounds, Microsoft::UI::Xaml::TextLineBounds::TrimToBaseline);
            VERIFY_ARE_EQUAL(cp2->TextWrapping, Microsoft::UI::Xaml::TextWrapping::Wrap);
            VERIFY_ARE_EQUAL(cp2->LineStackingStrategy, Microsoft::UI::Xaml::LineStackingStrategy::MaxHeight);
            VERIFY_ARE_EQUAL(cp2->MaxLines, 2);
            VERIFY_ARE_EQUAL(floor(cp2->LineHeight + 0.5), 37);

            // Validate the properties are pushed down to the TextBlock.
            VERIFY_ARE_EQUAL(textBlock2->OpticalMarginAlignment, Microsoft::UI::Xaml::OpticalMarginAlignment::TrimSideBearings);
            VERIFY_ARE_EQUAL(textBlock2->TextLineBounds, Microsoft::UI::Xaml::TextLineBounds::TrimToBaseline);
            VERIFY_ARE_EQUAL(textBlock2->TextWrapping, Microsoft::UI::Xaml::TextWrapping::Wrap);
            VERIFY_ARE_EQUAL(textBlock2->LineStackingStrategy, Microsoft::UI::Xaml::LineStackingStrategy::MaxHeight);
            VERIFY_ARE_EQUAL(textBlock2->MaxLines, 2);
            VERIFY_ARE_EQUAL(floor(textBlock2->LineHeight + 0.5), 37);

            VERIFY_ARE_EQUAL(cp3->OpticalMarginAlignment, textBlock->OpticalMarginAlignment);
            VERIFY_ARE_EQUAL(cp3->TextLineBounds, textBlock->TextLineBounds);
            VERIFY_ARE_EQUAL(cp3->TextWrapping, textBlock->TextWrapping);
            VERIFY_ARE_EQUAL(cp3->LineStackingStrategy, textBlock->LineStackingStrategy);
            VERIFY_ARE_EQUAL(cp3->MaxLines, textBlock->MaxLines);
            VERIFY_ARE_EQUAL(cp3->LineHeight, textBlock->LineHeight);
        });
    }

    void ContentPresenterIntegrationTests::CanImplicitlySetAndPropagateDataContext()
    {
        // Leak: TemplateContent peer not being unpegged
        TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

        TestCleanupWrapper cleanup;
        xaml_controls::Grid^ host;

        RunOnUIThread([&host]()
        {
            host = ref new xaml_controls::Grid();
            TestServices::WindowHelper->WindowContent = host;
            host->UpdateLayout();
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&host]()
        {
            auto recorder = ref new DataContextEventRecorder();
            xaml_controls::ContentPresenter^ outerCP;
            xaml_controls::ContentPresenter^ innerCP;

            // Loads the visual tree.
            {
                outerCP = safe_cast<xaml_controls::ContentPresenter^>(xaml_markup::XamlReader::Load(
                    "<ContentPresenter xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    "  <ContentPresenter.ContentTemplate>"
                    "    <DataTemplate>"
                    "      <ContentPresenter>"      // innerCP
                    "        <Button Content='{Binding}' />"
                    "      </ContentPresenter>"
                    "    </DataTemplate>"
                    "  </ContentPresenter.ContentTemplate>"
                    "</ContentPresenter>"));

                recorder->AttachTo(outerCP);

                // Expand the data template.
                // We set the content *before* the first layout pass because the inner CP
                // is going to clear its data context. This is the expected behavior because
                // its content is a UIElement.
                // Previously, instead of clearing the DC, it will explicitly set it to "content 1"
                // and remember it even when the content and DC on the outer CP changes.
                outerCP->Content = "content 1";

                // The Windows 8.1 behavior is that we get a DataContextChanged event on the CP
                // with the old and new value being both null.
                // It's not the ideal behavior but we need to assert it to make sure we don't
                // break it by accident.
                host->Children->Append(outerCP);
                VERIFY_ARE_EQUAL(1u, recorder->GetRecordedChanges().size());
                VERIFY_ARE_EQUAL(outerCP, recorder->GetRecordedChanges()[0].Presenter);
                VERIFY_ARE_EQUAL(DataContextEventRecorder::RecordedChangeType::FrameworkElement_DataContextChanged, recorder->GetRecordedChanges()[0].Type);
                VERIFY_IS_NULL(recorder->GetRecordedChanges()[0].NewValue);
                VERIFY_IS_NULL(recorder->GetRecordedChanges()[0].OldValue);
                PrintRecordedChanges(recorder, outerCP, innerCP);
                recorder->ClearEntries();

                // The outerCP.DataContext property is going to change and then we get a
                // DataContextChanged events with old value being null and the new value being "content 1"
                outerCP->UpdateLayout();
                VERIFY_ARE_EQUAL(2u, recorder->GetRecordedChanges().size());

                VERIFY_ARE_EQUAL(outerCP, recorder->GetRecordedChanges()[0].Presenter);
                VERIFY_ARE_EQUAL(DataContextEventRecorder::RecordedChangeType::DataContextPropertyChanged, recorder->GetRecordedChanges()[0].Type);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"content 1"), recorder->GetRecordedChanges()[0].NewValue);
                VERIFY_IS_NULL(recorder->GetRecordedChanges()[0].OldValue);

                VERIFY_ARE_EQUAL(outerCP, recorder->GetRecordedChanges()[1].Presenter);
                VERIFY_ARE_EQUAL(DataContextEventRecorder::RecordedChangeType::FrameworkElement_DataContextChanged, recorder->GetRecordedChanges()[1].Type);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"content 1"), recorder->GetRecordedChanges()[1].NewValue);

                // Attach to the inner CP now that it's available to us.
                innerCP = safe_cast<xaml_controls::ContentPresenter^>(xaml_media::VisualTreeHelper::GetChild(outerCP, 0));
                recorder->AttachTo(innerCP);

                PrintRecordedChanges(recorder, outerCP, innerCP);
                recorder->ClearEntries();
            }

            // Changes the content and run layout again.
            {
                outerCP->Content = "content 2";
                // ASSERT(outerCP recorded changes is 0)

                outerCP->UpdateLayout();

                VERIFY_ARE_EQUAL(3u, recorder->GetRecordedChanges().size());

                VERIFY_ARE_EQUAL(outerCP, recorder->GetRecordedChanges()[0].Presenter);
                VERIFY_ARE_EQUAL(DataContextEventRecorder::RecordedChangeType::DataContextPropertyChanged, recorder->GetRecordedChanges()[0].Type);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"content 2"), recorder->GetRecordedChanges()[0].NewValue);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"content 1"), recorder->GetRecordedChanges()[0].OldValue);

                VERIFY_ARE_EQUAL(outerCP, recorder->GetRecordedChanges()[1].Presenter);
                VERIFY_ARE_EQUAL(DataContextEventRecorder::RecordedChangeType::FrameworkElement_DataContextChanged, recorder->GetRecordedChanges()[1].Type);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"content 2"), recorder->GetRecordedChanges()[1].NewValue);

                VERIFY_ARE_EQUAL(innerCP, recorder->GetRecordedChanges()[2].Presenter);
                VERIFY_ARE_EQUAL(DataContextEventRecorder::RecordedChangeType::FrameworkElement_DataContextChanged, recorder->GetRecordedChanges()[2].Type);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"content 2"), recorder->GetRecordedChanges()[2].NewValue);

                PrintRecordedChanges(recorder, outerCP, innerCP);
                recorder->ClearEntries();
            }

            // Change the content and take the outer CP out of the tree.
            // Then re-insert the outer CP and observe it remembers the old "content 1".
            // This is the issue .
            {
                outerCP->Content = "content 3";
                outerCP->UpdateLayout();

                VERIFY_ARE_EQUAL(3u, recorder->GetRecordedChanges().size());

                VERIFY_ARE_EQUAL(outerCP, recorder->GetRecordedChanges()[0].Presenter);
                VERIFY_ARE_EQUAL(DataContextEventRecorder::RecordedChangeType::DataContextPropertyChanged, recorder->GetRecordedChanges()[0].Type);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"content 3"), recorder->GetRecordedChanges()[0].NewValue);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"content 2"), recorder->GetRecordedChanges()[0].OldValue);

                VERIFY_ARE_EQUAL(outerCP, recorder->GetRecordedChanges()[1].Presenter);
                VERIFY_ARE_EQUAL(DataContextEventRecorder::RecordedChangeType::FrameworkElement_DataContextChanged, recorder->GetRecordedChanges()[1].Type);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"content 3"), recorder->GetRecordedChanges()[1].NewValue);

                VERIFY_ARE_EQUAL(innerCP, recorder->GetRecordedChanges()[2].Presenter);
                VERIFY_ARE_EQUAL(DataContextEventRecorder::RecordedChangeType::FrameworkElement_DataContextChanged, recorder->GetRecordedChanges()[2].Type);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"content 3"), recorder->GetRecordedChanges()[2].NewValue);
                PrintRecordedChanges(recorder, outerCP, innerCP);
                recorder->ClearEntries();

                host->Children->Clear();
                VERIFY_ARE_EQUAL(0u, recorder->GetRecordedChanges().size());

                host->Children->Append(outerCP);

                // Again, nothing really changed but that's the windows 8.1 behavior that
                // we are validating.
                VERIFY_ARE_EQUAL(2u, recorder->GetRecordedChanges().size());

                VERIFY_ARE_EQUAL(outerCP, recorder->GetRecordedChanges()[0].Presenter);
                VERIFY_ARE_EQUAL(DataContextEventRecorder::RecordedChangeType::FrameworkElement_DataContextChanged, recorder->GetRecordedChanges()[0].Type);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"content 3"), recorder->GetRecordedChanges()[0].NewValue);

                VERIFY_ARE_EQUAL(innerCP, recorder->GetRecordedChanges()[1].Presenter);
                VERIFY_ARE_EQUAL(DataContextEventRecorder::RecordedChangeType::FrameworkElement_DataContextChanged, recorder->GetRecordedChanges()[1].Type);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"content 3"), recorder->GetRecordedChanges()[1].NewValue);
                PrintRecordedChanges(recorder, outerCP, innerCP);
                recorder->ClearEntries();

                outerCP->UpdateLayout();
                VERIFY_ARE_EQUAL(0u, recorder->GetRecordedChanges().size());
            }

            // Now let's change the content while we are out of the visual tree.
            // Both threshold and windows 8.1 produce the wrong result in this case.
            {
                host->Children->Clear();
                outerCP->UpdateLayout();
                outerCP->Content = "content 4";

                VERIFY_ARE_EQUAL(0u, recorder->GetRecordedChanges().size());

                host->Children->Append(outerCP);

                VERIFY_ARE_EQUAL(2u, recorder->GetRecordedChanges().size());

                // Pre existing bug:
                // Should be "content 4" instead of "content 3"
                VERIFY_ARE_EQUAL(outerCP, recorder->GetRecordedChanges()[0].Presenter);
                VERIFY_ARE_EQUAL(DataContextEventRecorder::RecordedChangeType::FrameworkElement_DataContextChanged, recorder->GetRecordedChanges()[0].Type);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"content 3"), recorder->GetRecordedChanges()[0].NewValue);

                // Pre existing bug:
                // Should be "content 4" instead of "content 3"
                VERIFY_ARE_EQUAL(innerCP, recorder->GetRecordedChanges()[1].Presenter);
                VERIFY_ARE_EQUAL(DataContextEventRecorder::RecordedChangeType::FrameworkElement_DataContextChanged, recorder->GetRecordedChanges()[1].Type);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"content 3"), recorder->GetRecordedChanges()[1].NewValue);
                PrintRecordedChanges(recorder, outerCP, innerCP);
                recorder->ClearEntries();

                // Things correct themselves though once we update layout.
                outerCP->UpdateLayout();

                VERIFY_ARE_EQUAL(3u, recorder->GetRecordedChanges().size());

                VERIFY_ARE_EQUAL(outerCP, recorder->GetRecordedChanges()[0].Presenter);
                VERIFY_ARE_EQUAL(DataContextEventRecorder::RecordedChangeType::DataContextPropertyChanged, recorder->GetRecordedChanges()[0].Type);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"content 4"), recorder->GetRecordedChanges()[0].NewValue);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"content 3"), recorder->GetRecordedChanges()[0].OldValue);

                VERIFY_ARE_EQUAL(outerCP, recorder->GetRecordedChanges()[1].Presenter);
                VERIFY_ARE_EQUAL(DataContextEventRecorder::RecordedChangeType::FrameworkElement_DataContextChanged, recorder->GetRecordedChanges()[1].Type);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"content 4"), recorder->GetRecordedChanges()[1].NewValue);

                VERIFY_ARE_EQUAL(innerCP, recorder->GetRecordedChanges()[2].Presenter);
                VERIFY_ARE_EQUAL(DataContextEventRecorder::RecordedChangeType::FrameworkElement_DataContextChanged, recorder->GetRecordedChanges()[2].Type);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"content 4"), recorder->GetRecordedChanges()[2].NewValue);
                PrintRecordedChanges(recorder, outerCP, innerCP);
                recorder->ClearEntries();
            }

            // Verify that clearing the data context on the outer CP
            // will propagate correctly down to the inner CP.
            // Notice that we don't change the content here.
            {
                outerCP->ClearValue(FrameworkElement::DataContextProperty);

                VERIFY_ARE_EQUAL(3u, recorder->GetRecordedChanges().size());

                VERIFY_ARE_EQUAL(outerCP, recorder->GetRecordedChanges()[0].Presenter);
                VERIFY_ARE_EQUAL(DataContextEventRecorder::RecordedChangeType::DataContextPropertyChanged, recorder->GetRecordedChanges()[0].Type);
                VERIFY_IS_NULL(recorder->GetRecordedChanges()[0].NewValue);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"content 4"), recorder->GetRecordedChanges()[0].OldValue);

                VERIFY_ARE_EQUAL(outerCP, recorder->GetRecordedChanges()[1].Presenter);
                VERIFY_ARE_EQUAL(DataContextEventRecorder::RecordedChangeType::FrameworkElement_DataContextChanged, recorder->GetRecordedChanges()[1].Type);
                VERIFY_IS_NULL(recorder->GetRecordedChanges()[1].NewValue);

                VERIFY_ARE_EQUAL(innerCP, recorder->GetRecordedChanges()[2].Presenter);
                VERIFY_ARE_EQUAL(DataContextEventRecorder::RecordedChangeType::FrameworkElement_DataContextChanged, recorder->GetRecordedChanges()[2].Type);
                VERIFY_IS_NULL(recorder->GetRecordedChanges()[1].NewValue);

                PrintRecordedChanges(recorder, outerCP, innerCP);
                recorder->ClearEntries();

                outerCP->UpdateLayout();
                VERIFY_ARE_EQUAL(0u, recorder->GetRecordedChanges().size());
            }

            // Checks that if we handle the DataContextChanged event in the outer CP,
            // it won't propagate to the inner CP.
            {
                outerCP->Content = "content 5";
                recorder->HandleDataContextChangedEvent();
                VERIFY_ARE_EQUAL(0u, recorder->GetRecordedChanges().size());

                outerCP->UpdateLayout();

                VERIFY_ARE_EQUAL(2u, recorder->GetRecordedChanges().size());

                VERIFY_ARE_EQUAL(outerCP, recorder->GetRecordedChanges()[0].Presenter);
                VERIFY_ARE_EQUAL(DataContextEventRecorder::RecordedChangeType::DataContextPropertyChanged, recorder->GetRecordedChanges()[0].Type);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"content 5"), recorder->GetRecordedChanges()[0].NewValue);
                VERIFY_IS_NULL(recorder->GetRecordedChanges()[0].OldValue);

                VERIFY_ARE_EQUAL(outerCP, recorder->GetRecordedChanges()[1].Presenter);
                VERIFY_ARE_EQUAL(DataContextEventRecorder::RecordedChangeType::FrameworkElement_DataContextChanged, recorder->GetRecordedChanges()[1].Type);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"content 5"), recorder->GetRecordedChanges()[1].NewValue);

                PrintRecordedChanges(recorder, outerCP, innerCP);
                recorder->ClearEntries();
            }

            // Verify that clearing an explicit data context on the inner CP
            // and reading it right away will get us the value of the outer CP's data context.
            {
                innerCP->DataContext = "inner DC";

                VERIFY_ARE_EQUAL(2u, recorder->GetRecordedChanges().size());

                // Pre existing bug:
                // Old value should be "content 5" instead of "content 1"
                VERIFY_ARE_EQUAL(innerCP, recorder->GetRecordedChanges()[0].Presenter);
                VERIFY_ARE_EQUAL(DataContextEventRecorder::RecordedChangeType::DataContextPropertyChanged, recorder->GetRecordedChanges()[0].Type);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"inner DC"), recorder->GetRecordedChanges()[0].NewValue);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"content 1"), recorder->GetRecordedChanges()[0].OldValue);

                VERIFY_ARE_EQUAL(innerCP, recorder->GetRecordedChanges()[1].Presenter);
                VERIFY_ARE_EQUAL(DataContextEventRecorder::RecordedChangeType::FrameworkElement_DataContextChanged, recorder->GetRecordedChanges()[1].Type);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"inner DC"), recorder->GetRecordedChanges()[1].NewValue);

                PrintRecordedChanges(recorder, outerCP, innerCP);
                recorder->ClearEntries();

                innerCP->ClearValue(FrameworkElement::DataContextProperty);

                VERIFY_ARE_EQUAL(2u, recorder->GetRecordedChanges().size());

                VERIFY_ARE_EQUAL(innerCP, recorder->GetRecordedChanges()[0].Presenter);
                VERIFY_ARE_EQUAL(DataContextEventRecorder::RecordedChangeType::DataContextPropertyChanged, recorder->GetRecordedChanges()[0].Type);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"content 5"), recorder->GetRecordedChanges()[0].NewValue);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"inner DC"), recorder->GetRecordedChanges()[0].OldValue);

                VERIFY_ARE_EQUAL(innerCP, recorder->GetRecordedChanges()[1].Presenter);
                VERIFY_ARE_EQUAL(DataContextEventRecorder::RecordedChangeType::FrameworkElement_DataContextChanged, recorder->GetRecordedChanges()[1].Type);
                VERIFY_ARE_EQUAL(ref new Platform::String(L"content 5"), recorder->GetRecordedChanges()[1].NewValue);

                PrintRecordedChanges(recorder, outerCP, innerCP);
                recorder->ClearEntries();

                outerCP->UpdateLayout();
                VERIFY_ARE_EQUAL(0u, recorder->GetRecordedChanges().size());

                // This should clear the circular reference between DataContextEventRecorder and the ContentPresenters.
                recorder->Reset();
            }
        });
    }

    void ContentPresenterIntegrationTests::VerifyBorderChrome()
    {
        xaml_controls::StackPanel^ stackPanel = nullptr;
        xaml_controls::ContentPresenter^ presenter1 = nullptr;
        xaml_controls::ContentPresenter^ presenter2 = nullptr;
        xaml_controls::ContentPresenter^ presenter3 = nullptr;
        xaml_controls::ContentPresenter^ presenter4 = nullptr;
        xaml_media::SolidColorBrush^ semiTransparentBrush = nullptr;
        xaml_media::SolidColorBrush^ transparentBrush = nullptr;

        const unsigned int width = 100;
        const unsigned int height = 50;
        const uint32_t transparent = ArgbToUint32(0, 0, 0, 0);
        const uint32_t yellow = ArgbToUint32(255, 255, 255, 0);
        const uint32_t cyan = ArgbToUint32(255, 0, 255, 255);
        const uint32_t semitransparentYellow = ArgbToUint32(128, 128, 128, 0);
        const uint32_t blended = ArgbToUint32(255, 128, 255, 127);

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(600, 600));
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        RunOnUIThread([&]()
        {
            semiTransparentBrush = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(128, 255, 255, 0));
            transparentBrush = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(0, 0, 0, 0));

            stackPanel = ref new xaml_controls::StackPanel();
            stackPanel->Padding = { 4,4,4,4 };
            stackPanel->Spacing = 4;

            presenter1 = CreateContentPresenterWithBorder(width, height);
            presenter2 = CreateContentPresenterWithBorder(width, height);
            presenter3 = CreateContentPresenterWithBorder(width, height);
            presenter4 = CreateContentPresenterWithBorder(width, height);

            LOG_OUTPUT(L"Validate solid borders.");
            presenter1->BackgroundSizing = xaml_controls::BackgroundSizing::InnerBorderEdge;
            presenter2->BackgroundSizing = xaml_controls::BackgroundSizing::OuterBorderEdge;
            presenter3->BackgroundSizing = xaml_controls::BackgroundSizing::InnerBorderEdge;
            presenter4->BackgroundSizing = xaml_controls::BackgroundSizing::OuterBorderEdge;

            presenter3->CornerRadius = { 10,10,10,10 };
            presenter4->CornerRadius = { 10,10,10,10 };

            stackPanel->Children->Append(presenter1);
            stackPanel->Children->Append(presenter2);
            stackPanel->Children->Append(presenter3);
            stackPanel->Children->Append(presenter4);

            TestServices::WindowHelper->WindowContent = stackPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");

        TestElementWithRenderTargetBitmap(presenter1, [&](Platform::Array<uint32_t>^ buffer, unsigned int w, unsigned int h)
        {
            VERIFY_ARE_EQUAL(w, width);
            VERIFY_ARE_EQUAL(h, height);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 0, width)], yellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 0, width)], yellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 0, width)], yellow);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 24, width)], yellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 24, width)], cyan);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 24, width)], yellow);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 49, width)], yellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 49, width)], yellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 49, width)], yellow);
        });

        TestElementWithRenderTargetBitmap(presenter2, [&](Platform::Array<uint32_t>^ buffer, unsigned int w, unsigned int h)
        {
            VERIFY_ARE_EQUAL(w, width);
            VERIFY_ARE_EQUAL(h, height);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 0, width)], yellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 0, width)], yellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 0, width)], yellow);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 24, width)], yellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 24, width)], cyan);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 24, width)], yellow);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 49, width)], yellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 49, width)], yellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 49, width)], yellow);
        });

        TestElementWithRenderTargetBitmap(presenter3, [&](Platform::Array<uint32_t>^ buffer, unsigned int w, unsigned int h)
        {
            VERIFY_ARE_EQUAL(w, width);
            VERIFY_ARE_EQUAL(h, height);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 0, width)], transparent);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 0, width)], yellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 0, width)], transparent);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 24, width)], yellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 24, width)], cyan);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 24, width)], yellow);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 49, width)], transparent);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 49, width)], yellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 49, width)], transparent);
        });

        TestElementWithRenderTargetBitmap(presenter4, [&](Platform::Array<uint32_t>^ buffer, unsigned int w, unsigned int h)
        {
            VERIFY_ARE_EQUAL(w, width);
            VERIFY_ARE_EQUAL(h, height);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 0, width)], transparent);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 0, width)], yellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 0, width)], transparent);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 24, width)], yellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 24, width)], cyan);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 24, width)], yellow);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 49, width)], transparent);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 49, width)], yellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 49, width)], transparent);
        });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Validate transparent borders.");
            presenter1->BorderBrush = transparentBrush;
            presenter2->BorderBrush = transparentBrush;
            presenter3->BorderBrush = transparentBrush;
            presenter4->BorderBrush = transparentBrush;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");

        TestElementWithRenderTargetBitmap(presenter1, [&](Platform::Array<uint32_t>^ buffer, unsigned int w, unsigned int h)
        {
            VERIFY_ARE_EQUAL(w, width);
            VERIFY_ARE_EQUAL(h, height);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 0, width)], transparent);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 0, width)], transparent);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 0, width)], transparent);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 24, width)], transparent);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 24, width)], cyan);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 24, width)], transparent);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 49, width)], transparent);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 49, width)], transparent);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 49, width)], transparent);
        });

        TestElementWithRenderTargetBitmap(presenter2, [&](Platform::Array<uint32_t>^ buffer, unsigned int w, unsigned int h)
        {
            VERIFY_ARE_EQUAL(w, width);
            VERIFY_ARE_EQUAL(h, height);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 0, width)], cyan);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 0, width)], cyan);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 0, width)], cyan);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 24, width)], cyan);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 24, width)], cyan);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 24, width)], cyan);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 49, width)], cyan);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 49, width)], cyan);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 49, width)], cyan);
        });

        TestElementWithRenderTargetBitmap(presenter3, [&](Platform::Array<uint32_t>^ buffer, unsigned int w, unsigned int h)
        {
            VERIFY_ARE_EQUAL(w, width);
            VERIFY_ARE_EQUAL(h, height);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 0, width)], transparent);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 0, width)], transparent);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 0, width)], transparent);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 24, width)], transparent);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 24, width)], cyan);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 24, width)], transparent);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 49, width)], transparent);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 49, width)], transparent);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 49, width)], transparent);
        });

        TestElementWithRenderTargetBitmap(presenter4, [&](Platform::Array<uint32_t>^ buffer, unsigned int w, unsigned int h)
        {
            VERIFY_ARE_EQUAL(w, width);
            VERIFY_ARE_EQUAL(h, height);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 0, width)], transparent);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 0, width)], cyan);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 0, width)], transparent);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 24, width)], cyan);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 24, width)], cyan);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 24, width)], cyan);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 49, width)], transparent);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 49, width)], cyan);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 49, width)], transparent);
        });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Validate semi-transparent borders.");
            presenter1->BorderBrush = semiTransparentBrush;
            presenter2->BorderBrush = semiTransparentBrush;
            presenter3->BorderBrush = semiTransparentBrush;
            presenter4->BorderBrush = semiTransparentBrush;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "3");

        TestElementWithRenderTargetBitmap(presenter1, [&](Platform::Array<uint32_t>^ buffer, unsigned int w, unsigned int h)
        {
            VERIFY_ARE_EQUAL(w, width);
            VERIFY_ARE_EQUAL(h, height);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 0, width)], semitransparentYellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 0, width)], semitransparentYellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 0, width)], semitransparentYellow);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 24, width)], semitransparentYellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 24, width)], cyan);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 24, width)], semitransparentYellow);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 49, width)], semitransparentYellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 49, width)], semitransparentYellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 49, width)], semitransparentYellow);
        });

        TestElementWithRenderTargetBitmap(presenter2, [&](Platform::Array<uint32_t>^ buffer, unsigned int w, unsigned int h)
        {
            VERIFY_ARE_EQUAL(w, width);
            VERIFY_ARE_EQUAL(h, height);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 0, width)], blended);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 0, width)], blended);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 0, width)], blended);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 24, width)], blended);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 24, width)], cyan);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 24, width)], blended);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 49, width)], blended);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 49, width)], blended);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 49, width)], blended);
        });

        TestElementWithRenderTargetBitmap(presenter3, [&](Platform::Array<uint32_t>^ buffer, unsigned int w, unsigned int h)
        {
            VERIFY_ARE_EQUAL(w, width);
            VERIFY_ARE_EQUAL(h, height);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 0, width)], transparent);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 0, width)], semitransparentYellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 0, width)], transparent);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 24, width)], semitransparentYellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 24, width)], cyan);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 24, width)], semitransparentYellow);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 49, width)], transparent);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 49, width)], semitransparentYellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 49, width)], transparent);
        });

        TestElementWithRenderTargetBitmap(presenter4, [&](Platform::Array<uint32_t>^ buffer, unsigned int w, unsigned int h)
        {
            VERIFY_ARE_EQUAL(w, width);
            VERIFY_ARE_EQUAL(h, height);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 0, width)], transparent);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 0, width)], blended);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 0, width)], transparent);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 24, width)], blended);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 24, width)], cyan);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 24, width)], blended);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 49, width)], transparent);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 49, width)], blended);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 49, width)], transparent);
        });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Validate borders that take up all the inner space.");
            presenter1->BorderThickness = xaml::Thickness({ 0, 25, 0, 25 });
            presenter2->BorderThickness = xaml::Thickness({ 0, 25, 0, 25 });
            presenter3->BorderThickness = xaml::Thickness({ 0, 25, 0, 25 });
            presenter4->BorderThickness = xaml::Thickness({ 0, 25, 0, 25 });
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "4");

        TestElementWithRenderTargetBitmap(presenter1, [&](Platform::Array<uint32_t>^ buffer, unsigned int w, unsigned int h)
        {
            VERIFY_ARE_EQUAL(w, width);
            VERIFY_ARE_EQUAL(h, height);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 0, width)], semitransparentYellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 0, width)], semitransparentYellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 0, width)], semitransparentYellow);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 24, width)], semitransparentYellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 24, width)], semitransparentYellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 24, width)], semitransparentYellow);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 49, width)], semitransparentYellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 49, width)], semitransparentYellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 49, width)], semitransparentYellow);
        });

        TestElementWithRenderTargetBitmap(presenter2, [&](Platform::Array<uint32_t>^ buffer, unsigned int w, unsigned int h)
        {
            VERIFY_ARE_EQUAL(w, width);
            VERIFY_ARE_EQUAL(h, height);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 0, width)], blended);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 0, width)], blended);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 0, width)], blended);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 24, width)], blended);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 24, width)], blended);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 24, width)], blended);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 49, width)], blended);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 49, width)], blended);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 49, width)], blended);
        });

        TestElementWithRenderTargetBitmap(presenter3, [&](Platform::Array<uint32_t>^ buffer, unsigned int w, unsigned int h)
        {
            VERIFY_ARE_EQUAL(w, width);
            VERIFY_ARE_EQUAL(h, height);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 0, width)], transparent);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 0, width)], semitransparentYellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 0, width)], transparent);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 24, width)], semitransparentYellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 24, width)], semitransparentYellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 24, width)], semitransparentYellow);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 49, width)], transparent);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 49, width)], semitransparentYellow);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 49, width)], transparent);
        });

        TestElementWithRenderTargetBitmap(presenter4, [&](Platform::Array<uint32_t>^ buffer, unsigned int w, unsigned int h)
        {
            VERIFY_ARE_EQUAL(w, width);
            VERIFY_ARE_EQUAL(h, height);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 0, width)], transparent);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 0, width)], blended);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 0, width)], transparent);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 24, width)], blended);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 24, width)], blended);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 24, width)], blended);

            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(0, 49, width)], transparent);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(49, 49, width)], blended);
            VERIFY_ARE_EQUAL(buffer[GetPixelIndex(99, 49, width)], transparent);
        });
    }

    xaml_controls::ContentPresenter^ ContentPresenterIntegrationTests::CreateContentPresenterWithBorder(unsigned int width, unsigned int height)
    {
        const auto& element = ref new xaml_controls::ContentPresenter();

        element->Width = width;
        element->Height = height;
        element->HorizontalContentAlignment = HorizontalAlignment::Center;
        element->VerticalContentAlignment = VerticalAlignment::Center;
        element->BorderThickness = { 10,10,10,10 };
        element->BorderBrush = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(255, 255, 255, 0));
        element->Background = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(255, 0, 255, 255));

        return element;
    }

    unsigned int ContentPresenterIntegrationTests::GetPixelIndex(unsigned int x, unsigned int y, unsigned int w)
    {
        return (y * w) + x;
    }

    uint32_t ContentPresenterIntegrationTests::ArgbToUint32(uint8_t a, uint8_t r, uint8_t g, uint8_t b)
    {
        return a << 24 | r << 16 | g << 8 | b;
    }

    void ContentPresenterIntegrationTests::TestElementWithRenderTargetBitmap(xaml::UIElement^ element, std::function<void(Platform::Array<uint32_t>^, int, int)> verifyFunc)
    {
        auto renderedEvent = std::make_shared<Event>();
        auto getPixelsEvent = std::make_shared<Event>();

        RenderTargetBitmap^ rtb = nullptr;
        wf::IAsyncOperation<IBuffer^>^ getPixelsAsyncOperation = nullptr;
        IBuffer^ buffer = nullptr;

        auto w = 0;
        auto h = 0;

        RunOnUIThread([&]()
        {
            rtb = ref new RenderTargetBitmap();
            LOG_OUTPUT(L"Invoking RenderTargetBitmap::RenderAsync.");

            create_task(rtb->RenderAsync(element)).then([&renderedEvent]()
            {
                LOG_OUTPUT(L"RenderTargetBitmap::RenderAsync completed.");

                renderedEvent->Set();
            });
        });
        renderedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            getPixelsAsyncOperation = rtb->GetPixelsAsync();

            w = rtb->PixelWidth;
            h = rtb->PixelHeight;

            auto getPixelsCallback = ref new wf::AsyncOperationCompletedHandler<IBuffer^>(
                [&buffer, getPixelsEvent](wf::IAsyncOperation<IBuffer^>^ operation, wf::AsyncStatus)
            {
                LOG_OUTPUT(L"GetPixelsAsync operation completed.");
                buffer = operation->GetResults();
                getPixelsEvent->Set();
            });
            VERIFY_IS_NOT_NULL(getPixelsCallback);
            getPixelsAsyncOperation->Completed = getPixelsCallback;
        });
        getPixelsEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Verifying bitmap content.");
            DataReader^ dataReader = DataReader::FromBuffer(buffer);
            Platform::Array<byte>^ generatedImage = ref new Platform::Array<byte>(buffer->Length);
            dataReader->ReadBytes(generatedImage);

            Platform::Array<uint32_t>^ rgbPixels = ref new Platform::Array<uint32_t>(buffer->Length / 4);
            for (unsigned int i = 0; i < buffer->Length; i += 4)
            {
                rgbPixels[i / 4] = ArgbToUint32(generatedImage[3 + i], generatedImage[2 + i], generatedImage[1 + i], generatedImage[0 + i]);
            }

            verifyFunc(rgbPixels, w, h);
        });

        TestServices::WindowHelper->WaitForIdle();
    }


} } } } } } // Microsoft::UI::Xaml::Tests::Controls::Button
