// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CWindow.h"
#include "DependencyObject.h"
#include "DXamlCore.h"
#include "Window.g.h"
#include "UIElement.g.h"
#include "SystemBackdrop.g.h"

// Respond to content changes during markup parsing
_Check_return_ HRESULT CWindow::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(CDependencyObject::OnPropertyChanged(args));

    switch (args.m_pDP->GetIndex())
    {
        // Window_Content is a private property that can only be set by the parser.  Any put or
        // get operations at the DXaml layer do not call in to the core layer.
        case KnownPropertyIndex::Window_Content:
        {
            // Window_Content can be intentionally set to NULL at the DXaml layer, but the parser
            // will only set the value on object instantiation, which should never be NULL.
            if (args.m_pNewValue->IsNullOrUnset())
            {
                return S_OK;
            }

            // Get or create the DirectUI::Window DXaml peer.  'GetPeer' will create
            // the DXaml peer if one does not already exist.
            ctl::ComPtr<DirectUI::Window> window;
            IFC_RETURN(DirectUI::DXamlCore::GetCurrent()->GetPeer<DirectUI::Window>(this, &window));
            IFCPTR_RETURN(window);

            // Get or create the root UIElement DXaml peer for the XAML tree
            ctl::ComPtr<DirectUI::UIElement> uielement;
            IFC_RETURN(DirectUI::DXamlCore::GetCurrent()->GetPeer<DirectUI::UIElement>(args.m_pNewValue->AsObject(), &uielement));
            IFCPTR_RETURN(uielement);

            // Set the DirectUI::Window content to the XAML tree passed in by the parser
            IFC_RETURN(window->put_Content(uielement.Get()));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CWindow::SetValue(_In_ const SetValueParams& args)
{
    switch (args.m_pDP->GetIndex())
    {
    // Window_Title is a private property that can only be set by the parser.  Any put or
    // get operations at the DXaml layer do not call in to the core layer.
    case KnownPropertyIndex::Window_Title:
        {
            // Get or create the DirectUI::Window DXaml peer.  'GetPeer' will create
            // the DXaml peer if one does not already exist.
            ctl::ComPtr<DirectUI::Window> window;
            IFC_RETURN(DirectUI::DXamlCore::GetCurrent()->GetPeer<DirectUI::Window>(this, &window));
            IFCPTR_RETURN(window);

            if (args.m_value.IsNullOrUnset())
            {
                window->put_Title(wrl_wrappers::HStringReference(L"").Get());
            }
            else
            {
                xstring_ptr strValue;
                IFC_RETURN(args.m_value.GetString(strValue));

                xruntime_string_ptr strValueRuntimeStr;
                IFC_RETURN(strValue.Promote(&strValueRuntimeStr));

                window->put_Title(strValueRuntimeStr.GetHSTRING());
            }
        }
        return S_OK; // EARLY EXIT! We don't want to call super::SetValue() because we don't want to set the DP

    // Window_SystemBackdrop is a private property that can only be set by the parser.  Any put or
    // get operations at the DXaml layer do not call in to the core layer.
    case KnownPropertyIndex::Window_SystemBackdrop:
        {
            // Get or create the DirectUI::Window DXaml peer.  'GetPeer' will create
            // the DXaml peer if one does not already exist.
            ctl::ComPtr<DirectUI::Window> window;
            IFC_RETURN(DirectUI::DXamlCore::GetCurrent()->GetPeer<DirectUI::Window>(this, &window));
            IFCPTR_RETURN(window);

            // Get or create the DXaml peer for the SystemBackdrop
            ctl::ComPtr<DirectUI::SystemBackdrop> systemBackdrop;
            IFC_RETURN(DirectUI::DXamlCore::GetCurrent()->GetPeer<DirectUI::SystemBackdrop>(args.m_value.AsObject(), &systemBackdrop));
            IFCPTR_RETURN(systemBackdrop);

            // Set the DirectUI::Window::SystemBackdrop property
            ctl::ComPtr<xaml_media::ISystemBackdrop> isystemBackdrop;
            IFC_RETURN(systemBackdrop.As(&isystemBackdrop));
            IFC_RETURN(window->put_SystemBackdropImpl(isystemBackdrop.Get()));
        }
        return S_OK; // EARLY EXIT! We don't want to call super::SetValue() because we don't want to set the DP

    // Window_Width/Height and Window_MinWidth/MinHeight/MaxWidth/MaxHeight are set from markup by the
    // parser on the core object. Forward each to the matching DXaml peer setter, which validates and
    // applies it - bad values throw E_INVALIDARG, which surfaces as a markup error.
    case KnownPropertyIndex::Window_Width:
    case KnownPropertyIndex::Window_Height:
    case KnownPropertyIndex::Window_MinWidth:
    case KnownPropertyIndex::Window_MinHeight:
    case KnownPropertyIndex::Window_MaxWidth:
    case KnownPropertyIndex::Window_MaxHeight:
        {
            // Get or create the DirectUI::Window DXaml peer.  'GetPeer' will create
            // the DXaml peer if one does not already exist.
            ctl::ComPtr<DirectUI::Window> window;
            IFC_RETURN(DirectUI::DXamlCore::GetCurrent()->GetPeer<DirectUI::Window>(this, &window));
            IFCPTR_RETURN(window);

            // These properties are all typed Double, so the parser always delivers a Double value.
            const double value = args.m_value.AsDouble();

            switch (args.m_pDP->GetIndex())
            {
            case KnownPropertyIndex::Window_Width:
                IFC_RETURN(window->put_Width(value));
                break;
            case KnownPropertyIndex::Window_Height:
                IFC_RETURN(window->put_Height(value));
                break;
            case KnownPropertyIndex::Window_MinWidth:
                IFC_RETURN(window->put_MinWidth(value));
                break;
            case KnownPropertyIndex::Window_MinHeight:
                IFC_RETURN(window->put_MinHeight(value));
                break;
            case KnownPropertyIndex::Window_MaxWidth:
                IFC_RETURN(window->put_MaxWidth(value));
                break;
            case KnownPropertyIndex::Window_MaxHeight:
                IFC_RETURN(window->put_MaxHeight(value));
                break;
            }
        }
        return S_OK; // EARLY EXIT! We don't want to call super::SetValue() because we don't want to set the DP
    }

    return CDependencyObject::SetValue(args);
}
