// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ResourceDictionary_partial.h"

namespace Parser
{
    struct XamlBuffer;
}

namespace DirectUI
{
    class Style;

    // encapsulates use of MRT by the default styles mechanism
    class StyleResourceHelper
    {
        public:
            StyleResourceHelper();
            ~StyleResourceHelper();

            // looks up a resource for the given library
            _Check_return_ HRESULT GetResourceUriPath(
                _In_opt_z_ const WCHAR* wszLibraryName,
                _In_z_ const WCHAR* wszResourceFileName,
                _Out_ xstring_ptr* pstrResourceUri
                );
    };

    class NamespaceEntry
    {
        public:
                NamespaceEntry() :
                    m_pStyles(NULL)
                {
                }

                NamespaceEntry(_In_opt_ ResourceDictionary* pStyles) :
                    m_pStyles(pStyles)
                {
                    if (pStyles)
                    {
                        ctl::addref_interface(pStyles);
                        pStyles->UpdatePeg(true);
                    }
                }

                NamespaceEntry(const NamespaceEntry& rhs) :
                    m_pStyles(rhs.m_pStyles)
                {
                    if (m_pStyles)
                    {
                        ctl::addref_interface(m_pStyles);
                        m_pStyles->UpdatePeg(true);
                    }
                }

                const NamespaceEntry& operator=(const NamespaceEntry& rhs)
                {
                    if (m_pStyles)
                    {
                        m_pStyles->UpdatePeg(false);
                        ctl::release_interface(m_pStyles);
                    }
                    m_pStyles = rhs.m_pStyles;
                    if (m_pStyles)
                    {
                        ctl::addref_interface(rhs.m_pStyles);
                        m_pStyles->UpdatePeg(true);
                    }
                    return *this;
                }

                ~NamespaceEntry()
                {
                    if (m_pStyles)
                    {
                        m_pStyles->UpdatePeg(false);
                        ctl::release_interface(m_pStyles);
                    }
                }

                void GetStyles(_Outptr_result_maybenull_ ResourceDictionary** ppStyles)
                {
                    *ppStyles = m_pStyles;
                    ctl::addref_interface(*ppStyles);
                }

        private:
                ResourceDictionary* m_pStyles;
    };

    class StyleCache
    {
        public:
            static _Check_return_ HRESULT LoadFromFile(_In_z_ const WCHAR* wszXamlFilePath,
                                                      _Outptr_ IInspectable** ppObject);

        private:
            containers::vector_map<xstring_ptr, NamespaceEntry> m_stylesMap;
            ctl::ComPtr<ResourceDictionary> m_pFrameworkStyles;
            bool m_fLoadedAppStyles;
            bool m_fLoadedThemeXaml;
            bool m_fLoadGenericXaml;
            ResourceDictionary* m_pAppStyles;
            StyleResourceHelper m_resourceHelper;

            _Check_return_ HRESULT LoadStylesFromFile(
                _In_z_ const WCHAR* wszXamlFilePath,
                _Outptr_ ResourceDictionary** ppStyles);

            _Check_return_ HRESULT LoadStylesFromString(
                _In_reads_(cchXaml) const WCHAR* wXaml,
                XUINT32 cchXaml,
                _Outptr_ ResourceDictionary** ppStyles);

            _Check_return_ HRESULT LoadStylesFromBuffer(
                _In_ const Parser::XamlBuffer& buffer,
                _In_ const xstring_ptr_view& strSourceAssemblyName,
                _In_ const xstring_ptr_view& strResourceUri,
                _Outptr_ ResourceDictionary** ppStyles);

            _Check_return_ HRESULT LoadStylesFromResourceUri(
                _In_ const xstring_ptr& strResourceUri,
                _Outptr_ ResourceDictionary** ppStyles,
                _Out_ bool *fResourceLocated);

            _Check_return_ HRESULT GetAppStyles(_Outptr_result_maybenull_ ResourceDictionary** ppStyles);

        public:
            StyleCache();
            ~StyleCache();

            _Check_return_ HRESULT GetFrameworkStyles(_Outptr_ ResourceDictionary** ppStyles);

            _Check_return_ HRESULT GetStyles(
                _In_z_ const WCHAR* wszNamespace,
                _In_opt_z_ const WCHAR* wszAssemblyName,
                _In_opt_ wf::IUriRuntimeClass* pUri,
                _Outptr_result_maybenull_ ResourceDictionary** ppStyles);

            _Check_return_ HRESULT LoadStylesFromResource(
                _In_ const xstring_ptr_view& strResourceName,
                _In_ const xstring_ptr_view& strResourceUri,
                _Outptr_ ResourceDictionary **ppStyles);

            _Check_return_ HRESULT LoadThemeResources();

            void Clear();

            bool IsGenericXamlFilePathAvailableFromMUX() const;
    };


    // Provides the default styles for controls.
    class DefaultStyles
    {
        private:
            StyleCache m_cache;

            // Get the default style for a control, given the control's typename.
            _Check_return_ HRESULT GetDefaultStyleByTypeName(
                _In_ const xstring_ptr& strTypeName,
                _In_ const xstring_ptr& strAssemblyName,
                _In_opt_ wf::IUriRuntimeClass* pUri,
                _Outptr_result_maybenull_ Style** ppStyle);

            _Check_return_ HRESULT GetDefaultStyleByTypeInfo(
                _In_ const CClassInfo* pType,
                _Outptr_result_maybenull_ Style** ppStyle);

            _Check_return_ HRESULT ResolveTypeName(
                _In_ IInspectable* pValue,
                _Out_ xstring_ptr* pstrTypeName,
                _Out_ xstring_ptr* pstrAssemblyName,
                _Outptr_result_maybenull_ const CClassInfo** ppType);

            _Check_return_ HRESULT ResolveStyle(
                _In_ const xstring_ptr& strTypeName,
                _In_opt_ ResourceDictionary* pStyles,
                _Outptr_result_maybenull_ Style** ppStyle);

            _Check_return_ HRESULT ResolveStyle(
                _In_ const CClassInfo* pType,
                _In_opt_ ResourceDictionary* pStyles,
                _Outptr_result_maybenull_ Style** ppStyle);

        public:
            // Get the default style for a control based on the DefaultStyleKey property.
            _Check_return_ HRESULT GetDefaultStyleByKey(
                _In_ DependencyObject* pDO,
                _Outptr_result_maybenull_ Style** ppStyle);

            _Check_return_ HRESULT RefreshImmersiveColors();

            // Get Style Cache. Returned object is not reference counted.
            StyleCache* GetStyleCache() { return &m_cache; }
    };
}
