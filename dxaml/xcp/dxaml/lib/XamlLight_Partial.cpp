// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlLight.g.h"
#include "Brush.g.h"
#include "UIElement.g.h"
#include <XamlLight.h>

using namespace DirectUI;

_Check_return_ HRESULT XamlLight::GetIdImpl(_Out_ HSTRING* id)
{
    return S_OK;
}

_Check_return_ HRESULT XamlLight::OnConnectedImpl(_In_ xaml::IUIElement* pNewElement)
{
    return S_OK;
}

_Check_return_ HRESULT XamlLight::OnDisconnectedImpl(_In_ xaml::IUIElement* pOldElement)
{
    return S_OK;
}

_Check_return_ HRESULT XamlLight::get_CompositionLightImpl(_Outptr_result_maybenull_ WUComp::ICompositionLight** ppValue)
{
    CXamlLight* xamlLight = static_cast<CXamlLight*>(GetHandle());
    wrl::ComPtr<WUComp::ICompositionLight> wucLight = xamlLight->GetWUCLight();
    *ppValue = wucLight.Detach();
    return S_OK;
}

_Check_return_ HRESULT XamlLight::put_CompositionLightImpl(_In_opt_ WUComp::ICompositionLight* pValue)
{
    CXamlLight* xamlLight = static_cast<CXamlLight*>(GetHandle());
    xamlLight->SetWUCLight(pValue);
    return S_OK;
}

_Check_return_ HRESULT XamlLightFactory::AddTargetElementImpl(_In_ HSTRING lightId, _In_ xaml::IUIElement* element)
{
    xstring_ptr lightIdXstringPtr;
    IFCFAILFAST(xstring_ptr::CloneRuntimeStringHandle(lightId, &lightIdXstringPtr));

    UIElement* wuxUIElement = static_cast<UIElement*>(element);
    CUIElement* pElement = static_cast<CUIElement*>(wuxUIElement->GetHandle());
    pElement->AddLightTargetId(lightIdXstringPtr);

    return S_OK;
}

_Check_return_ HRESULT XamlLightFactory::RemoveTargetElementImpl(_In_ HSTRING lightId, _In_ xaml::IUIElement* element)
{
    xstring_ptr lightIdXstringPtr;
    IFCFAILFAST(xstring_ptr::CloneRuntimeStringHandle(lightId, &lightIdXstringPtr));

    UIElement* wuxUIElement = static_cast<UIElement*>(element);
    CUIElement* pElement = static_cast<CUIElement*>(wuxUIElement->GetHandle());
    pElement->RemoveLightTargetId(lightIdXstringPtr);

    return S_OK;
}

_Check_return_ HRESULT XamlLightFactory::AddTargetBrushImpl(_In_ HSTRING lightId, _In_ xaml_media::IBrush* brush)
{
    xstring_ptr lightIdXstringPtr;
    IFCFAILFAST(xstring_ptr::CloneRuntimeStringHandle(lightId, &lightIdXstringPtr));

    Brush* wuxBrush = static_cast<Brush*>(brush);
    CBrush* cbrush = static_cast<CBrush*>(wuxBrush->GetHandle());
    cbrush->AddLightTargetId(lightIdXstringPtr);

    return S_OK;
}

_Check_return_ HRESULT XamlLightFactory::RemoveTargetBrushImpl(_In_ HSTRING lightId, _In_ xaml_media::IBrush* brush)
{
    xstring_ptr lightIdXstringPtr;
    IFCFAILFAST(xstring_ptr::CloneRuntimeStringHandle(lightId, &lightIdXstringPtr));

    Brush* wuxBrush = static_cast<Brush*>(brush);
    CBrush* cbrush = static_cast<CBrush*>(wuxBrush->GetHandle());
    cbrush->RemoveLightTargetId(lightIdXstringPtr);

    return S_OK;
}
