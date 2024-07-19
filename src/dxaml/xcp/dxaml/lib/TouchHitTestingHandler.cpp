// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TouchHitTestingHandler.h"
#include <isapipresent.h>
#include "VisualTreeHelper.h"
#include <XamlOneCoreTransforms.h>
#include <InputServices.h>
#include "Window.g.h"
#include "UIElement.g.h"
#include <RootScale.h>
#include <windows.ui.core.corewindow-defs.h>

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

using namespace DirectUI;

TouchHitTestingHandler::TouchHitTestingHandler()
{
    m_tokTouchHitTesting.value = 0;
}

bool TouchHitTestingHandler::IsTouchHitTestingEnabled(_In_ CCoreServices* core)
{
    return core->HandleInputOnMainWindow();
}

_Check_return_ HRESULT TouchHitTestingHandler::Register(_In_ wuc::ICoreWindow* pCoreWindow)
{
    HRESULT hr = S_OK;

    if (m_tokTouchHitTesting.value)
    {
        // already registered
        goto Cleanup;
    }

    IFC(pCoreWindow->add_TouchHitTesting(
        Microsoft::WRL::Callback<wuc::ITouchHitTestingEventHandler>(
                this,
                &TouchHitTestingHandler::OnCoreWindowTouchHitTesting).Get(),
        &m_tokTouchHitTesting));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT TouchHitTestingHandler::Unregister(_In_ wuc::ICoreWindow* pCoreWindow)
{
    HRESULT hr = S_OK;

    if (!m_tokTouchHitTesting.value)
    {
        // already unregistered
        goto Cleanup;
    }

    hr = pCoreWindow->remove_TouchHitTesting(m_tokTouchHitTesting);
    if (hr == HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE))
    {
        // Ignore. This will be returned if the window is destroyed before calling this function.
        // CoreWindow handles this case when CoreWindow.Close is called, but our test harness
        // closes the window directly using Win32 APIs.
        hr = S_OK;
    }

    m_tokTouchHitTesting.value = 0;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
// Registers for touch hit testing messages directly using the low-level
// Win32 API.
//
// This is used in hosting scenarios when we don't have a CoreWindow. We
// don't unregister in this case. It's fine to let the window be destroyed
// without unregistering via the low-level API.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TouchHitTestingHandler::Register(_In_ HWND hwnd)
{
    HRESULT hr = S_OK;

    if (IsRegisterTouchHitTestingWindowPresent())
    {
        IFCW32(RegisterTouchHitTestingWindow(hwnd, TOUCH_HIT_TESTING_CLIENT));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT TouchHitTestingHandler::GetPointList(_In_ wfc::IIterable<wf::Point>* pIterable, _Inout_ std::vector<wf::Point>& list)
{
    HRESULT hr = S_OK;
    wfc::IIterator<wf::Point>* pIterator = NULL;
    BOOLEAN hasCurrent = FALSE;
    wf::Point element;

    IFCPTR(pIterable);

    IFC(pIterable->First(&pIterator));
    IFCPTR(pIterator);
    IFC(pIterator->get_HasCurrent(&hasCurrent));

    list.clear();

    while (hasCurrent)
    {
        IFC(pIterator->get_Current(&element));
        list.push_back(element);

        IFC(pIterator->MoveNext(&hasCurrent));
    }

Cleanup:
    ReleaseInterface(pIterator);
    RRETURN(hr);
}


void TouchHitTestingHandler::GetPolygonBounds(XUINT32 numPoints, _In_reads_(numPoints) wf::Point* pPoints, _Out_ wf::Rect* pBounds)
{
    ASSERT(numPoints > 0);

    float minX = pPoints[0].X;
    float minY = pPoints[0].Y;

    float maxX = minX;
    float maxY = minY;

    for (XUINT32 i = 1; i < numPoints; i++)
    {
        float x = pPoints[i].X;
        float y = pPoints[i].Y;

        minX = std::min(minX, x);
        maxX = std::max(maxX, x);

        minY = std::min(minY, y);
        maxY = std::max(maxY, y);
    }

    pBounds->X = minX;
    pBounds->Y = minY;
    pBounds->Width = maxX - minX;
    pBounds->Height = maxY - minY;
}


_Check_return_ HRESULT TouchHitTestingHandler::UpdateBestEval(
    _In_ const mui::ProximityEvaluation& currentEval,
    _Inout_ mui::ProximityEvaluation& bestEval,
    _In_ const wf::Rect& currentBounds,
    _Inout_ wf::Rect& bestBounds,
    _Inout_ bool& elementEvaluated,
    _In_ CUIElement* pCurrentElement,
    _Inout_ CUIElement** ppBestEvalElement)
{
    if (currentEval.Score < bestEval.Score)
    {
        bool currentBoundsContainBestPoint = false;
        bool bestBoundsContainCurrentPoint = false;

        if (elementEvaluated)
        {
            ASSERT(*ppBestEvalElement != nullptr);

            // The containment tests below are designed to ignore hits on the ancestor of the
            // current best candidate.  This causes problems for overlapping siblings, so
            // an additional test is done, only perform the containment tests if the current
            // candidate is actually an ancestor of the best candidate.
            bool performContainmentTests = true;
            if (pCurrentElement != *ppBestEvalElement)
            {
                const bool isAncestor = pCurrentElement->IsAncestorOf(*ppBestEvalElement);
                if (!isAncestor)
                {
                    performContainmentTests = false;
                }
            }

            if (performContainmentTests)
            {
                // Even if this element has a better score than the previous element with higher z-order,
                // don't select this element if the previous adjusted point belongs to this element. In other words,
                // if this element overlaps the element with higher z-order, favor the element with higher z-order.
                currentBoundsContainBestPoint = RectUtil::Contains(currentBounds, bestEval.AdjustedPoint);

                // Don't select this element if the adjusted touch point does not hit the element.
                // That may happen if another element with higher z-order overlaps this element at the
                // adjusted touch point.
                // TODO: This should really be a true hit test rather than a simple bounds test.
                bestBoundsContainCurrentPoint = RectUtil::Contains(bestBounds, currentEval.AdjustedPoint);
            }
        }

        if (!elementEvaluated || (!currentBoundsContainBestPoint && !bestBoundsContainCurrentPoint))
        {
            bestEval = currentEval;
            bestBounds = currentBounds;
            *ppBestEvalElement = pCurrentElement;
            elementEvaluated = TRUE;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT TouchHitTestingHandler::OnCoreWindowTouchHitTesting(
    _In_ wuc::ICoreWindow *pSender,
    _In_ wuc::ITouchHitTestingEventArgs *pArgs)
{
    TouchHitTestingInputs inputs;

    inputs.subtreeRootElement = nullptr;

    IFC_RETURN(pArgs->get_BoundingBox(&inputs.boundingBox));

    {
        wuc::CoreProximityEvaluation coreProximityEvaluation;
        IFC_RETURN(pArgs->get_ProximityEvaluation(&coreProximityEvaluation));
        inputs.proximityEvaluation.Score = coreProximityEvaluation.Score;
        inputs.proximityEvaluation.AdjustedPoint = coreProximityEvaluation.AdjustedPoint;
    }

    IFC_RETURN(pArgs->get_Point(&inputs.point));

    inputs.evaluateProximityToRect = [pArgs](wf::Rect rect, mui::ProximityEvaluation* proximityEvaluation) -> HRESULT
    {
        wuc::CoreProximityEvaluation coreProximityEvaluation;
        IFC_RETURN(pArgs->EvaluateProximityToRect(rect, &coreProximityEvaluation));
        proximityEvaluation->Score = coreProximityEvaluation.Score;
        proximityEvaluation->AdjustedPoint = coreProximityEvaluation.AdjustedPoint;
        return S_OK;
    };

    inputs.evaluateProximityToPolygon = [pArgs](UINT32 numVertices, wf::Point* vertices, mui::ProximityEvaluation* proximityEvaluation) -> HRESULT
    {
        wuc::CoreProximityEvaluation coreProximityEvaluation;
        IFC_RETURN(pArgs->EvaluateProximityToPolygon(numVertices, vertices, &coreProximityEvaluation));
        proximityEvaluation->Score = coreProximityEvaluation.Score;
        proximityEvaluation->AdjustedPoint = coreProximityEvaluation.AdjustedPoint;
        return S_OK;
    };

    TouchHitTestingOutputs outputs;
    IFC_RETURN(TouchHitTestingInternal(inputs, outputs));

    if (outputs.handled)
    {
        wuc::CoreProximityEvaluation coreProximityEvaluation;
        coreProximityEvaluation.Score = outputs.proximityEvaluation.Score;
        coreProximityEvaluation.AdjustedPoint = outputs.proximityEvaluation.AdjustedPoint;
        IFC_RETURN(pArgs->put_ProximityEvaluation(coreProximityEvaluation));

        wuc::ICoreWindowEventArgs* coreWindowArgs = nullptr;
        IFC_RETURN(pArgs->QueryInterface(__uuidof(wuc::ICoreWindowEventArgs), reinterpret_cast<void**>(&coreWindowArgs)));
        coreWindowArgs->put_Handled(TRUE);
        ReleaseInterface(coreWindowArgs);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Touch targeting event handler. Calculates an adjusted touch point
//      based on the geometry of the elements that intersect with the touch
//      area. This causes all touch input to be offset until a new touch
//      begins.
//
//  Notes:
//      * All wf::Rect/Point values are in DIPs.
//      * TouchHitTestingEventArgs.BoundingBox/Point and
//        ProximityEvaluation.AdjustedPoint are in screen coordinates.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
TouchHitTestingHandler::TouchHitTesting(
    _In_opt_ CUIElement* subtreeRootElement,
    _In_ mui::ITouchHitTestingEventArgs *pArgs)
{
    TouchHitTestingInputs inputs;
    inputs.subtreeRootElement = subtreeRootElement;
    IFC_RETURN(pArgs->get_BoundingBox(&inputs.boundingBox));
    IFC_RETURN(pArgs->GetProximityEvaluation(&inputs.proximityEvaluation));
    IFC_RETURN(pArgs->get_Point(&inputs.point));

    inputs.evaluateProximityToRect = [pArgs](wf::Rect rect, mui::ProximityEvaluation* proximityEvaluation) -> HRESULT {
        return pArgs->EvaluateProximityToRect(rect, proximityEvaluation);
    };

    inputs.evaluateProximityToPolygon = [pArgs](UINT32 numVertices, wf::Point* vertices, mui::ProximityEvaluation* proximityEvaluation) -> HRESULT {
        return pArgs->EvaluateProximityToPolygon(numVertices, vertices, proximityEvaluation);
    };

    TouchHitTestingOutputs outputs;
    IFC_RETURN(TouchHitTestingInternal(inputs, outputs));

    if (outputs.handled)
    {
        IFC_RETURN(pArgs->SetProximityEvaluation(outputs.proximityEvaluation));
        IFC_RETURN(pArgs->put_Handled(TRUE));
    }

    return S_OK;
}

_Check_return_ HRESULT
TouchHitTestingHandler::TouchHitTestingInternal(
    _In_ const TouchHitTestingInputs& inputs,
    _Out_ TouchHitTestingOutputs& outputs)
{
    HRESULT hr = S_OK;
    DXamlCore* const pCore = DXamlCore::GetCurrent();
    wf::Point touchPointNoPlateau;
    wf::Rect touchBoundsNoPlateau;
    wf::Rect currentBoundsNoPlateau;
    wf::Rect bestBoundsNoPlateau;
    mui::ProximityEvaluation currentEval;
    mui::ProximityEvaluation bestEval;
    XRECTF hitTestRect = {0};
    XINT32 nCount = 0;
    CUIElement** pElements = NULL;
    bool elementEvaluated = false;
    DependencyObject* pDO = NULL;
    UIElement* pCurrentElementPeer = NULL;
    wfc::IIterable<wfc::IIterable<wf::Point>*>* pSubElementsIterable = NULL;
    wfc::IIterator<wfc::IIterable<wf::Point>*>* pSubElementsIterator = NULL;
    wfc::IIterable<wf::Point>* pPointsIterable = NULL;
    xaml_media::IGeneralTransform *pTransformToRoot = NULL;
    xaml_media::IGeneralTransform *pTransformToElement = NULL;
    bool shouldContinueEvaluation = true;
    bool isRootNull = false;
    wf::Rect windowBounds;
    CUIElement* pBestEvalElement = nullptr;

    bool isVisualRelative = XamlOneCoreTransforms::IsEnabled();
    CContentRoot* contentRoot = nullptr;
    if (!isVisualRelative && inputs.subtreeRootElement)
    {
        if (contentRoot = VisualTree::GetContentRootForElement(inputs.subtreeRootElement))
        {
            isVisualRelative = contentRoot->GetType() == CContentRoot::Type::XamlIslandRoot;
        }
    }

    TraceTouchHitTestingBegin();

    //
    // Get the list of elements that intersect with the touch bounds.
    //

    // Note: the rect returned by get_BoundingBox already has the inverse plateau applied.
    touchBoundsNoPlateau = inputs.boundingBox;

    if (isVisualRelative == false) { pCore->ScreenToClient(&touchBoundsNoPlateau); }
    else
    {
        // When we inspect the different points and rects in the event args, they are in visual relative coordinates. However, they also have the corewindow bounds
        // applied to the bounds for app compat reasons. Therefore, to get the true visual relative coordinates, we need to subtract the core window bounds.

        // TASK: Dummy window should no longer be used to fake window bounds and calculate hit tests
        // https://microsoft.visualstudio.com/OS/_workitems/edit/37097636
        IFC(pCore->GetDummyWindowNoRef()->get_Bounds(&windowBounds));

        touchBoundsNoPlateau.X -= windowBounds.X;
        touchBoundsNoPlateau.Y -= windowBounds.Y;
    }

    hitTestRect.X = touchBoundsNoPlateau.X;
    hitTestRect.Y = touchBoundsNoPlateau.Y;
    hitTestRect.Width = touchBoundsNoPlateau.Width;
    hitTestRect.Height = touchBoundsNoPlateau.Height;

    // CoreImports::UIElement_HitTestRect is relative to the public root visual and will apply the inverse plateau scale as needed.
    hr = CoreImports::UIElement_HitTestRect(
        inputs.subtreeRootElement,
        pCore->GetHandle(),
        hitTestRect,
        !!VisualTreeHelper::c_canHitDisabledElementsDefault,
        !!VisualTreeHelper::c_canHitInvisibleElementsDefault,
        &nCount,
        &pElements,
        &isRootNull);

    if (hr == E_FAIL && isRootNull)
    {
        // Allow the root to be null. Don't adjust anything in that case.
        hr = S_OK;
    }
    IFC(hr);

    //
    // Loop through elements by their z-order and find the one with the best
    // proximity score, modulo a few conditions.
    //

    bestEval = inputs.proximityEvaluation;
    // Note: the point returned by get_Point already has the inverse plateau applied.
    touchPointNoPlateau = inputs.point;

    bestEval.AdjustedPoint = touchPointNoPlateau;

    if (isVisualRelative == false) { pCore->ScreenToClient(&touchPointNoPlateau); }

    for (XINT32 i = 0; i < nCount && shouldContinueEvaluation; i++)
    {
        CUIElement* pCurrentElement = NULL;
        wf::Point elementTouchPoint;
        wf::Rect elementTouchBounds;

        pCurrentElement = static_cast<CUIElement*>(pElements[i]);
        IFC(pCore->TryGetPeer(pCurrentElement, &pDO));

        if (pDO)
        {
            pCurrentElementPeer = static_cast<UIElement*>(pDO);

            //
            // Call the UIElement FindSubElementsForTouchTargetingProtected virtual with the
            // touch information in the element's coordinate space.
            //

            IFC(pCurrentElementPeer->TransformToVisual(NULL, &pTransformToRoot));
            IFC(pTransformToRoot->get_Inverse(&pTransformToElement));
            // Note: TransformToVisual stops walking when it reaches the root, and will not include the plateau scale
            // in the transformer that it returns, so touchPoint and touchBounds can be safely passed through without
            // the inverse plateau applying a second time.
            IFC(pTransformToElement->TransformPoint(touchPointNoPlateau, &elementTouchPoint));
            IFC(pTransformToElement->TransformBounds(touchBoundsNoPlateau, &elementTouchBounds));
            IFC(pCurrentElementPeer->FindSubElementsForTouchTargetingProtected(elementTouchPoint, elementTouchBounds, &pSubElementsIterable));
        }

        if (pSubElementsIterable != NULL)
        {
            //
            // This is a custom element with custom sub-element bounds information. Iterate through the
            // subelement polygons and find the one with the best score.
            //

            BOOLEAN hasCurrent = FALSE;

            IFC(pSubElementsIterable->First(&pSubElementsIterator));
            IFCCATASTROPHIC(pSubElementsIterator);
            IFC(pSubElementsIterator->get_HasCurrent(&hasCurrent));

            while (hasCurrent)
            {
                std::vector<wf::Point> points;

                IFC(pSubElementsIterator->get_Current(&pPointsIterable));
                IFC(GetPointList(pPointsIterable, points));

                if (points.empty())
                {
                    ReleaseInterface(pPointsIterable);
                    continue;
                }

                // Transform the polygon from element to client to screen coordinates.
                for (size_t j = 0; j < points.size(); j++)
                {
                    IFC(pTransformToRoot->TransformPoint(points[j], &points[j]));

                    if (isVisualRelative == false) { pCore->ClientToScreen(&points[j]); }
                    else
                    {
                        // EvaluateProximityToRect expects the bounds to be in visual relative coordinates with the core window offsets applied. We express
                        // our bounds in true visual relative coordinates, therefore, we need to apply the core window offsets
                        // before passing this to the EvaluteProximity method

                        currentBoundsNoPlateau.X += windowBounds.X;
                        currentBoundsNoPlateau.Y += windowBounds.Y;
                    }
                }

                GetPolygonBounds(points.size(), &points[0], &currentBoundsNoPlateau);

                IFC(inputs.evaluateProximityToPolygon(points.size(), &points[0], &currentEval));
                IFC(UpdateBestEval(currentEval, bestEval, currentBoundsNoPlateau, bestBoundsNoPlateau, elementEvaluated, pCurrentElement, &pBestEvalElement));

                if (bestEval.Score == 0)
                {
                    // We can't do any better, so we can skip the rest of the elements.
                    shouldContinueEvaluation = FALSE;
                    break;
                }

                IFC(pSubElementsIterator->MoveNext(&hasCurrent));
                ReleaseInterface(pPointsIterable);
            }
            ReleaseInterface(pSubElementsIterator);
        }

        else
        {
            //
            // No custom touch targeting geometry. We will just use the rectangular bounds of the
            // element to calculate the proximity score.
            //

            XRECTF_RB elementBounds;
            IFC(pCurrentElement->GetGlobalBounds(&elementBounds));

            //
            // The global bounds go through the root visual, which contains the plateau scale. We want the bounds
            // without any plateau scaling. Note: We want the scale set on the root visual, which isn't always the
            // plateau scale. OneCoreTransforms mode always sets a 1x scale on the root visual.
            //
            const auto coreServices = pCore->GetHandle();
            const auto coordinator = coreServices->GetContentRootCoordinator();
            const auto mainRoot = coordinator->Unsafe_IslandsIncompatible_CoreWindowContentRoot();
            const auto root = contentRoot ? contentRoot : mainRoot;
            const float plateauScale = RootScale::GetRasterizationScaleForContentRoot(root);
            if (plateauScale != 1.0f)
            {
                elementBounds.left /= plateauScale;
                elementBounds.right /= plateauScale;
                elementBounds.top /= plateauScale;
                elementBounds.bottom /= plateauScale;
            }

            currentBoundsNoPlateau.X = elementBounds.left;
            currentBoundsNoPlateau.Y = elementBounds.top;
            currentBoundsNoPlateau.Width = elementBounds.right - elementBounds.left;
            currentBoundsNoPlateau.Height = elementBounds.bottom - elementBounds.top;

            if (isVisualRelative == false) { pCore->ClientToScreen(&currentBoundsNoPlateau); }
            else
            {
                // EvaluateProximityToRect expects the bounds to be in the same coordinate space that as the
                // event args, which is visual relative coordinates with the core window offsets applied. We express
                // our bounds in true visual relative coordinates, therefore, we need to apply the core window offsets
                // before passing this to the EvaluteProximity method

                currentBoundsNoPlateau.X += windowBounds.X;
                currentBoundsNoPlateau.Y += windowBounds.Y;
            }

            IFC(inputs.evaluateProximityToRect(currentBoundsNoPlateau, &currentEval));
            IFC(UpdateBestEval(currentEval, bestEval, currentBoundsNoPlateau, bestBoundsNoPlateau, elementEvaluated, pCurrentElement, &pBestEvalElement));

            if (bestEval.Score == 0)
            {
                // We can't do any better, so we can skip the rest of the elements.
                shouldContinueEvaluation = FALSE;
            }
        }

        ReleaseInterface(pTransformToRoot);
        ReleaseInterface(pTransformToElement);
        ReleaseInterface(pSubElementsIterable);
        ctl::release_interface(pDO);
    }

    if (elementEvaluated)
    {
        outputs.proximityEvaluation = bestEval;
        outputs.handled = true;
    }

Cleanup:
    VERIFYHR(CoreImports::UIElement_DeleteList(pElements, nCount));
    ReleaseInterface(pSubElementsIterable);
    ReleaseInterface(pSubElementsIterator);
    ReleaseInterface(pPointsIterable);
    ReleaseInterface(pTransformToRoot);
    ReleaseInterface(pTransformToElement);
    ctl::release_interface(pDO);

    TraceTouchHitTestingEnd(hitTestRect.X, hitTestRect.Y, hitTestRect.Width, hitTestRect.Height);

    RRETURN(hr);
}
