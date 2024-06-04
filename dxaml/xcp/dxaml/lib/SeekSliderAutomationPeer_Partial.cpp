// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents a Seek Slider Automation Peer used in MTC.

#include "precomp.h"
#include "localizedResource.h"
#include "SeekSliderAutomationPeer.g.h"
#include "Slider.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// simple factory
class __declspec(novtable) SeekSliderAutomationPeerFactory:
    public ctl::BetterAggregableCoreObjectActivationFactory
{
public:
    IFACEMETHOD(CreateInstanceWithOwner)(_In_ xaml_controls::ISlider* pOwner, _In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ xaml_automation_peers::ISliderAutomationPeer** ppInstance);

protected:
    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::SeekSliderAutomationPeer;
    }
};

_Check_return_ HRESULT SeekSliderAutomationPeerFactory::CreateInstanceWithOwner(
    _In_ xaml_controls::ISlider* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::ISliderAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::ISliderAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));

    IFC(ActivateInstance(pOuter,
            static_cast<Slider*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<SeekSliderAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

    if (ppInner)
    {
        *ppInner = pInner;
        pInner = NULL;
    }

    *ppInstance = pInstance;
    pInstance = NULL;

Cleanup:
    ReleaseInterface(ownerAsUIE);
    ReleaseInterface(pInstance);
    ReleaseInterface(pInner);
    RRETURN(hr);
}

_Check_return_ HRESULT SeekSliderAutomationPeer::CreateInstanceWithOwner(
    _In_ xaml_controls::ISlider* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::ISliderAutomationPeer** ppInstance)
{
    ctl::ComPtr<SeekSliderAutomationPeerFactory> spSeekSliderAutomationPeerFactory;
    IFC_RETURN(ctl::ComObject<SeekSliderAutomationPeerFactory>::CreateInstance(&spSeekSliderAutomationPeerFactory));
    IFC_RETURN(spSeekSliderAutomationPeerFactory->CreateInstanceWithOwner(owner, pOuter, ppInner, ppInstance));
    return S_OK;
}

// Initializes a new instance of the SeekSliderAutomationPeer class.
SeekSliderAutomationPeer::SeekSliderAutomationPeer()
{
}

// Deconstructor
SeekSliderAutomationPeer::~SeekSliderAutomationPeer()
{
}

IFACEMETHODIMP SeekSliderAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;

    if (patternInterface == xaml_automation_peers::PatternInterface_Value)
    {
        *ppReturnValue = ctl::as_iinspectable(this);
        ctl::addref_interface(this);
    }
    else
    {
        IFC(SeekSliderAutomationPeerGenerated::GetPatternCore(patternInterface, ppReturnValue));
    }
Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP SeekSliderAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    RRETURN(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"SeekSliderAutomationPeer")).CopyTo(returnValue));
}

_Check_return_ HRESULT SeekSliderAutomationPeer::get_ValueImpl(_Out_ HSTRING* pValue)
{
    HRESULT hr = S_OK;
    IUIElement* pOwner = NULL;
    wf::TimeSpan currentPosition;
    wf::TimeSpan duration;

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);

    duration = (static_cast<Slider*>(pOwner))->GetCurrentDuration();

    // If this is a livestream (determined by duration), get the current position reported by the player
    if(duration.Duration == INT64_MAX || duration.Duration == 0)
    {
        currentPosition = (static_cast<Slider*>(pOwner))->GetCurrentPosition();
    }
    else
    {
        double positionSliderMinimum, positionSliderMaximum, positionSliderValue;

        // Construct the current position timestamp from the slider position in normal cases.
        // This is necessary because in the scenario of seeking, the position reported by
        // the player will not have completed the seek and would otherwise report an old position.
        IFC((static_cast<Slider*>(pOwner))->get_Minimum(&positionSliderMinimum));
        IFC((static_cast<Slider*>(pOwner))->get_Maximum(&positionSliderMaximum));
        IFC((static_cast<Slider*>(pOwner))->get_Value(&positionSliderValue));

        currentPosition.Duration =
            static_cast<INT64>((positionSliderValue - positionSliderMinimum) / (positionSliderMaximum - positionSliderMinimum) *
                                static_cast<DOUBLE>(duration.Duration));
    }

    IFC(ConvertTimeSpanToHString(currentPosition, pValue));
Cleanup:
    ReleaseInterface(pOwner);
    RRETURN(hr);
}

_Check_return_ HRESULT SeekSliderAutomationPeer::SetValueImpl(_In_ HSTRING value)
{
    HRESULT hr = S_OK;

    IFC(ErrorHelper::OriginateError(AgError(UIA_OPERATION_CANNOT_BE_PERFORMED)));
    IFC(UIA_E_INVALIDOPERATION);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT SeekSliderAutomationPeer::ConvertTimeSpanToHString(
    _In_ wf::TimeSpan position,
    _Outptr_ HSTRING* pPositionTime)
{
    static const unsigned int HNSPerSecond = 10000000;

    WCHAR szPositionTime[MAX_PATH];
    wrl_wrappers::HString strPositionTime;
    wrl_wrappers::HString strUnformattedPositionTime;
    wrl_wrappers::HString strHours;
    wrl_wrappers::HString strMinutes;
    wrl_wrappers::HString strSeconds;

    HRESULT hr = S_OK;

    auto positionInSeconds = static_cast<INT64> (position.Duration / HNSPerSecond);
    INT32 totalSecondsInRange = (positionInSeconds > 0) ? (positionInSeconds % 86400) : 0;
    INT32 numHours = static_cast<INT32> (totalSecondsInRange / 3600);
    INT32 remainingSeconds = static_cast<INT32> (totalSecondsInRange % 3600);
    INT32 numMinutes = remainingSeconds / 60;
    INT32 numSeconds = remainingSeconds % 60;

    ASSERT (numHours < 24 && numMinutes < 60 && numSeconds < 60);

    // Assemble seconds portion of string
    IFC(CreatePositionTimeComponent(
        numSeconds == 1 ? UIA_MEDIA_SEEKSLIDER_SECOND : UIA_MEDIA_SEEKSLIDER_SECONDS,
        numSeconds,
        strSeconds));

    // Assemble minutes portion of string
    IFC(CreatePositionTimeComponent(
        numMinutes == 1 ? UIA_MEDIA_SEEKSLIDER_MINUTE : UIA_MEDIA_SEEKSLIDER_MINUTES,
        numMinutes,
        strMinutes));

    // Assemble final current position string.  Include hours if relevant.
    if(numHours > 0)
    {
        XUINT32 cch;
        IFC(CreatePositionTimeComponent(
            numHours == 1 ? UIA_MEDIA_SEEKSLIDER_HOUR : UIA_MEDIA_SEEKSLIDER_HOURS,
            numHours,
            strHours));
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_SEEKSLIDER_POSITION_3, strUnformattedPositionTime.ReleaseAndGetAddressOf()));
        cch = FormatMsg(szPositionTime, strUnformattedPositionTime.GetRawBuffer(NULL), strHours.GetRawBuffer(NULL), strMinutes.GetRawBuffer(NULL), strSeconds.GetRawBuffer(NULL));
        IFC(1 < cch ? S_OK : E_FAIL);
    }
    else
    {
        XUINT32 cch;
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_SEEKSLIDER_POSITION_2, strUnformattedPositionTime.ReleaseAndGetAddressOf()));
        cch = FormatMsg(szPositionTime, strUnformattedPositionTime.GetRawBuffer(NULL), strMinutes.GetRawBuffer(NULL), strSeconds.GetRawBuffer(NULL));
        IFC(1 < cch ? S_OK : E_FAIL);
    }

    IFC(strPositionTime.Set(szPositionTime));
    IFC(strPositionTime.CopyTo(pPositionTime));

Cleanup:
    return hr;
}

_Check_return_ HRESULT SeekSliderAutomationPeer::CreatePositionTimeComponent(
    _In_ XUINT32 localizationId,
    _In_ INT32 timeComponentValue,
    _Out_ wrl_wrappers::HString& strTimeComponent)
{
    static const unsigned int MaxNumberLength = 3;  // max of 2 digits per time component (hours, minutes, seconds) and a trailing '\0'

    wrl_wrappers::HString strUnformattedTimeComponent;
    WCHAR szNumber[MaxNumberLength];
    WCHAR szTimeComponentBuffer[128];
    XUINT32 cch;

    HRESULT hr = S_OK;
    IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(localizationId, strUnformattedTimeComponent.ReleaseAndGetAddressOf()));
    IFC(_itow_s(timeComponentValue, szNumber, 10) ? E_FAIL : S_OK);
    cch = FormatMsg(szTimeComponentBuffer, strUnformattedTimeComponent.GetRawBuffer(NULL), szNumber);
    IFC(1 < cch ? S_OK : E_FAIL);
    IFC(strTimeComponent.Set(szTimeComponentBuffer));
Cleanup:
    return hr;
}