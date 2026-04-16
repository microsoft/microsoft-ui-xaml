// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "MoCoAutomationGroupingIntegrationTests.h"
#include "Employee.h"
#include <SafeEventRegistration.h>
#include <ItemsControlHelper.h>
#include <TestEvent.h>
#include <MocoHelper.h>
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <AutomationClient\AutomationClientManager.h>
#include <UIAutomationHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace Tests::Native::External::Controls::MoCo;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml {
            namespace Tests {
                namespace Controls {
                    namespace MoCo {

                        bool MoCoAutomationGroupingIntegrationTests::ClassSetup()
                        {
                            CommonTestSetupHelper::CommonTestClassSetup();
                            return true;
                        }

    bool MoCoAutomationGroupingIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

                        bool MoCoAutomationGroupingIntegrationTests::TestCleanup()
                        {
                            test_infra::TestServices::WindowHelper->ShutdownXaml();
                            TestServices::WindowHelper->VerifyTestCleanup();
                            return true;
                        }

                        //
                        // Test Cases
                        //
                        void MoCoAutomationGroupingIntegrationTests::VerifySizePosLevelFromPropertySystem()
                        {
                            TestCleanupWrapper cleanup;
                            Platform::Collections::Vector<Object^>^ data = nullptr;
                            xaml_controls::ListViewBase^ list = nullptr;
                            xaml_controls::ListViewHeaderItem^ providedHeader1 = nullptr;
                            xaml_controls::ListViewHeaderItem^ providedHeader2 = nullptr;
                            Automation::AutomationClient::UIAElementInfo uiaInfo;
                            uiaInfo.m_Name = L"TestListViewName";
                            uiaInfo.m_AutomationID = L"TestListViewId";
                            uiaInfo.m_ItemStatus = L"TestListView";
                            uiaInfo.m_cType = UIA_PaneControlTypeId;

                            auto choosingGroupHeaderContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingGroupHeaderContainer);

                            // Setup ListView with grouping and items and set PositionInSet, SizeOfSet, Level explicitly on
                            // Group headers and Items.
                            RunOnUIThread([&]() {
                                xaml_controls::Grid^ rootPanel = nullptr;
                                xaml_data::CollectionViewSource^ cvs = nullptr;

                                providedHeader1 = ref new xaml_controls::ListViewHeaderItem();
                                providedHeader2 = ref new xaml_controls::ListViewHeaderItem();

                                // grouped ListView
                                Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>" \
                                    L"   <ListView x:Name='list' >" \
                                    L"       <ListView.ItemTemplate>" \
                                    L"          <DataTemplate>" \
                                    L"              <TextBlock Text = '{Binding}' />" \
                                    L"          </DataTemplate>" \
                                    L"       </ListView.ItemTemplate>" \
                                    L"       <ListView.GroupStyle>" \
                                    L"          <GroupStyle>" \
                                    L"              <GroupStyle.HeaderTemplate>" \
                                    L"                  <DataTemplate>" \
                                    L"                      <TextBlock Text='{Binding GroupName}' Foreground='White' FontSize='20' />" \
                                    L"                  </DataTemplate>" \
                                    L"              </GroupStyle.HeaderTemplate>" \
                                    L"          </GroupStyle>" \
                                    L"       </ListView.GroupStyle>" \
                                    L"   </ListView>" \
                                    L"</Grid>";

                                rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
                                list = safe_cast<xaml_controls::ListViewBase^>(rootPanel->FindName(L"list"));
                                xaml_automation::AutomationProperties::SetName(list, ref new Platform::String(uiaInfo.m_Name));
                                xaml_automation::AutomationProperties::SetAutomationId(list, ref new Platform::String(uiaInfo.m_AutomationID));

                                // Looks like this gets called for every item for a particular header.
                                choosingGroupHeaderContainerRegistration.Attach(list,
                                    ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingGroupHeaderContainerEventArgs^>(
                                    [&](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingGroupHeaderContainerEventArgs^ args)
                                {
                                    // provide new containers based on index //
                                    unsigned int index = 0;
                                    bool foundGroup = data->IndexOf(args->Group, &index);
                                    if (foundGroup)
                                    {
                                        // check the args index, to make sure it matches in the data collection
                                        VERIFY_ARE_EQUAL(static_cast<signed int>(index), args->GroupIndex);

                                        if (index == 0)
                                        {
                                            args->GroupHeaderContainer = providedHeader1;
                                            xaml_automation::AutomationProperties::SetPositionInSet(providedHeader1, 2);
                                            xaml_automation::AutomationProperties::SetSizeOfSet(providedHeader1, 5);
                                            xaml_automation::AutomationProperties::SetLevel(providedHeader1, 2);
                                        }
                                        else if (index == 1)
                                        {
                                            args->GroupHeaderContainer = providedHeader2;
                                            xaml_automation::AutomationProperties::SetPositionInSet(providedHeader2, 3);
                                            xaml_automation::AutomationProperties::SetSizeOfSet(providedHeader2, 6);
                                            xaml_automation::AutomationProperties::SetLevel(providedHeader2, 2);
                                        }
                                        else if (index == 2)
                                        {
                                            // dont give a header. a new one
                                            // should be created by the framework.
                                        }
                                        else
                                        {
                                            // we only have two groups
                                            VERIFY_FAIL();
                                        }
                                    }
                                    else
                                    {
                                        // the group data we got in the args was incorrect
                                        VERIFY_FAIL();
                                    }
                                }));

                                data = ref new Platform::Collections::Vector<Object^>();
                                // 3 groups
                                for (int i = 0; i < 3; i++)
                                {
                                    auto group = ref new Platform::Collections::Vector<xaml_controls::ListViewItem^>();
                                    xaml_controls::ListViewItem^ listItem1 = ref new xaml_controls::ListViewItem();
                                    xaml_automation::AutomationProperties::SetPositionInSet(providedHeader1, 1 + i);
                                    xaml_automation::AutomationProperties::SetSizeOfSet(providedHeader1, 5 + i);
                                    xaml_automation::AutomationProperties::SetLevel(providedHeader1, 3);
                                    group->Append(listItem1);
                                    data->Append(group);
                                }

                                // setup the collection view source and hook it up to the listview
                                cvs = ref new xaml_data::CollectionViewSource();
                                cvs->IsSourceGrouped = true;
                                cvs->Source = data;
                                list->ItemsSource = cvs->View;

                                test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
                            });

                            test_infra::TestServices::WindowHelper->WaitForIdle();

                            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
                            {
                                wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
                                wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
                                wrl::ComPtr<IUIAutomationTreeWalker> walker;
                                wrl::ComPtr<IUIAutomation> spUIAutomation;

                                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

                                spAutomationClientManager->GetAutomation(&spUIAutomation);
                                spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

                                LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating True PropertyCondition.");
                                LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &walker), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating TreeWalker.");

                                LOG_OUTPUT(L"Verifying UIA Client side node for ListView exist.");
                                VERIFY_IS_NOT_NULL(spUIAutomationElement);

                                Common::AutoVariant autoVar;

                                LOG_OUTPUT(L"Navigate to First Item in ListView that is first group header and verify properties.");
                                wrl::ComPtr<IUIAutomationElement> header1;
                                LogThrow_IfFailedWithMessage(walker->GetFirstChildElement(spUIAutomationElement.Get(), &header1), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get first ListHeaderItem.");
                                VERIFY_IS_NOT_NULL(header1);

                                header1->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 2);
                                header1->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 5);
                                header1->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 2);

                                LOG_OUTPUT(L"Navigate to first child of the header - which is the header text");
                                wrl::ComPtr<IUIAutomationElement> headerText;
                                LogThrow_IfFailedWithMessage(walker->GetFirstChildElement(header1.Get(), &headerText), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get first child of header.");
                                VERIFY_IS_NOT_NULL(headerText);

                                LOG_OUTPUT(L"Navigate to ListViewItem1 in first header and verify properties.");
                                wrl::ComPtr<IUIAutomationElement> header1Item1;
                                LogThrow_IfFailedWithMessage(walker->GetNextSiblingElement(headerText.Get(), &header1Item1), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to get first item item.");
                                VERIFY_IS_NOT_NULL(header1Item1);

                                header1Item1->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1);
                                header1Item1->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1);
                                header1Item1->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 2);

                                LOG_OUTPUT(L"Navigate to Second Item in ListView that is second group header and verify properties.");
                                wrl::ComPtr<IUIAutomationElement> header2;
                                LogThrow_IfFailedWithMessage(walker->GetNextSiblingElement(header1.Get(), &header2), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get second ListHeaderItem.");
                                VERIFY_IS_NOT_NULL(header2);

                                header2->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 3);
                                header2->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 6);
                                header2->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 2);

                                LOG_OUTPUT(L"Navigate to first child of the header - which is the header text");
                                wrl::ComPtr<IUIAutomationElement> header2Text;
                                LogThrow_IfFailedWithMessage(walker->GetFirstChildElement(header2.Get(), &header2Text), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get first child of header.");
                                VERIFY_IS_NOT_NULL(header2Text);

                                LOG_OUTPUT(L"Navigate to ListViewItem1 in second header and verify properties.");
                                wrl::ComPtr<IUIAutomationElement> header2Item1;
                                LogThrow_IfFailedWithMessage(walker->GetNextSiblingElement(header2Text.Get(), &header2Item1), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get ListItem1 in second header.");
                                VERIFY_IS_NOT_NULL(header2Item1);

                                header2Item1->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1);
                                header2Item1->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1);
                                header2Item1->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 2);
                            });
                        }

                        void MoCoAutomationGroupingIntegrationTests::VerifySizePosLevelFromGenerated()
                        {
                            TestCleanupWrapper cleanup;
                            Automation::AutomationClient::UIAElementInfo uiaInfo;
                            uiaInfo.m_Name = L"TestListViewName";
                            uiaInfo.m_AutomationID = L"TestListViewId";
                            uiaInfo.m_ItemStatus = L"TestListView";
                            uiaInfo.m_cType = UIA_PaneControlTypeId;
                            auto list = MocoHelper::SetUpGroupedEnvironment(MocoHelper::ListControlType::ListView, 20 /* groupCount */, 4 /* itemsCount */);
                            std::shared_ptr<AutomationClient::AutomationClientManager> spAutomationClientManager;

                            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

                            RunOnUIThread([&]()
                            {
                                list->Width = 400;
                                list->Height = 300;
                                xaml_automation::AutomationProperties::SetName(list, ref new Platform::String(uiaInfo.m_Name));
                                xaml_automation::AutomationProperties::SetAutomationId(list, ref new Platform::String(uiaInfo.m_AutomationID));
                            });

                            // Wait for all async activities to be done
                            TestServices::WindowHelper->WaitForIdle();

                            RunOnUIThread([&]()
                            {
                                auto isp = dynamic_cast<xaml_controls::ItemsStackPanel^>(list->ItemsPanelRoot);
                                VERIFY_IS_NOT_NULL(isp);
                                isp->CacheLength = 3;
                            });

                            // Wait for all async activities to be done
                            TestServices::WindowHelper->WaitForIdle();

                            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
                            {
                                wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
                                wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
                                wrl::ComPtr<IUIAutomationTreeWalker> walker;
                                wrl::ComPtr<IUIAutomation> spUIAutomation;

                                spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

                                spAutomationClientManager->GetAutomation(&spUIAutomation);
                                spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

                                LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating True PropertyCondition.");
                                LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &walker), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating TreeWalker.");

                                LOG_OUTPUT(L"Verifying UIA Client side node for ListView exist.");
                                VERIFY_IS_NOT_NULL(spUIAutomationElement);

                                Common::AutoVariant autoVar;

                                LOG_OUTPUT(L"Navigate to First Item in ListView that is first group header and verify properties.");
                                wrl::ComPtr<IUIAutomationElement> spUIListViewHeaderItem1AutomationElement;
                                LogThrow_IfFailedWithMessage(walker->GetFirstChildElement(spUIAutomationElement.Get(), &spUIListViewHeaderItem1AutomationElement), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get first ListHeaderItem.");
                                VERIFY_IS_NOT_NULL(spUIListViewHeaderItem1AutomationElement);

                                spUIListViewHeaderItem1AutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1);
                                spUIListViewHeaderItem1AutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 20);
                                spUIListViewHeaderItem1AutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1);

                                LOG_OUTPUT(L"Navigate to first child of the header - which is the header text");
                                wrl::ComPtr<IUIAutomationElement> header1Text;
                                LogThrow_IfFailedWithMessage(walker->GetFirstChildElement(spUIListViewHeaderItem1AutomationElement.Get(), &header1Text), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get first child of header.");
                                VERIFY_IS_NOT_NULL(header1Text);

                                LOG_OUTPUT(L"Navigate to ListViewItem1 in first header and verify properties.");
                                wrl::ComPtr<IUIAutomationElement> spUIListViewItem1AutomationElement;
                                LogThrow_IfFailedWithMessage(walker->GetNextSiblingElement(header1Text.Get(), &spUIListViewItem1AutomationElement), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get ListItem1 in first header.");
                                VERIFY_IS_NOT_NULL(spUIListViewItem1AutomationElement);

                                spUIListViewItem1AutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1);
                                spUIListViewItem1AutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 4);
                                spUIListViewItem1AutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 2);

                                LOG_OUTPUT(L"Navigate to ListViewItem2 in first header and verify properties.");
                                wrl::ComPtr<IUIAutomationElement> spUIListViewItem2AutomationElement;
                                LogThrow_IfFailedWithMessage(walker->GetNextSiblingElement(spUIListViewItem1AutomationElement.Get(), &spUIListViewItem2AutomationElement), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get ListItem2 in first header.");
                                VERIFY_IS_NOT_NULL(spUIListViewItem2AutomationElement);

                                spUIListViewItem2AutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 2);
                                spUIListViewItem2AutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 4);
                                spUIListViewItem2AutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 2);

                                LOG_OUTPUT(L"Navigate to Second Item in ListView that is second group header and verify properties.");
                                wrl::ComPtr<IUIAutomationElement> spUIListViewHeaderItem2AutomationElement;
                                LogThrow_IfFailedWithMessage(walker->GetNextSiblingElement(spUIListViewHeaderItem1AutomationElement.Get(), &spUIListViewHeaderItem2AutomationElement), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get second ListHeaderItem.");
                                VERIFY_IS_NOT_NULL(spUIListViewHeaderItem2AutomationElement);

                                spUIListViewHeaderItem2AutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 2);
                                spUIListViewHeaderItem2AutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 20);
                                spUIListViewHeaderItem2AutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1);
                            });

                            RunOnUIThread([&]()
                            {
                                unsigned int index = list->Items->Size -1;
                                list->ScrollIntoView(list->Items->GetAt(index));
                            });

                            // Wait for all async activities to be done
                            TestServices::WindowHelper->WaitForIdle();

                            RunOnUIThread([&]()
                            {
                                // Calling update layout so the virtualization path can kick in.
                                list->UpdateLayout();

                                LOG_OUTPUT(L"Check if the last item is realized");
                                auto lastItem = list->ContainerFromIndex(list->Items->Size - 1);
                                VERIFY_IS_NOT_NULL(lastItem);

                                LOG_OUTPUT(L"Check if first item is virtualized.");
                                auto firstItem = list->ContainerFromIndex(1);
                                VERIFY_IS_NULL(firstItem);
                            });

                            // Wait for all async activities to be done
                            TestServices::WindowHelper->WaitForIdle();

                            int firstCacheIndex;

                            RunOnUIThread([&]()
                            {
                                //Initialize Test variables
                                auto isp = dynamic_cast<xaml_controls::ItemsStackPanel^>(list->ItemsPanelRoot);
                                LOG_OUTPUT(L"first cache: %d, first visible: %d, lastvisible: %d, lastcache: %d", isp->FirstCacheIndex, isp->FirstVisibleIndex, isp->LastVisibleIndex, isp->LastCacheIndex);

                                firstCacheIndex = isp->FirstCacheIndex;
                            });

                            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
                            {
                                wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
                                wrl::ComPtr<IUIAutomationElement> spUIListItemTempAAutomationElement;
                                wrl::ComPtr<IUIAutomationElement> spUIListItemTempBAutomationElement;
                                wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
                                wrl::ComPtr<IUIAutomationTreeWalker> walker;
                                wrl::ComPtr<IUIAutomation> spUIAutomation;

                                spAutomationClientManager->GetAutomation(&spUIAutomation);
                                spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

                                LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating True PropertyCondition.");
                                LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &walker), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating TreeWalker.");

                                LOG_OUTPUT(L"Verifying UIA Client side node for ListView exist.");
                                VERIFY_IS_NOT_NULL(spUIAutomationElement);

                                Common::AutoVariant autoVar;

                                LOG_OUTPUT(L"Navigate to First Item in ListView that is first cached group header after scrolling and verify properties.");
                                wrl::ComPtr<IUIAutomationElement> spUIListViewHeaderItem1AutomationElement;
                                LogThrow_IfFailedWithMessage(walker->GetFirstChildElement(spUIAutomationElement.Get(), &spUIListViewHeaderItem1AutomationElement), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get first ListHeaderItem.");
                                VERIFY_IS_NOT_NULL(spUIListViewHeaderItem1AutomationElement);
                                int firstCachedGroupPos = (firstCacheIndex / 4) + 1;
                                spUIListViewHeaderItem1AutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == firstCachedGroupPos);
                                spUIListViewHeaderItem1AutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 20);
                                spUIListViewHeaderItem1AutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1);

                                LOG_OUTPUT(L"Navigate to first child of the header - which is the header text");
                                wrl::ComPtr<IUIAutomationElement> header1Text;
                                LogThrow_IfFailedWithMessage(walker->GetFirstChildElement(spUIListViewHeaderItem1AutomationElement.Get(), &header1Text), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get first child of header.");
                                VERIFY_IS_NOT_NULL(header1Text);

                                LOG_OUTPUT(L"Navigate to ListViewItem1 in first header and verify properties.");
                                wrl::ComPtr<IUIAutomationElement> spUIListViewItem1AutomationElement;
                                LogThrow_IfFailedWithMessage(walker->GetNextSiblingElement(header1Text.Get(), &spUIListViewItem1AutomationElement), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get ListItem1 in first header.");
                                VERIFY_IS_NOT_NULL(spUIListViewItem1AutomationElement);

                                spUIListViewItem1AutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1);
                                spUIListViewItem1AutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 4);
                                spUIListViewItem1AutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 2);

                                LOG_OUTPUT(L"Navigate to ListViewItem2 in first header and verify properties.");
                                wrl::ComPtr<IUIAutomationElement> spUIListViewItem2AutomationElement;
                                LogThrow_IfFailedWithMessage(walker->GetNextSiblingElement(spUIListViewItem1AutomationElement.Get(), &spUIListViewItem2AutomationElement), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get ListItem2 in first header.");
                                VERIFY_IS_NOT_NULL(spUIListViewItem2AutomationElement);

                                spUIListViewItem2AutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 2);
                                spUIListViewItem2AutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 4);
                                spUIListViewItem2AutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 2);

                                LOG_OUTPUT(L"Navigate to Second Item in ListView that is second cached group header after scrolling and verify properties.");
                                wrl::ComPtr<IUIAutomationElement> spUIListViewHeaderItem2AutomationElement;
                                LogThrow_IfFailedWithMessage(walker->GetNextSiblingElement(spUIListViewHeaderItem1AutomationElement.Get(), &spUIListViewHeaderItem2AutomationElement), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get second ListHeaderItem.");
                                VERIFY_IS_NOT_NULL(spUIListViewHeaderItem2AutomationElement);

                                spUIListViewHeaderItem2AutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == firstCachedGroupPos + 1);
                                spUIListViewHeaderItem2AutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 20);
                                spUIListViewHeaderItem2AutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1);
                            });
                        }

                        void MoCoAutomationGroupingIntegrationTests::VerifyItemAndGroupNameProperties()
                        {
                            TestCleanupWrapper cleanup;
                            xaml_controls::SemanticZoom^ semanticZoom;

                            RunOnUIThread([&]()
                            {
                                xaml_controls::Grid^ gridRoot = SetupSemanticZoomTest();
                                semanticZoom = safe_cast<xaml_controls::SemanticZoom^>(gridRoot->FindName(L"semanticZoom"));
                                xaml_controls::ListView^ zoomedOutPatListView = safe_cast<xaml_controls::ListView^>(gridRoot->FindName(L"ZoomedOutPatListView"));
                                xaml_controls::ListView^ recordingsList = safe_cast<xaml_controls::ListView^>(gridRoot->FindName(L"RecordingsList"));
                                wfc::IObservableVector<MocoBasicGroupedDataModel^>^ _groups = ref new Platform::Collections::Vector<MocoBasicGroupedDataModel^>();

                                // Today
                                MocoBasicGroupedDataModel^ _employeeGroup1 = ref new MocoBasicGroupedDataModel("Today");

                                Employee^ _employee1 = ref new Employee();
                                _employee1->Name = "Employee11";

                                Employee^ _employee2 = ref new Employee();
                                _employee2->Name = "Employee12";

                                wfc::IObservableVector<Employee^>^ _vectorOfEmployee1 = ref new Platform::Collections::Vector<Employee^>();
                                _employeeGroup1->Append(_employee1);
                                _employeeGroup1->Append(_employee2);

                                // Yesterday
                                MocoBasicGroupedDataModel^ _employeeGroup2 = ref new MocoBasicGroupedDataModel("Yesterday");

                                Employee^ _employee21 = ref new Employee();
                                _employee21->Name = "Employee21";

                                Employee^ _employee22 = ref new Employee();
                                _employee22->Name = "Employee22";

                                _employeeGroup2->Append(_employee21);
                                _employeeGroup2->Append(_employee22);

                                _groups->Append(_employeeGroup1);
                                _groups->Append(_employeeGroup2);

                                ::Microsoft::UI::Xaml::Data::CollectionViewSource^ cvs2 = ref new ::Microsoft::UI::Xaml::Data::CollectionViewSource();
                                cvs2->Source = _groups;
                                cvs2->IsSourceGrouped = true;
                                zoomedOutPatListView->ItemsSource = cvs2->View->CollectionGroups;
                                recordingsList->ItemsSource = cvs2->View;
                            });

                            // Wait for all async activities to be done
                            TestServices::WindowHelper->WaitForIdle();

                            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
                            {
                                Automation::AutomationClient::UIAElementInfo uiaInfoZoomedInView;
                                uiaInfoZoomedInView.m_Name = L"RecordingsList";
                                uiaInfoZoomedInView.m_AutomationID = L"RecordingsList";
                                uiaInfoZoomedInView.m_ItemStatus = L"RecordingsList";
                                uiaInfoZoomedInView.m_cType = UIA_ListControlTypeId;

                                std::shared_ptr<AutomationClient::AutomationClientManager> spAutomationClientManager;
                                wrl::ComPtr<IUIAutomation> spUIAutomation;
                                wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;

                                spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoZoomedInView);
                                spAutomationClientManager->GetAutomation(&spUIAutomation);
                                spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

                                LOG_OUTPUT(L"Verifying UIA Client side node for Zoomed in ListView exist.");
                                VERIFY_IS_NOT_NULL(spUIAutomationElement);

                                wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
                                wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
                                LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"MoCoAutomationIntegrationTests::VerifyItemAndGroupNameProperties: Failed in creating True PropertyCondition.");
                                LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"MoCoAutomationIntegrationTests::VerifyItemAndGroupNameProperties: Failed in creating TreeWalker.");

                                Common::AutoVariant autoVar;

                                LOG_OUTPUT(L"Navigate to First Item in ListView that is first group header and verify it's name.");
                                wrl::ComPtr<IUIAutomationElement> spUIListViewHeaderItem1AutomationElement;
                                LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetFirstChildElement(spUIAutomationElement.Get(), &spUIListViewHeaderItem1AutomationElement), L"MoCoAutomationIntegrationTests::VerifyItemAndGroupNameProperties: Failed to Get first ListHeaderItem.");
                                VERIFY_IS_NOT_NULL(spUIListViewHeaderItem1AutomationElement);

                                spUIListViewHeaderItem1AutomationElement->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(!wcscmp(L"Today", (autoVar.Storage())->bstrVal));

                                LOG_OUTPUT(L"Navigate to ListViewItem1 in first header and verify properties.");
                                wrl::ComPtr<IUIAutomationElement> spUIListViewItem1AutomationElement;
                                LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetFirstChildElement(spUIListViewHeaderItem1AutomationElement.Get(), &spUIListViewItem1AutomationElement), L"MoCoAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed to Get ListItem1 in first header.");
                                VERIFY_IS_NOT_NULL(spUIListViewItem1AutomationElement);
                            });

                            RunOnUIThread([&]()
                            {
                                semanticZoom->ToggleActiveView();
                            });

                            // Wait for all async activities to be done
                            TestServices::WindowHelper->WaitForIdle();

                            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
                            {
                                Automation::AutomationClient::UIAElementInfo uiaInfoZoomedOutView;
                                uiaInfoZoomedOutView.m_Name = L"ZoomedOutPatListView";
                                uiaInfoZoomedOutView.m_AutomationID = L"ZoomedOutPatListView";
                                uiaInfoZoomedOutView.m_ItemStatus = L"ZoomedOutPatListView";
                                uiaInfoZoomedOutView.m_cType = UIA_ListControlTypeId;

                                std::shared_ptr<AutomationClient::AutomationClientManager> spAutomationClientManager;
                                wrl::ComPtr<IUIAutomation> spUIAutomation;
                                wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;

                                spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoZoomedOutView);
                                spAutomationClientManager->GetAutomation(&spUIAutomation);
                                spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

                                LOG_OUTPUT(L"Verifying UIA Client side node for Zoomed out ListView exist.");
                                VERIFY_IS_NOT_NULL(spUIAutomationElement);

                                wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
                                wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
                                LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"MoCoAutomationIntegrationTests::VerifyItemAndGroupNameProperties: Failed in creating True PropertyCondition.");
                                LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"MoCoAutomationIntegrationTests::VerifyItemAndGroupNameProperties: Failed in creating TreeWalker.");

                                Common::AutoVariant autoVar;

                                LOG_OUTPUT(L"Navigate to First Item in ListView that is first list item in zommed out view and verify it's name.");
                                wrl::ComPtr<IUIAutomationElement> spUIListViewItem1AutomationElement;
                                LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetFirstChildElement(spUIAutomationElement.Get(), &spUIListViewItem1AutomationElement), L"MoCoAutomationIntegrationTests::VerifyItemAndGroupNameProperties: Failed to Get first ListItem.");
                                VERIFY_IS_NOT_NULL(spUIListViewItem1AutomationElement);

                                spUIListViewItem1AutomationElement->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(!wcscmp(L"Today", (autoVar.Storage())->bstrVal));
                            });
                        }

                        xaml_controls::Grid^ MoCoAutomationGroupingIntegrationTests::SetupSemanticZoomTest()
                        {
                            auto gridRoot = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                                L"<Grid \r\n"
                                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' \r\n"
                                L"  x:Name='root' Background='#000000' Width='480' Height='768' HorizontalAlignment='Left' VerticalAlignment='Top'> \r\n"
                                L"      <SemanticZoom x:Name='semanticZoom' Width='100' Height='400'> \r\n"
                                L"          <SemanticZoom.ZoomedOutView> \r\n"
                                L"              <ListView x:Name='ZoomedOutPatListView' ScrollViewer.VerticalScrollBarVisibility='Visible' AutomationProperties.Name='ZoomedOutPatListView'> \r\n"
                                L"                  <ListView.ItemsPanel> \r\n"
                                L"                      <ItemsPanelTemplate> \r\n"
                                L"                          <StackPanel Orientation ='Vertical' VerticalAlignment ='Top'  HorizontalAlignment ='Left' /> \r\n"
                                L"                      </ItemsPanelTemplate> \r\n"
                                L"                  </ListView.ItemsPanel> \r\n"
                                L"                  <ListView.GroupStyle> \r\n"
                                L"                      <GroupStyle> \r\n"
                                L"                          <GroupStyle.HeaderContainerStyle> \r\n"
                                L"                              <Style TargetType ='ListViewHeaderItem'> \r\n"
                                L"                                  <Setter Property ='Margin' Value ='0,0,15,0' /> \r\n"
                                L"                              </Style> \r\n"
                                L"                          </GroupStyle.HeaderContainerStyle> \r\n"
                                L"                      </GroupStyle> \r\n"
                                L"                  </ListView.GroupStyle> \r\n"
                                L"              </ListView> \r\n"
                                L"          </SemanticZoom.ZoomedOutView> \r\n"
                                L"          <SemanticZoom.ZoomedInView> \r\n"
                                L"              <ListView x:Name='RecordingsList' AutomationProperties.Name='RecordingsList' Height='400'> \r\n"
                                L"                  <ListView.GroupStyle> \r\n"
                                L"                      <GroupStyle> \r\n"
                                L"                          <GroupStyle.HeaderContainerStyle> \r\n"
                                L"                              <Style TargetType = 'ListViewHeaderItem'> \r\n"
                                L"                                  <Setter Property = 'Margin' Value = '0,0,15,0' /> \r\n"
                                L"                              </Style> \r\n"
                                L"                          </GroupStyle.HeaderContainerStyle> \r\n"
                                L"                      </GroupStyle> \r\n"
                                L"                </ListView.GroupStyle> \r\n"
                                L"              </ListView> \r\n"
                                L"          </SemanticZoom.ZoomedInView> \r\n"
                                L"      </SemanticZoom> \r\n"
                                L"</Grid>"));
                            TestServices::WindowHelper->WindowContent = gridRoot;
                            return gridRoot;
                        }

                        void MoCoAutomationGroupingIntegrationTests::VerifyParentRelationInProvider()
                        {
                            TestCleanupWrapper cleanup;
                            Platform::Collections::Vector<Object^>^ data = nullptr;
                            xaml_controls::ListViewBase^ list = nullptr;
                            xaml_controls::ListViewItem^ listItem = nullptr;

                            // Setup ListView with grouping and item.
                            RunOnUIThread([&]() {
                                xaml_controls::Grid^ rootPanel = nullptr;
                                xaml_data::CollectionViewSource^ cvs = nullptr;

                                // grouped ListView
                                Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root' Margin='0,30,0,0'>" \
                                    L"   <ListView x:Name='list' Margin='0,30,0,0'>" \
                                    L"       <ListView.ItemTemplate>" \
                                    L"          <DataTemplate>" \
                                    L"              <TextBlock Text = '{Binding}' />" \
                                    L"          </DataTemplate>" \
                                    L"       </ListView.ItemTemplate>" \
                                    L"       <ListView.GroupStyle>" \
                                    L"          <GroupStyle>" \
                                    L"              <GroupStyle.HeaderTemplate>" \
                                    L"                  <DataTemplate>" \
                                    L"                      <TextBlock Text='{Binding GroupName}' Foreground='White' FontSize='20' />" \
                                    L"                  </DataTemplate>" \
                                    L"              </GroupStyle.HeaderTemplate>" \
                                    L"          </GroupStyle>" \
                                    L"       </ListView.GroupStyle>" \
                                    L"   </ListView>" \
                                    L"</Grid>";

                                rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
                                list = safe_cast<xaml_controls::ListViewBase^>(rootPanel->FindName(L"list"));


                                data = ref new Platform::Collections::Vector<Object^>();
                                // 3 groups
                                for (int i = 0; i < 3; i++)
                                {
                                    auto group = ref new Platform::Collections::Vector<xaml_controls::ListViewItem^>();
                                    listItem = ref new xaml_controls::ListViewItem();
                                    group->Append(listItem);
                                    data->Append(group);
                                }

                                // setup the collection view source and hook it up to the listview
                                cvs = ref new xaml_data::CollectionViewSource();
                                cvs->IsSourceGrouped = true;
                                cvs->Source = data;
                                list->ItemsSource = cvs->View;

                                test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
                            });

                            test_infra::TestServices::WindowHelper->WaitForIdle();

                            RunOnUIThread([&]()
                            {
                                LOG_OUTPUT(L"SetFocus on ListItem so that it's directly retrievable by UIA client. (triggers bottom up approach)");
                                listItem->Focus(FocusState::Pointer);
                            });

                            test_infra::TestServices::WindowHelper->WaitForIdle();

                            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
                            {
                                wrl::ComPtr<IUIAutomation> spAutomation;
                                wrl::ComPtr<IUIAutomationElement> spAutomationElement;

                                auto spAutomationClientManager = std::make_shared<AutomationClient::AutomationClientManager>();
                                spAutomationClientManager->GetAutomation(&spAutomation);

                                LOG_OUTPUT(L"Get the focused element from UIA client, this will be client side object corresponding to DataItemPeer");
                                spAutomation->GetFocusedElement(&spAutomationElement);
                                VERIFY_IS_NOT_NULL(spAutomationElement);

                                Common::AutoVariant autoVar;
                                spAutomationElement->GetCurrentPropertyValue(UIA_ClassNamePropertyId, autoVar.ReleaseAndGetAddressOf());
                                LOG_OUTPUT(L"ClassName of FocusedElement: %s", (autoVar.Storage())->bstrVal);
                                spAutomationElement->GetCurrentPropertyValue(UIA_ControlTypePropertyId, autoVar.ReleaseAndGetAddressOf());
                                LOG_OUTPUT(L"ControlType of FocusedElement: %d", (autoVar.Storage())->lVal);
                                wrl::ComPtr<IUIAutomationElement> spAutomationElementParent;
                                LOG_OUTPUT(L"Get Parent of ListItem 'Bottom up' approach and verify that it is a Group Control Type");
                                spAutomationElementParent.Attach(spAutomationClientManager->GetParent(spAutomationElement.Get()));
                                VERIFY_IS_NOT_NULL(spAutomationElementParent);
                                spAutomationElementParent->GetCurrentPropertyValue(UIA_ClassNamePropertyId, autoVar.ReleaseAndGetAddressOf());
                                LOG_OUTPUT(L"ClassName of Parent: %s", (autoVar.Storage())->bstrVal);
                                spAutomationElementParent->GetCurrentPropertyValue(UIA_ControlTypePropertyId, autoVar.ReleaseAndGetAddressOf());
                                LOG_OUTPUT(L"ControlType of Parent: %d", (autoVar.Storage())->lVal);
                                VERIFY_IS_TRUE(UIA_GroupControlTypeId == ((autoVar.Storage())->lVal));

                            });
                        }

                        // Verify that empty groups do not contribute to the PositionInSet and SizeOfSet automation properties when HidesIfEmpty is true
                        void MoCoAutomationGroupingIntegrationTests::EmptyGroupNotAddedToPositionInAndSizeOfSet()
                        {
                            TestCleanupWrapper cleanup;

                            Automation::AutomationClient::UIAElementInfo uiaInfo;
                            uiaInfo.m_Name = L"TestListViewName";
                            uiaInfo.m_AutomationID = L"TestListViewId";
                            uiaInfo.m_ItemStatus = L"TestListView";
                            uiaInfo.m_cType = UIA_PaneControlTypeId;

                            Platform::Collections::Vector<Object^>^ data = nullptr;
                            Platform::Collections::Vector<Object^>^ group1 = nullptr;
                            xaml_controls::ListViewBase^ list = nullptr;
                            wrl::ComPtr<IUIAutomationElement> spUIListViewHeaderItem2AutomationElement;

                            RunOnUIThread([&]() {
                                xaml_controls::Grid^ rootPanel = nullptr;
                                xaml_data::CollectionViewSource^ cvs = nullptr;

                                // Grouped ListView with HidesIfEmpty==True.
                                Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>" \
                                    L"   <ListView Height='400' x:Name='list' >" \
                                    L"       <ListView.GroupStyle>" \
                                    L"           <GroupStyle HidesIfEmpty='True'/>" \
                                    L"       </ListView.GroupStyle>" \
                                    L"   </ListView>" \
                                    L"</Grid>";

                                rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
                                list = safe_cast<xaml_controls::ListViewBase^>(rootPanel->FindName(L"list"));
                                VERIFY_IS_NOT_NULL(list);

                                xaml_automation::AutomationProperties::SetName(list, ref new Platform::String(uiaInfo.m_Name));
                                xaml_automation::AutomationProperties::SetAutomationId(list, ref new Platform::String(uiaInfo.m_AutomationID));

                                LOG_OUTPUT(L"Setting up ListView grouped data source.");
                                // Create a data source that has two groups with one item each.
                                data = ref new Platform::Collections::Vector<Object^>();
                                group1 = ref new Platform::Collections::Vector<Object^>();
                                group1->Append("Item1");
                                data->Append(group1);

                                Platform::Collections::Vector<Object^>^ group2 = ref new Platform::Collections::Vector<Object^>();
                                group2->Append("Item2");
                                data->Append(group2);

                                // Setup the collection view source and hook it up to the ListView.
                                cvs = ref new xaml_data::CollectionViewSource();
                                cvs->IsSourceGrouped = true;
                                cvs->Source = data;
                                list->ItemsSource = cvs->View;

                                test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
                            });

                            test_infra::TestServices::WindowHelper->WaitForIdle();

                            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
                            {
                                wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
                                wrl::ComPtr<IUIAutomationElement> spUIListItemTempAAutomationElement;
                                wrl::ComPtr<IUIAutomationElement> spUIListItemTempBAutomationElement;
                                wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
                                wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
                                wrl::ComPtr<IUIAutomation> spUIAutomation;

                                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

                                spAutomationClientManager->GetAutomation(&spUIAutomation);
                                spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);

                                LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"MoCoAutomationGroupingIntegrationTests::EmptyGroupNotAddedToPositionInAndSizeOfSet: Failed in creating True PropertyCondition.");
                                LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"MoCoAutomationGroupingIntegrationTests::EmptyGroupNotAddedToPositionInAndSizeOfSet: Failed in creating TreeWalker.");

                                LOG_OUTPUT(L"Verifying UIA Client side node for ListView exist.");
                                VERIFY_IS_NOT_NULL(spUIAutomationElement);

                                Common::AutoVariant autoVar;

                                LOG_OUTPUT(L"Navigating to first group header and verify its automation properties.");
                                wrl::ComPtr<IUIAutomationElement> spUIListViewHeaderItem1AutomationElement;
                                LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetFirstChildElement(spUIAutomationElement.Get(), &spUIListViewHeaderItem1AutomationElement), L"MoCoAutomationGroupingIntegrationTests::EmptyGroupNotAddedToPositionInAndSizeOfSet: Failed to get first ListHeaderItem.");
                                VERIFY_IS_NOT_NULL(spUIListViewHeaderItem1AutomationElement);

                                // Each group has one item. All groups are visible and the data indexes are expected to match the layout indexes.

                                spUIListViewHeaderItem1AutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_ARE_EQUAL(autoVar.Storage()->lVal, 1);
                                spUIListViewHeaderItem1AutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_ARE_EQUAL(autoVar.Storage()->lVal, 2);
                                spUIListViewHeaderItem1AutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_ARE_EQUAL(autoVar.Storage()->lVal, 1);

                                LOG_OUTPUT(L"Navigating to second group header and verify its automation properties.");
                                LogThrow_IfFailedWithMessage(spUIAutomationTreeWalker->GetNextSiblingElement(spUIListViewHeaderItem1AutomationElement.Get(), &spUIListViewHeaderItem2AutomationElement), L"MoCoAutomationGroupingIntegrationTests::EmptyGroupNotAddedToPositionInAndSizeOfSet: Failed to get second ListHeaderItem.");
                                VERIFY_IS_NOT_NULL(spUIListViewHeaderItem2AutomationElement);

                                LOG_OUTPUT(L"Verifying PositionInSet, SizeOfSet and Level automation properties.");
                                spUIListViewHeaderItem2AutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 2);
                                spUIListViewHeaderItem2AutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 2);
                                spUIListViewHeaderItem2AutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1);
                            });

                            test_infra::TestServices::WindowHelper->WaitForIdle();

                            RunOnUIThread([&]() {
                                LOG_OUTPUT(L"Removing item in first group.");
                                group1->RemoveAt(0);
                            });

                            test_infra::TestServices::WindowHelper->WaitForIdle();

                            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
                            {
                                // The first group has no item and is hidden. This second group's layout index is expected to be decreased by one.

                                Common::AutoVariant autoVar;

                                LOG_OUTPUT(L"Verifying new PositionInSet, SizeOfSet and Level automation properties.");
                                spUIListViewHeaderItem2AutomationElement->GetCurrentPropertyValue(UIA_PositionInSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1);
                                spUIListViewHeaderItem2AutomationElement->GetCurrentPropertyValue(UIA_SizeOfSetPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1);
                                spUIListViewHeaderItem2AutomationElement->GetCurrentPropertyValue(UIA_LevelPropertyId, autoVar.ReleaseAndGetAddressOf());
                                VERIFY_IS_TRUE(autoVar.Storage()->lVal == 1);
                            });
                        }
                    }
                }
            }
        }
    }
} // Microsoft::UI::Xaml::Tests::Controls::Moco
