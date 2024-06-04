// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ObjectWriter.h"
#include "ObjectWriterStack.h"
#include "ObjectWriterFrame.h"
#include "ParserErrorService.h"
#include "XamlOptimizedNodeList.h"
#include "XamlReader.h"
#include "SavedContext.h"
#include "XamlSchemaContext.h"
#include "XamlServiceProviderContext.h"

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Destroy any saved context.
//
//---------------------------------------------------------------------------
CDeferredKeys::~CDeferredKeys()
{
    SP_ObjectWriterStack spObjectWriterStack;
    std::shared_ptr<XamlQualifiedObject> spQO;
    CResourceDictionary* pResourceDictionary = NULL;

    if (SUCCEEDED(m_spXamlSavedContext->get_Stack(spObjectWriterStack)))
    {
        ASSERT(!spObjectWriterStack->empty());
        if (spObjectWriterStack->Current().exists_CompressedStack())
        {
            // release resources that were captured in the internal
            // resource dictionary so that we could defer this node stream
            spQO = spObjectWriterStack->Current().get_CompressedStack()->get_Resources();
            if (spQO != NULL)
            {
                pResourceDictionary = static_cast<CResourceDictionary*>(spQO->GetDependencyObject());

                // Clear the resource dictionary out of the compressed stack (we put it there during PreResolve)
                spObjectWriterStack->Current().get_CompressedStack()->set_Resources(nullptr);
                spQO->SetDependencyObjectNoAddRef(nullptr);

                // And remove the peer reference to the dictionary from the host collection.
                // Note that if we're being destructed here because the parent dictionary (m_pCollection) is being destructed,
                // then the peer is already gone.  So skip if the parent is on its way out.
                CorePeggedPtr<CDependencyObject> autoPeg;
                autoPeg.TrySet(m_pCollection);
                if(autoPeg)
                {
                    IGNOREHR(m_pCollection->RemovePeerReferenceToItem(pResourceDictionary));
                }
            }
        }
    }

    // release any non deferred key indices
    if (m_spNonDeferredKeys)
    {
        m_spNonDeferredKeys->clear();
    }
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Save context required for playback of the keys stored as Xaml.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
CDeferredKeys::RecordXaml(
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
    _In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList,
    _In_ const std::shared_ptr<XamlSavedContext>& spContext,
    _In_ const bool fIsResourceDictionaryWithKeyProperty
    )
{
    HRESULT hr = S_OK;

    // store away some context that will be required for playback
    m_spNodeList = spNodeList;
    m_spXamlSavedContext = spContext;
    m_bContentHasResourceReferences = false;
    m_bHasKeyLookupBoundary = false;
    m_uiMaxAllowedIndexForLookup = 0;

    // we shouldn't be in a reentrant call
    ASSERT(!m_bResolvingResources);

    // don't allow faulting in keys during a preresolve stage
    m_bResolvingResources = true;

    // preresolve the node stream
    IFC(PreResolve(!fIsResourceDictionaryWithKeyProperty /* fResolveItems */, TRUE /* fResolveResources */, spServiceProviderContext));

    // capture ambient properties
    IFC(SaveAmbientProperties(spServiceProviderContext));

Cleanup:
    VERIFYHR(RemoveObjectReferencesFromStack());

    // safe to allow key resolve
    m_bResolvingResources = false;

    RRETURN(hr);
}

_Check_return_ HRESULT
CDeferredKeys::get_X_ConnectionIdProperty(
    _In_ const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
    _Out_ std::shared_ptr<DirectiveProperty>& spConnectionIdProperty)
{
    std::shared_ptr<XamlNamespace> spDirectiveNamespace;

    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strDirectiveNamespaceString, L"http://schemas.microsoft.com/winfx/2006/xaml");
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strConnectionId, L"ConnectionId");

    IFC_RETURN(spSchemaContext->GetXamlXmlNamespace(c_strDirectiveNamespaceString, spDirectiveNamespace));
    IFC_RETURN(spSchemaContext->GetXamlDirective(spDirectiveNamespace, c_strConnectionId, spConnectionIdProperty));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Preresolve StaticResource and ThemeResources into internal resource dictionary
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
CDeferredKeys::PreResolve(
    _In_ const bool fResolveItems,
    _In_ const bool fResolveResources,
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext
    )
{
    HRESULT hr = S_OK;

    XamlTypeToken tokResourceDictionary(tpkNative, KnownTypeIndex::ResourceDictionary);
    XamlTypeToken tokColorPaletteResources(tpkNative, KnownTypeIndex::ColorPaletteResources);
    XamlPropertyToken tokStaticResourceKey(tpkNative, KnownPropertyIndex::StaticResource_ResourceKey);

    std::shared_ptr<XamlReader>         spReader;
    std::shared_ptr<DirectiveProperty>  spKeyProperty;
    std::shared_ptr<DirectiveProperty>  spNameProperty;
    std::shared_ptr<DirectiveProperty>  spConnectionIdProperty;
    xstring_ptr                         spKeyValue;
    std::shared_ptr<XamlSchemaContext>  spSchemaContext;

    CResourceDictionary               *pResourceDictionary = NULL;

    XUINT32 uiNodeDepth              = 0;
    XUINT32 uiStartObjectStreamIndex = 0;
    XUINT32 uiLastNodeStreamIndex    = 0;
    bool fNonDeferrable              = false;
    bool fHasNamespacePreamble       = false;
    std::stack<bool> shouldResolveResource;

    ASSERT(m_spNodeList);
    IFC(m_spNodeList->get_Reader(spReader));

    if (fResolveItems)
    {
        IFC(spReader->GetSchemaContext(spSchemaContext));
        IFC(spSchemaContext->get_X_KeyProperty(spKeyProperty));
        IFC(spSchemaContext->get_X_NameProperty(spNameProperty));

        IFC(get_X_ConnectionIdProperty(spSchemaContext, spConnectionIdProperty));
        m_spKeyMap.reset(new containers::vector_map<xstring_ptr, XUINT32>);
    }

    while ((hr = spReader->Read()) == S_OK)
    {
        switch (spReader->CurrentNode().get_NodeType())
        {
            case XamlNodeType::xntNamespace:
                {
                    if (fResolveItems && !fHasNamespacePreamble)
                    {
                        if (uiNodeDepth == 0)
                        {
                            uiStartObjectStreamIndex = uiLastNodeStreamIndex;
                            fHasNamespacePreamble = TRUE;
                        }
                    }
                }
                break;

            case XamlNodeType::xntStartObject:
                {
                    if (fResolveItems)
                    {
                        if (uiNodeDepth == 0)
                        {
                            XamlTypeToken xamlTypeToken = spReader->CurrentNode().get_XamlType()->get_TypeToken();

                            if (!fHasNamespacePreamble)
                            {
                                uiStartObjectStreamIndex = uiLastNodeStreamIndex;
                            }

                            spKeyValue.Reset();
                            fHasNamespacePreamble = FALSE;
                            fNonDeferrable = xamlTypeToken == tokResourceDictionary ||
                                             xamlTypeToken == tokColorPaletteResources;
                        }

                        uiNodeDepth++;
                    }
                }
                break;

            case XamlNodeType::xntEndObject:
                {
                    if (fResolveItems)
                    {
                        uiNodeDepth--;
                        if (uiNodeDepth == 0)
                        {
                            if (!fNonDeferrable && !spKeyValue.IsNull())
                            {
                                (*m_spKeyMap)[spKeyValue] = uiStartObjectStreamIndex;
                                spKeyValue.Reset();
                                fNonDeferrable = FALSE;
                                fHasNamespacePreamble = FALSE;
                            }
                            else
                            {
                                if (!m_spNonDeferredKeys)
                                {
                                    m_spNonDeferredKeys = std::make_shared<xvector< XUINT32 >>();
                                }
                                IFC(m_spNonDeferredKeys->push_back(uiStartObjectStreamIndex));
                            }
                         }
                    }
                }
                break;

            case XamlNodeType::xntStartProperty:
                {
                    if (fResolveResources)
                    {
                        bool isStaticResourceKey = !!spReader->CurrentNode().get_Property()->get_PropertyToken().Equals(tokStaticResourceKey);
                        shouldResolveResource.push(isStaticResourceKey);
                    }

                    if (fResolveItems)
                    {
                        // BUG 308581: Workaround fix for MP to disqualify dictionary elements with x:ConnectionId
                        //             so that they are not deferred.
                        if (!fNonDeferrable)
                        {
                            XamlNode currentXamlNode = spReader->CurrentNode();

                            // x:ConnectionId Property encountered
                            if (XamlProperty::AreEqual(spConnectionIdProperty, currentXamlNode.get_Property()))
                            {
                                fNonDeferrable = TRUE;
                            }
                        }

                        if (uiNodeDepth == 1 && !fNonDeferrable)
                        {
                            XamlNode currentXamlNode = spReader->CurrentNode();

                            // x:Key Property encountered
                            if (XamlProperty::AreEqual(spKeyProperty, currentXamlNode.get_Property()))
                            {
                                if ((hr = spReader->Read()) == S_OK)
                                {
                                    currentXamlNode = spReader->CurrentNode();
                                    if (currentXamlNode.get_NodeType() == XamlNodeType::xntValue)
                                    {
                                        // lookup the string from CValue or CString
                                        if (spReader->CurrentNode().get_Value()->GetValue().GetType() == valueString)
                                        {
                                            spKeyValue = spReader->CurrentNode().get_Value()->GetValue().AsString();
                                        }
                                        else if (spReader->CurrentNode().get_Value()->GetTypeToken().GetHandle() == KnownTypeIndex::String &&
                                                 spReader->CurrentNode().get_Value()->GetDependencyObject() != NULL)
                                        {
                                            spKeyValue = static_cast<CString*>(spReader->CurrentNode().get_Value()->GetDependencyObject())->m_strString;
                                        }
                                        else
                                        {
                                            ASSERT(FALSE);
                                            spKeyValue.Reset();
                                        }
                                    }
                                }
                                else
                                {
                                    ASSERT(FALSE);
                                    IFC(E_FAIL);
                                }
                            }
                            else if (XamlProperty::AreEqual(spNameProperty, currentXamlNode.get_Property()))
                            {
                                fNonDeferrable = TRUE;
                            }
                        }
                    }
                }
                break;

            case XamlNodeType::xntValue:
                if (!shouldResolveResource.empty() && shouldResolveResource.top())
                {
                    // We enter here if we detected the StartProperty node for StaticResource.ResourceKey,
                    // and we're still at that scope, so we know that this value is a resource reference key.
                    // Try to resolve and save the resource now.
                    bool bResourceFromReadOnlyDictionary = false;
                    if (!pResourceDictionary)
                    {
                        CREATEPARAMETERS cp(m_pCollection->GetContext());
                        IFC(CResourceDictionary::Create(((CDependencyObject**)&pResourceDictionary), &cp));
                        IFC(pResourceDictionary->EnsurePeerDuringCreate());
                    }
                    IFC(ResolveAndSaveResource(m_pCollection->GetContext(), spServiceProviderContext, spReader, pResourceDictionary, &bResourceFromReadOnlyDictionary));
                    if (!bResourceFromReadOnlyDictionary)
                    {
                        m_bContentHasResourceReferences = true;
                    }
                }
                break;

            case XamlNodeType::xntEndProperty:
                if (fResolveResources)
                {
                    shouldResolveResource.pop();
                }
                break;

            default:
                break;
        }

        IFC(spReader->get_NextIndex(&uiLastNodeStreamIndex));
    }

    if (pResourceDictionary)
    {
        SP_ObjectWriterStack spObjectWriterStack;
        std::shared_ptr<XamlQualifiedObject> spQO;

        IFC(m_spXamlSavedContext->get_Stack(spObjectWriterStack));

        ASSERT(!spObjectWriterStack->empty());
        spObjectWriterStack->Current().ensure_CompressedStack();

        IFC(XamlQualifiedObject::CreateNoAddRef(m_pCollection->GetContext(), XamlTypeToken(tpkNative, KnownTypeIndex::ResourceDictionary),
                                    pResourceDictionary,
                                    spQO));

        IFC(m_pCollection->SetParticipatesInManagedTreeDefault());
        IFC(m_pCollection->AddPeerReferenceToItem(pResourceDictionary));
        pResourceDictionary->UnpegManagedPeerNoRef();

        pResourceDictionary = NULL;

        spObjectWriterStack->Current().get_CompressedStack()->set_Resources(spQO);
    }

Cleanup:
    if (FAILED(hr) && pResourceDictionary != NULL)
    {
        // Something went wrong - unpeg pResourceDictionary so it will not cause a leak.
        pResourceDictionary->UnpegManagedPeerNoRef();
    }

    ReleaseInterface(pResourceDictionary);
    RRETURN(hr);
}

_Check_return_ HRESULT
CDeferredKeys::ResolveAndSaveResource(
    _In_ CCoreServices *pCore,
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
    _In_ const std::shared_ptr<XamlReader>& spReader,
    _In_ CResourceDictionary *pResourceDictionary,
    _Out_ bool* pbResourceFromReadOnlyDictionary)
{
    HRESULT hr = S_OK;

    CDependencyObject *pValueNoRef = NULL;
    CDependencyObjectWrapper *pDOWrapper = NULL;
    *pbResourceFromReadOnlyDictionary = FALSE;
    ASSERT(spServiceProviderContext);

    ASSERT(spReader->CurrentNode().get_NodeType() == XamlNodeType::xntValue);

    xstring_ptr strKey;

    // Get the string from either the CValue or CString
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

    // first look the value up in the cached ResourceDictionary
    xref_ptr<CResourceDictionary> dictionaryReadFrom;
    // first look the value up in the cached ResourceDictionary
    IFC(pResourceDictionary->GetKeyForResourceResolutionNoRef(
        strKey,
        Resources::LookupScope::LocalOnly,
        &pValueNoRef,
        &dictionaryReadFrom));

    if (dictionaryReadFrom)
    {
        *pbResourceFromReadOnlyDictionary = dictionaryReadFrom->IsReadOnlyDictionary();
    }

    if (!pValueNoRef)
    {
        if (m_spKeyMap)
        {
            if (m_spKeyMap->find(strKey) != m_spKeyMap->end())
            {
                // key is part of the deferred keys
                // so we can just skip this resource resolution
                goto Cleanup;
            }
        }

        // second:  if not found in the cached ResourceDictionary look it up in the current context.
        IFC(CStaticResourceExtension::LookupResourceNoRef(strKey, spServiceProviderContext, pCore, &pValueNoRef, FALSE /* bShouldCheckThemeResources */));

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
            IFC(pResourceDictionary->Add(strKey, &vValue, NULL, FALSE /*bKeyIsType*/));

            // [Bug fix: #95299]
            // The DO wrapper prevents the DO resource from being correctly parented to ResourceDictionary so fix that here
            if (pDOWrapper != NULL)
            {
                IFC(pResourceDictionary->AddPeerReferenceToItem(pValueNoRef));
            }
        }

        // if the value isn't found, that's fine.  It's entirely possible that the value was somewhere in a
        // resource dictionary either in the nodestream, or on a property of something in the nodestream.
    }
    else
    {
        // If the value is found, unpeg the peg that happened during GetKey
        pValueNoRef->UnpegManagedPeerNoRef();
    }

    ASSERT(hr != S_FALSE);
    IFC(hr);

Cleanup:
    ReleaseInterface(pDOWrapper);

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Preserve ambient properties.
//
//  Notes:
//
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
CDeferredKeys::SaveAmbientProperties(
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

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Create a node stream reader.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
CDeferredKeys::GetReader(_Out_ std::shared_ptr<XamlReader>& spReader)
{
    if (m_spNodeList && m_spXamlSavedContext)
    {
        IFC_RETURN(m_spNodeList->get_Reader(spReader));
    }
    else
    {
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Synopsis:
//      Create an object writer that can be used to playback the nodestream.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
CDeferredKeys::GetWriter(_In_ const std::shared_ptr<XamlReader>& spReader, _Out_ std::shared_ptr<ObjectWriter>& spObjectWriter)
{
    ObjectWriterSettings objectWriterSettings;
    std::shared_ptr<XamlQualifiedObject>  qoEventRoot;
    std::shared_ptr<XamlSchemaContext>    spSchemaContext;
    xref_ptr<INameScope>              spNameScope;
    xref_ptr<IPALUri>                 spBaseUri;
    auto core = m_pCollection->GetContext();

    auto spErrorService = std::make_shared<ParserErrorService>();
    spErrorService->Initialize(core);

    IFC_RETURN(spReader->GetSchemaContext(spSchemaContext));
    IFC_RETURN(spSchemaContext->SetErrorService(spErrorService));

    IFC_RETURN(XamlQualifiedObject::Create(core, XamlTypeToken(), m_pCollection, qoEventRoot));

    objectWriterSettings.set_NameScope(spNameScope);
    objectWriterSettings.set_EventRoot(qoEventRoot);
    IFC_RETURN(m_spXamlSavedContext->get_BaseUri(spBaseUri));
    objectWriterSettings.set_BaseUri(spBaseUri);

    IFC_RETURN(ObjectWriter::Create(m_spXamlSavedContext, objectWriterSettings, spObjectWriter));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//     This should be called when we are ready to fault in the Nodestream
//
//  Notes:
//
//
//---------------------------------------------------------------------------

_Check_return_ HRESULT
CDeferredKeys::Load(_In_ const xstring_ptr_view& strKey, _Out_ bool *pbFound)
{
    XUINT32 uiKeyIndex = 0;

    *pbFound = FALSE;

    IFC_RETURN(Ensure());

    {
        auto itUiKeyIndex = m_spKeyMap->find(strKey);

        // return value only if key exists and key is within range of MaxAllowedIndexForLookup
        if (itUiKeyIndex == m_spKeyMap->end() ||
            (m_bHasKeyLookupBoundary && (itUiKeyIndex->second >= m_uiMaxAllowedIndexForLookup)))
        {
            return S_OK;
        }

        uiKeyIndex = itUiKeyIndex->second;

        m_spKeyMap->erase(itUiKeyIndex);
        IFC_RETURN(PlayXaml(uiKeyIndex));

        *pbFound = TRUE;
    }

    return S_OK;
}

_Check_return_ HRESULT
CDeferredKeys::PlayXaml(_In_ XUINT32 startIndex)
{
    HRESULT hr = S_OK;
    std::shared_ptr<ObjectWriter> spObjectWriter;
    std::shared_ptr<XamlReader> spReader = m_spReader;
    bool fDone = false;
    XUINT32 uiNodeDepth = 0;
    bool fResetExpandingState = false;
    bool fResetHasKeyLookupBoundary = (m_bHasKeyLookupBoundary == false);
    XUINT32 uiOldMaxAllowedIndexForLookup = m_uiMaxAllowedIndexForLookup;

    //
    // Without deferral, the following Xaml will indicate an error trying to lookup the key.
    //
    // <Page.Resources>
    //     <Rectangle x:Key="rect" Height="300" Width="300"  Fill="{StaticResource Red}" />
    //     <Brush x:Key="Red">Red</Brush>
    // </Page.Resources>
    // <Grid Background="{StaticResource ApplicationPageBackgroundThemeBrush}">
    //     <StaticResource ResourceKey="rect" />
    // </Grid>
    //
    // With deferral, we need to explicitly remember the boundary for a key lookup so that a recursive lookup will only be allowed
    // within the defined boundary of the last key looked up and disallow this case.
    //
    // A key lookup in this deferred dictionary will start at Key 'Rect' through DeferredKeys::Load("Rect"). This in turn will call PlayXaml()
    // which will setup a boundary at the node stream index of start object of the Rectangle in the Resource Dictionary. The
    // boundary is defined by the flag m_bHasKeyLookupBoundary and index m_uiMaxAllowedIndexForLookup.
    //
    // As the node stream is played back, a subsequent key lookup for 'Red' is made. Although Load() will return a node stream index,
    // since m_bHasKeyLookupBoundary is TRUE, this node stream index will not fall in the range of m_uiMaxAllowedIndexForLookup and
    // and hence no result will be returned.
    //
    m_uiMaxAllowedIndexForLookup = startIndex;
    m_bHasKeyLookupBoundary = true;

    if (m_bExpandingKey)
    {
        IFC(GetReader(spReader));
    }

    IFC(GetWriter(spReader, spObjectWriter));

    if (!m_bExpandingKey)
    {
        fResetExpandingState = true;
        m_bExpandingKey = true;
        spObjectWriter->EnableResourceDictionaryDefer();
    }

    // position the node list reader
    IFC(spReader->set_NextIndex(startIndex));

    // Playback Xaml using ObjectWriter
    while ((hr = spReader->Read()) == S_OK)
    {
        if (spReader->CurrentNode().get_NodeType() == XamlNodeType::xntStartObject)
        {
            uiNodeDepth++;
        }
        else if (spReader->CurrentNode().get_NodeType() == XamlNodeType::xntEndObject)
        {
            uiNodeDepth--;
            if (uiNodeDepth == 0)
            {
                bool bAllowItems = static_cast<CResourceDictionary*>(m_pCollection)->m_bAllowItems;

                // if this is the end object, we want to force the attachment to the
                // resource dictionary. to be able to to do this, setup the
                // parent instance (since we had previously cleared the instances when
                // we saved the context) before we perform the assignment to parent
                // collection through the parser.

                XamlTypeToken tokCollection(tpkNative, m_pCollection->GetTypeIndex());
                std::shared_ptr<XamlQualifiedObject> qoCollection;

                IFC(XamlQualifiedObject::Create(m_pCollection->GetContext(), tokCollection, m_pCollection, qoCollection));

                if (!bAllowItems)
                {
                    static_cast<CResourceDictionary*>(m_pCollection)->m_bAllowItems = TRUE;
                }

                IFC(spObjectWriter->ForceAssignmentToParentCollection(qoCollection));

                if (!bAllowItems)
                {
                    static_cast<CResourceDictionary*>(m_pCollection)->m_bAllowItems = FALSE;
                }
                fDone = TRUE;
            }
        }

        IFC(spObjectWriter->WriteNode(spReader->CurrentNode()));
        if (fDone) break;
    }

Cleanup:
    if (fResetExpandingState)
    {
        m_bExpandingKey = false;
    }

    if (fResetHasKeyLookupBoundary)
    {
        // top most boundary, when we unwind there are no lookup boundaries anymore.
        m_bHasKeyLookupBoundary = false;
        m_uiMaxAllowedIndexForLookup = 0;
    }
    else
    {
        // unwind one level on the stack of lookup boundaries.
        ASSERT(m_bHasKeyLookupBoundary);

        // restore the original lookup boundary value
        m_uiMaxAllowedIndexForLookup = uiOldMaxAllowedIndexForLookup;
    }

    RRETURN(hr);
}

_Check_return_ HRESULT
CDeferredKeys::Ensure()
{
    // ensure default reader
    if (!m_spReader)
    {
        IFC_RETURN(GetReader(m_spReader));
    }

    // ensure key map table
    if (!m_spKeyMap)
    {
        std::shared_ptr<XamlServiceProviderContext> spServiceProviderContext;
        IFC_RETURN(PreResolve(TRUE /* fResolveItems */, FALSE /* fResolveResources */, spServiceProviderContext));
    }

    // prevent reentrancy
    if (!m_bExpandingKey)
    {
        if (m_spNonDeferredKeys)
        {
            for (XUINT32 i=0; i<m_spNonDeferredKeys->size(); i++)
            {
                XUINT32 value = 0;
                IFC_RETURN(m_spNonDeferredKeys->get_item(i, value));

                IFC_RETURN(PlayXaml(value));
            }

            m_spNonDeferredKeys->clear();
            m_spNonDeferredKeys.reset();
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CDeferredKeys::EnsureAll()
{
    IFC_RETURN(Ensure());

    if (m_spKeyMap)
    {
        // While undeferring each item, we could undefer trailing items as
        // resource references are resolved, which will modify the map.
        for (auto it = m_spKeyMap->begin(); it != m_spKeyMap->end(); ++it)
        {
            VERIFYHR(PlayXaml(it->second));
        }
        m_spKeyMap.reset();
    }
    return S_OK;
}

_Check_return_ HRESULT
CDeferredKeys::PlayNonDeferredContent(_In_ ObjectWriter *pObjectWriter)
{
    HRESULT hr = S_OK;
    std::shared_ptr<XamlReader> spReader;
    XUINT32 uiNodeDepth = 0;
    bool fDone = false;

    ASSERT(!m_bExpandingKey);
    ASSERT(!m_bResolvingResources);
    ASSERT(!m_bExpandingNonDeferredContent);

    IFC(GetReader(spReader));

    if (m_spNonDeferredKeys)
    {
        std::shared_ptr< xvector< XUINT32 > > spNonDeferredKeys = m_spNonDeferredKeys;
        m_spNonDeferredKeys.reset();

        m_bExpandingNonDeferredContent = true;
        for (XUINT32 i=0; i<spNonDeferredKeys->size(); i++)
        {
            XUINT32 startIndex = 0;
            IFC(spNonDeferredKeys->get_item(i, startIndex));

            // position the node list reader
            IFC(spReader->set_NextIndex(startIndex));

            // setup a lookup boundary starting at this node stream index
            m_bHasKeyLookupBoundary = true;
            m_uiMaxAllowedIndexForLookup = startIndex;

            fDone = FALSE;
            // Playback Xaml using ObjectWriter
            while ((hr = spReader->Read()) == S_OK)
            {
                if (spReader->CurrentNode().get_NodeType() == XamlNodeType::xntStartObject)
                {
                    uiNodeDepth++;
                }
                else if (spReader->CurrentNode().get_NodeType() == XamlNodeType::xntEndObject)
                {
                    uiNodeDepth--;
                    if (uiNodeDepth == 0) fDone = TRUE;
                }

                IFC(pObjectWriter->WriteNode(spReader->CurrentNode()));
                if (fDone) break;
            }
        }

        spNonDeferredKeys->clear();
    }

Cleanup:
    // no more lookup boundaries
    m_bHasKeyLookupBoundary = false;
    m_uiMaxAllowedIndexForLookup = 0;
    m_bExpandingNonDeferredContent = false;
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//     Discard instance references in the object writer stack to prevent
//     circular references.
//
//---------------------------------------------------------------------------

_Check_return_ HRESULT
CDeferredKeys::RemoveObjectReferencesFromStack()
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


