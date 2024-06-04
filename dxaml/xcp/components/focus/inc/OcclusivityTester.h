// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "minxcptypes.h"
#include "DoPointerCast.h"
#include "HitTestPolygon.h"
#include "HitTestParams.h"

#undef max
#undef min

template<class RootVisualType, class UIType, class UIFrameworkCastType>
class OcclusivityTester
{
private:
    RootVisualType* m_pRootVisual;
public:
    OcclusivityTester(_In_ RootVisualType* pRootVisual) : m_pRootVisual(pRootVisual) {}

    _Check_return_ HRESULT Test(_In_ UIType* pElement, _In_ const XRECTF_RB& elementBounds, _Out_ bool* result)
    {
        HRESULT hr = S_OK;
        XUINT32 size = 0;
        UIType** pUIElements = nullptr;

        *result = true;

        {
            if (IsEmptyRectF(elementBounds))
            {
                // global bounds after clip is 0. Either our bounds are 0 or we got completely clipped.
                // we report that the element is occluded in this case.
                *result = true;
                goto Cleanup;
            }

            const XRECTF hitBounds = GetHitTestBounds(pElement, elementBounds);

            bool canHitDisabledElements = false;
            // allow HitTest on disabled element if it has AllowFocusWhenDisabled set to true
            if (pElement->template OfTypeByIndex<KnownTypeIndex::FrameworkElement>() &&
                static_cast<UIFrameworkCastType*>(pElement)->AllowFocusWhenDisabled() &&
                !pElement->IsEnabled())
            {
                canHitDisabledElements = true;
            }
            IFC(HitTestEntry(hitBounds, canHitDisabledElements, &size, &pUIElements));
        }

        if (size == 0)
        {
            // The hit test returned no elements, not even pElement.
            // This can happen when pElement is outside of the bounds of the current root
            // So for now we are going to assume pElement is not occluded
            // (Technically we dont know the occluded state of pElement, we should use a tri-state enum as return type to indicate Unknown)

            *result = false;
            goto Cleanup;
        }

        for (XUINT32 i = 0; i<size; i++)
        {
            UIFrameworkCastType* const pHitFrameworkElement = do_pointer_cast<UIFrameworkCastType>(pUIElements[i]);

            if (pHitFrameworkElement)
            {
                const auto templatedParent = pHitFrameworkElement->GetTemplatedParent();
                const bool shouldSkip = ((templatedParent && templatedParent->template OfTypeByIndex<KnownTypeIndex::ScrollBar>()) ||
                    pHitFrameworkElement->template OfTypeByIndex<KnownTypeIndex::ScrollBar>());

                if (shouldSkip) { continue; }
            }

            UIType* pHitElement = *(pUIElements + i);
            XRECTF_RB hitBounds;

            if (!pHitElement)
            {
                continue;
            }

            IFC(pHitElement->GetGlobalBoundsLogical(&hitBounds, false /* ignoreClipping */, false /* useTargetInformation */));

            if (!DoesRectContainRect(&elementBounds, &hitBounds))
            {
                *result = true;
                goto Cleanup;
            }
            else if (pHitElement == pElement)
            {
                *result = false;
                goto Cleanup;
            }
        }
    Cleanup:
        for (XUINT32 i = 0; i<size; i++)
        {
            ReleaseInterface(*(pUIElements + i));
        }
        delete[]pUIElements;
        return hr;
    }

private:
    _Check_return_ HRESULT HitTestEntry(_In_ const XRECTF& hitBounds, _In_ const bool canHitDisabledElements, _Out_ XUINT32* size, _Outptr_result_buffer_(*size) UIType*** pUIElements)
    {
        HRESULT hr = S_OK;

        CHitTestResults hitTestResults;

        HitTestPolygon polygon;
        polygon.SetRect(hitBounds);

        HitTestPerfData hitTestPerfData;
        HitTestParams hitTestParams(&hitTestPerfData);
        hitTestParams.SaveWorldSpaceHitTarget(polygon);

        IFC_RETURN(m_pRootVisual->HitTestEntry(
            hitTestParams,
            polygon,
            true, //canHitMultiple
            canHitDisabledElements, // canHitDisabledElements
            false, //canHitInvisibleElements
            &hitTestResults
            ));

        IFC_RETURN(hitTestResults.GetAnswer(size, pUIElements));

        return hr;
    }

    XRECTF GetHitTestBounds(_In_ const UIType* const element, _In_ const XRECTF_RB& candidateBounds) const
    {
        const float width = std::abs(candidateBounds.right - candidateBounds.left);
        const float height = std::abs(candidateBounds.bottom - candidateBounds.top);
        const XPOINTF centerPt = { candidateBounds.left + (width / 2), candidateBounds.top + (height / 2) };

        const float scaleFactor = .90f;

        // We want our hitbounds to be dialated a certain percentage within our element. We don't hittest the full bounds
        // of the element because there are scenarios (such as scrollbar) where it is okay to focus an element although it
        // is being occluded from something. To allow these scenarios, we shrink the hittest area so that the occluding elements
        // are not included in the hittest result.
        const float dilatedX = centerPt.x - ((width / 2) * scaleFactor);
        const float dilatedY = centerPt.y - ((height / 2) * scaleFactor);
        const XRECTF hitBounds = { dilatedX, dilatedY, width * scaleFactor, height * scaleFactor };

        return hitBounds;
    }
};
