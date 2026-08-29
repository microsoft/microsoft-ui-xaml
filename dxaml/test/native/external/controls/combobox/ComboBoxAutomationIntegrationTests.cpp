// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ComboBoxAutomationIntegrationTests.h"
#include <AutomationClient\AutomationClientManager.h>
#include <collection.h>

#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <UIAutomationHelper.h>

#include <CustomPropertySupport.h>
#include <ComboBoxHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ComboBox {

    ref class PersonObject sealed : public Microsoft::UI::Xaml::Tests::Common::CustomPropertyProviderBase
    {
    public:
        PersonObject(Platform::String^ firstName, Platform::String^ lastName)
        {
            this->FirstName = firstName;
            this->LastName = lastName;
        }

        Platform::String^ GetStringRepresentation() override
        {
            return FirstName + LastName;
        }

    protected:
        void AddCustomProperties() override
        {
            AddCustomProperty(L"FirstName", Platform::String::typeid,
                MAKEPROPGET(PersonObject^, FirstName),
                MAKEPROPSET(PersonObject^, FirstName, Platform::String^)
                );

            AddCustomProperty(L"LastName", Platform::String::typeid,
                MAKEPROPGET(PersonObject^, LastName),
                MAKEPROPSET(PersonObject^, LastName, Platform::String^)
                );
        }

    public:
        property Platform::String^ FirstName;
        property Platform::String^ LastName;
    };


    bool ComboBoxAutomationIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }
     
    bool ComboBoxAutomationIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ComboBoxAutomationIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void ComboBoxAutomationIntegrationTests::ValidateSizeOfPropertiesForFaceplate()
    {
        TestCleanupWrapper cleanup;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, Loaded);

        xaml_controls::ComboBox^ comboBox = nullptr;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestComboBoxName";
        uiaInfo.m_AutomationID = L"TestComboBoxId";
        uiaInfo.m_ItemStatus = L"TestComboBox";
        uiaInfo.m_cType = UIA_PaneControlTypeId;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <ComboBox x:Name='comboBox' Background='RoyalBlue' SelectedIndex='1'  Width='350' AutomationProperties.Name='TestComboBoxName'> "
                L"    <TextBlock Text='item one' />"
                L"    <TextBlock Text='item two' />"
                L"    <TextBlock Text='item three' />"
                L"  </ComboBox>"
                L"</Grid>"));

            comboBox = dynamic_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBox"));
            VERIFY_IS_NOT_NULL(comboBox);

            loadedRegistration.Attach(comboBox, [loadedEvent]()
            {
                LOG_OUTPUT(L"ComboBox.Loaded event raised.");
                loadedEvent->Set();
            });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        LOG_OUTPUT(L"Waiting for ComboBox.Loaded event...");
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationSelectionPattern> spUIAutomationSelectionPattern;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

            LOG_OUTPUT(L"Verifying UIA Client side node for ComboBox exists.");
            VERIFY_IS_NOT_NULL(spUIAutomationElement);

            LogThrow_IfFailedWithMessage(spUIAutomationElement->GetCurrentPatternAs(UIA_SelectionPatternId, __uuidof(IUIAutomationSelectionPattern), &spUIAutomationSelectionPattern),
                 L"ComboBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed getting Selection Pattern.");

            wrl::ComPtr<IUIAutomationElementArray> spCurrentUIASelection;
            LogThrow_IfFailedWithMessage(spUIAutomationSelectionPattern->GetCurrentSelection(&spCurrentUIASelection), L"ComboBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed while getting selected Items.");

            int count = 0;
            LogThrow_IfFailedWithMessage(spCurrentUIASelection->get_Length(&count), L"ComboBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed while getting selected count.");
            LOG_OUTPUT(L"Verifying that selected count is 1.");
            VERIFY_IS_TRUE(count == 1);

            Common::AutoVariant autoVar;
            wrl::ComPtr<IUIAutomationElement> spSelectedUIAutomationElement;
            LOG_OUTPUT(L"Get the selected element in ComboBox");
            LogThrow_IfFailedWithMessage(spCurrentUIASelection->GetElement(0, &spSelectedUIAutomationElement), L"ComboBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed while getting selected item");
            VERIFY_IS_NOT_NULL(spSelectedUIAutomationElement);

            spSelectedUIAutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            LOG_OUTPUT(L"Verifying PositionInSet is 1.");
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 2);
            spSelectedUIAutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
            LOG_OUTPUT(L"Verifying SizeOfSet is 3.");
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == 3);
            spSelectedUIAutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
            LOG_OUTPUT(L"Verifying Level is -1.");
            VERIFY_IS_TRUE(autoVar.Storage()->lVal == -1);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void ComboBoxAutomationIntegrationTests::ValidateDefaultAutomationNameFallback()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ComboBox^ cbDirect = nullptr;
        xaml_controls::ComboBox^ cbDisplayMember = nullptr;
        xaml_controls::ComboBox^ cbStraightList = nullptr;
        xaml_controls::ComboBox^ cbStraightListWithDataTemplate = nullptr;
        xaml_controls::ComboBox^ cbStraightListWithOldDefault = nullptr;

        RunOnUIThread([&]()
        {
            Platform::Collections::Vector<PersonObject^>^ itemList = ref new Platform::Collections::Vector<PersonObject^>();
            itemList->Append(ref new PersonObject(L"Roger", L"Federer"));

            auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <ComboBox x:Name='cbDirect' AutomationProperties.AutomationId='cbDirect' Width='350'> "
                L"    <TextBlock Text='item one' AutomationProperties.Name='test' />"
                L"  </ComboBox>"
                L"  <ComboBox x:Name='cbDisplayMember' AutomationProperties.AutomationId='cbDisplayMember' Width='350' />"
                L"  <ComboBox x:Name='cbStraightList' AutomationProperties.AutomationId='cbStraightList' Width='350' />"
                L"  <ComboBox x:Name='cbStraightListWithDataTemplate' AutomationProperties.AutomationId='cbStraightListWithDataTemplate' Width='350'>"
                L"    <ComboBox.ItemTemplate><DataTemplate><Grid AutomationProperties.Name='{Binding LastName}'><TextBlock Text='{Binding FirstName}'/></Grid></DataTemplate></ComboBox.ItemTemplate>"
                L"  </ComboBox>"
                L"  <ComboBox x:Name='cbStraightListWithOldDefault' AutomationProperties.AutomationId='cbStraightListWithOldDefault' Width='350'>"
                L"    <ComboBox.ItemTemplate><DataTemplate><Grid><TextBlock Text='{Binding FirstName}'/></Grid></DataTemplate></ComboBox.ItemTemplate>"
                L"  </ComboBox>"
                L"</StackPanel>"));

            cbDirect = safe_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"cbDirect"));
            cbDisplayMember = safe_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"cbDisplayMember"));
            cbStraightList = safe_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"cbStraightList"));
            cbStraightListWithDataTemplate = safe_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"cbStraightListWithDataTemplate"));
            cbStraightListWithOldDefault = safe_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"cbStraightListWithOldDefault"));

            cbDisplayMember->ItemsSource = itemList;
            cbDisplayMember->DisplayMemberPath = L"LastName";
            cbStraightList->ItemsSource = itemList;
            cbStraightListWithDataTemplate->ItemsSource = itemList;
            cbStraightListWithOldDefault->ItemsSource = itemList;
            
            cbDirect->SelectedIndex = 0;
            cbDisplayMember->SelectedIndex = 0;
            cbStraightList->SelectedIndex = 0;
            cbStraightListWithDataTemplate->SelectedIndex = 0;
            cbStraightListWithOldDefault->SelectedIndex = 0;

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            VerifySelectedUIAutomationElementNameMatchesExpectedString(L"cbDirect", L"test");
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            VerifySelectedUIAutomationElementNameMatchesExpectedString(L"cbDisplayMember", L"Federer");
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            VerifySelectedUIAutomationElementNameMatchesExpectedString(L"cbStraightList", L"RogerFederer");
        });
        TestServices::WindowHelper->WaitForIdle();
        
        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            VerifySelectedUIAutomationElementNameMatchesExpectedString(L"cbStraightListWithDataTemplate", L"Federer");
        });
        TestServices::WindowHelper->WaitForIdle();
        
        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            VerifySelectedUIAutomationElementNameMatchesExpectedString(L"cbStraightListWithOldDefault", L"Roger");
        });        
    }

    void ComboBoxAutomationIntegrationTests::VerifyAutomationWindowPattern()
    {
        TestCleanupWrapper cleanup;

        auto loadedEvent = std::make_shared<Event>();
        auto dropDownOpenedEvent = std::make_shared<Event>();
        auto dropDownClosedEvent = std::make_shared<Event>();

        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, Loaded);
        auto dropDownOpenedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownOpened);
        auto dropDownClosedRegistration = CreateSafeEventRegistration(xaml_controls::ComboBox, DropDownClosed);

        xaml_controls::ComboBox^ comboBox;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                LR"(<Grid x:Name="root" xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                       <ComboBox x:Name="comboBox" SelectedIndex="1"  Width="350" AutomationProperties.Name="TestComboBoxName">
                          <TextBlock Text="item one" />
                          <TextBlock Text="item two" />
                          <TextBlock Text="item three" />
                       </ComboBox>
                    </Grid>)"));

            comboBox = safe_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBox"));
            VERIFY_IS_NOT_NULL(comboBox);

            loadedRegistration.Attach(comboBox, [loadedEvent]()
            {
                LOG_OUTPUT(L"ComboBox.Loaded event raised.");
                loadedEvent->Set();
            });

            dropDownOpenedRegistration.Attach(comboBox, [dropDownOpenedEvent]()
            {
                LOG_OUTPUT(L"ComboBox.DropDownOpened event raised.");
                dropDownOpenedEvent->Set();
            });

            dropDownClosedRegistration.Attach(comboBox, [dropDownClosedEvent]()
            {
                LOG_OUTPUT(L"ComboBox.DropDownClosed event raised.");
                dropDownClosedEvent->Set();
            });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        LOG_OUTPUT(L"Waiting for ComboBox.Loaded event...");
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            comboBox->IsDropDownOpen = true;
        });

        LOG_OUTPUT(L"Waiting for ComboBox.DropDownOpened event...");
        dropDownOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestComboBoxName";

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationComboBox;
            wrl::ComPtr<IUIAutomationWindowPattern> spUIAutomationComboBoxWindowPattern;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationComboBox);

            LOG_OUTPUT(L"Verifying UIA Client side node for ComboBox exists.");
            VERIFY_IS_NOT_NULL(spUIAutomationComboBox);

            LOG_OUTPUT(L"Verifying that the ComboBox supports the Window pattern when it is opened.");
            spUIAutomationComboBox->GetCurrentPatternAs(UIA_WindowPatternId, __uuidof(IUIAutomationWindowPattern), &spUIAutomationComboBoxWindowPattern);

            VERIFY_IS_NOT_NULL(spUIAutomationComboBoxWindowPattern);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto comboBoxAP = safe_cast<Microsoft::UI::Xaml::Automation::Provider::IWindowProvider^>(xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(comboBox));

            // Verify IWindowProdiver properties
            VERIFY_IS_TRUE(comboBoxAP->IsModal);
            VERIFY_IS_TRUE(comboBoxAP->IsTopmost);
            VERIFY_IS_FALSE(comboBoxAP->Maximizable);
            VERIFY_IS_FALSE(comboBoxAP->Minimizable);
            VERIFY_ARE_EQUAL(comboBoxAP->InteractionState, xaml_automation::WindowInteractionState::Running);
            VERIFY_ARE_EQUAL(comboBoxAP->VisualState, xaml_automation::WindowVisualState::Normal);

            // Verify IWindowProvider methods that don't do anything
            comboBoxAP->Close();
            comboBoxAP->SetVisualState(xaml_automation::WindowVisualState::Normal);
            comboBoxAP->WaitForInputIdle(0 /* milliseconds */);

            comboBox->IsDropDownOpen = false;
        });

        LOG_OUTPUT(L"Waiting for ComboBox.DropDownClosed event...");
        dropDownClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationComboBox;
            wrl::ComPtr<IUIAutomationWindowPattern> spUIAutomationComboBoxWindowPattern;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationComboBox);

            LOG_OUTPUT(L"Verifying UIA Client side node for ComboBox exists.");
            VERIFY_IS_NOT_NULL(spUIAutomationComboBox);

            LOG_OUTPUT(L"Verifying that the ComboBox doesn't support the Window pattern when it is closed.");
            spUIAutomationComboBox->GetCurrentPatternAs(UIA_WindowPatternId, __uuidof(IUIAutomationWindowPattern), &spUIAutomationComboBoxWindowPattern);

            VERIFY_IS_NULL(spUIAutomationComboBoxWindowPattern);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void ComboBoxAutomationIntegrationTests::VerifyClosedComboBoxReportsCorrectNameWithDisplayMemberPath()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ComboBox^ comboBox = nullptr;
        Platform::String^ firstNameString = L"Roger";
        Platform::String^ lastNameString = L"Federer";

        RunOnUIThread([&]()
        {
            Platform::Collections::Vector<PersonObject^>^ itemList = ref new Platform::Collections::Vector<PersonObject^>();
            itemList->Append(ref new PersonObject(firstNameString, lastNameString));

            auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <ComboBox x:Name='comboBox' AutomationProperties.AutomationId='comboBox' Width='350' />"
                L"</StackPanel>"));

            comboBox = safe_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBox"));

            comboBox->ItemsSource = itemList;
            comboBox->DisplayMemberPath = L"LastName";
            comboBox->SelectedIndex = 0;

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            VerifySelectedUIAutomationElementNameMatchesExpectedString(L"comboBox", lastNameString);
        });
        TestServices::WindowHelper->WaitForIdle(); 

        // Open and close the ComboBox to realize the items. 
        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);
        ComboBoxHelper::CloseComboBox(comboBox);

        // After opening, the items have been realized. Ensure that the realized item still reports the correct value.
        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            VerifySelectedUIAutomationElementNameMatchesExpectedString(L"comboBox", lastNameString);
        });
        TestServices::WindowHelper->WaitForIdle(); 
    }

    void ComboBoxAutomationIntegrationTests::VerifyFaceplateContentPresenterAutomationPeerName()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ComboBox^ comboBox = nullptr;
        Platform::Collections::Vector<Platform::String^>^ itemList = nullptr;

        RunOnUIThread([&]()
        {
            itemList = ref new Platform::Collections::Vector<Platform::String^>();
            itemList->Append(L"Bandol");
            itemList->Append(L"Cassis");
            itemList->Append(L"Cogolin");
            itemList->Append(L"Gassin");
            itemList->Append(L"Grimaux");
            itemList->Append(L"Ramatuelle");
            itemList->Append(L"Toulon");

            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                L"<StackPanel x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <ComboBox x:Name='comboBox' AutomationProperties.AutomationId='comboBox' Width='150' />"
                L"</StackPanel>"));

            comboBox = safe_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBox"));
            comboBox->ItemsSource = itemList;
            comboBox->SelectedIndex = 0;

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        for (int i = 0; i < 6; i++)
        {
            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                VerifySelectedUIAutomationElementNameMatchesExpectedString(L"comboBox", itemList->GetAt(i));
            });
            
            TestServices::WindowHelper->WaitForIdle();
            
            RunOnUIThread([&]()
            {
                comboBox->SelectedIndex++;
            });

            TestServices::WindowHelper->WaitForIdle();            
        }
    }

    void ComboBoxAutomationIntegrationTests::VerifyClosedComboBoxReportsCorrectAutomationNameWhenSet()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ComboBox^ comboBox = nullptr;
        Platform::String^ firstNameString = L"Roger";
        Platform::String^ lastNameString = L"Federer";

        RunOnUIThread([&]()
        {
            Platform::Collections::Vector<PersonObject^>^ itemList = ref new Platform::Collections::Vector<PersonObject^>();
            itemList->Append(ref new PersonObject(firstNameString, lastNameString));

            auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                LR"(<StackPanel x:Name="root" xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <ComboBox x:Name="comboBox" AutomationProperties.AutomationId='comboBox' Width="350">
                            <ComboBox.ItemTemplate>
                                <DataTemplate>
                                    <StackPanel Orientation="Horizontal" AutomationProperties.Name="{Binding FirstName}">
                                        <Rectangle Width="20" Height="20" Fill="Orange" />
                                        <TextBlock Text="{Binding FirstName}" />
                                    </StackPanel>
                                </DataTemplate>
                            </ComboBox.ItemTemplate>
                        </ComboBox>
                    </StackPanel>)"));

            comboBox = safe_cast<xaml_controls::ComboBox^>(rootPanel->FindName(L"comboBox"));

            comboBox->ItemsSource = itemList;
            comboBox->SelectedIndex = 0;

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            VerifySelectedUIAutomationElementNameMatchesExpectedString(L"comboBox", firstNameString);
        });
        TestServices::WindowHelper->WaitForIdle(); 

        // Open and close the ComboBox to realize the items. 
        ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);
        ComboBoxHelper::CloseComboBox(comboBox);

        // After opening, the items have been realized. Ensure that the realized item still reports the correct value.
        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            VerifySelectedUIAutomationElementNameMatchesExpectedString(L"comboBox", firstNameString);
        });
        TestServices::WindowHelper->WaitForIdle(); 
    }
    
    void ComboBoxAutomationIntegrationTests::VerifyComboBoxDoesNotStopHeaderNavigation()
    {
        TestCleanupWrapper cleanup;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"FirstTextBox";

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"  <TextBox AutomationProperties.Name='FirstTextBox' />"
                L"  <TextBlock Text='First heading' AutomationProperties.HeadingLevel='Level1' />"
                L"  <TextBlock Text='Second heading' AutomationProperties.HeadingLevel='Level1' />"
                L"  <ComboBox AutomationProperties.Name='TestComboBox' SelectedIndex='0' Width='350'>"
                L"    <ComboBoxItem AutomationProperties.Name='ComboBoxItem1'>Item 1</ComboBoxItem>"
                L"    <ComboBoxItem AutomationProperties.Name='ComboBoxItem2'>Item 2</ComboBoxItem>"
                L"    <ComboBoxItem AutomationProperties.Name='ComboBoxItem3'>Item 3</ComboBoxItem>"
                L"  </ComboBox>"
                L"  <TextBlock Text='Third heading' AutomationProperties.HeadingLevel='Level1' />"
                L"  <TextBox />"
                L"</StackPanel>"));

            loadedRegistration.Attach(rootPanel, [loadedEvent]()
            {
                LOG_OUTPUT(L"StackPanel.Loaded event raised.");
                loadedEvent->Set();
            });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        LOG_OUTPUT(L"Waiting for StackPanel.Loaded event...");
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomation> automation;
            wrl::ComPtr<IUIAutomationElement> currentElement;
            wrl::ComPtr<IUIAutomationElement> windowElement;
            wrl::ComPtr<IUIAutomationElementArray> headers;
            int headerCount;

            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            automationClientManager->GetAutomation(&automation);
            LOG_OUTPUT(L"Verifying IUIAutomation for TextBox exists.");
            VERIFY_IS_NOT_NULL(automation);

            automationClientManager->GetCurrentUIAutomationElement(&currentElement);
            LOG_OUTPUT(L"Verifying IUIAutomationElement for TextBox exists.");
            VERIFY_IS_NOT_NULL(currentElement);

            windowElement.Attach(automationClientManager->GetParent(currentElement.Get()));
            
            wrl::ComPtr<IUIAutomationCondition> hasNoHeadingCondition;
            Common::AutoVariant headingLevel;
            headingLevel.SetInt(HeadingLevel_None);
            
            THROW_IF_FAILED(automation->CreatePropertyCondition(UIA_HeadingLevelPropertyId, *(headingLevel.Storage()), &hasNoHeadingCondition));

            wrl::ComPtr<IUIAutomationCondition> searchCondition;
            THROW_IF_FAILED(automation->CreateNotCondition(hasNoHeadingCondition.Get(), &searchCondition));
            LogThrow_IfFailed(windowElement->FindAll(TreeScope_Subtree, searchCondition.Get(), &headers));
            LogThrow_IfFailed(headers->get_Length(&headerCount));
            VERIFY_ARE_EQUAL(3, headerCount);
            
            wrl::ComPtr<IUIAutomationElement> headerItem;
            AutoBSTR name;
            
            LogThrow_IfFailed(headers->GetElement(0, &headerItem));
            headerItem->get_CurrentName(name.ReleaseAndGetAddressOf());
            AutoBSTR::VerifyAreEqual(L"First heading", name);
            
            LogThrow_IfFailed(headers->GetElement(1, &headerItem));
            headerItem->get_CurrentName(name.ReleaseAndGetAddressOf());
            AutoBSTR::VerifyAreEqual(L"Second heading", name);
            
            LogThrow_IfFailed(headers->GetElement(2, &headerItem));
            headerItem->get_CurrentName(name.ReleaseAndGetAddressOf());
            AutoBSTR::VerifyAreEqual(L"Third heading", name);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void ComboBoxAutomationIntegrationTests::VerifyEditableComboBoxHasTextBox()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ComboBox^ defaultComboBox = nullptr;
        xaml_controls::ComboBox^ editableComboBox = nullptr;

        RunOnUIThread([&]()
        {
            auto stackPanel = ref new xaml_controls::StackPanel();
            defaultComboBox = ref new xaml_controls::ComboBox();
            editableComboBox = ref new xaml_controls::ComboBox();
            editableComboBox->IsEditable = true;

            stackPanel->Children->Append(defaultComboBox);
            stackPanel->Children->Append(editableComboBox);
            TestServices::WindowHelper->WindowContent = stackPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Verify default ComboBox's AutomationPeer has no children.");
            auto defaultComboBoxAutomationPeer = xaml_automation_peers::ComboBoxAutomationPeer::CreatePeerForElement(defaultComboBox);
            VERIFY_ARE_EQUAL(defaultComboBoxAutomationPeer->GetChildren()->Size, 0u);

            LOG_OUTPUT(L"Verify editable ComboBox's AutomationPeer has one child and that it is a TextBox.");
            auto editableComboBoxAutomationPeer = xaml_automation_peers::ComboBoxAutomationPeer::CreatePeerForElement(editableComboBox);
            VERIFY_ARE_EQUAL(editableComboBoxAutomationPeer->GetChildren()->Size, 1u);
            auto textBoxAutomationPeer = dynamic_cast<xaml_automation_peers::TextBoxAutomationPeer^>(editableComboBoxAutomationPeer->GetChildren()->GetAt(0));
            VERIFY_IS_NOT_NULL(textBoxAutomationPeer);
        });
        TestServices::WindowHelper->WaitForIdle();
    }
    
    void ComboBoxAutomationIntegrationTests::VerifySelectedUIAutomationElementNameMatchesExpectedString(Platform::String^ automationId, Platform::String^ expectedString)
    {
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_AutomationID = automationId->Data();

        wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
        wrl::ComPtr<IUIAutomationSelectionPattern> spUIAutomationSelectionPattern;

        auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfoId(uiaInfo);
        spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

        VERIFY_IS_NOT_NULL(spUIAutomationElement);
        LogThrow_IfFailedWithMessage(spUIAutomationElement->GetCurrentPatternAs(UIA_SelectionPatternId, __uuidof(IUIAutomationSelectionPattern), &spUIAutomationSelectionPattern), L"Failed getting Selection Pattern.");

        wrl::ComPtr<IUIAutomationElementArray> spCurrentUIASelection;
        LogThrow_IfFailedWithMessage(spUIAutomationSelectionPattern->GetCurrentSelection(&spCurrentUIASelection), L"Failed while getting selected Items.");

        Common::AutoVariant selectedUIAutomationElementName;
        wrl::ComPtr<IUIAutomationElement> spSelectedUIAutomationElement;
        LOG_OUTPUT(L"Get the selected element in ComboBox");
        LogThrow_IfFailedWithMessage(spCurrentUIASelection->GetElement(0, &spSelectedUIAutomationElement), L"Failed while getting selected item");
        
        VERIFY_IS_NOT_NULL(spSelectedUIAutomationElement);
        spSelectedUIAutomationElement->GetCurrentPropertyValue(UIA_NamePropertyId, selectedUIAutomationElementName.ReleaseAndGetAddressOf());
        
        VERIFY_IS_TRUE(expectedString->Data(), (selectedUIAutomationElementName.Storage())->bstrVal);
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::ComboBox
