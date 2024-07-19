// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "EventCallbacks.h"
#include "Storyboard.g.h"
#include <windows.graphics.display.h>
#include "ModernCollectionBasePanel.g.h"

using namespace DirectUI;

_Check_return_ HRESULT DependencyPropertyChangedTraits::attach_handler(
    DependencyPropertyChangedTraits::event_interface* pSource,
    DirectUI::IDPChangedEventHandler* pHandler)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IDPChangedEventSource> spEventSource;

    IFC(static_cast<DependencyObject*>(pSource)->GetDPChangedEventSource(&spEventSource));
    IFC(spEventSource->AddHandler(pHandler));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DependencyPropertyChangedTraits::detach_handler(
    DependencyPropertyChangedTraits::event_interface* pSource,
    DirectUI::IDPChangedEventHandler* pHandler)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IDPChangedEventSource> spEventSource;

    IFC(static_cast<DependencyObject*>(pSource)->GetDPChangedEventSource(&spEventSource));
    IFC(spEventSource->RemoveHandler(pHandler));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT StoryboardCompletedTraits::attach_handler(
    StoryboardCompletedTraits::event_interface *pSource,
    wf::IEventHandler<IInspectable*> *pHandler,
    EventRegistrationToken *pToken)
{
    RRETURN(static_cast<Storyboard*>(pSource)->add_Completed(pHandler, pToken));
}

_Check_return_ HRESULT StoryboardCompletedTraits::detach_handler(
    StoryboardCompletedTraits::event_interface *pSource,
    EventRegistrationToken token)
{
    RRETURN(static_cast<Storyboard*>(pSource)->remove_Completed(token));
}

_Check_return_ HRESULT DisplayInformationOrientationChangedTraits::attach_handler(
    event_interface *pSource,
    wf::ITypedEventHandler<wgrd::DisplayInformation*, IInspectable*> *pHandler,
    EventRegistrationToken *pToken)
{
    RRETURN(pSource->add_OrientationChanged(pHandler, pToken));
}

_Check_return_ HRESULT DisplayInformationOrientationChangedTraits::detach_handler(
    event_interface *pSource,
    EventRegistrationToken token)
{
    RRETURN(pSource->remove_OrientationChanged(token));
}

_Check_return_ HRESULT VisibleIndicesUpdatedTraits::attach_handler(
    IModernCollectionBasePanel *pSource,
    wf::IEventHandler<IInspectable*> *pHandler,
    EventRegistrationToken *pToken)
{
    return static_cast<ModernCollectionBasePanel*>(pSource)->add_VisibleIndicesUpdated(pHandler, pToken);
}

_Check_return_ HRESULT VisibleIndicesUpdatedTraits::detach_handler(
    IModernCollectionBasePanel *pSource,
    EventRegistrationToken token)
{
    return static_cast<ModernCollectionBasePanel*>(pSource)->remove_VisibleIndicesUpdated(token);
}
