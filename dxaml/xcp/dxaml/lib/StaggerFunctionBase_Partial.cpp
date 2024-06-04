// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "StaggerFunctionBase.g.h"
#include "LayoutTransitionStaggerItem.g.h"
#include "PVLStaggerFunction.g.h"

using namespace DirectUI;

// static
_Check_return_ HRESULT StaggerFunctionBase::GetTransitionDelayValues(
    _In_ CDependencyObject* pStaggerFunction,
    _In_ XINT32 cElements,
    _In_reads_(cElements) CUIElement** ppElements,
    _In_reads_(cElements) XRECTF *pBounds,
    _Out_writes_(cElements) XFLOAT* pDelays)
{
    // endpoint for core. Will create staggeritem collection and call into this staggerfunction.

    HRESULT hr = S_OK;

    DependencyObject *pDO = NULL;
    DependencyObject* pTarget = NULL;
    xaml::IUIElement* pIUIElement = NULL;
    ctl::ComPtr<LayoutTransitionStaggerItem> spStaggerItem;
    wfc::IVector<xaml::DependencyObject*>* pItems = NULL;

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFC(pCore->GetPeer(pStaggerFunction, &pDO));

    IFC(ctl::ComObject<TrackerCollection<xaml::DependencyObject*>>::CreateInstance(&pItems));

    for (INT32 index = 0; index < cElements; index++)
    {
        // getting target element
        IFC(pCore->GetPeer(ppElements[index], &pTarget));
        VERIFYHR(ctl::do_query_interface(pIUIElement, ctl::as_iinspectable(pTarget)));

        // getting bounds
        XRECTF coreBound = pBounds[index];
        wf::Rect dxamlBound;
        dxamlBound.Height = coreBound.Height;
        dxamlBound.Width = coreBound.Width;
        dxamlBound.X = coreBound.X;
        dxamlBound.Y = coreBound.Y;


        // creating a staggeritem to store this information

        IFC(ctl::make(&spStaggerItem));

        // setting the information we have
        IFC(spStaggerItem->put_Element(pIUIElement));
        IFC(spStaggerItem->put_Index(index));
        IFC(spStaggerItem->put_Bounds(dxamlBound));

        IFC(pItems->Append(spStaggerItem.Get()));
        ctl::release_interface(pTarget);
        ReleaseInterface(pIUIElement);
    }

    IFC(static_cast<StaggerFunctionBase*>(pDO)->GetTransitionDelays(pItems));

    // retrieving new information
    for (INT32 index = 0; index < cElements; index++)
    {
        ctl::ComPtr<xaml::IDependencyObject> spDO;
        IFC(pItems->GetAt(index, &spDO));
        IFC(spDO.As(&spStaggerItem));
        wf::TimeSpan timeSpan;
        IFC(spStaggerItem->get_StaggerTime(&timeSpan));
        pDelays[index] = static_cast<XFLOAT>(timeSpan.Duration / 10000000.0);  // timespan.Duration is ticks
    }
Cleanup:
    ReleaseInterface(pIUIElement);
    ctl::release_interface(pTarget);
    ctl::release_interface(pDO);
    ReleaseInterface(pItems);
    RRETURN(hr);
}

_Check_return_ HRESULT PVLStaggerFunction::GetTransitionDelays(
    _In_ wfc::IVector<xaml::DependencyObject*>* staggerItems)
{
    HRESULT hr = S_OK;
    UINT count = 0;
    xaml::IDependencyObject* pStaggerItemAsI = NULL;
    DOUBLE Delay = 0;
    DOUBLE DelayReduce = 0;
    DOUBLE Maximum = 0;
    BOOLEAN Inverse = FALSE;
    wf::TimeSpan runningTime = {};

    IFC(staggerItems->get_Size(&count));
    IFC(get_Delay(&Delay));
    IFC(get_DelayReduce(&DelayReduce));
    IFC(get_Maximum(&Maximum));
    IFC(get_Reverse(&Inverse)); // use value

    //DelayReduce = 1 - DelayReduce; // we will work with the remainder.
    runningTime.Duration = static_cast<INT64>(Delay * 10000.0f);   // sets the first delay
    for (UINT index = 0; index < count; ++index)
    {
        IFC(staggerItems->GetAt(index, &pStaggerItemAsI));

        LayoutTransitionStaggerItem* pStaggerItem = static_cast<LayoutTransitionStaggerItem*>(pStaggerItemAsI);

        IFC(pStaggerItem->put_StaggerTime(runningTime));
        ReleaseInterface(pStaggerItemAsI);

        Delay *= DelayReduce;
        runningTime.Duration += static_cast<INT64>(Delay * 10000.0f);
    }

Cleanup:
    ReleaseInterface(pStaggerItemAsI);
    RRETURN(hr);
}
