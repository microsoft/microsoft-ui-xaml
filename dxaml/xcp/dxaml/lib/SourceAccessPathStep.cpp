// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SourceAccessPathStep.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ 
HRESULT 
SourceAccessPathStep::GetValue(_Out_ IInspectable **ppValue)
{
    HRESULT hr = S_OK;
    
    IFCPTR(ppValue);

    *ppValue = ValueWeakReference::get_value_as<IInspectable>(m_Source.Get());

Cleanup:

    RRETURN(hr);
}

_Check_return_ 
HRESULT 
SourceAccessPathStep::GetType(_Outptr_ const CClassInfo **ppType)
{
    // This code is inaccessible right now: GetType on PropertyPathSteps and PropertyAccess
    // instances is only used for IValueConverter::ConvertBack's TypeInfo. SourceAccessPathStep
    // is ONLY used when you have an empty binding expression (e.g. binding to the instance itself),
    // a scenario that doesn't cannot TwoWay binding. For correctness it is left here
    // in the event we use the GetType methods for other purposes.
    IInspectable* source = m_Source.Get();
    if (source)
    {
        IFC_RETURN(MetadataAPI::GetClassInfoFromObject_SkipWinRTPropertyOtherType(source, ppType));
    }
    else 
    {
        *ppType = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Object);
    }
    return S_OK;
}

_Check_return_
HRESULT
SourceAccessPathStep::GetSourceType(_Outptr_ const CClassInfo **ppType)
{
    HRESULT hr = S_OK;
    IFCEXPECT(m_Source);
    IFC(GetType(ppType));
Cleanup:
    return hr;
}
