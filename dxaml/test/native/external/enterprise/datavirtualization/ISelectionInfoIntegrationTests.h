// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <UIAutomationHelper.h>

#include "DataVirtualizationDataSourceHelper.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace DataVirtualization {

    class ISelectionInfoIntegrationTests : public WEX::TestClass<ISelectionInfoIntegrationTests>
    {
    public:
        ISelectionInfoIntegrationTests()
        {
            m_NumberOfItems = 1000;
            m_DataSource = ref new Microsoft::UI::Xaml::Tests::Common::DataVirtualizationDataSource(m_NumberOfItems);
        }

        BEGIN_TEST_CLASS(ISelectionInfoIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"309fb554-f012-4ac9-bcf1-833e4a372493;375cd7bd-e448-4315-b2a1-bc02d75b0c4f")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //        
        BEGIN_TEST_METHOD(CanInvokeDataSourceSelectRangeAfterSelectedIndexChanged)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully invoke the SelectRange method for a ListView data source deriving from ISelectionInfo after SelectedIndex has been changed.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(CanInvokeDataSourceSelectRange)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully invoke the SelectRange method for a ListView data source deriving from ISelectionInfo.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(CanInvokeDataSourceDeselectRange)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully invoke the DeselectRange method for a ListView data source deriving from ISelectionInfo.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(CanInvokeDataSourceIsSelected)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully invoke the IsSelected method for a ListView data source deriving from ISelectionInfo.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(CanInvokeDataSourceGetSelectedRanges)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully invoke the GetSelectedRanges method for a ListView data source deriving from ISelectionInfo.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(ValidateSelectorItemIsSelectedValue)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the SelectorItem IsSelected value still functions.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(ValidateSelectorItemIsSelectedBinding)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the SelectorItem IsSelected binding still functions.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(ValidateCanChangeSelectionMode)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that changing selection mode won't break selection.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateCtrlAScenarios)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully invoke Ctrl+A scenarios to SelectAll then DeselectAll.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateCtrlAScenariosWithoutInterfaceImplementation)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully invoke Ctrl+A scenarios to SelectAll then DeselectAll when the Selection Interface is not implemented.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(ValidateClearingSelection)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can DeselectRange is called when we clear selection by setting the SelectedIndex to -1.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(ValidateSelectionAutomation)
            TEST_METHOD_PROPERTY(L"Description", L"Validates using automation to select and deselect actually calls into IISelectionInfo")
        END_TEST_METHOD()

        //
        // Platform:Desktop
        //
        BEGIN_TEST_METHOD(ValidateTapItem)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can tap to select/deselect an item.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()
        
        //
        // Platform:Phone
        //

    private:
        unsigned int m_NumberOfItems;
        Microsoft::UI::Xaml::Tests::Common::DataVirtualizationDataSource^ m_DataSource;
        xaml_controls::ListView^ SetupEnvironment();
    };

} } } } } }
