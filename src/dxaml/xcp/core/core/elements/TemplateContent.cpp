// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "BinaryFormatObjectWriter.h"
#include "ObjectWriter.h"
#include "ObjectWriterFrame.h"
#include "ObjectWriterNodeList.h"
#include "ObjectWriterStack.h"
#include "ParserErrorService.h"
#include "SavedContext.h"
#include "XamlBinaryFormatSubReader2.h"
#include "XamlOptimizedNodeList.h"
#include "XamlReader.h"

#undef max

CTemplateContent::CTemplateContent(_In_ CCoreServices *pCore)
    : CShareableDependencyObject(pCore)
    , m_bContentAlreadyOptimized(false)
    , m_bContentHasResourceReferences(false)
{
}

CTemplateContent::~CTemplateContent()
{
    SP_ObjectWriterStack spObjectWriterStack;
    std::shared_ptr<XamlQualifiedObject> spQO;
    CResourceDictionary* pResourceDictionary;

    if (SUCCEEDED(m_spXamlSavedContext->get_Stack(spObjectWriterStack)))
    {
        ASSERT(!spObjectWriterStack->empty());
        if (spObjectWriterStack->Current().exists_CompressedStack())
        {
            spQO = spObjectWriterStack->Current().get_CompressedStack()->get_Resources();
            if (spQO != NULL)
            {
                pResourceDictionary = static_cast<CResourceDictionary*>(spQO->GetDependencyObject());
                IGNOREHR(this->RemovePeerReferenceToItem(pResourceDictionary));
            }
        }
    }
}

_Check_return_ HRESULT
CTemplateContent::RecordXaml(
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
    _In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList,
    _In_ const std::shared_ptr<XamlSavedContext>& spTemplateContext,
    _In_ const std::shared_ptr<XamlBinaryFormatSubReader2>& spSubReader,
    _In_ const std::vector<std::pair<bool,xstring_ptr>>& vecResourceList
    )
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    std::shared_ptr<ObjectWriterStack> spObjectWriterStack;

    m_spSubReader = spSubReader;
    m_spNodeList = spNodeList;
    m_spXamlSavedContext = spTemplateContext;
    m_bContentHasResourceReferences = false;

    IFC(PreResolveResources(spServiceProviderContext, vecResourceList));
    IFC(SaveEventRootWeakReference(spServiceProviderContext));
    IFC(SaveAmbientProperties(spServiceProviderContext));

    if (m_spSubReader)
    {
        IFC(m_spXamlSavedContext->get_Stack(spObjectWriterStack));
        spObjectWriterStack->PushScope();
    }

Cleanup:
    VERIFYHR(RemoveObjectReferencesFromStack());

    RRETURN(S_OK);
}

std::shared_ptr<XamlSavedContext> CTemplateContent::GetSavedContext() const
{
    return m_spXamlSavedContext;
}

_Check_return_ HRESULT
CTemplateContent::PreResolveResources(
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
    _In_ const std::vector<std::pair<bool, xstring_ptr>>& vecResourceList
    )
{
    if (m_spSubReader)
    {
        IFC_RETURN(PreResolveResourcesFromListOfReferences(spServiceProviderContext, vecResourceList));
    }
    else
    {
        IFC_RETURN(PreResolveResourcesFromNodeList(spServiceProviderContext));
    }

    return S_OK;
}

_Check_return_ HRESULT
CTemplateContent::PreResolveResourcesFromNodeList(
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext
    )
{
    HRESULT hr = S_OK;
    std::shared_ptr<XamlReader> spReader;
    xref_ptr<CResourceDictionary> spResourceDictionary;
    auto core = GetContext();
    CREATEPARAMETERS cp(core);

    IFC(m_spNodeList->get_Reader(spReader));
    while ((hr = spReader->Read()) == S_OK)
    {
        if (spReader->CurrentNode().get_NodeType() == XamlNodeType::xntStartObject)
        {
            XamlTypeToken xamlTypeToken = spReader->CurrentNode().get_XamlType()->get_TypeToken();

            if ((xamlTypeToken == XamlTypeToken(tpkNative, KnownTypeIndex::StaticResource)) ||
                (xamlTypeToken == XamlTypeToken(tpkNative, KnownTypeIndex::ThemeResource)))
            {
                bool bResourceFromReadOnlyDictionary = false;

                if (!spResourceDictionary)
                {
                    IFC(CResourceDictionary::Create(((CDependencyObject**)spResourceDictionary.ReleaseAndGetAddressOf()), &cp));
                    IFC(spResourceDictionary->EnsurePeerDuringCreate());
                }
                IFC(ResolveAndSaveResource(core, spServiceProviderContext, spReader, spResourceDictionary, &bResourceFromReadOnlyDictionary));
                if (!bResourceFromReadOnlyDictionary)
                {
                    m_bContentHasResourceReferences = true;
                }
            }
        }
    }

    IFC(CompleteInitializationOfLocalDictionary(spResourceDictionary));

Cleanup:
    if (FAILED(hr) && spResourceDictionary)
    {
        // Something went wrong - unpeg pResourceDictionary so it will not cause a leak.
        spResourceDictionary->UnpegManagedPeerNoRef();
    }
    RRETURN(hr);
}

_Check_return_ HRESULT
CTemplateContent::PreResolveResourcesFromListOfReferences(
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
    _In_ const std::vector<std::pair<bool, xstring_ptr>>& vecResourceList
    )
{
    HRESULT hr = S_OK;

    xref_ptr<CResourceDictionary> spResourceDictionary;
    auto core = GetContext();
    CREATEPARAMETERS cp(core);

    for (auto& strResource : vecResourceList)
    {
        bool bResourceFromReadOnlyDictionary = false;
        if (!spResourceDictionary)
        {
            IFC(CResourceDictionary::Create(((CDependencyObject**)spResourceDictionary.ReleaseAndGetAddressOf()), &cp));
            IFC(spResourceDictionary->EnsurePeerDuringCreate());
        }
        IFC(ResolveReferenceAndAddToLocalDictionary(core, spServiceProviderContext, strResource.second, strResource.first, spResourceDictionary, &bResourceFromReadOnlyDictionary));
        if (!bResourceFromReadOnlyDictionary)
        {
            m_bContentHasResourceReferences = true;
        }
    }

    IFC(CompleteInitializationOfLocalDictionary(spResourceDictionary));

Cleanup:
    if (FAILED(hr) && spResourceDictionary)
    {
        // Something went wrong - unpeg pResourceDictionary so it will not cause a leak.
        spResourceDictionary->UnpegManagedPeerNoRef();
    }
    RRETURN(hr);
}

_Check_return_ HRESULT
CTemplateContent::CompleteInitializationOfLocalDictionary(
    _In_ xref_ptr<CResourceDictionary>& spResourceDictionary)
{
    if (spResourceDictionary)
    {
        xref_ptr<CResourceDictionary> spDictionaryCopy(spResourceDictionary);
        SP_ObjectWriterStack spObjectWriterStack;
        std::shared_ptr<XamlQualifiedObject> spQO;

        IFC_RETURN(m_spXamlSavedContext->get_Stack(spObjectWriterStack));

        ASSERT(!spObjectWriterStack->empty());
        spObjectWriterStack->Current().ensure_CompressedStack();

        IFC_RETURN(XamlQualifiedObject::CreateNoAddRef(GetContext(), XamlTypeToken(tpkNative, KnownTypeIndex::ResourceDictionary),
                                    spDictionaryCopy.detach(),
                                    spQO));

        IFC_RETURN(this->SetParticipatesInManagedTreeDefault());
        IFC_RETURN(this->AddPeerReferenceToItem(spResourceDictionary.get()));
        spResourceDictionary->UnpegManagedPeerNoRef();

        spObjectWriterStack->Current().get_CompressedStack()->set_Resources(spQO);
    }

    return S_OK;
}

_Check_return_ HRESULT
CTemplateContent::ResolveAndSaveResource(
    _In_ CCoreServices *pCore,
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
    _In_ const std::shared_ptr<XamlReader>& spReader,
    _In_ xref_ptr<CResourceDictionary>& spResourceDictionary,
    _Out_ bool* pbResourceFromReadOnlyDictionary)
{
    HRESULT hr = S_OK;

    // We need to find the ResourceKey property, which means skipping over any directives
    // (these may be present if the Static/ThemeResource was specified using object-element syntax)
    // We'll read until we've encountered (and consumed) the matching XamlNodeType::xntEndObject.
    int depth = 1;
    while ((hr = spReader->Read()) == S_OK && depth > 0)
    {
        switch (spReader->CurrentNode().get_NodeType())
        {
            case XamlNodeType::xntStartObject:
            {
                ++depth;
            }
            break;

            case XamlNodeType::xntEndObject:
            {
                --depth;
            }
            break;

            case XamlNodeType::xntStartProperty:
            {
                ++depth;
                if (!spReader->CurrentNode().get_Property()->IsDirective())
                {
                    // StaticResourceExtension & ThemeResourceExtension have only one property.
                    ASSERT(spReader->CurrentNode().get_Property()->get_PropertyToken().GetHandle() == KnownPropertyIndex::StaticResource_ResourceKey ||
                        spReader->CurrentNode().get_Property()->get_PropertyToken().GetHandle() == KnownPropertyIndex::ThemeResource_ResourceKey);
                    bool isStaticResource = (spReader->CurrentNode().get_Property()->get_PropertyToken().GetHandle() == KnownPropertyIndex::StaticResource_ResourceKey);

                    if ((hr = spReader->Read()) == S_OK)
                    {
                        XamlNodeType nodeType = spReader->CurrentNode().get_NodeType();

                        if (nodeType == XamlNodeType::xntValue)
                        {
                            xstring_ptr strKey;

                            // lookup the string from CValue or CString
                            if (spReader->CurrentNode().get_Value()->GetValue().GetType() == valueString)
                            {
                                strKey = spReader->CurrentNode().get_Value()->GetValue().AsString();
                            }
                            else if (spReader->CurrentNode().get_Value()->GetTypeToken().GetHandle() == KnownTypeIndex::String &&
                                     spReader->CurrentNode().get_Value()->GetDependencyObject() != NULL)
                            {
                                strKey = static_cast<CString*>(spReader->CurrentNode().get_Value()->GetDependencyObject())->m_strString;
                            }
                            else
                            {
                                ASSERT(FALSE);
                            }

                            IFC_RETURN(ResolveReferenceAndAddToLocalDictionary(pCore, spServiceProviderContext, strKey, isStaticResource, spResourceDictionary, pbResourceFromReadOnlyDictionary));
                        }
                        else
                        {
                            // complex case.
                            // TODO: when do we hit this case?
                        }
                    }

                    ASSERT(hr != S_FALSE);
                }
            }
            break;

            case XamlNodeType::xntEndProperty:
            {
                --depth;
            }
            break;
        }
        ASSERT(hr != S_FALSE);
    }
    ASSERT(hr != S_FALSE);
    IFC_RETURN(hr);

    return S_OK;
}

_Check_return_ HRESULT
CTemplateContent::ResolveReferenceAndAddToLocalDictionary(
    _In_ CCoreServices *pCore,
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
    _In_ const xstring_ptr& strKey,
    _In_ const bool isStaticResource,
    _In_ xref_ptr<CResourceDictionary>& spResourceDictionary,
    _Out_ bool* pbResourceFromReadOnlyDictionary)
{
    HRESULT hr = S_OK;
    CDependencyObject *pValueNoRef = nullptr;
    CDependencyObject *pValue = nullptr;
    CDependencyObjectWrapper *pDOWrapper = nullptr;
    *pbResourceFromReadOnlyDictionary = FALSE;

    xref_ptr<CResourceDictionary> dictionaryReadFrom;
    // first look the value up in the cached ResourceDictionary
    IFC(spResourceDictionary->GetKeyForResourceResolutionNoRef(
        strKey,
        Resources::LookupScope::All,
        &pValueNoRef,
        &dictionaryReadFrom));

    if (dictionaryReadFrom)
    {
        *pbResourceFromReadOnlyDictionary = dictionaryReadFrom->IsReadOnlyDictionary();
    }

    if (!pValueNoRef)
    {
        // second:  if not found in the cached ResourceDictionary look it up in the current context.
        if (isStaticResource)
        {
            IFC(CStaticResourceExtension::LookupResourceNoRef(strKey, spServiceProviderContext, pCore, &pValueNoRef, FALSE /* bShouldCheckThemeResources */));
        }
        else
        {
            IFC(CThemeResourceExtension::LookupResource(
                strKey,
                spServiceProviderContext,
                pCore,
                FALSE /* bShouldCheckThemeResources */,
                &pValue));
            pValueNoRef = pValue;
        }

        // third:  if found in the current context, cache it.
        if (pValueNoRef)
        {
            CValue vValue;

            // [Bug fix: #95299]
            // If we're dealing with a DO that doesn't allow multiple parents then wrap it to hide
            // it's already parented (by the mail ResourceDictionary) and add to this ResourceDictionary
            IFCEXPECT(!pValueNoRef->OfTypeByIndex<KnownTypeIndex::DependencyObjectWrapper>());
            if (!pValueNoRef->DoesAllowMultipleAssociation())
            {
                CREATEPARAMETERS cp(pCore);

                IFC(CDependencyObjectWrapper::Create(((CDependencyObject**)&pDOWrapper), &cp));
                IFC(pDOWrapper->SetWrappedDO(pValueNoRef));
                vValue.SetObjectNoRef(pDOWrapper);
                pDOWrapper = nullptr;
            }
            else
            {
                vValue.SetObjectAddRef(pValueNoRef);
            }
            IFC(spResourceDictionary->Add(strKey, &vValue, NULL, FALSE /*bKeyIsType*/));

            // [Bug fix: #95299]
            // The DO wrapper prevents the DO resource from being correctly parented to ResourceDictionary so fix that here
            if (pDOWrapper != NULL)
            {
                IFC(spResourceDictionary->AddPeerReferenceToItem(pValueNoRef));
            }
        }

        // if the value isn't found, that's fine.  It's entirely possible that the value was somewhere in a
        // resource dictionary either in the nodestream, or on a property of something in the nodestream.
    }
    else
    {
        // If the value is found, unpeg the peg that happened during GetKeyNoRef
        pValueNoRef->UnpegManagedPeerNoRef();
    }

Cleanup:
    ReleaseInterface(pDOWrapper);
    ReleaseInterface(pValue);

    RRETURN(hr);
}

_Check_return_ HRESULT
CTemplateContent::RemoveObjectReferencesFromStack()
{
    SP_ObjectWriterStack spObjectWriterStack;

    IFC_RETURN(m_spXamlSavedContext->get_Stack(spObjectWriterStack));

    for(auto it = spObjectWriterStack->begin();
        it != spObjectWriterStack->end();
        ++it)
    {
        (*it).clear_Instance();
        (*it).clear_Collection();
    }
    return S_OK;
}

xref_ptr<CDependencyObject> CTemplateContent::GetSavedEventRoot() const
{
    return m_spEventRoot.lock();
}

_Check_return_ HRESULT
CTemplateContent::SaveEventRootWeakReference(
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext
    )
{
    // The event root used in resolving the event handler.
    auto spEventRoot = spServiceProviderContext->GetRootObject();

    if (spEventRoot)
    {
        m_spEventRoot = xref::get_weakref(spEventRoot->GetDependencyObject());
    }

    return S_OK;
}

_Check_return_ HRESULT
CTemplateContent::SaveAmbientProperties(
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext
    )
{
    SP_ObjectWriterStack spObjectWriterStack;
    std::shared_ptr<XamlQualifiedObject> qoTargetType;

    IFC_RETURN(m_spXamlSavedContext->get_Stack(spObjectWriterStack));

    ASSERT(!spObjectWriterStack->empty());

    // Cache target type from the saved part of the stack.
    IFC_RETURN(spServiceProviderContext->GetAmbientTargetType(qoTargetType));
    spObjectWriterStack->Current().ensure_CompressedStack();

    ASSERT(spObjectWriterStack->Current().exists_CompressedStack());

    spObjectWriterStack->Current().get_CompressedStack()->set_TargetType(qoTargetType);

    return S_OK;
}

// Load create a DO from this TemplateContent's saved XamlSavedContext
// and XamlOptimizedNodeList.
_Check_return_ HRESULT
CTemplateContent::Load(
    _In_ const ObjectWriterSettings& objectWriterSettings,
    _In_ bool bTryOptimizeContent,
    _Outptr_result_maybenull_ CDependencyObject **ppDependencyObject
    )
{
    std::shared_ptr<XamlQualifiedObject> spLastValue;

    if (m_spNodeList)
    {
        IFC_RETURN(LoadXbfVersion1(objectWriterSettings, bTryOptimizeContent, spLastValue));
    }
    else if (m_spSubReader)
    {
        IFC_RETURN(LoadXbfVersion2(objectWriterSettings, bTryOptimizeContent, spLastValue));
    }

    // The saved object may have been conditionally declared, and the condition evaluated
    // to false at runtime, in which case no object will have been created.
    if (spLastValue)
    {
        *ppDependencyObject = spLastValue->GetDependencyObject();

        // AddRef the object so that it can live beyond the lifetime of the qoResult
        AddRefInterface(*ppDependencyObject);

        // We need to leave the root pegged until it's returned to the managed caller.
        spLastValue->ClearHasPeggedManagedPeer();
    }

    return S_OK;
}

_Check_return_ HRESULT
CTemplateContent::LoadXbfVersion1(
    _In_ const ObjectWriterSettings& objectWriterSettings,
    _In_ bool bTryOptimizeContent,
    _Out_ std::shared_ptr<XamlQualifiedObject>& spRootInstance
    )
{
    HRESULT hr = S_OK;
    std::shared_ptr<XamlSchemaContext> spSchemaContext;
    std::shared_ptr<ObjectWriter> spObjectWriter;
    std::shared_ptr<XamlReader> spReader;

    auto spErrorService = std::make_shared<ParserErrorService>();
    spErrorService->Initialize(GetContext());

    IFC(PlayXaml(spReader));
    IFC(spReader->GetSchemaContext(spSchemaContext));
    IFC(spSchemaContext->SetErrorService(spErrorService));

    IFC(ObjectWriter::Create(m_spXamlSavedContext, objectWriterSettings, spObjectWriter));

    // Deferring ResourceDictionaries inside Templates
    // will hinder performance, so have it switched off.
    spObjectWriter->DisableResourceDictionaryDefer();

    // only optimize the template content once
    if (bTryOptimizeContent && !m_bContentAlreadyOptimized)
    {
        std::shared_ptr<XamlOptimizedNodeList> spOptimizedNodeList;

        IFC(LoadAndOptimize(spReader, spObjectWriter, spSchemaContext, spOptimizedNodeList));
        m_bContentAlreadyOptimized = TRUE;

        if (spOptimizedNodeList)
        {
            spReader.reset();
            m_spNodeList = spOptimizedNodeList;
        }
    }
    else
    {
        std::size_t nodeCount = 0;
        std::size_t startObjectExpectedPosition = 1;
        while ((hr = spReader->Read()) == S_OK)
        {
            ++nodeCount;
            // The first node (or second, if the first is a ConditionalScope, e.g.
            // a Storyboard assigned to VisualState.Storyboard that was declared
            // conditionally) *should* be a StartObject. If the deferred property it
            // would have been set on is a "create value on demand", however, the node
            // will be marked as "retrieved from member", which means the ObjectWriter
            // will look for the object on the stack instead of creating it now/later.
            // If we are in a case where TemplateContent's deferral mechanism was
            // repurposed for non-TemplateContent properties via property replacement
            // at parse-time, then the parser possibly may not have known to create
            // the object previously in the StartMember node. Therefore, we will
            // clear the first node's "IsRetrieved" flag to force creation of the object.
            //
            // See comment in WriteToMatchingEnd() when handling a node of type
            // StartObject for more details.
            XamlNode currentNode = spReader->CurrentNode();

            if (nodeCount == startObjectExpectedPosition)
            {
                if (currentNode.get_NodeType() == xntStartObject)
                {
                    ASSERT(nodeCount == 1 || nodeCount == 2);

                    currentNode.InitStartObjectNode(
                        currentNode.get_XamlType(),
                        false,
                        currentNode.IsUnknown());
                }
                else if (   currentNode.get_NodeType() == xntStartConditionalScope
                         && nodeCount == 1)
                {
                    startObjectExpectedPosition = 2;
                }
                else
                {
                    // Either first node is StartObject, or first node is
                    // StartConditionalNode and second is StartObject
                    ASSERT(false);
                }
            }

            IFC(spObjectWriter->WriteNode(currentNode));
        }
    }

    spRootInstance = spObjectWriter->get_Result();

Cleanup:
    return hr;
}

namespace
{
    bool IsCacheableNodeType(ObjectWriterNodeType nodeType)
    {
        switch (nodeType)
        {
            case ObjectWriterNodeType::SetDeferredProperty:
            {
                 return true;
            }
            default:
            {
                return false;
            }
        }
    }
}

ObjectWriterNode CTemplateContent::GenerateOptimizedNode(unsigned int cacheLookupIndex, const ObjectWriterNode& inNode)
{
    auto nodeType = inNode.GetNodeType();
    switch (nodeType)
    {
        case ObjectWriterNodeType::SetDeferredProperty:
        {
            ASSERT(cacheLookupIndex < m_cachedTemplateContentObjects.size());
            std::shared_ptr<XamlQualifiedObject> spCachedValue = m_cachedTemplateContentObjects[cacheLookupIndex];
            return ObjectWriterNode::MakeSetDeferredPropertyNodeWithValue(inNode.GetLineInfo(), inNode.GetXamlProperty(), spCachedValue);
        }
        case ObjectWriterNodeType::ProvideThemeResourceValue:
        case ObjectWriterNodeType::ProvideStaticResourceValue:
        {
            ASSERT(cacheLookupIndex < m_cachedTemplateContentObjects.size());
            std::shared_ptr<XamlQualifiedObject> spCachedValue = m_cachedTemplateContentObjects[cacheLookupIndex];
            return ObjectWriterNode::MakePushConstantNode(inNode.GetLineInfo(), spCachedValue);
        }
        case ObjectWriterNodeType::SetValueFromStaticResource:
        case ObjectWriterNodeType::SetValueFromThemeResource:
        {
            ASSERT(cacheLookupIndex < m_cachedTemplateContentObjects.size());
            std::shared_ptr<XamlQualifiedObject> spCachedValue = m_cachedTemplateContentObjects[cacheLookupIndex];
            return ObjectWriterNode::MakeSetValueConstantNode(inNode.GetLineInfo(), inNode.GetXamlProperty(), spCachedValue);
        }
        default:
        {
            ASSERT(false);
            return inNode;
        }
    }
}

_Check_return_ HRESULT
CTemplateContent::LoadXbfVersion2(
    _In_ const ObjectWriterSettings& objectWriterSettings,
    _In_ bool bTryOptimizeContent,
    _Out_ std::shared_ptr<XamlQualifiedObject>& spRootInstance
    )
{
    std::shared_ptr<BinaryFormatObjectWriter> spBinaryFormatObjectWriter;

    IFC_RETURN(BinaryFormatObjectWriter::Create(
        m_spXamlSavedContext,
        objectWriterSettings,
        spBinaryFormatObjectWriter));
    m_spSubReader->Reset();

    unsigned int cacheIndexCount = 0;
    for (;;)
    {
        bool endOfStream = false;
        ObjectWriterNode node;

        IFC_RETURN(m_spSubReader->TryReadHRESULT(node, &endOfStream));
        if (endOfStream) break;

        if (m_bContentAlreadyOptimized)
        {
            // we are assuming that the stream is always played back in the same order
            // every time and can hence assume the cache indexes always align during replay.
            if (IsCacheableNodeType(node.GetNodeType()))
            {
                auto optimizedNode = GenerateOptimizedNode(cacheIndexCount, node);
                cacheIndexCount++;
                IFC_RETURN(spBinaryFormatObjectWriter->WriteNode(optimizedNode));
            }
            else
            {
                IFC_RETURN(spBinaryFormatObjectWriter->WriteNode(node));
            }
        }
        else
        {
            // write the node through without any pre processing
            IFC_RETURN(spBinaryFormatObjectWriter->WriteNode(node));

            // the stream hasn't be preprocessed, so lets attempt to cache some
            // data that is repeated across template expansions.
            if (bTryOptimizeContent && IsCacheableNodeType(node.GetNodeType()))
            {
                std::shared_ptr<XamlQualifiedObject> spLastValue = spBinaryFormatObjectWriter->get_Result();

                if (spLastValue->GetDependencyObject() != nullptr)
                {
                    IFC_RETURN(AddPeerReferenceToItem(spLastValue->GetDependencyObject()));
                }
                m_cachedTemplateContentObjects.emplace_back(std::move(spLastValue));
            }

        }
    }

    spRootInstance = spBinaryFormatObjectWriter->get_Result();
    if (bTryOptimizeContent && !m_bContentAlreadyOptimized)
    {
        m_bContentAlreadyOptimized = true;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   LoadAndOptimize
//
//  Synopsis:
//      Load the nodes from the passed XamlReader into the passed ObjectWriter
//      and at the same time identify and try to share any property values
//      that would not change from Load() to Load().
//
//------------------------------------------------------------------------
//
_Check_return_ HRESULT
CTemplateContent::LoadAndOptimize(
    _In_ const std::shared_ptr<XamlReader>& spReader,
    _In_ const std::shared_ptr<ObjectWriter>& spObjectWriter,
    _In_ const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
    _Out_ std::shared_ptr<XamlOptimizedNodeList>& rspOptimizedNodeList
    )
{
    HRESULT hr = S_OK;

    std::shared_ptr<XamlWriter> spOptimizedWriter;

    auto spOptimizedNodeList = std::make_shared<XamlOptimizedNodeList>(spSchemaContext);
    spOptimizedNodeList->ReserveBasedOn(m_spNodeList);
    IFC(spOptimizedNodeList->get_Writer(spOptimizedWriter));

    //
    // Iterate over each node from the reader...
    //
    while ((hr = spReader->Read()) == S_OK)
    {
        //
        // Write the node to both the ObjectWriter that will create the tree and
        // the optimized node-list.
        //
        IFC(spObjectWriter->WriteNode(spReader->CurrentNode()));
        IFC(spOptimizedWriter->WriteNode(spReader->CurrentNode()));

        //
        // If the current node is a XamlNodeType::xntStartProperty, and we should
        // optimize it, then we will do 2 things:
        //
        //      1)  Write to the ObjectWriter that is creating the tree until we get the matching
        //          EndProperty.
        //      2)  To the optimized node-list, just write a value node that contains the value
        //          from the ObjectWriter (which will be the value that it just set to the property,
        //          after conversions, etc.
        //
        if (spReader->CurrentNode().get_NodeType() == XamlNodeType::xntStartProperty &&
            !spObjectWriter->IsCustomWriterActive())
        {
            bool bShouldOptimizeProperty = false;
            std::shared_ptr<XamlProperty> spProperty;
            std::shared_ptr<XamlType> spPropertyType;

            spProperty = spReader->CurrentNode().get_Property();
            IFC(spProperty->get_Type(spPropertyType));

            IFC(ShouldOptimizeProperty(spProperty, &bShouldOptimizeProperty));

            if (bShouldOptimizeProperty)
            {
                IFC(TryOptimizeValue(spReader, spObjectWriter, spOptimizedWriter));
            }
            else if (spPropertyType)
            {
                bool bIsTemplate = false;
                IFC(spPropertyType->IsTemplate(bIsTemplate));

                if (bIsTemplate)
                {
                    IFC(WriteToMatchingEnd(spReader, std::static_pointer_cast<XamlWriter>(spObjectWriter), spOptimizedWriter));
                }
                else
                {
                    IFC(TryOptimizeTypeConvertedProperty(spProperty, spReader, spObjectWriter, spOptimizedWriter));
                }
            }
        }

    }

    IFC(spOptimizedWriter->Close());
    rspOptimizedNodeList = spOptimizedNodeList;

Cleanup:
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Method:   ShouldOptimizeProperty
//
//  Synopsis:
//      Returns true from pbShouldOptimizeProperty if the property is one
//      that we should *attempt* to optimize.
//
//------------------------------------------------------------------------
//
_Check_return_ HRESULT
CTemplateContent::ShouldOptimizeProperty(
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _Out_ bool *pbShouldOptimizeProperty
    )
{
    bool bShouldOptimizeProperty = false;

    // Handful of properties that we know are safe to optimize, and that we know are
    // in generic.xaml
    if (spProperty->get_PropertyToken().GetProviderKind() == tpkNative)
    {
        // for the time being we could limit the optimizable properties to the VisualState's deferred
        // content, but for gains later we can optimize by using pre-parsed valuetype properties.
        bShouldOptimizeProperty = spProperty->get_PropertyToken().GetHandle() == KnownPropertyIndex::VisualState___DeferredStoryboard ||
            spProperty->get_PropertyToken().GetHandle() == KnownPropertyIndex::FrameworkTemplate_Template ||
            spProperty->get_PropertyToken().GetHandle() == KnownPropertyIndex::VisualState___DeferredSetters;
    }

    *pbShouldOptimizeProperty = bShouldOptimizeProperty;

    RRETURN(S_OK);
}



//------------------------------------------------------------------------
//
//  Method:   TryOptimizeValue
//
//  Synopsis:
//      Will attempt to convert the series of nodes from the spReader
//      to a value that can be shared by all instances trees created from
//      the TemplateContent.
//
//------------------------------------------------------------------------
//
_Check_return_ HRESULT
CTemplateContent::TryOptimizeValue(
    _In_ const std::shared_ptr<XamlReader>& spReader,
    _In_ const std::shared_ptr<ObjectWriter>& spObjectWriter,
    _In_ const std::shared_ptr<XamlWriter>& spOptimizedWriter
    )
{
    std::shared_ptr<XamlWriter> spEmpty;
    bool bWriteValue = true;
    CDependencyObject* pdoLastValue = nullptr;

    IFC_RETURN(WriteToMatchingEnd(spReader, std::static_pointer_cast<XamlWriter>(spObjectWriter), spEmpty));
    std::shared_ptr<XamlQualifiedObject> spLastValue = spObjectWriter->get_Result();

    if (spLastValue && !spLastValue->IsEmpty())
    {
        //
        // If the Value in question is a TemplateContent, then we have to make sure that
        // there is nothing that will disqualify from sharing a value.
        //
        // If this is the case then we would have wanted to store the nodes rather than storing
        // the resultant TemplateContent, but have a problem:  We have already avoided storing
        // the nodes for the TemplateContent's nodelist in the spOptimizedWriter.  Thankfully,
        // there's simple fallback: just use the stored nodes from the TemplateContent.
        //
        // **N.B.: Using this method, TemplateContent is the only type of property sharing
        // that we can recover from after the fact if we find that the sharing shouldn't have happened.
        //
        pdoLastValue = spLastValue->GetDependencyObject();

        if (pdoLastValue && pdoLastValue->GetTypeIndex() == KnownTypeIndex::TemplateContent)
        {
            if (static_cast<CTemplateContent*>(pdoLastValue)->m_bContentHasResourceReferences)
            {
                //
                // there is a possibility that this template content might rely on context
                // that could differ from one Load to another and.  We can't chance it,
                // so we'll have to play the nodes into the OptimizedWriter
                //
                std::shared_ptr<XamlReader> spTemplateContentReader;
                IFC_RETURN(static_cast<CTemplateContent*>(pdoLastValue)->PlayXaml(spTemplateContentReader));
                if (spTemplateContentReader->Read() == S_OK)
                {
                    IFC_RETURN(spOptimizedWriter->WriteNode(spTemplateContentReader->CurrentNode()));
                    IFC_RETURN(WriteToMatchingEnd(spTemplateContentReader, spOptimizedWriter, spEmpty));
                }
                bWriteValue = false;
            }
        }

    }

    if (bWriteValue)
    {
        IFC_RETURN(spOptimizedWriter->WriteValue(spLastValue));

        if (pdoLastValue)
        {
            IFC_RETURN(this->AddPeerReferenceToItem(pdoLastValue));
        }
    }

    IFC_RETURN(spOptimizedWriter->WriteNode(spReader->CurrentNode()));

    return S_OK;
}

_Check_return_ HRESULT
CTemplateContent::TryOptimizeTypeConvertedProperty(
    _In_ const std::shared_ptr<XamlProperty>& spXamlProperty,
    _In_ const std::shared_ptr<XamlReader>& spReader,
    _In_ const std::shared_ptr<ObjectWriter>& spObjectWriter,
    _In_ const std::shared_ptr<XamlWriter>& spOptimizedWriter
    )
{
    HRESULT hr = S_OK;

    XamlTypeToken stringTypeToken = XamlTypeToken(tpkNative, KnownTypeIndex::String);

    if (spXamlProperty->get_PropertyToken().GetProviderKind() == tpkNative
        && spXamlProperty->get_PropertyToken().GetHandle() == KnownPropertyIndex::TemplateBinding_Property)
    {
        IFC(spReader->Read());
        IFCEXPECT(hr != S_FALSE);
        {
            if(spReader->CurrentNode().get_NodeType() == XamlNodeType::xntValue && spReader->CurrentNode().get_Value()->GetTypeToken() == stringTypeToken)
            {
                // write the text node.
                IFC(spObjectWriter->WriteNode(spReader->CurrentNode()));

                // the next node should be an end-property.
                IFC(spReader->Read());
                IFCEXPECT(hr != S_FALSE);

                // write the end-property to the ObjectWriter.
                IFC(spObjectWriter->WriteNode(spReader->CurrentNode()));

                // now the ObjectWriter should have the result of the
                // last property-set ready.  Write that as a value
                // to the OptimizedWriter
                {
                    std::shared_ptr<XamlQualifiedObject> spResult = spObjectWriter->get_Result();
                    IFC(spOptimizedWriter->WriteValue(spResult));
                }

                IFC(spOptimizedWriter->WriteNode(spReader->CurrentNode()));
            }
            else
            {
                // something other than converting text-to-property.
                // write as-is and then we're done.
                IFC(spObjectWriter->WriteNode(spReader->CurrentNode()));
                IFC(spOptimizedWriter->WriteNode(spReader->CurrentNode()));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   WriteToMatchingEnd
//
//  Synopsis:
//      Read from the passed XamlReader and write
//      to the passed XamlWriters until a matching
//      XamlNodeType::xntEndXxxxx has been reached.
//
//      It is assumed that the XamlNodeType::xntStartXxxxx node has
//      already been read from spReader.
//
//------------------------------------------------------------------------
//
_Check_return_ HRESULT
CTemplateContent::WriteToMatchingEnd(
    _In_ const std::shared_ptr<XamlReader>& spReader,
    _In_ const std::shared_ptr<XamlWriter>& spWriter,
    _In_opt_ const std::shared_ptr<XamlWriter>& spSecondaryWriter
    )
{
    HRESULT hr = S_OK;
    // start at depth 1 for the StartXxxx that
    // we should have already gotten.
    unsigned int nodeDepth = 1;
    unsigned int expectedFirstStartObjectNodeDepth = 1;

    while(nodeDepth > 0 && (hr = spReader->Read()) == S_OK)
    {
        XamlNode currentNode = spReader->CurrentNode();

        switch (currentNode.get_NodeType())
        {
            case XamlNodeType::xntStartObject:
            {
                if (nodeDepth == expectedFirstStartObjectNodeDepth)
                {
                    // Legacy deferral of properties involved "hijacking" the TemplateContent deferral
                    // mechanism by replacing the original property (of type Foo) with an internal property
                    // of type TemplateContent during the parse phase. This is currently done in two places:
                    // VisualState.Storyboard and VisualState.Setters.
                    //
                    // In the latter case, VisualState.Setters is a "create value on demand" property of type
                    // SettersBaseCollection, which results in the next StartObject node, corresponding to the
                    // SettersBaseCollection, being marked as "retrieved from member". This means that the
                    // ObjectWriter will, instead of creating a new object, will look for it on the previous
                    // frame of the ObjectWriterStack, since it would've been created during the processing of
                    // the previous WriteMember node rather than when the current StartObject's matching EndObject
                    // node is processed. However, because of the property replacement done in order to trigger
                    // deferral of VisualState.Setters via TemplateContent, the expected SettersBaseCollection
                    // is never created and placed on the previous frame.
                    //
                    // Therefore, we will modify the current node and clear its "IsRetrieved" flag to force
                    // creation of the expected object.
                    currentNode.InitStartObjectNode(
                        currentNode.get_XamlType(),
                        false,
                        currentNode.IsUnknown());
                }
                ++nodeDepth;
                break;
            }
            case XamlNodeType::xntStartConditionalScope:
            {
                // If StartConditionalScope is the first node, the StartObject is the second node
                if (nodeDepth == 1)
                {
                    expectedFirstStartObjectNodeDepth = 2;
                }
                ++nodeDepth;
                break;
            }
            case XamlNodeType::xntStartProperty:
            {
                ++nodeDepth;
                break;
            }
            case XamlNodeType::xntEndConditionalScope:
            case XamlNodeType::xntEndObject:
            case XamlNodeType::xntEndProperty:
            {
                --nodeDepth;
                break;
            }
        }

        IFC(spWriter->WriteNode(currentNode));
        if (spSecondaryWriter)
        {
            IFC(spSecondaryWriter->WriteNode(currentNode));
        }
    }

    // if there's an actual failing hr, goto cleanup;
    IFC(hr);

    // If if we couldn't read valid nodes up to the balancing
    // XamlNodeType::xntEndProperty then things are seriously wrong.
    // Assert in debug builds, and bail out otherwise.
    IFCEXPECT_ASSERT(hr == S_OK);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
CTemplateContent::PlayXaml(_Out_ std::shared_ptr<XamlReader>& rspReader)
{
    if (m_spNodeList && m_spXamlSavedContext)
    {
        RRETURN(m_spNodeList->get_Reader(rspReader));
    }
    else
    {
        RRETURN(E_FAIL);
    }
}

_Check_return_
size_t CTemplateContent::GetOrCreateNameIndex(_In_ const xstring_ptr_view& strName)
{
    auto it = m_nameTableMap.find(strName);
    if (it == m_nameTableMap.end())
    {
        xstring_ptr promotedName;
        IFCFAILFAST(strName.Promote(&promotedName));

        ASSERT(m_nameTableMap.size() <= std::numeric_limits<UINT32>::max());
        auto result = m_nameTableMap.insert(std::make_pair(promotedName, static_cast<UINT32>(m_nameTableMap.size())));
        ASSERT(result.second);
        it = result.first;
    }
    return it->second;
}

_Check_return_
size_t CTemplateContent::TryGetNameIndex(_In_ const xstring_ptr_view& strName) const
{
    auto index = static_cast<size_t>(-1);
    auto it = m_nameTableMap.find(strName);
    if (it != m_nameTableMap.end())
    {
        ASSERT(it->second <= std::numeric_limits<UINT32>::max());
        index = it->second;
    }
    return index;
}
