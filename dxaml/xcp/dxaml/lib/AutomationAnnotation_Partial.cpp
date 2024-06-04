// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AutomationAnnotation.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT
AutomationAnnotationFactory::CreateInstanceImpl(
    _In_ xaml_automation::AnnotationType type,
    _Outptr_ xaml_automation::IAutomationAnnotation** ppInstance)
{
    ctl::ComPtr<AutomationAnnotation> spAnnotation;

    IFC_RETURN(ctl::make<AutomationAnnotation>(&spAnnotation));

    IFC_RETURN(spAnnotation->put_Type(type));

    *ppInstance = spAnnotation.Detach();

    return S_OK;
}

_Check_return_ HRESULT
AutomationAnnotationFactory::CreateWithElementParameterImpl(
    _In_ xaml_automation::AnnotationType type,
    _In_ xaml::IUIElement* pElement,
    _Outptr_ xaml_automation::IAutomationAnnotation** ppInstance)
{
    ctl::ComPtr<AutomationAnnotation> spAnnotation;

    IFC_RETURN(ctl::make<AutomationAnnotation>(&spAnnotation));

    IFC_RETURN(spAnnotation->put_Type(type));
    IFC_RETURN(spAnnotation->put_Element(pElement));

    *ppInstance = spAnnotation.Detach();

    return S_OK;
}
