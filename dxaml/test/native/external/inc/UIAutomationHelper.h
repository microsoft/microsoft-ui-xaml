// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <UIAutomation.h>
#include <XamlTailored.h>

#include <AutoVariant.h>
#include <AutoBSTR.h>
#include <FeatureFlags.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    class UIAutomationHelper
    {
    public:

        static bool DoesInProcClientRunOnBackgroundThread()
        {
#if WI_IS_FEATURE_PRESENT(Feature_UiaEndpointsForContentIslands)
            // WPF Mode tests for UIA Endpoints will go out-of-proc and must originate from a Non-UI-Thread or they will be blocked.
            return Feature_UiaEndpointsForContentIslands::IsEnabled() && test_infra::TestServices::Utilities->IsWPF;
#else
            return false;
#endif
        }

        static void RunOnCorrectThreadForUIA(std::function<void()> func)
        {
            // UIA tests on OneCore have different threading requirements to UIA tests on Desktop.
            // On Desktop, UIA calls that will get handled by WUX must be made on the UI Thread as the in-proc call will be executed
            // directly.
            // On other platforms, the same calls must will go out-of-proc before calling back into the xaml process. In this case the UI Thread
            // must not be blocked, or the call will not be able to be services. As a result the call to the UIA API must be made
            // on the Non-UI-Thread.
            if (!test_infra::TestServices::Utilities->IsDesktop)
            {
                func();
            }
            else if (DoesInProcClientRunOnBackgroundThread())
            {
                func();
            }
            else
            {
                RunOnUIThread(func);
            }
        }

        static wrl::ComPtr<IUIAutomationElementArray> GetChildren(_In_ IUIAutomation *pAutomation, _In_ IUIAutomationElement *pElement)
        {
            wrl::ComPtr<IUIAutomationCacheRequest> spAutomationCacheRequest;
            wrl::ComPtr<IUIAutomationCondition> spUIAutomationCondition;
            wrl::ComPtr<IUIAutomationElement> spCachedElement;
            wrl::ComPtr<IUIAutomationElementArray> spItems;

            LogThrow_IfFailedWithMessage(pAutomation->CreateCacheRequest(&spAutomationCacheRequest), L"Failed to create CacheRequest.");
            LogThrow_IfFailedWithMessage(pAutomation->CreateTrueCondition(&spUIAutomationCondition), L"Failed to create PropertyCondition.");
            LogThrow_IfFailedWithMessage(spAutomationCacheRequest->put_TreeFilter(spUIAutomationCondition.Get()), L"Failed to set CacheRequest.TreeFilter.");
            LogThrow_IfFailedWithMessage(spAutomationCacheRequest->put_TreeScope(TreeScope_Children), L"Failed to set CacheRequest.TreeScope.");

            LogThrow_IfFailedWithMessage(pElement->BuildUpdatedCache(spAutomationCacheRequest.Get(), &spCachedElement), L"Failed in applying CacheRequest to element.");
            LogThrow_IfFailedWithMessage(spCachedElement->GetCachedChildren(&spItems), L"Failed to get cached children.");

            return spItems;
        }

        static wrl::ComPtr<IUIAutomationElement> GetSingleSelectedItem(_In_ IUIAutomationSelectionPattern *pSelectionPattern)
        {
            wrl::ComPtr<IUIAutomationElementArray> spSelectedItems;
            wrl::ComPtr<IUIAutomationElement> spSelectedItem;
            int itemCount = 0;

            LogThrow_IfFailedWithMessage(pSelectionPattern->GetCurrentSelection(&spSelectedItems), L"Failed to get current selection.");

            LogThrow_IfFailedWithMessage(spSelectedItems->get_Length(&itemCount), L"Failed to retrieve item count.");
            VERIFY_ARE_EQUAL(1, itemCount);
            LogThrow_IfFailedWithMessage(spSelectedItems->GetElement(0, &spSelectedItem), L"Failed to retrieve selected item.");

            return spSelectedItem;
        }

        static void VerifyElementAtIndexIsSelected(_In_ int expectedIndex, _In_ IUIAutomation *pAutomation, _In_ IUIAutomationElement *pSelectionElement)
        {
            wrl::ComPtr<IUIAutomationSelectionPattern> spSelectionPattern;
            wrl::ComPtr<IUIAutomationElement> spSelectedItem;
            wrl::ComPtr<IUIAutomationElement> spExpectedSelectedItem;
            wrl::ComPtr<IUIAutomationElementArray> spItems;
            int itemCount = 0;

            LogThrow_IfFailedWithMessage(pSelectionElement->GetCurrentPatternAs(UIA_SelectionPatternId, __uuidof(IUIAutomationSelectionPattern), &spSelectionPattern), L"Failed in fetching element's SelectionPattern.");
            VERIFY_IS_NOT_NULL(spSelectionPattern);

            spSelectedItem = GetSingleSelectedItem(spSelectionPattern.Get());

            spItems = GetChildren(pAutomation, pSelectionElement);
            VERIFY_IS_NOT_NULL(spItems);

            LogThrow_IfFailedWithMessage(spItems->get_Length(&itemCount), L"Failed to retrieve item count.");
            VERIFY_IS_LESS_THAN(expectedIndex, itemCount);

            LogThrow_IfFailedWithMessage(spItems->GetElement(expectedIndex, &spExpectedSelectedItem), L"Failed to retrieve expected selected item.");
            VERIFY_IS_NOT_NULL(spExpectedSelectedItem);

            VerifyAutomationElementsAreEqualByName(spExpectedSelectedItem.Get(), spSelectedItem.Get());
        }

        static void VerifyAutomationElementsAreEqualByName(_In_ IUIAutomationElement *pItem1, _In_ IUIAutomationElement *pItem2)
        {
            BSTR item1Name;
            BSTR item2Name;

            LOG_OUTPUT(L"Retrieving automation element names for comparison.");

            VERIFY_SUCCEEDED(pItem1->get_CurrentName(&item1Name));
            VERIFY_SUCCEEDED(pItem2->get_CurrentName(&item2Name));

            LOG_OUTPUT(L"Item 1 name = '%s', item 2 name = '%s'", item1Name, item2Name);

            VERIFY_ARE_EQUAL(VARCMP_EQ, VarBstrCmp(item1Name, item2Name, GetUserDefaultLCID(), 0));
        }

        static Platform::String^ StringFromVariant(LPVARIANT var)
        {
            VERIFY_ARE_EQUAL(VT_BSTR, var->vt);

            return ref new Platform::String(var->bstrVal);
        }

        static bool BoolFromVariant(LPVARIANT var)
        {
            VERIFY_ARE_EQUAL(VT_BOOL, var->vt);

            return !!var->boolVal;
        }

        static wf::Point PointFromVariant(LPVARIANT var)
        {
            VERIFY_ARE_EQUAL(VT_ARRAY | VT_R8, var->vt);

            LPSAFEARRAY safeArray = var->parray;

            // NB: Contrary to expectations based on the name, cbElements represents the
            // size of a single element in this array in bytes, not the total byte count in the array.
            // The element count is found in rgsabound[0].cElements.  cDims represents
            // how many dimensions there are in the array - e.g., int a[2][5] would have cDims = 2.
            // For reasons that are unclear, ClickablePoint returns an array of two *doubles*,
            // despite the fact that wf::Point is a struct of two floats.
            VERIFY_ARE_EQUAL(8ul, safeArray->cbElements);
            VERIFY_ARE_EQUAL(1u, safeArray->cDims);
            VERIFY_ARE_EQUAL(2u, safeArray->rgsabound[0].cElements);

            double* dataAsDoubles = reinterpret_cast<double*>(safeArray->pvData);

            return wf::Point(
                (float)dataAsDoubles[0],
                (float)dataAsDoubles[1]);
        }

        static wf::Rect RectFromVariant(LPVARIANT var)
        {
            VERIFY_ARE_EQUAL(VT_ARRAY | VT_R8, var->vt);

            LPSAFEARRAY safeArray = var->parray;

            // NB: Contrary to expectations based on the name, cbElements represents the
            // size of a single element in this array in bytes, not the total byte count in the array.
            // The element count is found in rgsabound[0].cElements.  cDims represents
            // how many dimensions there are in the array - e.g., int a[2][5] would have cDims = 2.
            // For reasons that are unclear, BoundingRectangle returns an array of four *doubles*,
            // despite the fact that wf::Rect is a struct of four floats.
            VERIFY_ARE_EQUAL(8ul, safeArray->cbElements);
            VERIFY_ARE_EQUAL(1u, safeArray->cDims);
            VERIFY_ARE_EQUAL(4u, safeArray->rgsabound[0].cElements);

            double* dataAsDoubles = reinterpret_cast<double*>(safeArray->pvData);

            return wf::Rect(
                (float)dataAsDoubles[0],
                (float)dataAsDoubles[1],
                (float)dataAsDoubles[2],
                (float)dataAsDoubles[3]);
        }
    };

} } } } }
