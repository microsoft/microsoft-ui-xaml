// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DataTemplateSelector.g.h"
#include "DataTemplate.g.h"
#include "UIElement.g.h"

_Check_return_ HRESULT DirectUI::DataTemplateSelector::SelectTemplateCoreImpl(_In_opt_ IInspectable* item, _In_ xaml::IDependencyObject* container, _Outptr_ xaml::IDataTemplate** returnValue)
{
    *returnValue = nullptr;
    RRETURN(S_OK);
}

_Check_return_ HRESULT DirectUI::DataTemplateSelector::SelectTemplateForItemCoreImpl(_In_opt_ IInspectable* item, _Outptr_ xaml::IDataTemplate** returnValue)
{
    *returnValue = nullptr;
    RRETURN(S_OK);
}

_Check_return_ HRESULT DirectUI::DataTemplateSelector::GetElementImpl(_In_ xaml::IElementFactoryGetArgs* args, _Outptr_ xaml::IUIElement** result)
{
    *result = nullptr;
    ctl::ComPtr<IInspectable> dataContext;
    IFC_RETURN(args->get_Data(&dataContext));

    ctl::ComPtr<xaml::IDataTemplate> selectedTemplate;
    IFC_RETURN(SelectTemplateForItem(dataContext.Get(), &selectedTemplate));

    if (!selectedTemplate)
    {
        // SelectTemplateForItem was not implemented or returned null, fallback
        // to the SelectTemplate overload with null container.
        IFC_RETURN(SelectTemplate(dataContext.Get(), nullptr /* container */, &selectedTemplate));
    }

    if (selectedTemplate)
    {
        ctl::ComPtr<xaml::IUIElement> element;
        IFC_RETURN(static_cast<DataTemplate*>(selectedTemplate.Get())->GetElement(args, &element));
        *result = element.Detach();
    }

    return S_OK;
}

_Check_return_ HRESULT DirectUI::DataTemplateSelector::RecycleElementImpl(_In_ xaml::IElementFactoryRecycleArgs* args)
{
    ctl::ComPtr<xaml::IUIElement> element;
    IFC_RETURN(args->get_Element(&element));

    auto container = static_cast<UIElement*>(element.Get());
    auto virtualizationInformation = container->GetVirtualizationInformation();
    auto dataTemplate = virtualizationInformation->GetSelectedTemplate();
    IFC_RETURN(static_cast<DataTemplate*>(dataTemplate.Get())->RecycleElement(args));
    return S_OK;
}