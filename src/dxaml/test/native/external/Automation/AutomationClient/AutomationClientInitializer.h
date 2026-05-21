// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <UIAutomation.h>
#include "AutomationClientManager.h"
#include "AutomationEventHandler.h"

using namespace test_infra;
using namespace ::Windows::UI::Core;
using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationClient {

    class AutomationClientInitializer
    {
    public:
        AutomationClientInitializer()
        {
        }

        // Temporary fix for UiaEndpoint synchronization. Must be called before any other Uia
        // operations to ensure the UiaEndpoint registration is complete before proceeding.
        static void TEMP_WaitForDefault()
        {
            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                auto automationClientManager = AutomationClientManager::CreateAutomationClientManagerFromWindow();

                // Find the root automation provider
                wrl::ComPtr<IUIAutomationElement> windowElement;
                automationClientManager->GetCurrentUIAutomationElement(&windowElement);

                // We should only be listening for a structure change event if UiaEndpoints are
                // enabled. ContentIslands UiaEndpoints will return a root provider to sihost
                // UiaManager but the UiaTree may not be complete.
                if (!automationClientManager->HasChildren(windowElement.Get()))
                {
                    // The root provider will raise a StructureChangedEvent_ChildAdded once the
                    // UiaEndpoint registration is complete. This event is only raised once.
                    auto structureChangeEvent = std::make_shared<Event>();
                    wrl::ComPtr<AutomationStructureChangeHandler> automationStructureChangeHandler;
                    automationStructureChangeHandler.Attach(
                        new AutomationStructureChangeHandler(
                            automationClientManager,
                            structureChangeEvent,
                            TreeScope_Subtree));
                    automationStructureChangeHandler->Init(
                        StructureChangeType::StructureChangeType_ChildAdded);
                    automationStructureChangeHandler->AttachEventHandler();
                    automationStructureChangeHandler->Confirm();
                }
            });
        }

        static void TEMP_WaitForOpenWindowedPopup(xaml::UIElement^ xamlRootReferenceObject, UINT index = 0)
        {
            HWND hwnd = AutomationClientManager::GetHwndFromOpenWindowedPopup(xamlRootReferenceObject, index);

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                auto automationClientManager = std::make_shared<AutomationClientManager>();

                wrl::ComPtr<IUIAutomation> automation;
                automationClientManager->GetAutomation(&automation);

                // Find the root automation provider
                wrl::ComPtr<IUIAutomationElement> windowElement;
                VERIFY_SUCCEEDED(automation->ElementFromHandle(hwnd, &windowElement)); 

                automationClientManager->SetCurrentUIAutomationElement(windowElement.Get());

                // We should only be listening for a structure change event if UiaEndpoints are
                // enabled. ContentIslands UiaEndpoints will return a root provider to sihost
                // UiaManager but the UiaTree may not be complete.
                if (!automationClientManager->HasChildren(windowElement.Get()))
                {
                    // The root provider will raise a StructureChangedEvent_ChildAdded once the
                    // UiaEndpoint registration is complete. This event is only raised once.
                    auto structureChangeEvent = std::make_shared<Event>();
                    wrl::ComPtr<AutomationStructureChangeHandler> automationStructureChangeHandler;
                    automationStructureChangeHandler.Attach(
                        new AutomationStructureChangeHandler(
                            automationClientManager,
                            structureChangeEvent,
                            TreeScope_Subtree));
                    automationStructureChangeHandler->Init(
                        StructureChangeType::StructureChangeType_ChildAdded);
                    automationStructureChangeHandler->AttachEventHandler();
                    automationStructureChangeHandler->Confirm();
                }
            });
        }
    };
} } } } } } // namespace Microsoft::UI::Xaml::Tests::Automation::AutomationClient
