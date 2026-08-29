// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ContainerContentChangingEventArgs.g.h"
#include "UIElement.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(ContainerContentChangingEventArgs)
    {
    public:
        static const INT NoBindingNeededToken = -1;
    protected:
        ContainerContentChangingEventArgs();

    public:
        void RegisterBackpointerToContainer(_In_ UIElement* pOwner) { m_pContainerBackPointer = pOwner; }

        _Check_return_ HRESULT get_ItemContainerImpl(_Outptr_ xaml_primitives::ISelectorItem** pValue);

        _Check_return_ HRESULT RegisterUpdateCallbackImpl(_In_ wf::ITypedEventHandler<xaml_controls::ListViewBase*, xaml_controls::ContainerContentChangingEventArgs*>* callback);
        _Check_return_ HRESULT RegisterUpdateCallbackWithPhaseImpl(_In_ UINT callbackPhase, _In_ wf::ITypedEventHandler<xaml_controls::ListViewBase*, xaml_controls::ContainerContentChangingEventArgs*>* callback);
        _Check_return_ HRESULT ResetLifetimeImpl();

        INT GetBindingPhase() { return m_xbindingPhase; }
        void SetBindingPhase(INT phase) { m_xbindingPhase = phase; }
        bool GetWantsBindingPhaseCallback() { return m_xbindingPhase != NoBindingNeededToken; }

        UINT GetLowestWantedPhase()
        {
            UINT lowestPhase = GetWantsBindingPhaseCallback() ? m_xbindingPhase : -1; // wants _public_ callback, which is what m_phase is indicating
            if (m_wantsCallBack && (GetWantsBindingPhaseCallback() && m_phase < static_cast<UINT64>(m_xbindingPhase)) || !GetWantsBindingPhaseCallback())
            {
                lowestPhase = m_phase;
            }
            return lowestPhase;
        }

    private:
        xaml::IUIElement* m_pContainerBackPointer;
        INT m_xbindingPhase;

    };
}
