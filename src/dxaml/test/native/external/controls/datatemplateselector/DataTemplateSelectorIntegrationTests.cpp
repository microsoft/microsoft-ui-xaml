// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "DataTemplateSelectorIntegrationTests.h"
#include "BasicDataTemplateSelector.h"

#include <TreeHelper.h>
#include <XamlTailored.h>
#include <collection.h>
#include <TestCleanupWrapper.h>

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>


using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace DataTemplateSelector {

    bool DataTemplateSelectorIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }
     
    bool DataTemplateSelectorIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool DataTemplateSelectorIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void DataTemplateSelectorIntegrationTests::CanSelectDataTemplate()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ListBox^ list = nullptr;

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Setting up environment.");
            auto root = ref new xaml_controls::Grid();
            VERIFY_IS_NOT_NULL(root);
            list = ref new xaml_controls::ListBox();
            VERIFY_IS_NOT_NULL(list);
            auto source = ref new Platform::Collections::Vector<Platform::String^>();

            list->Height = 500;
            list->Width = 500;

            root->Children->Append(list);

            for (int i = 0; i < 10 ; i++)
            {
                source->Append(L"Item-" + i);
            }

            auto basicDTS = ref new BDTS::BasicDataTemplateSelector();
            VERIFY_IS_NOT_NULL(basicDTS);
            basicDTS->TemplateWithButton = dynamic_cast<xaml::DataTemplate^> (xaml_markup::XamlReader::Load(
                L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel Background='Blue' Width='150' Height='150'>"
                L"      <TextBlock Text='{Binding}' />"
                L"      <Button Background='Orange' Width='50' Height='50' />"
                L"  </StackPanel>"
                L"</DataTemplate>"));

            list->ItemTemplateSelector = basicDTS;

            list->ItemsSource = source;

            TestServices::WindowHelper->WindowContent = root;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto container = list->ItemContainerGenerator->ContainerFromIndex(0);
            VERIFY_IS_NOT_NULL(container);

            auto button = TreeHelper::GetVisualChildByType<xaml_controls::Button>(
                dynamic_cast<xaml::FrameworkElement^>(container));
            VERIFY_IS_NOT_NULL(button);
            VERIFY_ARE_NOT_EQUAL(0, button->ActualWidth);
            VERIFY_ARE_NOT_EQUAL(0, button->ActualHeight);
        });
    }


} } } } } } // Microsoft::UI::Xaml::Tests::Controls::FontIcon
