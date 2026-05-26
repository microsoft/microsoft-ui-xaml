// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlLogging.h"
#include "SolidColorBrushUnitTests.h"
#include "SolidColorBrush.h"
#include <MockDComp-UnitTestHelpers.h>
#include "UIElement.h"
#include "BrushParams.h"
#include "WUCBrushManager.h"
#include "SharedTransitionAnimations.h"
#include "MockEasingFunctions.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Brushes {

void SolidColorBrushUnitTests::ValidateSetUpSolidColorBrushTransition()
{
    SharedTransitionAnimations sharedTransitionAnimations;
    WUCBrushManager brushManager;
    CSolidColorBrush brush;
    CSolidColorBrush brush2;
    brush2.m_rgb = 0x12345678;
    CUIElement element1;
    CUIElement element2;

    wrl::ComPtr<MockEasingFunctionStatics> easingFunctionStatics;
    easingFunctionStatics.Attach(new MockEasingFunctionStatics());

    Microsoft::WRL::ComPtr<WUComp::ICompositor> compositor;
    VERIFY_SUCCEEDED(MockDComp::CreateMockCompositor(&compositor));
    brushManager.m_sharedTransitionAnimationsNoRef = &sharedTransitionAnimations;
    brushManager.m_easingFunctionStatics = easingFunctionStatics.Get();
    brushManager.m_compositor = compositor.Get();

    WUComp::ICompositionBrush* defaultBrush = brush.GetWUCBrush(compositor.Get());
    WUComp::ICompositionBrush* wucTransitioningBrush = nullptr;
    WUComp::ICompositionScopedBatch* wucScopedBatch = nullptr;

    {
        LOG_OUTPUT(L"> Neither element has a brush transition. Both should return the same default brush.");
        WUComp::ICompositionBrush* wucBrush1 = brushManager.GetColorBrush(&brush, { &element1, ElementBrushProperty::Fill }).Get();
        VERIFY_ARE_EQUAL(defaultBrush, wucBrush1);

        WUComp::ICompositionBrush* wucBrush2 = brushManager.GetColorBrush(&brush, { &element2, ElementBrushProperty::Fill }).Get();
        VERIFY_ARE_EQUAL(defaultBrush, wucBrush2);
    }

    {
        LOG_OUTPUT(L"> Set up a SolidColorBrush transition for element1.Fill.");
        wf::TimeSpan duration = {0};
        brushManager.SetUpBrushTransitionIfAllowedHelper(&brush2, &brush, element1, ElementBrushProperty::Fill, duration);

        LOG_OUTPUT(L"  > Verify the brush transition has been set up.");
        VERIFY_ARE_EQUAL(static_cast<size_t>(1), brushManager.m_wucColorBrushTransitions->size());
        {
            const SolidColorBrushTransitionState& transitionState = brushManager.m_wucColorBrushTransitions->at(0);
            VERIFY_IS_NULL(transitionState.m_wucBrush.Get());
            VERIFY_ARE_EQUAL(0x12345678, transitionState.m_fromARGB.value());
            VERIFY_IS_NULL(transitionState.m_wucScopedBatch.Get());
            VERIFY_ARE_EQUAL(0, transitionState.m_wucCompletedEventToken.value);
        }

        LOG_OUTPUT(L"> Access the WUC brush. That WUC brush should no longer be the default.");
        wucTransitioningBrush = brushManager.GetColorBrush(&brush, { &element1, ElementBrushProperty::Fill }).Get();
        VERIFY_ARE_NOT_EQUAL(defaultBrush, wucTransitioningBrush);

        LOG_OUTPUT(L"  > Verify the brush transition animation has started.");
        VERIFY_ARE_EQUAL(static_cast<size_t>(1), brushManager.m_wucColorBrushTransitions->size());
        {
            const SolidColorBrushTransitionState& transitionState = brushManager.m_wucColorBrushTransitions->at(0);
            VERIFY_ARE_EQUAL(0x12345678, transitionState.m_fromARGB.value());
            wucScopedBatch = transitionState.m_wucScopedBatch.Get();
            VERIFY_IS_NOT_NULL(wucScopedBatch);
            LOG_OUTPUT(L"  > This token should exist if we were running against real DComp.");
            VERIFY_ARE_EQUAL(0, transitionState.m_wucCompletedEventToken.value);
        }

        LOG_OUTPUT(L"  > Verify other usages of this brush still return the default.");
        WUComp::ICompositionBrush* wucBrush2 = brushManager.GetColorBrush(&brush, { &element1, ElementBrushProperty::Stroke }).Get();
        VERIFY_ARE_EQUAL(defaultBrush, wucBrush2);

        WUComp::ICompositionBrush* wucBrush3 = brushManager.GetColorBrush(&brush, { &element2, ElementBrushProperty::Fill }).Get();
        VERIFY_ARE_EQUAL(defaultBrush, wucBrush3);

        LOG_OUTPUT(L"> Access the WUC brush again should return the same transitioning brush.");
        WUComp::ICompositionBrush* wucBrush4 = brushManager.GetColorBrush(&brush, { &element1, ElementBrushProperty::Fill }).Get();
        VERIFY_ARE_EQUAL(wucTransitioningBrush, wucBrush4);

        LOG_OUTPUT(L"> Tearing down the SolidColorBrush transition for element1.Fill. That WUC brush revert to the default.");
        WUCBrushManager::OnBrushTransitionCompleted(brushManager.m_wucColorBrushTransitions, wucScopedBatch);

        LOG_OUTPUT(L"  > Verify the brush transition has been cleaned up.");
        VERIFY_ARE_EQUAL(static_cast<size_t>(0), brushManager.m_wucColorBrushTransitions->size());
        WUComp::ICompositionBrush* wucBrush5 = brushManager.GetColorBrush(&brush, { &element1, ElementBrushProperty::Fill }).Get();
        VERIFY_ARE_EQUAL(defaultBrush, wucBrush5);
    }
}

} } } } } }
