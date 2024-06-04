// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "BinaryFormatObjectWriter.h"
#include "CVisualStateManager2.h"
#include "DependencyLocator.h"
#include "NodeStreamCache.h"
#include "ObjectWriter.h"
#include "ObjectWriterNodeList.h"
#include "XamlParser.h"
#include "ParserAPI.h"
#include "ParserSettings.h"
#include "RuntimeEnabledFeatures.h"
#include "XamlBinaryFormatReader.h"
#include "XamlBinaryFormatReader2.h"
#include "XamlBinaryFormatSubReader2.h"
#include "XbfWriter.h"
#include "DesignMode.h"

using namespace RuntimeFeatureBehavior;

_Check_return_ HRESULT
CParser::LoadXaml(
    _In_ CCoreServices *pCore,
    _In_ const CParserSettings& parserSettings,
    _In_ const Parser::XamlBuffer& buffer,
    _Outptr_ CDependencyObject **ppDependencyObject,
    _In_ const xstring_ptr_view& strSourceAssemblyName,
    _In_ const std::array<byte, Parser::c_xbfHashSize>& hashForBinaryXaml
)
{
    RRETURN(LoadXamlCore(pCore,
            parserSettings,
            buffer,
            ppDependencyObject,
            strSourceAssemblyName,
            hashForBinaryXaml));
}


//------------------------------------------------------------------------
//
//  Method:   LoadXamlCore
//
//  Synopsis:
//         Parses the xaml from pSource and returns an instance
//         of a xamlObject reprtesented by its root.
//
//         When error occurs, it saves error informattion into Core's ErroInfo
//         instance,  the method then returns failure, The caller of LoadXaml
//         can then trigger the error report.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CParser::LoadXamlCore(
    _In_ CCoreServices *pCore,
    _In_ const CParserSettings& parserSettings,
    _In_ const Parser::XamlBuffer& buffer,
    _Outptr_ CDependencyObject **ppDependencyObject,
    _In_ const xstring_ptr_view& strSourceAssemblyName,
    _In_ const std::array<byte, Parser::c_xbfHashSize>& hashForBinaryXaml
)
{
    auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
    const bool shouldEnforceXbfV2 = runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeEnabledFeature::EnforceXbfV2Stream) || DesignerInterop::GetDesignerMode(DesignerMode::V2Only);

    xref_ptr<INameScope> spNameScope;

    xref_ptr<IPALUri> spXamlResourceUri;
    spXamlResourceUri.reset(parserSettings.get_XamlResourceUri());

    xref_ptr<IPALUri> spBaseUri;
    auto pFrameworkRoot = static_cast<CDependencyObject*>(parserSettings.get_ExistingFrameworkRoot());
    if( pFrameworkRoot )
    {
        spBaseUri.init(pFrameworkRoot->GetBaseUri());

    }

    IFC_RETURN(CreateNamescope( pCore, pFrameworkRoot, parserSettings, spNameScope));

    // Store the namescope, and restore the previous value when we're done.  This is necessary for compatibility
    // in CDependencyObject::SetName, so that it can service a put_Name by the app during parse.
    auto pPreviousNamescope = pCore->SetParserNamescope(spNameScope);
    auto previousNamescopeGuard = wil::scope_exit([&] {
        pCore->SetParserNamescope(pPreviousNamescope);
    });

    std::shared_ptr<XamlSchemaContext> spSchemaContext = pCore->GetSchemaContext();

    xstring_ptr ssSourceAssemblyName;
    if (!strSourceAssemblyName.IsNullOrEmpty())
    {
        IFC_RETURN(strSourceAssemblyName.Promote(&ssSourceAssemblyName));
    }
    else
    {
        if (parserSettings.get_ExistingFrameworkRoot())
        {
            ssSourceAssemblyName = spSchemaContext->GetDefaultAssemblyName();
            IFCEXPECT_RETURN(!ssSourceAssemblyName.IsNullOrEmpty());
        }
    }

    // TODO: This isn't ideal that we are keeping this context on the stack
    // but we have no easy access back to the parser context from the type providers.
    IFC_RETURN(spSchemaContext->PushSourceAssembly(ssSourceAssemblyName));
    auto sourceAssemblyGuard = wil::scope_exit([&] {
        spSchemaContext->PopSourceAssembly();
    });

    std::shared_ptr<ParserErrorService> spErrorService = std::make_shared<ParserErrorService>();
    spErrorService->Initialize(pCore);
    IFC_RETURN(spSchemaContext->SetErrorService(spErrorService));

    std::shared_ptr<XamlNodeStreamCacheManager> spNodeStreamCacheManager;
    IFC_RETURN(pCore->GetXamlNodeStreamCacheManager(spNodeStreamCacheManager));

    std::shared_ptr<XamlReader> spXamlReader;
    std::shared_ptr<XamlBinaryFormatReader2> spVersion2Reader;
    bool bBinaryXamlLoaded;
    if (!buffer.IsBinary())
    {
        // When a LoadXamlCore is passed a logical resource, the physical resource URI (Xaml Resource URI)
        // will be different from the base URI (requested resource URI.) If specified, the physical resource
        // URI (Xaml Resource URI), not requested resource URI (base URI), should be used as the cache key.
        xref_ptr<IPALUri> spCacheLookupUri;
        if (parserSettings.get_UseXamlResourceUri())
        {
            spCacheLookupUri = spXamlResourceUri;
        }
        else
        {
            // If we have a Xaml resource URI (e.g. themes\generic.xaml) and couldn't determine a base URI,
            // use that instead
            if (spBaseUri)
            {
                spCacheLookupUri = spBaseUri;
            }
            else
            {
                spCacheLookupUri = spXamlResourceUri;
            }
        }

        // get a unique name
        xstring_ptr spUniqueName;
        if (spCacheLookupUri)
        {
            xref_ptr<IPALResourceManager> spResourceManager;
            bool canCache = false;

            IFC_RETURN(pCore->GetResourceManager(spResourceManager.ReleaseAndGetAddressOf()));
            IFC_RETURN(spResourceManager->CanCacheResource(spCacheLookupUri, &canCache));

            if (canCache)
            {
                IFC_RETURN(GetUniqueName(spCacheLookupUri, &spUniqueName));
            }
        }

        XamlTextReaderSettings readerSettings(parserSettings.get_RequireDefaultNamespace(), true /* shouldProcessUid */, parserSettings.get_IsUtf16Encoded());
        IFC_RETURN(spNodeStreamCacheManager->GetXamlReader(spSchemaContext, spUniqueName, buffer.m_count, buffer.m_buffer, readerSettings, spXamlReader, spVersion2Reader, &bBinaryXamlLoaded));
    }
    else
    {
        IFCPTR_RETURN(buffer.m_buffer);

        xref_ptr<IPALMemory> spBuffer;
        IFC_RETURN(gps->CreatePALMemoryFromBuffer(buffer.m_count, const_cast<XUINT8*>(buffer.m_buffer), false /*ownsBuffer*/, spBuffer.ReleaseAndGetAddressOf()));

        bBinaryXamlLoaded = false;
        IFC_RETURN(spNodeStreamCacheManager->CreateXamlReaderFromBinaryBuffer(spBuffer, buffer.m_bufferType, spSchemaContext, spXamlReader, spVersion2Reader));
    }

    ASSERT((spXamlReader && !spVersion2Reader) || (!spXamlReader && spVersion2Reader));

    std::shared_ptr<XamlQualifiedObject> qoResult;
    if (spXamlReader)
    {
        std::shared_ptr<ObjectWriter> spObjectWriter;
        std::shared_ptr<BinaryFormatObjectWriter> spBinaryFormatObjectWriter;
        IFC_RETURN(CreateObjectWriter(spSchemaContext,
            parserSettings,
            shouldEnforceXbfV2, /* enable encoding */
            !bBinaryXamlLoaded /* No Duplicate Property Check for XBF */,
            spBaseUri,
            spNameScope,
            spObjectWriter,
            spBinaryFormatObjectWriter));

        {
            HRESULT hr;
            while (S_OK == (hr = spXamlReader->Read()))
            {
                IFC_RETURN(spObjectWriter->WriteNode(spXamlReader->CurrentNode()));
            }
            IFC_RETURN(hr);
        }

        auto spObjectNodeList = spObjectWriter->GetNodeList();
        ASSERT(!shouldEnforceXbfV2 == !spObjectNodeList);

        // If this is a v1 XBF, let's read it and transform it into a V2 XBF in memory
        if (shouldEnforceXbfV2 && spObjectNodeList)
        {
            IFC_RETURN(spObjectNodeList->Optimize());

            std::shared_ptr<XbfWriter> spXbfWriter = std::make_shared<XbfWriter>(pCore, Parser::Versioning::OSVersions::Latest());

            xref_ptr<IPALMemory> spBuffer;
            XUINT32 bufferSize = 0;
            {
                std::array<byte, Parser::c_xbfHashSize> xbfEmptyHash = { 0 };
                XBYTE* tempBuffer = nullptr;
                IFC_RETURN(spXbfWriter->GetOptimizedBinaryEncodingFromReader(spXamlReader, spObjectNodeList, xbfEmptyHash, true, &bufferSize, &tempBuffer));
                IFC_RETURN(gps->CreatePALMemoryFromBuffer(bufferSize, tempBuffer, true /*ownsBuffer*/, spBuffer.ReleaseAndGetAddressOf()));
            }
            IFC_RETURN(spNodeStreamCacheManager->CreateXamlReaderFromBinaryBuffer(spBuffer, Parser::XamlBufferType::Binary, spSchemaContext, spXamlReader, spVersion2Reader));

            std::shared_ptr<XamlBinaryFormatSubReader2> spXbfReader;
            IFC_RETURN(spVersion2Reader->GetSubReader(0, spXbfReader));

            {
                ObjectWriterNode node;
                while (true)
                {
                    bool endOfStream = false;
                    IFC_RETURN(spXbfReader->TryReadHRESULT(node, &endOfStream));
                    if (endOfStream) break;

                    IFC_RETURN(spBinaryFormatObjectWriter->WriteNode(node));
                }
            }

#if DBG
            //LOG(L"***Dumping Tree****");
            //IFC_RETURN(spObjectNodeList->Dump());
#endif
            qoResult = spBinaryFormatObjectWriter->get_Result();
        }
        else
        {
            // Just go through the old conversion
            qoResult = spObjectWriter->get_Result();
        }
    }
    else
    {
        std::shared_ptr<BinaryFormatObjectWriter> spBinaryFormatObjectWriter;
        IFC_RETURN(CreateBinaryFormatObjectWriter(spSchemaContext,
            parserSettings,
            !bBinaryXamlLoaded /* No Duplicate Property Check for XBF */,
            spBaseUri,
            spNameScope,
            spVersion2Reader->GetXbfHash(),
            spBinaryFormatObjectWriter));

        std::shared_ptr<XamlBinaryFormatSubReader2> spXbfReader;
        IFC_RETURN(spVersion2Reader->GetSubReader(0, spXbfReader));

        {
            ObjectWriterNode node;
            for (;;)
            {
                bool endOfStream = false;
                IFC_RETURN(spXbfReader->TryReadHRESULT(node, &endOfStream));
                if (endOfStream) break;

                IFC_RETURN(spBinaryFormatObjectWriter->WriteNode(node));
            }
        }

        qoResult = spBinaryFormatObjectWriter->get_Result();
    }

    ASSERT(!!qoResult);
    *ppDependencyObject = qoResult->GetDependencyObject();

    // Need to addref it if you want to keep it past the lifetime of qoResult
    AddRefInterface(*ppDependencyObject);

    // We need to leave the root pegged until it's returned to the managed caller.
    qoResult->ClearHasPeggedManagedPeer();

    // If this is a UserControl, initialize StateTriggers
    CUserControl* pUserControl = do_pointer_cast<CUserControl>(*ppDependencyObject);
    if (pUserControl)
    {
        IFC_RETURN(CVisualStateManager2::InitializeStateTriggers(pUserControl));
    }

    return S_OK;
}



// Create a namescope for LoadXamlCore
_Check_return_ HRESULT
CParser::CreateNamescope(
    _In_ CCoreServices *pCore,
    _In_ CDependencyObject *pFrameworkRoot,
    _In_ const CParserSettings& parserSettings,
    _Out_ xref_ptr<INameScope>& spNameScope
)
{

    // Create a name scope if requested (defaults to true)
    CDependencyObject* pNamescopeOwner = NULL;
    if( parserSettings.get_CreateNamescope() )
    {
        // If we have an existing framework root, use that as the namescope owner.
        // Otherwise, use the helper, which will establish the namescope owner once the
        // parser has created it.

        if( pFrameworkRoot )
        {
            pCore->GetNameScopeRoot().EnsureNameScope(pFrameworkRoot, nullptr);
            pFrameworkRoot->SetIsStandardNameScopeOwner(TRUE);
            pFrameworkRoot->SetIsStandardNameScopeMember(TRUE);

            pNamescopeOwner = pFrameworkRoot;

            // The existing framework root might have already set some names.  (For example, set some named elements
            // in ts content before making the call to InitializeComponent).  Pick those names up before we add more.
            IFC_RETURN(pCore->PostParseRegisterNames(pFrameworkRoot));

        }
    }
    else
    {
        ASSERT( pFrameworkRoot );
        pNamescopeOwner = pFrameworkRoot->GetStandardNameScopeOwner();
    }


    // Create a name scope wrapper whether or not we have an existing root.  If haven't set pNamescopeOwner yet,
    // we'll determine the owner later (and create a core namescope).
    spNameScope = make_xref<NameScopeHelper>(pCore, pNamescopeOwner, Jupiter::NameScoping::NameScopeType::StandardNameScope);

    return S_OK;

}

_Check_return_ HRESULT
CreateObjectWriterSettings(
    _In_ const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
    _In_ const CParserSettings& parserSettings,
    _In_ bool fEnableEncoding,
    _In_ bool fCheckDuplicateProperty,
    _In_ const xref_ptr<IPALUri>& spBaseUri,
    _In_ const xref_ptr<INameScope> spNameScope,
    _In_ const xstring_ptr& strHash,
    _Out_ ObjectWriterSettings& objectWriterSettings
)
{
    objectWriterSettings.set_ExpandTemplates(parserSettings.get_ExpandTemplatesDuringParse());
    objectWriterSettings.set_CheckDuplicateProperty(fCheckDuplicateProperty);
    objectWriterSettings.set_BaseUri(spBaseUri);
    objectWriterSettings.set_EnableEncoding(fEnableEncoding);
    objectWriterSettings.set_XamlResourceUri(xref_ptr<IPALUri>(parserSettings.get_XamlResourceUri()));
    objectWriterSettings.set_NameScope(spNameScope);
    objectWriterSettings.set_XbfHash(strHash);

    // Setup the root object - used for managed apps x:Class
    if (parserSettings.get_ExistingFrameworkRoot())
    {
        auto rootQO = std::make_shared<XamlQualifiedObject>();

        IFC_RETURN(rootQO->SetDependencyObject(static_cast<CDependencyObject*>(parserSettings.get_ExistingFrameworkRoot())));
        objectWriterSettings.set_RootObjectInstance(rootQO);
        objectWriterSettings.set_XBindParentConnector(rootQO);
        objectWriterSettings.set_EventRoot(rootQO);
    }

    return S_OK;
}

_Check_return_ HRESULT
CParser::CreateObjectWriter(
    _In_ const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
    _In_ const CParserSettings& parserSettings,
    _In_ bool fEnableEncoding,
    _In_ bool fCheckDuplicateProperty,
    _In_ const xref_ptr<IPALUri>& spBaseUri,
    _In_ xref_ptr<INameScope> spNameScope,
    _Out_ std::shared_ptr<ObjectWriter>& spObjectWriter,
    _Out_ std::shared_ptr<BinaryFormatObjectWriter>& spBinaryFormatObjectWriter
    )
{
    ObjectWriterSettings objectWriterSettings;
    IFC_RETURN(CreateObjectWriterSettings(spSchemaContext, parserSettings, fEnableEncoding, fCheckDuplicateProperty, spBaseUri, spNameScope, xstring_ptr() /* no hash for XBFv1 */, objectWriterSettings));

    IFC_RETURN(ObjectWriter::Create(spSchemaContext, objectWriterSettings, spObjectWriter));
    if (fEnableEncoding)
    {
        IFC_RETURN(BinaryFormatObjectWriter::Create(spSchemaContext, objectWriterSettings, spBinaryFormatObjectWriter));
    }

    return S_OK;

}

_Check_return_ HRESULT
CParser::CreateBinaryFormatObjectWriter(
    _In_ const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
    _In_ const CParserSettings& parserSettings,
    _In_ bool fCheckDuplicateProperty,
    _In_ const xref_ptr<IPALUri>& spBaseUri,
    _In_ xref_ptr<INameScope> spNameScope,
    _In_ const xstring_ptr& strHash,
    _Out_ std::shared_ptr<BinaryFormatObjectWriter>& spBinaryFormatObjectWriter
)
{
    ObjectWriterSettings objectWriterSettings;
    IFC_RETURN(CreateObjectWriterSettings(spSchemaContext, parserSettings, true, fCheckDuplicateProperty, spBaseUri, spNameScope, strHash, objectWriterSettings));

    IFC_RETURN(BinaryFormatObjectWriter::Create(spSchemaContext, objectWriterSettings, spBinaryFormatObjectWriter));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Gets a name that can be used to uniquely identify the BaseUri such that there
//      wouldn't be any false positive matches or false negatives.
//
//---------------------------------------------------------------------------
_Check_return_
HRESULT CParser::GetUniqueName(
    _In_ const xref_ptr<IPALUri>& spBaseUri,
    _Out_ xstring_ptr* pstrUniqueName
    )
{
    IFC_RETURN(spBaseUri->GetCanonical(pstrUniqueName));

    return S_OK;
}


