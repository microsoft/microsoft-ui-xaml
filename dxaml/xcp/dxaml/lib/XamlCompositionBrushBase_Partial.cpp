// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlCompositionBrushBase.g.h"
#include "XamlCompositionBrush.h"
#include "UIElement.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT
XamlCompositionBrushBase::get_CompositionBrushImpl(_Outptr_result_maybenull_ WUComp::ICompositionBrush** ppValue)
{
    CXamlCompositionBrush* coreBrush = static_cast<CXamlCompositionBrush*>(GetHandle());
    IFC_RETURN(coreBrush->GetCompositionBrush(nullptr, nullptr, ppValue));

    return S_OK;
}

_Check_return_ HRESULT
XamlCompositionBrushBase::put_CompositionBrushImpl(_In_opt_ WUComp::ICompositionBrush* pValue)
{
    CXamlCompositionBrush* coreBrush = static_cast<CXamlCompositionBrush*>(GetHandle());
    coreBrush->SetCompositionBrush(pValue);

    return S_OK;
}


// Default implementation
_Check_return_ HRESULT
XamlCompositionBrushBase::OnConnectedImpl()
{
    return S_OK;
}

// Triggers call to app's OnDisconnected()
_Check_return_ HRESULT
XamlCompositionBrushBase::OnConnectedFromCore(_In_ CDependencyObject* object)
{
    ctl::ComPtr<DependencyObject> peerAsDO;
    ctl::ComPtr<XamlCompositionBrushBase> peer;

    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(object, &peerAsDO));
    IFC_RETURN(peerAsDO.As(&peer));

    IFC_RETURN(peer->OnConnectedHelper());

    return S_OK;
}

// Call app's OnConnected override. Typically, an app would construct the WUC brush
// in this function and assign XCBB.CompositionBrush property, however the brush could also
// have been assigned earlier.
_Check_return_ HRESULT
XamlCompositionBrushBase::OnConnectedHelper()
{
    IFC_RETURN(OnConnectedProtected());
    return S_OK;
}

// Default implementation (no app-level cleanup)
_Check_return_ HRESULT
XamlCompositionBrushBase::OnDisconnectedImpl()
{
    return S_OK;
}

// Triggers call to app's OnDisconnected()
_Check_return_ HRESULT
XamlCompositionBrushBase::OnDisconnectedFromCore(_In_ CDependencyObject* object)
{
    ctl::ComPtr<DependencyObject> peerAsDO;
    ctl::ComPtr<XamlCompositionBrushBase> peer;

    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(object, &peerAsDO));
    IFC_RETURN(peerAsDO.As(&peer));

    IFC_RETURN(peer->OnDisconnectedHelper());

    return S_OK;
}

// Invoke user provided OnDisconnected function. Note we do not dispose the WUC brush here.
_Check_return_ HRESULT
XamlCompositionBrushBase::OnDisconnectedHelper()
{
    IFC_RETURN(OnDisconnectedProtected());
    return S_OK;
}

_Check_return_ HRESULT
XamlCompositionBrushBase::OnElementConnectedFromCore(_In_ CDependencyObject* object, _In_ CDependencyObject* connectedElement)
{
    ctl::ComPtr<XamlCompositionBrushBase> thisPeer;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer<XamlCompositionBrushBase>(object, &thisPeer));

    if (thisPeer->GetPrivateOverridesNoRef())
    {
        ctl::ComPtr<UIElement> elementPeer;
        IFC_RETURN(DXamlCore::GetCurrent()->GetPeer<UIElement>(connectedElement, &elementPeer));

        IFC_RETURN(thisPeer->GetPrivateOverridesNoRef()->OnElementConnected(elementPeer.Get()));
    }

    return S_OK;
}

bool XamlCompositionBrushBase::HasPrivateOverrides(_In_ CDependencyObject* object)
{
    ctl::ComPtr<XamlCompositionBrushBase> thisPeer;
    IFCFAILFAST(DXamlCore::GetCurrent()->GetPeer<XamlCompositionBrushBase>(object, &thisPeer));
    return thisPeer->GetPrivateOverridesNoRef() != nullptr;
}

IXamlCompositionBrushBaseOverridesPrivate* XamlCompositionBrushBase::GetPrivateOverridesNoRef()
{
    if (!m_didQueryForPrivateOverrides)
    {
        ctl::ComPtr<xaml_media::IXamlCompositionBrushBaseOverridesPrivate> thisAsPrivateOverrides;
        // It's OK if this fails, we just won't call the private overrides in this case
        thisAsPrivateOverrides.Attach(ctl::query_interface<IXamlCompositionBrushBaseOverridesPrivate>(this));

        m_privateOverridesNoRef = thisAsPrivateOverrides.Get();
        m_didQueryForPrivateOverrides = true;
    }
    return m_privateOverridesNoRef;
}

_Check_return_ HRESULT XamlCompositionBrushBase::SetBrushForXamlRootImpl(
    _In_ IInspectable* contentRoot,
    _In_ WUComp::ICompositionBrush* brush)
{
    ctl::ComPtr<xaml::IXamlRoot> contentRootPeer;
    IFC_RETURN(contentRoot->QueryInterface(IID_PPV_ARGS(&contentRootPeer)));

    ctl::ComPtr<xaml::IUIElement> root;
    VERIFYHR(contentRootPeer->get_Content(&root));
    CContentRoot* coreContentRoot = VisualTree::GetContentRootForElement(static_cast<UIElement*>(root.Get())->GetHandle());

    CXamlCompositionBrush* coreBrush = static_cast<CXamlCompositionBrush*>(GetHandle());
    coreBrush->SetBrushForContentRoot(coreContentRoot, brush);

    return S_OK;
}

_Check_return_ HRESULT XamlCompositionBrushBase::GetBrushForXamlRootImpl(
    _In_ IInspectable* contentRoot,
    _Outptr_result_maybenull_ WUComp::ICompositionBrush** ppBrush)
{
    *ppBrush = nullptr;

    ctl::ComPtr<xaml::IXamlRoot> contentRootPeer;
    IFC_RETURN(contentRoot->QueryInterface(IID_PPV_ARGS(&contentRootPeer)));

    ctl::ComPtr<xaml::IUIElement> root;
    VERIFYHR(contentRootPeer->get_Content(&root));
    CContentRoot* coreContentRoot = VisualTree::GetContentRootForElement(static_cast<UIElement*>(root.Get())->GetHandle());

    CXamlCompositionBrush* coreBrush = static_cast<CXamlCompositionBrush*>(GetHandle());

    WUComp::ICompositionBrush* pBrush = coreBrush->GetBrushForContentRootNoRef(coreContentRoot);
    SetInterface(*ppBrush, pBrush);

    return S_OK;
}

_Check_return_ HRESULT XamlCompositionBrushBase::ClearCompositionBrushMapImpl()
{
    CXamlCompositionBrush* coreBrush = static_cast<CXamlCompositionBrush*>(GetHandle());
    coreBrush->ClearCompositionBrushMap();
    return S_OK;
}

_Check_return_ HRESULT XamlCompositionBrushBase::ClearBrushForXamlRootImpl(
    _In_ IInspectable* contentRoot,
    _In_ WUComp::ICompositionBrush* brush)
{
    ctl::ComPtr<xaml::IXamlRoot> contentRootPeer;
    IFC_RETURN(contentRoot->QueryInterface(IID_PPV_ARGS(&contentRootPeer)));

    ctl::ComPtr<xaml::IUIElement> root;
    VERIFYHR(contentRootPeer->get_Content(&root));
    CContentRoot* coreContentRoot = VisualTree::GetContentRootForElement(static_cast<UIElement*>(root.Get())->GetHandle());

    CXamlCompositionBrush* coreBrush = static_cast<CXamlCompositionBrush*>(GetHandle());
    coreBrush->ClearBrushForContentRoot(coreContentRoot, brush);

    return S_OK;
}