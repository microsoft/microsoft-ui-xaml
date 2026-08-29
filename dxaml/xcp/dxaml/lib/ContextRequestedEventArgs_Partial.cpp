// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ContextRequestedEventArgs.g.h"
#include "CoreEventArgsGroup.h"

using namespace DirectUI;

_Check_return_ HRESULT ContextRequestedEventArgs::TryGetPositionImpl(
    _In_opt_ xaml::IUIElement* pRelativeTo,
    _Out_ wf::Point* pPosition,
    _Out_ BOOLEAN* pReturnValue)
{
    xref_ptr<CEventArgs> pCoreEventArgs;

    CUIElement* pRelativeToCore = static_cast<CUIElement*>(pRelativeTo ? static_cast<DirectUI::UIElement*>(pRelativeTo)->GetHandle() : nullptr);
    wf::Point* pPositionCore = pPosition;
    BOOLEAN returnValueCore;

    ARG_VALIDRETURNPOINTER(pReturnValue);

    pCoreEventArgs.attach(GetCorePeer());
    IFC_RETURN(static_cast<CContextRequestedEventArgs*>(pCoreEventArgs.get())->TryGetPosition(pRelativeToCore, pPositionCore, &returnValueCore));

    IFC_RETURN(CValueBoxer::ConvertToFramework(returnValueCore, pReturnValue, /* fReleaseCoreValue */ TRUE));

    return S_OK;
}
