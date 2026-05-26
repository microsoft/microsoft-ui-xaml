// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Timeline.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(Timeline)
    {
    public:

        _Check_return_ HRESULT CreateTimelines(
            _In_ BOOLEAN bOnlyGenerateSteadyState,
            _In_ wfc::IVector<xaml_animation::Timeline*>* timelineCollection) override
        { RRETURN(S_OK); }

        _Check_return_ HRESULT OnInheritanceContextChanged() override;

    protected:

        template <class TKEYFRAMESANIMATION, class TKEYFRAME, class TKEYFRAMETYPE, class TDUIITEMTYPE>
        static _Check_return_ HRESULT ProcessKeyFrames(_In_ xaml_animation::ITimeline *pTimeline)
        {
            HRESULT hr = S_OK;
            TKEYFRAMESANIMATION *pAnimation = NULL;
            wfc::IVector<TKEYFRAMETYPE *> *pVector = NULL;
            wfc::IIterable<TKEYFRAMETYPE *> *pIterable = NULL;
            wfc::IIterator<TKEYFRAMETYPE *> *pIterator = NULL;
            TKEYFRAME *pItem = NULL;
            DirectUI::Timeline *pAnimationAsDO = NULL;
            TDUIITEMTYPE *pObj = NULL;
            BOOLEAN hasCurrent = false;

            IFC(ctl::do_query_interface(pAnimation, pTimeline));
            pAnimationAsDO = static_cast<DirectUI::Timeline *>(pTimeline);

            // If the animation is in the live tree then
            // there's nothing else to do
            if (pAnimationAsDO->IsInLiveTree())
            {
                goto Cleanup;
            }

            IFC(pAnimation->get_KeyFrames(&pVector));

            IFC(ctl::do_query_interface(pIterable, pVector));
            IFC(pIterable->First(&pIterator));

            IFC(pIterator->get_HasCurrent(&hasCurrent));
            while (hasCurrent)
            {
                IFC(pIterator->get_Current(&pItem));
                pObj = static_cast<TDUIITEMTYPE *>(pItem);
                IFC(pObj->NotifyInheritanceContextChanged(InheritanceContextChangeKind::ForceTopLevelEvent));

                ReleaseInterface(pItem);
                pObj = NULL;

                IFC(pIterator->MoveNext(&hasCurrent));
            }

        Cleanup:

            ReleaseInterface(pAnimation);
            ReleaseInterface(pVector);
            ReleaseInterface(pIterable);
            ReleaseInterface(pIterator);
            ReleaseInterface(pItem);

            RRETURN(hr);
        }
    };
}
