// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PointerSourceWrapper.h"
#include "DXamlCore.h"
#include "Window_Partial.h"

#include <dcomprestricted.h>

HRESULT PointerSourceWrapper::Initialize(_In_ CUIElement* hoverPointerSourceElement)
{
    wrl::ComPtr<IUnknown> handOffVisual;
    IFC_RETURN(hoverPointerSourceElement->GetHandOffVisual(&handOffVisual));

    wrl::ComPtr<WUComp::IVisual> visual;
    IFC_RETURN(handOffVisual.As(&visual));

    wrl::ComPtr<WUComp::ICompositor> compositor;
    {
        wrl::ComPtr<WUComp::ICompositionObject> visualAsCO;
        IFC_RETURN(visual.As(&visualAsCO));
        IFC_RETURN(visualAsCO->get_Compositor(&compositor));
    }

    // m_realPointerSourceCO
    {
        wrl::ComPtr<WUComp::ICompositorRestricted> compositorRestricted;
        IFC_RETURN(compositor.As(&compositorRestricted));

        wrl::ComPtr<WUComp::IHoverPointerSourcePartner> hoverPointerSourcePartner;
        IFC_RETURN(compositorRestricted->CreateHoverPointerSource(visual.Get(), &hoverPointerSourcePartner));
        IFC_RETURN(hoverPointerSourcePartner.As(&m_realPointerSourceCO));
    }

    // m_realPointerPointAnimation
    {
        wrl::ComPtr<WUComp::IExpressionAnimation> expressionAnimation;
        IFC_RETURN(compositor->CreateExpressionAnimationWithExpression(
            wrl_wrappers::HStringReference(L"hover.Point").Get(),
            &expressionAnimation));
        IFC_RETURN(expressionAnimation.As(&m_realPointerPointAnimation));

        IFC_RETURN(m_realPointerPointAnimation->SetReferenceParameter(
            wrl_wrappers::HStringReference(L"hover").Get(),
            m_realPointerSourceCO.Get()));
    }

    IFC_RETURN(compositor->CreatePropertySet(&m_pointerSourceProxy));
    IFC_RETURN(m_pointerSourceProxy.As(&m_pointerSourceProxyCO));

    IFC_RETURN(m_pointerSourceProxy->InsertVector2(wrl_wrappers::HStringReference(L"Point").Get(), { 0, 0 }));
    IFC_RETURN(m_pointerSourceProxyCO->StartAnimation(wrl_wrappers::HStringReference(L"Point").Get(), m_realPointerPointAnimation.Get()));

    return S_OK;
}