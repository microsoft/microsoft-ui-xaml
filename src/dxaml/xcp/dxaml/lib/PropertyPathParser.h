// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <stack_vector.h>
#include "PropertyPathStepDescriptor.h"
class XamlServiceProviderContext;
namespace DirectUI
{
    // This type is part of every DirectUI::Binding, and also created stand-alone by other types.
    class PropertyPathParser
    {
    public:
        
        PropertyPathParser() noexcept = default;
        ~PropertyPathParser() = default;

        // Move only (descriptors are move-only)
        PropertyPathParser(PropertyPathParser&&) = default;
        PropertyPathParser& operator=(PropertyPathParser&&) = default;
        PropertyPathParser(const PropertyPathParser&) = delete;
        PropertyPathParser& operator=(const PropertyPathParser&) = delete;

        _Check_return_ HRESULT SetSource(_In_opt_z_ const WCHAR *szPath, _In_opt_ XamlServiceProviderContext* context);

        // Iteration support - handles both inline and heap storage transparently
        PropertyPathStepDescriptor* begin() noexcept;
        PropertyPathStepDescriptor* end() noexcept;

        size_t size() const noexcept;
        bool empty() const noexcept { return m_descriptors[0].GetKind() == PropertyPathStepDescriptorKind::None; }
        bool HasParsedPath() const noexcept { return !empty(); }

    private:

        _Check_return_ HRESULT Parse(_In_opt_z_ const WCHAR *szPropertyPath, _In_opt_ XamlServiceProviderContext* context);

        // Transfers parsed descriptors from the temporary vector to our storage
        void FinalizeDescriptors(Jupiter::stack_vector<PropertyPathStepDescriptor, 2>& source);

        // Helper function that works with strings that may not be null-terminated
        static bool IsNumericIndex(_In_reads_(length) const WCHAR* pIndex, size_t length);

        _Check_return_ HRESULT CreateDependencyPropertyPathStepDescriptor(
            _In_ XUINT32 nPropertyLength,
            _In_reads_(nPropertyLength + 1) const WCHAR *pchProperty,
            _In_opt_ XamlServiceProviderContext* context,
            _Out_ PropertyPathStepDescriptor *pDescriptor);

        _Check_return_ HRESULT GetDPFromName(
            _In_ XUINT32 nPropertyLength,
            _In_reads_(nPropertyLength + 1) const WCHAR *pchProperty,
            _In_opt_ XamlServiceProviderContext* context,
            _Outptr_result_maybenull_ const CDependencyProperty **ppDP);

    private:
        // Inline storage for 0-2 descriptors (common case).
        // If we have more than 2, m_descriptors[0] becomes a HeapStorage descriptor
        // pointing to all descriptors, and m_descriptors[1] is unused.
        PropertyPathStepDescriptor m_descriptors[2];
    };

}
