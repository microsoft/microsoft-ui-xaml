// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AutomationPeerAnnotation.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT 
AutomationPeerAnnotationFactory::CreateInstanceImpl(
    _In_ xaml_automation::AnnotationType type,
    _Outptr_ xaml_automation_peers::IAutomationPeerAnnotation** ppInstance)
{
    ctl::ComPtr<AutomationPeerAnnotation> spAnnotation;

    IFC_RETURN(ctl::make<AutomationPeerAnnotation>(&spAnnotation));

    IFC_RETURN(spAnnotation->put_Type(type));

    *ppInstance = spAnnotation.Detach();

    return S_OK;
}

_Check_return_ HRESULT
AutomationPeerAnnotationFactory::CreateWithPeerParameterImpl(
    _In_ xaml_automation::AnnotationType type,
    _In_ xaml_automation_peers::IAutomationPeer* pPeer,
    _Outptr_ xaml_automation_peers::IAutomationPeerAnnotation** ppInstance)
{
    ctl::ComPtr<AutomationPeerAnnotation> spAnnotation;

    IFC_RETURN(ctl::make<AutomationPeerAnnotation>(&spAnnotation));

    IFC_RETURN(spAnnotation->put_Type(type));
    IFC_RETURN(spAnnotation->put_Peer(pPeer));

    *ppInstance = spAnnotation.Detach();

    return S_OK;
}
