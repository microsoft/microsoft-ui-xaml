// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CommandingHelpers.h"
#include "AutomationProperties.h"
#include "BindingExpression.g.h"
#include "IconElement.g.h"
#include "IconSource.g.h"
#include "IconSourceElement.g.h"
#include "KeyboardAccelerator.g.h"
#include "KeyboardAcceleratorCollection.g.h"
#include "ToolTipService.g.h"
#include "Value.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

class IconSourceToIconSourceElementConverter :
    public xaml_data::IValueConverter,
    public ctl::ComBase
{
public:
    IFACEMETHOD(Convert)(
        _In_ IInspectable* value,
        _In_ wxaml_interop::TypeName targetType,
        _In_opt_ IInspectable* parameter,
        _In_ HSTRING language,
        _Outptr_ IInspectable** returnValue)
    {
        if (value)
        {
            ctl::ComPtr<IInspectable> valueAsI(value);
            ctl::ComPtr<IIconSource> valueAsIconSource;
            ctl::ComPtr<IconSourceElement> returnValueAsIconElement;

            IFC_RETURN(valueAsI.As(&valueAsIconSource));
            IFC_RETURN(ctl::make(&returnValueAsIconElement));
            IFC_RETURN(returnValueAsIconElement->put_IconSource(valueAsIconSource.Get()));
            IFC_RETURN(returnValueAsIconElement.CopyTo(returnValue));
        }
        else
        {
            *returnValue = nullptr;
        }

        return S_OK;
    }

    IFACEMETHOD(ConvertBack)(
        _In_ IInspectable* value,
        _In_ wxaml_interop::TypeName targetType,
        _In_opt_ IInspectable* parameter,
        _In_ HSTRING language,
        _Outptr_ IInspectable** returnValue)
    {
        return E_NOTIMPL;
    }

protected:
    HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** object) override
    {
        if (InlineIsEqualGUID(iid, __uuidof(xaml_data::IValueConverter)))
        {
            *object = static_cast<xaml_data::IValueConverter*>(this);
        }
        else
        {
            return ctl::ComBase::QueryInterfaceImpl(iid, object);
        }

        AddRefOuter();
        return S_OK;
    }
};

class KeyboardAcceleratorCopyConverter :
    public xaml_data::IValueConverter,
    public ctl::ComBase
{
public:
    IFACEMETHOD(Convert)(
        _In_ IInspectable* value,
        _In_ wxaml_interop::TypeName targetType,
        _In_opt_ IInspectable* parameter,
        _In_ HSTRING language,
        _Outptr_ IInspectable** returnValue)
    {
        if (value)
        {
            ctl::ComPtr<IInspectable> valueAsI(value);
            ctl::ComPtr<wfc::IVector<xaml_input::KeyboardAccelerator*>> valueAsKeyboardAccelerators;
            ctl::ComPtr<KeyboardAcceleratorCollection> returnValueAsKeyboardAcceleratorCollection;

            IFC_RETURN(valueAsI.As(&valueAsKeyboardAccelerators));
            ctl::make(&returnValueAsKeyboardAcceleratorCollection);
            UINT keyboardAcceleratorCount;

            IFC_RETURN(valueAsKeyboardAccelerators->get_Size(&keyboardAcceleratorCount));

            // Keyboard accelerators can't have two parents,
            // so we'll need to copy them and bind to the original properties
            // instead of assigning them.
            // We set up bindings so that modifications to the app-defined accelerators
            // will propagate to the accelerators that are used by the framework.
            for (UINT i = 0; i < keyboardAcceleratorCount; i++)
            {
                ctl::ComPtr<xaml_input::IKeyboardAccelerator> keyboardAccelerator;
                IFC_RETURN(valueAsKeyboardAccelerators->GetAt(i, &keyboardAccelerator));

                ctl::ComPtr<KeyboardAccelerator> keyboardAcceleratorCopy;
                IFC_RETURN(ctl::make(&keyboardAcceleratorCopy));

                IFC_RETURN(DXamlCore::SetBinding(keyboardAccelerator.Get(), wrl_wrappers::HStringReference(L"IsEnabled").Get(), keyboardAcceleratorCopy.Get(), KnownPropertyIndex::KeyboardAccelerator_IsEnabled));
                IFC_RETURN(DXamlCore::SetBinding(keyboardAccelerator.Get(), wrl_wrappers::HStringReference(L"Key").Get(), keyboardAcceleratorCopy.Get(), KnownPropertyIndex::KeyboardAccelerator_Key));
                IFC_RETURN(DXamlCore::SetBinding(keyboardAccelerator.Get(), wrl_wrappers::HStringReference(L"Modifiers").Get(), keyboardAcceleratorCopy.Get(), KnownPropertyIndex::KeyboardAccelerator_Modifiers));
                IFC_RETURN(DXamlCore::SetBinding(keyboardAccelerator.Get(), wrl_wrappers::HStringReference(L"ScopeOwner").Get(), keyboardAcceleratorCopy.Get(), KnownPropertyIndex::KeyboardAccelerator_ScopeOwner));
                IFC_RETURN(returnValueAsKeyboardAcceleratorCollection->Append(keyboardAcceleratorCopy.Get()));
            }

            IFC_RETURN(returnValueAsKeyboardAcceleratorCollection.CopyTo(returnValue));
        }
        else
        {
            *returnValue = nullptr;
        }

        return S_OK;
    }

    IFACEMETHOD(ConvertBack)(
        _In_ IInspectable* value,
        _In_ wxaml_interop::TypeName targetType,
        _In_opt_ IInspectable* parameter,
        _In_ HSTRING language,
        _Outptr_ IInspectable** returnValue)
    {
        return E_NOTIMPL;
    }

protected:
    HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** object) override
    {
        if (InlineIsEqualGUID(iid, __uuidof(xaml_data::IValueConverter)))
        {
            *object = static_cast<xaml_data::IValueConverter*>(this);
        }
        else
        {
            return ctl::ComBase::QueryInterfaceImpl(iid, object);
        }

        AddRefOuter();
        return S_OK;
    }
};

_Check_return_ HRESULT CommandingHelpers::BindToLabelPropertyIfUnset(
    _In_ IXamlUICommand* uiCommand,
    _In_ DependencyObject* target,
    _In_ KnownPropertyIndex labelPropertyIndex)
{
    ctl::ComPtr<IInspectable> localLabelAsI;
    wrl_wrappers::HString localLabel;
    IFC_RETURN(target->ReadLocalValue(MetadataAPI::GetDependencyPropertyByIndex(labelPropertyIndex), &localLabelAsI));

    if (localLabelAsI)
    {
        IFC_RETURN(FrameworkElement::GetStringFromObject(localLabelAsI.Get(), localLabel.ReleaseAndGetAddressOf()));
    }

    if (!localLabel || localLabel.IsEmpty())
    {
        IFC_RETURN(DXamlCore::SetBinding(ctl::as_iinspectable(uiCommand), wrl_wrappers::HStringReference(L"Label").Get(), target, labelPropertyIndex));
    }

    return S_OK;
}

_Check_return_ HRESULT CommandingHelpers::BindToIconPropertyIfUnset(
    _In_ IXamlUICommand* uiCommand,
    _In_ DependencyObject* target,
    _In_ KnownPropertyIndex iconPropertyIndex)
{
    ctl::ComPtr<IInspectable> localIconAsI;
    ctl::ComPtr<xaml_controls::IIconElement> localIcon;
    IFC_RETURN(target->ReadLocalValue(MetadataAPI::GetDependencyPropertyByIndex(iconPropertyIndex), &localIconAsI));

    localIcon = localIconAsI.AsOrNull<xaml_controls::IIconElement>();

    if (!localIcon)
    {
        ctl::ComPtr<IconSourceToIconSourceElementConverter> converter;
        IFC_RETURN(ctl::make(&converter));
        IFC_RETURN(DXamlCore::SetBinding(ctl::as_iinspectable(uiCommand), wrl_wrappers::HStringReference(L"IconSource").Get(), target, iconPropertyIndex, converter.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT CommandingHelpers::BindToIconSourcePropertyIfUnset(
    _In_ IXamlUICommand* uiCommand,
    _In_ DependencyObject* target,
    _In_ KnownPropertyIndex iconSourcePropertyIndex)
{
    ctl::ComPtr<IInspectable> localIconSourceAsI;
    ctl::ComPtr<xaml_controls::IIconSource> localIconSource;
    IFC_RETURN(target->ReadLocalValue(MetadataAPI::GetDependencyPropertyByIndex(iconSourcePropertyIndex), &localIconSourceAsI));

    localIconSource = localIconSourceAsI.AsOrNull<xaml_controls::IIconSource>();

    if (!localIconSource)
    {
        IFC_RETURN(DXamlCore::SetBinding(ctl::as_iinspectable(uiCommand), wrl_wrappers::HStringReference(L"IconSource").Get(), target, iconSourcePropertyIndex));
    }

    return S_OK;
}

_Check_return_ HRESULT CommandingHelpers::BindToKeyboardAcceleratorsIfUnset(
    _In_ IXamlUICommand* uiCommand,
    _In_ UIElement* target)
{
    ctl::ComPtr<wfc::IVector<xaml_input::KeyboardAccelerator*>> targetKeyboardAccelerators;
    UINT targetKeyboardAcceleratorCount;

    IFC_RETURN(target->get_KeyboardAccelerators(&targetKeyboardAccelerators));
    IFC_RETURN(targetKeyboardAccelerators->get_Size(&targetKeyboardAcceleratorCount));

    if (targetKeyboardAcceleratorCount == 0)
    {
        ctl::ComPtr<KeyboardAcceleratorCopyConverter> converter;
        IFC_RETURN(ctl::make(&converter));
        IFC_RETURN(DXamlCore::SetBinding(ctl::as_iinspectable(uiCommand), wrl_wrappers::HStringReference(L"KeyboardAccelerators").Get(), target, KnownPropertyIndex::UIElement_KeyboardAccelerators, converter.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT CommandingHelpers::BindToAccessKeyIfUnset(
    _In_ IXamlUICommand* uiCommand,
    _In_ UIElement* target)
{
    wrl_wrappers::HString localAccessKey;
    IFC_RETURN(target->get_AccessKey(localAccessKey.ReleaseAndGetAddressOf()));

    if (!localAccessKey || localAccessKey.IsEmpty())
    {
        IFC_RETURN(DXamlCore::SetBinding(ctl::as_iinspectable(uiCommand), wrl_wrappers::HStringReference(L"AccessKey").Get(), target, KnownPropertyIndex::UIElement_AccessKey));
    }

    return S_OK;
}

_Check_return_ HRESULT CommandingHelpers::BindToDescriptionPropertiesIfUnset(
    _In_ IXamlUICommand* uiCommand,
    _In_ DependencyObject* target)
{
    wrl_wrappers::HString descriptionFromCommand;
    IFC_RETURN(uiCommand->get_Description(descriptionFromCommand.ReleaseAndGetAddressOf()));

    wrl_wrappers::HString localHelpText;
    IFC_RETURN(AutomationProperties::GetHelpTextStatic(target, localHelpText.ReleaseAndGetAddressOf()));

    if (!localHelpText || localHelpText.IsEmpty())
    {
        IFC_RETURN(DXamlCore::SetBinding(ctl::as_iinspectable(uiCommand), wrl_wrappers::HStringReference(L"Description").Get(), target, KnownPropertyIndex::AutomationProperties_HelpText));
    }

    ctl::ComPtr<IInspectable> localToolTipAsI;
    IFC_RETURN(ToolTipServiceFactory::GetToolTipStatic(target, &localToolTipAsI));

    wrl_wrappers::HString localToolTipAsString;
    ctl::ComPtr<IToolTip> localToolTip;

    if (localToolTipAsI)
    {
        IFC_RETURN(FrameworkElement::GetStringFromObject(localToolTipAsI.Get(), localToolTipAsString.ReleaseAndGetAddressOf()));
        localToolTip = localToolTipAsI.AsOrNull<IToolTip>();
    }

    if ((!localToolTipAsString || localToolTipAsString.IsEmpty()) && !localToolTip)
    {
        IFC_RETURN(DXamlCore::SetBinding(ctl::as_iinspectable(uiCommand), wrl_wrappers::HStringReference(L"Description").Get(), target, KnownPropertyIndex::ToolTipService_ToolTip));
    }

    return S_OK;
}

_Check_return_ HRESULT CommandingHelpers::ClearBindingIfSet(
    _In_ xaml_input::IXamlUICommand* uiCommand,
    _In_ FrameworkElement* target,
    _In_ KnownPropertyIndex targetPropertyIndex)
{
    ctl::ComPtr<IBindingExpression> bindingExpression;
    IFC_RETURN(target->GetBindingExpression(MetadataAPI::GetDependencyPropertyByIndex(targetPropertyIndex), &bindingExpression));

    if (bindingExpression)
    {
        ctl::ComPtr<IInspectable> bindingSource;
        IFC_RETURN(bindingExpression.Cast<BindingExpression>()->GetSource(&bindingSource));

        if (bindingSource && ctl::are_equal(bindingSource.Get(), uiCommand))
        {
            IFC_RETURN(target->ClearValue(MetadataAPI::GetDependencyPropertyByIndex(targetPropertyIndex)));
        }
    }

    return S_OK;
}
