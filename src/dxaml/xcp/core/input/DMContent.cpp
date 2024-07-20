// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  CDMContent class.
//
//------------------------------------------------------------------------

//------------------------------------------------------------------------
//
//  Method:   CDMContent::Create    (static)
//
//  Synopsis:
//      Creates an instance of the CDMContent class
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CDMContent::Create(
_In_ CUIElement* pContentElement,
_In_ XDMContentType contentType,
_Outptr_ CDMContent** ppContent)
{
    HRESULT hr = S_OK;
    CDMContent* pContent = NULL;

    IFCPTR(ppContent);
    *ppContent = NULL;

    IFCPTR(pContentElement);

    pContent = new CDMContent(
        pContentElement,
        contentType);

    *ppContent = pContent;
    pContent = NULL;

Cleanup:
    delete pContent;
    RRETURN(hr);
}

void
CDMContent::SetSecondaryContentRelationship(
_In_ CSecondaryContentRelationship *pSecondaryContentRelationship)
{
    ReplaceInterface(m_pSecondaryContentRelationship, pSecondaryContentRelationship);
}

CDMContent::~CDMContent()
{
    ReleaseInterface(m_pContentElement);
    ReleaseInterface(m_pCompositorSecondaryContent);
    ReleaseInterface(m_pSecondaryContentRelationship);

#ifdef DM_DEBUG
    if (DMCNTNT_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMCNTNT_DBG) /*traceType*/, L"DMC[0x%p]:   ~CDMContent destructor.", this));
    }
#endif // DM_DEBUG
}

