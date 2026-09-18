// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ContentControlIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>

#include <ControlHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

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

    [wf::Metadata::WebHostHidden]
    public ref class TestTextBox sealed : public xaml_controls::TextBox
    {
    public:
        TestTextBox()
            : m_isMeasured(false)
        {}

    public:
        void ClearMeasuredFlag()
        {
            m_isMeasured = false;
        }

        bool IsMeasured() { return m_isMeasured; }

    protected:

        ::Windows::Foundation::Size MeasureOverride(::Windows::Foundation::Size availableSize) override
        {
            m_isMeasured = true;
            return xaml_controls::Control::MeasureOverride(availableSize);
        }

    private:
        bool m_isMeasured;
    }; // pubic ref class TestTextBox


} // namespace Local

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ContentControl {


    bool ContentControlIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ContentControlIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ContentControlIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void ContentControlIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::ContentControl>::CanInstantiate();
    }

    void ContentControlIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::ContentControl>::CanEnterAndLeaveLiveTree();
    }

    void ContentControlIntegrationTests::CanSetAndGetContentProperty()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([]()
        {
            auto contentControl = ref new xaml_controls::ContentControl();
            auto rectangle = ref new xaml_shapes::Rectangle();
            Platform::String^ stringContent = L"Content!";

            contentControl->Content = rectangle;
            VERIFY_IS_TRUE(contentControl->Content->Equals(rectangle));

            contentControl->Content = stringContent;
            VERIFY_IS_TRUE(contentControl->Content->Equals(stringContent));
        });
    }

    void ContentControlIntegrationTests::CanSetAndGetContentTemplateProperty()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ContentControl^ contentControl = nullptr;
        Platform::String^ stringContent = L"I'm some good looking content";

        RunOnUIThread([&]()
        {
            contentControl = ref new xaml_controls::ContentControl;

            contentControl->ContentTemplate = safe_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                L"    <TextBox Text='{Binding}'/>"
                L"</DataTemplate>"
                ));

            TestServices::WindowHelper->WindowContent = contentControl;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto textbox = safe_cast<xaml_controls::TextBox^>(contentControl->ContentTemplateRoot);
            VERIFY_IS_NOT_NULL(textbox);

            contentControl->Content = stringContent;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto textbox = safe_cast<xaml_controls::TextBox^>(contentControl->ContentTemplateRoot);
            VERIFY_ARE_EQUAL(textbox->Text, stringContent);
        });
    }

    void ContentControlIntegrationTests::CanGetContentTemplateRootProperty()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ContentControl^ contentControl = nullptr;
        Platform::String^ stringContent = L"I'm some good looking content";

        RunOnUIThread([&]()
        {
            contentControl = ref new xaml_controls::ContentControl;

            contentControl->ContentTemplate = safe_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                L"    <TextBox Text='{Binding}'/>"
                L"</DataTemplate>"
                ));

            TestServices::WindowHelper->WindowContent = contentControl;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto textbox = safe_cast<xaml_controls::TextBox^>(contentControl->ContentTemplateRoot);
            VERIFY_IS_NOT_NULL(textbox);

            contentControl->ContentTemplate = safe_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                L"    <Rectangle Width='404' />"
                L"</DataTemplate>"
                ));

        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto rectangle = safe_cast<xaml::Shapes::Rectangle^>(contentControl->ContentTemplateRoot);
            VERIFY_IS_NOT_NULL(rectangle);
            VERIFY_ARE_EQUAL(rectangle->Width, 404);
        });
    }
    void ContentControlIntegrationTests::DoesContentTemplateSelectorChooseTemplateBasedOnContent()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ContentControl^ contentControl = nullptr;
        Platform::String^ expectedStringText = L"STRING DataTemplate";
        Platform::String^ expectedIntText = L"INT DataTemplate";
        Platform::String^ expectedDoubleText = L"DOUBLE DataTemplate";

        RunOnUIThread([&]()
        {
            // Setup our custom data template selector with some data templates.
            auto myDataTemplateSelector = ref new Local::TestDataTemplateSelector();
            myDataTemplateSelector->AddEntry(
                Platform::String::typeid,
                safe_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                    L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                    L"    <TextBox Text='" + expectedStringText + L"'/>"
                    L"</DataTemplate>"
                )));

            myDataTemplateSelector->AddEntry(
                int::typeid,
                safe_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                    L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                    L"    <TextBox Text='" + expectedIntText + L"'/>"
                    L"</DataTemplate>"
                )));

            myDataTemplateSelector->AddEntry(
                double::typeid,
                safe_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                    L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                    L"    <TextBox Text='" + expectedDoubleText + L"'/>"
                    L"</DataTemplate>"
                )));

            contentControl = ref new xaml_controls::ContentControl;
            contentControl->ContentTemplateSelector = myDataTemplateSelector;
            TestServices::WindowHelper->WindowContent = contentControl;

            // Setup first test case: string content
            contentControl->Content = ref new Platform::String(L"some string");
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validate content template selector with string content.");
        RunOnUIThread([&]()
        {
            auto textbox = safe_cast<xaml_controls::TextBox^>(contentControl->ContentTemplateRoot);
            VERIFY_ARE_EQUAL(textbox->Text, expectedStringText);

            // Setup next test case: int content
            contentControl->Content = 123;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validate content template selector with int content.");
        RunOnUIThread([&]()
        {
            auto textbox = safe_cast<xaml_controls::TextBox^>(contentControl->ContentTemplateRoot);
            VERIFY_ARE_EQUAL(textbox->Text, expectedIntText);

            // Setup next test case: double content
            contentControl->Content = 123.456;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validate content template selector with double content.");
        RunOnUIThread([&]()
        {
            auto textbox = safe_cast<xaml_controls::TextBox^>(contentControl->ContentTemplateRoot);
            VERIFY_ARE_EQUAL(textbox->Text, expectedDoubleText);
        });
    }

    void ContentControlIntegrationTests::CanSetAndGetContentTransitionsProperty()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([]()
        {
            auto contentControl = ref new xaml_controls::ContentControl();
            contentControl->ContentTransitions = safe_cast<xaml_animation::TransitionCollection^>(
                xaml_markup::XamlReader::Load(
                        L"<TransitionCollection xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                        L"    <ContentThemeTransition/>"
                        L"</TransitionCollection>"
                    ));

            auto transition = contentControl->ContentTransitions->GetAt(0);
            VERIFY_IS_NOT_NULL(transition);
            VERIFY_ARE_EQUAL(transition->GetType()->FullName, ref new Platform::String(L"Microsoft.UI.Xaml.Media.Animation.ContentThemeTransition"));
        });
    }

    void ContentControlIntegrationTests::DoNotPropragateMeasureDirtyDownWhenReassignSamePropertyValue()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ContentControl^ cc = nullptr;
        Local::TestTextBox^ textBox = nullptr;

        RunOnUIThread([&]()
        {
            cc = ref new xaml_controls::ContentControl();
            textBox = ref new Local::TestTextBox();
            cc->Content = textBox;
            TestServices::WindowHelper->WindowContent = cc;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            textBox->ClearMeasuredFlag();

            // Reassign same value, textbox is not measure dirty
            LOG_OUTPUT(L"Setting flowdirection to LTR (not changed).");
            cc->FlowDirection = cc->FlowDirection;
            cc->UpdateLayout();
            VERIFY_IS_FALSE(textBox->IsMeasured());

            // Assign a new value, textbox is measure dirty
            LOG_OUTPUT(L"Setting flowdirection to RTL (changed).");
            cc->FlowDirection = Microsoft::UI::Xaml::FlowDirection::RightToLeft;
            cc->UpdateLayout();
            VERIFY_IS_TRUE(textBox->IsMeasured());
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::ContentControl
