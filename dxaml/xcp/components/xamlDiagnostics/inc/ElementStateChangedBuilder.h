// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <string>
#include "XamlOM.WinUI.h"

class CClassInfo;
class CDependencyProperty;
class CVisualState;
class CDependencyObject;
class CResourceDictionary;
class CDOCollection;
class CUIElement;
class HandleMap;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace XamlDiagnostics {
    class ElementStateChangedBuilderUnitTests;
} } } } }

namespace Diagnostics {
    enum AccessorType
    {
        Array = 0,
        Key = 1,
        ImplicityKey = 2,

        Last = 3,
    };

    struct ElementStateChangedContext
    {
        std::wstring    PathToError;
        InstanceHandle  RootHandle = 0;
    };

    // Builds the context string for the ElementStateChanged callback.
    class ElementStateChangedBuilder final
    {
        friend class ::Windows::UI::Xaml::Tests::XamlDiagnostics::ElementStateChangedBuilderUnitTests;

        static constexpr const wchar_t* c_delim = L"/";
        static constexpr const wchar_t* c_propDelim = L":";
        static constexpr const wchar_t* c_accessorsStart[] = {
            L"[", L"['", L"[{x:Type "
        };
        static_assert(_countof(c_accessorsStart) == AccessorType::Last, L"");
        static constexpr const wchar_t* c_accessorsEnd[] = {
            L"]", L"']", L"}]"
        };
        static_assert(_countof(c_accessorsEnd) == AccessorType::Last, L"");
    public:

        ElementStateChangedBuilder(_In_ CDependencyObject* invalidObj, _In_ const CDependencyProperty* invalidProperty);
        
        _Check_return_ HRESULT AddCollectionContext(_In_ CDOCollection* collection, _In_ CDependencyObject* collectionItem, _Out_ bool* isDictionaryItem);
        _Check_return_ HRESULT AddResourceDictionaryContext(_In_ CResourceDictionary* dictionary, _In_ CDependencyObject* dictionaryItem, _Out_ bool* isDictionaryItem);
        _Check_return_ HRESULT AddParentContext(_In_ CDependencyObject* parent, _In_ CDependencyObject* child, _Out_ bool* contextAdded);
        ElementStateChangedContext GetContext();
        bool IsContextReady();

    private:
        ElementStateChangedBuilder() {} // Private constructor for tests to use since there isn't a CDO.

        // Private AddAccessor methods intended to be used by the corresponding public AddCollection/ResourceDictionaryContext methods.
        // These methods add a new step if m_lastContextWasAccessor is false
        _Check_return_ HRESULT AddAccessor(_In_ unsigned int index);
        _Check_return_ HRESULT AddAccessor(_In_z_ const wchar_t* key);
        _Check_return_ HRESULT AddAccessor(_In_ const CClassInfo* type);
        _Check_return_ HRESULT AddProperty(_In_ const CDependencyProperty* prop);

        // Internal Add methods that build the string.
        _Check_return_ HRESULT AddTypeInternal(_In_ const CClassInfo* type);
        void AddAccessorInternal(_In_ const std::wstring& accessor, _In_ AccessorType accessorType);
        _Check_return_ HRESULT AddPropertyInternal(_In_ const CDependencyProperty* prop);

        // Helper methods for building a new step and accessor string
        void            StartNewStep();
        size_t          GetAccessorCount(_In_ AccessorType accessorType);
        const wchar_t*  GetAccessorStart(_In_ AccessorType accessorType);
        const wchar_t*  GetAccessorEnd(_In_ AccessorType accessorType);
        
        // Helper methods for finding relationships between two objects
        _Check_return_ HRESULT FindParentProperty(_In_ CDependencyObject* parent, _In_ CDependencyObject* child, _Outptr_result_maybenull_ const CDependencyProperty** parentProperty);
        bool                                        TryFindIndexInCollection(_In_ CDOCollection* collection, _In_ CDependencyObject* collectionItem, _Out_ UINT32* index);

    private:
        ElementStateChangedContext  m_context;
        bool                        m_lastContextWasAccessor = false;
    };
}