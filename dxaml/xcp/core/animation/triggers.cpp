// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Triggers.h"
#include "MetadataAPI.h"
#include "Storyboard.h"
#include "CValueUtil.h"

CBeginStoryboard::~CBeginStoryboard()
{
    ReleaseInterface(m_pStoryboard);
}

// Override SetValue to hookup storyboard to the time manager.
_Check_return_ HRESULT CBeginStoryboard::SetValue(_In_ const SetValueParams& args)
{
    // We don't want two Storyboard's from the parser, but we will allow managed
    // to overwrite the Storyboard property
    if (m_pStoryboard != NULL && !GetContext()->IsSettingValueFromManaged(this))
    {
        IFC_RETURN(E_INVALIDARG);
    }

    IFC_RETURN(CDependencyObject::SetValue(args));

    return S_OK;
}

_Check_return_ HRESULT CBeginStoryboard::InvokeImpl(_In_ const CDependencyProperty *pdp, _In_opt_ CDependencyObject *pNamescopeOwner)
{
    // The only property we need to check for is 'Storyboard' either as a content
    // or explicit property.
    //
    // If the Storyboad is entering the live tree (either because an ancestor is entering
    // or because of a SetValue()), call BeginPrivate().
    if (IsActive()
        && (pdp->GetIndex() == KnownPropertyIndex::BeginStoryboard_Storyboard)
        && (m_pStoryboard))
    {
        // Only top-level Storyboards enter the tree as part of the Triggers tag
        IFC_RETURN(m_pStoryboard->BeginPrivate( TRUE /* fIsTopLevel */ ));
    }
    else
    {
        IFC_RETURN(CDependencyObject::InvokeImpl(pdp, pNamescopeOwner));
    }

    return S_OK;
}

CEventTrigger::~CEventTrigger()
{
    ReleaseInterface(m_pTriggerActions);
}

// Override to base SetValue to do value validation for some properties.
_Check_return_ HRESULT CEventTrigger::SetValue(_In_ const SetValueParams& args)
{
    if (args.m_pDP->GetIndex() == KnownPropertyIndex::EventTrigger_RoutedEvent)
    {
        //
        // The passed value stands for a valid event name on a targe Element.
        // The value must be a string in format:  "ElementType.EventType",
        // Such as "Canvas.Loaded",  "Rectangle.Loaded" etc.
        //
        const CClassInfo *pElementClass = NULL;

        const WCHAR *pElementName = NULL;
        XUINT32     cElementName = 0;
        const WCHAR *pEventName = NULL;
        XUINT32     cEventName = 0;

        //
        // The type of RoutedEvent property in CEventTrigger is CString.
        // The accepted type of value is either one of below :
        //     1> valueString
        //     2> valueObject with STRING value in pdoValue.
        //
        xephemeral_string_ptr strStringValue;
        switch (args.m_value.GetType())
        {
            case valueString:
                // This is valid value.
                CValueUtil::GetEphemeralString(
                    args.m_value,
                    strStringValue);
                break;

            case valueObject:
            {
                const CString* pdoString = do_pointer_cast<CString>(args.m_value);

                // Valid only when its value type is String.
                if (pdoString != nullptr &&
                    !pdoString->m_strString.IsNull())
                {
                    pdoString->m_strString.Demote(&strStringValue);
                }
                else
                {
                    IFC_RETURN(E_INVALIDARG);
                }
                break;
            }

            default:
                IFC_RETURN(E_INVALIDARG);
                break;
        }

        //
        // Return E_INVALIDARG for below scenarios:
        //
        //  1. The text string doesn't contain a separator ".", (return value is XUINT32(~0)).
        //  2. Contains the "." separator at wrong position,
        //     such as at the begining or at the end.
        //

        const auto cOffsetSeparator = strStringValue.FindChar(L'.');
        if (cOffsetSeparator == xstring_ptr_view::npos
            || cOffsetSeparator == 0 || (cOffsetSeparator == strStringValue.GetCount() -1))
        {
            IFC_RETURN(E_INVALIDARG);
        }

        //
        // There is a separator '.' inside the value string.
        // The first part will be treated as element name, the second part is the event name.
        //
        pElementName = strStringValue.GetBuffer();
        pEventName = strStringValue.GetBuffer() + cOffsetSeparator + 1;

        cElementName = cOffsetSeparator;
        cEventName = strStringValue.GetCount() - cOffsetSeparator - 1;

        pElementClass = DirectUI::MetadataAPI::GetBuiltinClassInfoByName(XSTRING_PTR_EPHEMERAL2(pElementName, cElementName));

        // if the element part is not for a valid type, returns failure here.
        if (pElementClass == NULL)
        {
            IFC_RETURN(E_INVALIDARG);
        }

        // Verify the type derives from FrameworkElement, because we currently only support the FrameworkElement.Loaded event.
        if (!DirectUI::MetadataAPI::IsAssignableFrom(
            DirectUI::MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::FrameworkElement),
            pElementClass))
        {
            IFC_RETURN(E_INVALIDARG);
        }

        //
        // For now, only Loaded event is accepted as EventTrigger.
        // When we support more events as EventTrigger, remove the below condition.
        //
        if (!XSTRING_PTR_EPHEMERAL2(pEventName, cEventName).Equals(STR_LEN_PAIR(L"Loaded")))
        {
            IFC_RETURN(E_INVALIDARG);
        }
    }

    // Just call the base class to set the value
    IFC_RETURN(CDependencyObject::SetValue(args));

    return S_OK;
}
