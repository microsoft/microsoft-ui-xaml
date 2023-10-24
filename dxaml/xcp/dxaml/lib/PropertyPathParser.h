// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <vector>
#include "PropertyPathStepDescriptor.h"
class XamlServiceProviderContext;
namespace DirectUI
{
    class PropertyPathParser
    {
    public:

        PropertyPathParser();
        ~PropertyPathParser();
        
    public:

        _Check_return_ HRESULT SetSource(_In_opt_z_ const WCHAR *szPath, _In_opt_ XamlServiceProviderContext* context);
            
        std::vector<PropertyPathStepDescriptor *>& Descriptors() 
        { return m_descriptors; }

    private:

        _Check_return_ HRESULT Parse(_In_opt_z_ const WCHAR *szPropertyPath, _In_opt_ XamlServiceProviderContext* context);

        bool IsNumericIndex(_In_z_ const WCHAR *szIndex);
        
        _Check_return_ HRESULT AppendStepDescriptor(_In_ PropertyPathStepDescriptor *pDescriptor);

        _Check_return_ HRESULT CreateDependencyPropertyPathStepDescriptor(
            _In_ XUINT32 nPropertyLength,
            _In_reads_(nPropertyLength + 1) const WCHAR *pchProperty,
            _In_opt_ XamlServiceProviderContext* context,
            _Outptr_ PropertyPathStepDescriptor **ppDescriptor);

        _Check_return_ HRESULT GetDPFromName(
            _In_ XUINT32 nPropertyLength,
            _In_reads_(nPropertyLength + 1) const WCHAR *pchProperty,
            _In_opt_ XamlServiceProviderContext* context,
            _Outptr_result_maybenull_ const CDependencyProperty **ppDP);

    private:

        std::vector<PropertyPathStepDescriptor *> m_descriptors;
    };

}
