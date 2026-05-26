// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents a Seek Slider Automation Peer used in MTC.

#pragma once

#include "SeekSliderAutomationPeer.g.h"

namespace DirectUI
{
    // Exposes a SeekSliderAutomationPeer to the UI Automation framework, used by accessibility aids.
    PARTIAL_CLASS(SeekSliderAutomationPeer)
    {
    public:
        static _Check_return_ HRESULT CreateInstanceWithOwner(_In_ xaml_controls::ISlider* pOwner, _In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ xaml_automation_peers::ISliderAutomationPeer** ppInstance);

        // Initializes a new instance of the SeekSliderAutomationPeer class.
        SeekSliderAutomationPeer();
        virtual ~SeekSliderAutomationPeer();

        IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue);
        IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);

        //IValueProvider
        // Properties.
        _Check_return_ HRESULT get_ValueImpl(_Out_ HSTRING* value);

        // Methods.
        _Check_return_ HRESULT SetValueImpl(_In_ HSTRING value);

    private:


        _Check_return_ HRESULT ConvertTimeSpanToHString(
            _In_ wf::TimeSpan position,
            _Outptr_ HSTRING* pPositionTime);

        _Check_return_ HRESULT CreatePositionTimeComponent(
            _In_ XUINT32 localizationId,
            _In_ INT32 timeComponentValue,
            _Out_ wrl_wrappers::HString& strTimeComponent);
    };
}
