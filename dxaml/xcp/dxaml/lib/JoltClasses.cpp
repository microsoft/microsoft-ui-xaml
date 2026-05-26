// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "JoltClasses.h"
#include "IsEnabledChangedEventArgs.g.h"
#include "Value.h"
#include "DependencyPropertyChangedEventArgs.g.h"
#include "Control.g.h"
#include "DXamlCore.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// This helper method decouples this header (widely included) from DXamlCore.h (much less commonly needed)
_Check_return_ HRESULT DirectUI::RegisterUntypedEventSourceInCore(_In_ IUntypedEventSource* pEventSource, _In_ bool bUseEventManager)
{
    RRETURN(DXamlCore::GetCurrent()->RegisterEventSource(pEventSource, bUseEventManager));
}
_Check_return_ HRESULT DirectUI::UnregisterUntypedEventSourceInCore(_In_ IUntypedEventSource* pEventSource, _In_ bool bUseEventManager)
{
    RRETURN(DXamlCore::GetCurrent()->UnregisterEventSource(pEventSource, bUseEventManager));
}


// Overrides CEventSource::UntypedRaise to handle raising IsEnabledChanged specially.
// pArgs are safely converted from IsEnabledChangedEventArgs to DependencyPropertyChangedEventArgs before
// calling Raise().
//
// CEventSource::UntypedRaise fails for the IsEnabledChanged event because cannot QI from
// IsEnabledChangedEventArgs, the internal args type, to DependencyPropertyChangedEventArgs.  IsEnabledChanged
// uses these internal event args which have no COM interface, but the public hander for the IsEnabledChanged
// event is a normal DependencyPropertyChangedHandler that expects DependencyPropertyChangedEventArgs.
// Therefore, we need to transmute the IsEnabledChangedEventArgs to DependencyPropertyChangedEventArgs before
// calling Raise().
_Check_return_
HRESULT
CIsEnabledChangedEventSource::UntypedRaise(
    _In_opt_ IInspectable* source,
    _In_opt_ IInspectable* args)
{
    ctl::ComPtr<DependencyPropertyChangedEventArgs> argsConverted;

    if (args)
    {
        ctl::ComPtr<IsEnabledChangedEventArgs> isEnabledChangedEventArgs;
        
        isEnabledChangedEventArgs.Attach(ctl::query_interface<IsEnabledChangedEventArgs>(args));

        if (isEnabledChangedEventArgs)
        {
            BOOLEAN oldValue = FALSE;
            BOOLEAN newValue = FALSE;
            ctl::ComPtr<IInspectable> oldValueWrapped;
            ctl::ComPtr<IInspectable> newValueWrapped;

            IFC_RETURN(isEnabledChangedEventArgs->get_OldValue(&oldValue));
            IFC_RETURN(PropertyValue::CreateFromBoolean(oldValue, &oldValueWrapped));
            IFC_RETURN(isEnabledChangedEventArgs->get_NewValue(&newValue));
            IFC_RETURN(PropertyValue::CreateFromBoolean(newValue, &newValueWrapped));
            IFC_RETURN(DependencyPropertyChangedEventArgs::Create(KnownPropertyIndex::Control_IsEnabled, oldValueWrapped.Get(), newValueWrapped.Get(), &argsConverted));
        }
    }

    IFC_RETURN(Raise(source, argsConverted.Get()));

    return S_OK;
}

_Check_return_ HRESULT DataContextChangedParams::GetNewDataContext(_COM_Outptr_result_maybenull_ IInspectable** ppNewValue) const
{
    if (m_pNewDataContextOuterNoRef != nullptr)
    {
        CValueBoxer::UnwrapExternalObjectReferenceIfPresent(m_pNewDataContextOuterNoRef, ppNewValue);
    }
    else
    {
        IFC_RETURN(CValueBoxer::UnboxObjectValue(m_pNewDataContext, /* pTargetType */ nullptr, ppNewValue));
    }
    return S_OK;
}
