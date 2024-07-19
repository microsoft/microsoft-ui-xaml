// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines the step that connects directly to the source

#pragma once

#include "PropertyPathStep.h"

namespace DirectUI
{
    
class SourceAccessPathStep: public PropertyPathStep
{
public:

    _Check_return_ HRESULT ReConnect(_In_ IInspectable *pSource) override
    { 
        SetPtrValue(m_Source, pSource);
        return S_OK;
    }
    
    _Check_return_ HRESULT GetValue(_Out_ IInspectable **ppValue) override;
    _Check_return_ HRESULT SetValue(_In_  IInspectable *pValue) override
    { return E_NOTIMPL; }

    _Check_return_ HRESULT GetType(_Outptr_ const CClassInfo **ppType) override;

    _Check_return_ HRESULT GetSourceType(_Outptr_ const CClassInfo **ppType) override;
    bool IsConnected() override
    { return m_Source.Get() != NULL; }

private:

    TrackerPtr<IInspectable> m_Source;
};

}
