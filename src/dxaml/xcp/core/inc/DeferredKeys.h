// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class DirectiveProperty;
class ObjectWriter;
class XamlOptimizedNodeList;
class XamlReader;
class XamlSavedContext;
class XamlSchemaContext;
class XamlServiceProviderContext;

//------------------------------------------------------------------------
//
//  Class:  CDeferredKeys
//
//  Synopsis:
// 
//------------------------------------------------------------------------
class CDeferredKeys
{
public:
    CDeferredKeys(_In_ CDependencyObject *pCollection)
    {
        m_pCollection = pCollection;
        m_bContentHasResourceReferences = false;
        m_bResolvingResources = false;
        m_bExpandingKey = false;
        m_bExpandingNonDeferredContent = false;
        m_bHasKeyLookupBoundary = false;
        m_uiMaxAllowedIndexForLookup = 0;
    }

    ~CDeferredKeys();

    _Check_return_ 
    HRESULT RecordXaml(
                _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
                _In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList, 
                _In_ const std::shared_ptr<XamlSavedContext>& spContext,
                _In_ const bool fIsResourceDictionaryWithKeyProperty);

    _Check_return_ 
    HRESULT Load(_In_ const xstring_ptr_view& strKey, _Out_ bool *pbFound);
  
    bool IsExpanding() { return m_bResolvingResources; }
    bool IsEmpty() const{ return (!m_bExpandingKey && !m_bExpandingNonDeferredContent && (!m_spKeyMap || (m_spKeyMap->empty()))); }

    _Check_return_ 
    HRESULT Ensure();

    _Check_return_
    HRESULT PlayNonDeferredContent(_In_ ObjectWriter* pObjectWriter);
    
    _Check_return_ 
    HRESULT EnsureAll();

private:
    _Check_return_ 
    HRESULT PreResolve(
        _In_ const bool fResolveItems,
        _In_ const bool fResolveResources,
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext);
    
    _Check_return_ 
    HRESULT ResolveAndSaveResource(
        _In_ CCoreServices *pCore, 
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _In_ const std::shared_ptr<XamlReader>& spReader,
        _In_ CResourceDictionary *pResourceDictionary,
        _Out_ bool* pbResourceFromReadOnlyDictionary);

    _Check_return_ 
    HRESULT SaveAmbientProperties(
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext
        );

    _Check_return_ 
    HRESULT GetReader(_Out_ std::shared_ptr<XamlReader>& spReader);

    _Check_return_ 
    HRESULT GetWriter(_In_ const std::shared_ptr<XamlReader>& spReader, _Out_ std::shared_ptr<ObjectWriter>& spObjectWriter);

    _Check_return_
    HRESULT PlayXaml(_In_ XUINT32 startIndex);

    _Check_return_ 
    HRESULT RemoveObjectReferencesFromStack();

    _Check_return_
    HRESULT get_X_ConnectionIdProperty(_In_ const std::shared_ptr<XamlSchemaContext>& spSchemaContext, _Out_ std::shared_ptr<DirectiveProperty>& spConnectionIdProperty);

private:
    std::shared_ptr<XamlOptimizedNodeList> m_spNodeList;
    std::shared_ptr< xvector< XUINT32 > > m_spNonDeferredKeys;
    std::unique_ptr<containers::vector_map<xstring_ptr, XUINT32>> m_spKeyMap;
    std::shared_ptr<XamlSavedContext> m_spXamlSavedContext;
    std::shared_ptr<XamlReader> m_spReader;
    CDependencyObject *m_pCollection;
  
    // maximum allowed nodestream index (not including the index value itself) for a recursive lookup of a key 
    XUINT32 m_uiMaxAllowedIndexForLookup;

    bool m_bResolvingResources : 1;
    bool m_bExpandingKey : 1;
    bool m_bExpandingNonDeferredContent : 1;

    // flag to indicate a recursive lookup will require validating against a node stream boundary index
    bool m_bHasKeyLookupBoundary : 1;  

    bool m_bContentHasResourceReferences : 1;
};
