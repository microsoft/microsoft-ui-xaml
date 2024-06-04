// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Span.g.h"
#include "InlineCollection.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

//-----------------------------------------------------------------------------
//
//  OnDisconnectVisualChildren
//
//  During a DisconnectVisualChildrenRecursive tree walk, clear Inlines property as well.
//
//-----------------------------------------------------------------------------

IFACEMETHODIMP
Span::OnDisconnectVisualChildren()
{
    HRESULT hr = S_OK;
    wfc::IVector<xaml_docs::Inline*> *pInlines = NULL;
    InlineCollection *pInlineCollection = NULL;

    IFC( get_Inlines( &pInlines ));
    pInlineCollection = static_cast<InlineCollection*>( pInlines );

    IFC( pInlineCollection->DisconnectVisualChildrenRecursive() );

    IFC( SpanGenerated::OnDisconnectVisualChildren() );

Cleanup:

    ReleaseInterface( pInlines );
    RRETURN(hr);
}

_Check_return_ HRESULT Span::AppendAutomationPeerChildren(_In_ wfc::IVector<xaml_automation_peers::AutomationPeer*>* pAutomationPeerChildren, _In_ INT startPos, _In_ INT endPos)
{
    HRESULT hr = S_OK;
    UINT count = 0;
    ctl::ComPtr<wfc::IVector<xaml_docs::Inline*>> spInlines;
    ctl::ComPtr<IInline> spInline;
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;
    ctl::ComPtr<xaml_docs::ITextPointer> spInlineStart;
    INT posInlineStart = -1;

    IFCPTR(pAutomationPeerChildren);

    IFC(GetOrCreateAutomationPeer(&spAutomationPeer));
    if (spAutomationPeer)
    {
        IFC(pAutomationPeerChildren->Append(spAutomationPeer.Get()));
    }
    else
    {
        IFC(get_Inlines(&spInlines));
        IFC(spInlines->get_Size(&count));
        for (UINT i = 0; i < count; i++)
        {
            IFC(spInlines->GetAt(i, &spInline));
            IFC(spInline.Cast<Inline>()->get_ContentStart(&spInlineStart));
            IFC(spInlineStart->get_Offset(&posInlineStart));
            if (startPos <= posInlineStart && posInlineStart <= endPos)
            {
                IFC(spInline.Cast<Inline>()->AppendAutomationPeerChildren(pAutomationPeerChildren, startPos, endPos));
            }
        }
    }
Cleanup:
    RRETURN(hr);
}


