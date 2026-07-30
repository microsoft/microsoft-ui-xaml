// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CommandingHelpers.h"
#include "AutomationProperties.h"
#include "BindingExpression.g.h"
#include "BindingExpressionBase_Partial.h"
#include "DirectSourceBindingExpression.h"
#include "FrameworkUdk/Containment.h"

// Bug 62639407: Use lightweight DirectSourceBindingExpression for XamlUICommand bindings
#define WINAPPSDK_CHANGEID_62639407 62639407
// Bug 62676766: Skip binding keyboard accelerators when command has none
#define WINAPPSDK_CHANGEID_62676766 62676766
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
        wxaml_interop::TypeName targetType,
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
        wxaml_interop::TypeName targetType,
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
        wxaml_interop::TypeName targetType,
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

                if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_62639407>())
                {
                    if (auto sourceDO = ctl::query_interface_cast<DependencyObject>(keyboardAccelerator.Get()))
                    {
                        IFC_RETURN(DXamlCore::SetDirectBinding(sourceDO.Get(), KnownPropertyIndex::KeyboardAccelerator_IsEnabled, keyboardAcceleratorCopy.Get(), KnownPropertyIndex::KeyboardAccelerator_IsEnabled));
                        IFC_RETURN(DXamlCore::SetDirectBinding(sourceDO.Get(), KnownPropertyIndex::KeyboardAccelerator_Key, keyboardAcceleratorCopy.Get(), KnownPropertyIndex::KeyboardAccelerator_Key));
                        IFC_RETURN(DXamlCore::SetDirectBinding(sourceDO.Get(), KnownPropertyIndex::KeyboardAccelerator_Modifiers, keyboardAcceleratorCopy.Get(), KnownPropertyIndex::KeyboardAccelerator_Modifiers));
                        IFC_RETURN(DXamlCore::SetDirectBinding(sourceDO.Get(), KnownPropertyIndex::KeyboardAccelerator_ScopeOwner, keyboardAcceleratorCopy.Get(), KnownPropertyIndex::KeyboardAccelerator_ScopeOwner));
                        IFC_RETURN(returnValueAsKeyboardAcceleratorCollection->Append(keyboardAcceleratorCopy.Get()));
                        continue;
                    }
                }

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
        wxaml_interop::TypeName targetType,
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
        if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_62639407>())
        {
            if (auto sourceDO = ctl::query_interface_cast<DependencyObject>(uiCommand))
            {
                IFC_RETURN(DXamlCore::SetDirectBinding(sourceDO.Get(), KnownPropertyIndex::XamlUICommand_Label, target, labelPropertyIndex));
                return S_OK;
            }
        }

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

        if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_62639407>())
        {
            if (auto sourceDO = ctl::query_interface_cast<DependencyObject>(uiCommand))
            {
                IFC_RETURN(DXamlCore::SetDirectBinding(sourceDO.Get(), KnownPropertyIndex::XamlUICommand_IconSource, target, iconPropertyIndex, converter.Get()));
                return S_OK;
            }
        }

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
        if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_62639407>())
        {
            if (auto sourceDO = ctl::query_interface_cast<DependencyObject>(uiCommand))
            {
                IFC_RETURN(DXamlCore::SetDirectBinding(sourceDO.Get(), KnownPropertyIndex::XamlUICommand_IconSource, target, iconSourcePropertyIndex));
                return S_OK;
            }
        }

        IFC_RETURN(DXamlCore::SetBinding(ctl::as_iinspectable(uiCommand), wrl_wrappers::HStringReference(L"IconSource").Get(), target, iconSourcePropertyIndex));
    }

    return S_OK;
}

_Check_return_ HRESULT CommandingHelpers::BindToKeyboardAcceleratorsIfUnset(
    _In_ IXamlUICommand* uiCommand,
    _In_ UIElement* target)
{
    if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_62676766>())
    {
        if (auto sourceDO = ctl::query_interface_cast<DependencyObject>(uiCommand))
        {
            // Early out if the command has no keyboard accelerators materialized.
            // This avoids expensive binding setup work (converter creation, target collection
            // access, etc.) when the command never had accelerators set. Since we don't
            // subscribe to collection change events, accelerators added to the command
            // later won't be picked up anyway, so this is safe.
            if (!sourceDO->GetHandle()->IsEffectiveValueInSparseStorage(
                    KnownPropertyIndex::XamlUICommand_KeyboardAccelerators))
            {
                return S_OK;
            }

            // The collection was materialized but might be empty (e.g. if the getter was
            // called without adding any items). Check the count before proceeding.
            {
                ctl::ComPtr<wfc::IVector<xaml_input::KeyboardAccelerator*>> commandKeyboardAccelerators;
                UINT commandKeyboardAcceleratorCount = 0;
                IFC_RETURN(uiCommand->get_KeyboardAccelerators(&commandKeyboardAccelerators));
                if (!commandKeyboardAccelerators)
                {
                    return S_OK;
                }

                IFC_RETURN(commandKeyboardAccelerators->get_Size(&commandKeyboardAcceleratorCount));
                if (commandKeyboardAcceleratorCount == 0)
                {
                    return S_OK;
                }
            }
        }
    }

    ctl::ComPtr<wfc::IVector<xaml_input::KeyboardAccelerator*>> targetKeyboardAccelerators;
    UINT targetKeyboardAcceleratorCount;

    IFC_RETURN(target->get_KeyboardAccelerators(&targetKeyboardAccelerators));
    IFC_RETURN(targetKeyboardAccelerators->get_Size(&targetKeyboardAcceleratorCount));

    if (targetKeyboardAcceleratorCount == 0)
    {
        ctl::ComPtr<KeyboardAcceleratorCopyConverter> converter;
        IFC_RETURN(ctl::make(&converter));

        if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_62639407>())
        {
            if (auto sourceDO = ctl::query_interface_cast<DependencyObject>(uiCommand))
            {
                IFC_RETURN(DXamlCore::SetDirectBinding(sourceDO.Get(), KnownPropertyIndex::XamlUICommand_KeyboardAccelerators, target, KnownPropertyIndex::UIElement_KeyboardAccelerators, converter.Get()));
                return S_OK;
            }
        }

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
        if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_62639407>())
        {
            if (auto sourceDO = ctl::query_interface_cast<DependencyObject>(uiCommand))
            {
                IFC_RETURN(DXamlCore::SetDirectBinding(sourceDO.Get(), KnownPropertyIndex::XamlUICommand_AccessKey, target, KnownPropertyIndex::UIElement_AccessKey));
                return S_OK;
            }
        }

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

    if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_62639407>())
    {
        if (auto sourceDO = ctl::query_interface_cast<DependencyObject>(uiCommand))
        {
            if (!localHelpText || localHelpText.IsEmpty())
            {
                IFC_RETURN(DXamlCore::SetDirectBinding(sourceDO.Get(), KnownPropertyIndex::XamlUICommand_Description, target, KnownPropertyIndex::AutomationProperties_HelpText));
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
                IFC_RETURN(DXamlCore::SetDirectBinding(sourceDO.Get(), KnownPropertyIndex::XamlUICommand_Description, target, KnownPropertyIndex::ToolTipService_ToolTip));
            }

            return S_OK;
        }
    }

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

    if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_62639407>())
    {
        if (!bindingExpression)
        {
            const auto* pProperty = MetadataAPI::GetDependencyPropertyByIndex(targetPropertyIndex);

            // GetBindingExpression only finds BindingExpression (IBindingExpression).
            // Also check for DirectSourceBindingExpression which extends BindingExpressionBase
            // but does not implement IBindingExpression.
            ctl::ComPtr<IInspectable> localValue;
            IFC_RETURN(target->ReadLocalValue(pProperty, &localValue));

            ctl::ComPtr<IExpressionBase> expressionBase;
            if (localValue && SUCCEEDED(localValue.As(&expressionBase)) && expressionBase)
            {
                // This is an internally-set binding expression (DirectSourceBindingExpression).
                // Since only our commanding code creates these, it's safe to clear it.
                IFC_RETURN(target->ClearValue(pProperty));
            }
        }
    }

    return S_OK;
}
