// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CDependencyObject.h>
#include <ShareableDependencyObject.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <vector_map.h>
#include <ObjectWriterNodeType.h>

class ObjectWriterNode;
class ObjectWriterNodeList;
class XamlBinaryFormatSubReader2;
class XamlOptimizedNodeList;
class XamlSavedContext;
class XamlReader;
class ObjectWriter;
class XamlWriter;
class ObjectWriterSettings;

class CTemplateContent final : public CShareableDependencyObject
{
public:
    // Creation method
    DECLARE_CREATE(CTemplateContent);

    _Check_return_ HRESULT RecordXaml(
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList,
        _In_ const std::shared_ptr<XamlSavedContext>& spTemplateContext,
        _In_ const std::shared_ptr<XamlBinaryFormatSubReader2>& spSubReader,
        _In_ const std::vector<std::pair<bool, xstring_ptr>>& vecResourceList);

    _Check_return_ HRESULT Load(
        _In_ const ObjectWriterSettings& spObjectWriterSettings,
        _In_ bool bTryOptimizeContent,
        _Outptr_result_maybenull_ CDependencyObject **ppDependencyObject
        );

    xref_ptr<CDependencyObject> GetSavedEventRoot() const;

    std::shared_ptr<XamlSavedContext> GetSavedContext() const;

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return OPTIONALLY_PARTICIPATES_IN_MANAGED_TREE;
    }

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTemplateContent>::Index;
    }


    // Get the shared slot index for the CTemplateValueStore's slot table.
    _Check_return_ std::size_t GetOrCreateNameIndex(
        _In_ const xstring_ptr_view& name);

    // If the slot has not been allocated, return -1, indicating that no values have
    // been set in the CTemplateValueStores associated with this template
    // content.
    _Check_return_ std::size_t TryGetNameIndex(
        _In_ const xstring_ptr_view& name) const;

private:
    ObjectWriterNode GenerateOptimizedNode(unsigned int cacheLookupIndex, const ObjectWriterNode& inNode);

    _Check_return_ HRESULT PlayXaml(
        _Out_ std::shared_ptr<XamlReader>& rspReader);

    _Check_return_ HRESULT LoadXbfVersion1(
        _In_ const ObjectWriterSettings& objectWriterSettings,
        _In_ bool bTryOptimizeContent,
        _Out_ std::shared_ptr<XamlQualifiedObject>& spRootInstance);

    _Check_return_ HRESULT LoadXbfVersion2(
        _In_ const ObjectWriterSettings& objectWriterSettings,
        _In_ bool bTryOptimizeContent,
        _Out_ std::shared_ptr<XamlQualifiedObject>& spRootInstance);

    _Check_return_ HRESULT PreResolveResources(
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _In_ const std::vector<std::pair<bool, xstring_ptr>>& vecResourceList);

    _Check_return_ HRESULT PreResolveResourcesFromNodeList(
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext);

    _Check_return_ HRESULT PreResolveResourcesFromListOfReferences(
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _In_ const std::vector<std::pair<bool, xstring_ptr>>& vecResourceList);

    _Check_return_ HRESULT CreateDictionaryOfLocalResourceReferences(
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _In_ const std::vector<xstring_ptr>& vecResourceList);

    _Check_return_ HRESULT CompleteInitializationOfLocalDictionary(
        _In_ xref_ptr<CResourceDictionary>& spResourceDictionary);

    _Check_return_ HRESULT static ResolveAndSaveResource(
        _In_ CCoreServices *pCore,
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _In_ const std::shared_ptr<XamlReader>& spReader,
        _In_ xref_ptr<CResourceDictionary>& pResourceDictionary,
        _Out_ bool* pbResourceFromReadOnlyDictionary);

    _Check_return_ static HRESULT ResolveReferenceAndAddToLocalDictionary(
        _In_ CCoreServices *pCore,
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _In_ const xstring_ptr& strKey,
        _In_ const bool isStaticResource,
        _In_ xref_ptr<CResourceDictionary>& pResourceDictionary,
        _Out_ bool* pbResourceFromReadOnlyDictionary);

    _Check_return_ HRESULT RemoveObjectReferencesFromStack();

    _Check_return_ HRESULT SaveEventRootWeakReference(
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext);

    _Check_return_ HRESULT SaveAmbientProperties(
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext);

    _Check_return_ HRESULT LoadAndOptimize(
        _In_ const std::shared_ptr<XamlReader>& spReader,
        _In_ const std::shared_ptr<ObjectWriter>& spObjectWriter,
        _In_ const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
        _Out_ std::shared_ptr<XamlOptimizedNodeList>& rspOptimizedNodeList);

    _Check_return_ HRESULT TryOptimizeValue(
        _In_ const std::shared_ptr<XamlReader>& spReader,
        _In_ const std::shared_ptr<ObjectWriter>& spObjectWriter,
        _In_ const std::shared_ptr<XamlWriter>& spOptimizedWriter);

    _Check_return_ HRESULT TryOptimizeTypeConvertedProperty(
        _In_ const std::shared_ptr<XamlProperty>& spXamlProperty,
        _In_ const std::shared_ptr<XamlReader>& spReader,
        _In_ const std::shared_ptr<ObjectWriter>& spObjectWriter,
        _In_ const std::shared_ptr<XamlWriter>& spOptimizedWriter);

    _Check_return_ HRESULT WriteToMatchingEnd(
        _In_ const std::shared_ptr<XamlReader>& spReader,
        _In_ const std::shared_ptr<XamlWriter>& spWriter,
        _In_opt_ const std::shared_ptr<XamlWriter>& spSecondaryWriter);

    _Check_return_ HRESULT ShouldOptimizeProperty(
        _In_ const std::shared_ptr<XamlProperty>& spProperty,
        _Out_ bool *pbShouldOptimizeProperty);


protected:
    CTemplateContent(
        _In_ CCoreServices *pCore);

    ~CTemplateContent() override;

private:
    containers::vector_map<xstring_ptr, UINT32> m_nameTableMap;

    // version 1 XBF fields
    std::shared_ptr<XamlOptimizedNodeList> m_spNodeList;

    // version 2 XBF fields
    std::vector< std::shared_ptr<XamlQualifiedObject> > m_cachedTemplateContentObjects;
    std::shared_ptr<XamlBinaryFormatSubReader2> m_spSubReader;

    std::shared_ptr<XamlSavedContext> m_spXamlSavedContext;
    xref::weakref_ptr<CDependencyObject> m_spEventRoot;
    bool m_bContentAlreadyOptimized : 1;
    bool m_bContentHasResourceReferences : 1;
};

