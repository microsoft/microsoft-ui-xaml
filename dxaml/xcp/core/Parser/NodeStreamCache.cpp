// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "NodeStreamCache.h"
#include "XamlBinaryFormatReader.h"
#include "XamlBinaryFormatReader2.h"
#include "XamlBinaryMetadataReader.h"
#include "XamlBinaryFormatValidator.h"
#include "XamlBinaryFileAccessFactories.h"
#include <MsResourceHelpers.h>
#include <ParserAPI.h>
#include <winuri.h>

using namespace Parser;

static const WCHAR FILE_EXTENSION_XBF[] = L".xbf";

// Set the XamlOptimizedNodeList that will be used from now on to provide
// XamlReader instances for this CacheEntry.
_Check_return_ HRESULT
XamlNodeStreamCacheManager::NodeStreamCacheEntry::SetNodeList(
    const std::shared_ptr<XamlOptimizedNodeList>& spXamlOptimizedNodeList)
{
    IFCEXPECT_ASSERT_RETURN(!m_spNodeList);
    m_spNodeList = spXamlOptimizedNodeList;
    return S_OK;
}

// Gets a XamlReader from the NodeList.  The XamlReader is then used in
// place of the XamlTextReader to feed XamlNodes to ObjectWriter.
_Check_return_ HRESULT
XamlNodeStreamCacheManager::NodeStreamCacheEntry::GetXamlReader(
    _Out_ std::shared_ptr<XamlReader>& spXamlReader)
{
    IFCEXPECT_RETURN(m_spNodeList);
    IFC_RETURN(m_spNodeList->get_Reader(spXamlReader));
    IncrementAccessCount();
    return S_OK;
}

bool
XamlNodeStreamCacheManager::NodeStreamCacheEntry::HasNodeList() const
{
    return !!m_spNodeList;
}

UINT32
XamlNodeStreamCacheManager::NodeStreamCacheEntry::GetAccessCount() const
{
    return m_uiAccessCount;
}

void
XamlNodeStreamCacheManager::NodeStreamCacheEntry::IncrementAccessCount()
{
    if (m_uiAccessCount < UINT32_MAX)
    {
        m_uiAccessCount++;
    }
}

_Check_return_ HRESULT
XamlNodeStreamCacheManager::Create(
    _In_ IParserCoreServices* pCore,
    _Out_ std::shared_ptr<XamlNodeStreamCacheManager>& spXamlNodeStreamCacheManager)
{
    spXamlNodeStreamCacheManager = std::make_shared<XamlNodeStreamCacheManager>(pCore);
    return S_OK;
}

// Clears cached data, such as the map of Uris to NodeLists and the XBF
// lookup cache. This is done:
// - In response to an event that casts into doubt whether any of the
//   cached NodeLists are current.
// - When the scale changes, so that if the application subsequently
//   parses XAML, we will re-resolve resources from MRT, and
//   potentially pick up new resources for the new scale.
void XamlNodeStreamCacheManager::Flush()
{
    m_UriToNodelistMap.clear();

    m_UriToXbfResourceMap.clear();
}

// Clears the XBFv2 reader cache. This is distinct from Flush()
// because we only want to do this if metadata was reset (which leads
// to our cached readers holding onto stale type information).
_Check_return_ HRESULT 
XamlNodeStreamCacheManager::ResetCachedXbfV2Readers()
{
    for (auto& kvp : m_XBFv2ReaderCache)
    {
        // We don't want to forget entirely about the cached readers
        // because they own string storage buffers, and there is no guarantee
        // that none of the xstring_ptrs being backed by the buffers are still
        // in use.
        m_staleXBFv2Readers.push_back(kvp.second);
    }
    m_XBFv2ReaderCache.clear();

    return S_OK;
}

// Checks whether we currently have a cached nodelist for the Uri.
// This is a different question from whether we have a NodeStreamCacheEntry.
// It is possible that we have a NodeStreamCacheEntry for tracking purposes,
//  but have never actually cached the NodeList.
_Check_return_ HRESULT
XamlNodeStreamCacheManager::HasCacheForUri(
    _In_ const xstring_ptr& strUri,
    _Out_ bool* pfHasCachedNodelist)
{
    std::shared_ptr<NodeStreamCacheEntry> spNodeStreamCacheEntry;
    *pfHasCachedNodelist = false;

    IFC_RETURN(EnsureCacheEntry(strUri, spNodeStreamCacheEntry));

    *pfHasCachedNodelist = spNodeStreamCacheEntry->HasNodeList();

    return S_OK;
}

_Check_return_ HRESULT
XamlNodeStreamCacheManager::GetBinaryResourceForXamlUri(_In_ const xstring_ptr& strUri, _Out_ xref_ptr<IPALResource>& spXbfResourceOut)
{
    xref_ptr<IPALResource> spXbfResource;
    xref_ptr<IPALResourceManager> spResourceManager;
    xref_ptr<IPALUri> pXbfUri;
    xstring_ptr strScheme;
    xstring_ptr strPhysicalUri;
    bool isNewResource = false;

    spXbfResourceOut = nullptr;

    auto itXbfResource = m_UriToXbfResourceMap.find(strUri);
    if (itXbfResource == m_UriToXbfResourceMap.end())
    {
        if (strUri.GetCount() > 0)
        {
            auto uiDotExtensionIndex = strUri.FindLastChar(L'.');
            if (xstring_ptr_view::npos == uiDotExtensionIndex)
            {
                // if we don't find a '.' then simply append the 'xbf' extension for lookup.
                uiDotExtensionIndex = strUri.GetCount();
            }

            XStringBuilder xbfUriBuilder;

            IFC_RETURN(xbfUriBuilder.Initialize(uiDotExtensionIndex + SZ_COUNT(FILE_EXTENSION_XBF)));
            IFC_RETURN(xbfUriBuilder.Append(strUri.GetBuffer(), uiDotExtensionIndex));
            IFC_RETURN(xbfUriBuilder.Append(STR_LEN_PAIR(FILE_EXTENSION_XBF)));
            IFC_RETURN(CWinUriFactory::Create(xbfUriBuilder.GetCount(), xbfUriBuilder.GetBuffer(), pXbfUri.ReleaseAndGetAddressOf()));

            // Limit XBF lookup only for ms-resource and ms-appx schemes
            IFC_RETURN(UriXStringGetters::GetScheme(pXbfUri, &strScheme));
            if (strScheme.Equals(MsUriHelpers::GetAppxScheme(), xstrCompareCaseInsensitive) ||
                strScheme.Equals(MsUriHelpers::GetResourceScheme(), xstrCompareCaseInsensitive))
            {
                IFC_RETURN(m_pCore->GetResourceManager(spResourceManager.ReleaseAndGetAddressOf()));
                IFC_RETURN(spResourceManager->TryGetLocalResource(pXbfUri, spXbfResource.ReleaseAndGetAddressOf()));

                if (spXbfResource)
                {
                    // The resource exists.
                    IFC_RETURN(spXbfResource->GetPhysicalResourceUriNoRef()->GetCanonical(&strPhysicalUri));
                }
            }
        }

        if (strPhysicalUri.GetCount() > 0)
        {
            // Now we have a qualified URI. Check if we've already created the resource.
            // Use the existing one if possible so we don't load duplicates or add them
            // to long term storage (see m_XbfResourceStorage below).

            auto itExistingResource = m_UriToXbfResourceMap.find(strPhysicalUri);
            if (itExistingResource != m_UriToXbfResourceMap.end())
            {
                spXbfResource.reset();
                spXbfResource = itExistingResource->second;
            }
            else
            {
                m_UriToXbfResourceMap.insert({ strPhysicalUri, spXbfResource });
                isNewResource = true;
            }
        }
        else
        {
            m_UriToXbfResourceMap.insert({ strUri, spXbfResource });
            isNewResource = true;
        }
    }
    else{
        spXbfResource = itXbfResource->second;
    }

    if (spXbfResource)
    {
        // Add new resource to long term storage so it and its mapped file/stream
        // memory remain valid as long as this manager, even following a Flush().
        if (isNewResource)
        {
            m_XbfResourceStorage.emplace_back(spXbfResource);
        }

        spXbfResourceOut = spXbfResource;
    }

    return S_OK;
}


// Returns a XamlReader for a Binary Xaml Uri.
_Check_return_ HRESULT
XamlNodeStreamCacheManager::GetXamlBinaryReader(
    _In_ std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext,
    _In_ const xstring_ptr& strUniqueName,
    _In_ std::shared_ptr<NodeStreamCacheEntry> spNodeStreamCacheEntry,
    _Out_ std::shared_ptr<XamlReader>& spXamlReader,
    _Out_ std::shared_ptr<XamlBinaryFormatReader2>& spVersion2Reader)
{
    xref_ptr<IPALResource> spXbfResource;
    xref_ptr<IPALMemory> spXBFBuffer;

    // Get resource, load XBF buffer.
    IFC_RETURN(GetBinaryResourceForXamlUri(strUniqueName, spXbfResource));

    if (spXbfResource)
    {
        IFC_RETURN(spXbfResource->Load(spXBFBuffer.ReleaseAndGetAddressOf()));
    }

    // Create reader from XBF buffer.
    if (spXBFBuffer)
    {
        IFC_RETURN(CreateXamlReaderFromBinaryBuffer(spXBFBuffer,
                                             Parser::XamlBufferType::MemoryMappedResource,
                                             spXamlSchemaContext,
                                             spXamlReader,
                                             spVersion2Reader));
        if (spXamlReader)
        {
            std::shared_ptr<XamlWriter> spNodeListWriter;
            auto spNodeList = std::make_shared<XamlOptimizedNodeList>(spXamlSchemaContext);
            IFC_RETURN(spNodeList->get_Writer(spNodeListWriter));

            HRESULT hr;
            while ((hr = spXamlReader->Read()) == S_OK)
            {
                IFC_RETURN(spNodeListWriter->WriteNode(spXamlReader->CurrentNode()))
            }

            IFC_RETURN(hr);

            TRACE(TraceAlways, L"XBFPARSER: Loaded XBF: %s", strUniqueName.GetBuffer());

            IFC_RETURN(spNodeListWriter->Close());
            IFC_RETURN(spNodeList->get_Reader(spXamlReader));

            // cache the node list if required.
            if (spNodeStreamCacheEntry)
            {
                IFC_RETURN(spNodeStreamCacheEntry->SetNodeList(spNodeList));
            }
        }
    }

    return S_OK;
}

// Returns a XamlReader for a Text Xaml Uri.
_Check_return_ HRESULT
XamlNodeStreamCacheManager::GetXamlTextReader(
    _In_ std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext,
    _In_ const XamlTextReaderSettings& textReaderSettings,
    _In_ UINT32 cSource,
    _In_reads_bytes_(cSource) const UINT8* pSource,
    _In_ std::shared_ptr<NodeStreamCacheEntry> spNodeStreamCacheEntry,
    _Out_ std::shared_ptr<XamlReader>& spXamlReader)
{
    std::shared_ptr<XamlTextReader> spXamlTextReader;

    IFC_RETURN(XamlTextReader::Create(
                               spXamlSchemaContext,
                               textReaderSettings,
                               cSource,
                               pSource,
                               spXamlTextReader));

    if (!spNodeStreamCacheEntry)
    {
        spXamlReader = std::static_pointer_cast<XamlReader>(spXamlTextReader);
    }
    else
    {
        std::shared_ptr<XamlWriter> spNodeListWriter;
        auto spNodeList = std::make_shared<XamlOptimizedNodeList>(spXamlSchemaContext);
        IFC_RETURN(spNodeList->get_Writer(spNodeListWriter));

        HRESULT hr;
        while ((hr = spXamlTextReader->Read()) == S_OK)
        {
            IFC_RETURN(spNodeListWriter->WriteNode(spXamlTextReader->CurrentNode()));
        }

        IFC_RETURN(hr);
        hr = S_OK;

        IFC_RETURN(spNodeListWriter->Close());
        IFC_RETURN(spNodeList->get_Reader(spXamlReader));
        IFC_RETURN(spNodeStreamCacheEntry->SetNodeList(spNodeList));
    }

    return S_OK;
}

// Return TRUE if there is buffer content and buffer content is not L" "
bool IsValidXamlTextBufferContent(_In_reads_bytes_(cSource) const UINT8* pSource, _In_ UINT32 cSource)
{
    if (!pSource || cSource <= 2)
    {
        return false;
    }

    return true;
}

// Get the XamlReader for a Uri.
// - If we have no cache for this Uri, and we have no intention
//   of caching it, then return:
//   - In Design Mode, if no Text Xaml is present, probe for Xbf and return a reader.
//   - In all other cases return a XamlTextReader for the text xaml.
// - If we have the NodeStream cached for the Uri, then we will return a XamlReader
//   from that XamlOptimizedNodeList.
// - If we have the XamlBinaryFormat for the Uri, then we will return a
//   XamlBinaryFormatReader from that.
// - If we don't have the NodeStream cached for the Uri, and there is no
//   XamlBinaryFormat, but it is a Uri for which we should cache the node-stream,
//   then we'll read from the XamlTextReader into a  XamlOptimizedNodeList, and then
//   return a XamlReader from that XamlOptimizedNodeList.
_Check_return_ HRESULT
XamlNodeStreamCacheManager::GetXamlReader(
    _In_ std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext,
    _In_ const xstring_ptr& strUniqueName,
    _In_ UINT32 cSource,
    _In_reads_bytes_(cSource) const UINT8* pSource,
    _In_ const XamlTextReaderSettings& textReaderSettings,
    _Out_ std::shared_ptr<XamlReader>& spXamlReader,
    _Out_ std::shared_ptr<XamlBinaryFormatReader2>& spVersion2Reader,
    _Out_ bool* pfBinaryXamlLoaded)
{
    std::shared_ptr<NodeStreamCacheEntry> spNodeStreamCacheEntry;

    *pfBinaryXamlLoaded = false;

    if (strUniqueName.IsNullOrEmpty())
    {
        //
        //  If there's no Uri (which is the basis for the key in the cache),
        //  parse the text directly and return that.
        //
        IFC_RETURN(GetXamlTextReader(spXamlSchemaContext, textReaderSettings, cSource, pSource, spNodeStreamCacheEntry, spXamlReader));
    }
    else
    {
        IFC_RETURN(EnsureCacheEntry(strUniqueName, spNodeStreamCacheEntry));

        // if the entry is cached, return the cached node list
        if (spNodeStreamCacheEntry->HasNodeList())
        {
            IFC_RETURN(spNodeStreamCacheEntry->GetXamlReader(spXamlReader));
        }
        else
        {
            // try the xbf
            IFC_RETURN(GetXamlBinaryReader(spXamlSchemaContext, strUniqueName, spNodeStreamCacheEntry, spXamlReader, spVersion2Reader));
            if (spXamlReader || spVersion2Reader)
            {
                *pfBinaryXamlLoaded = true;
            }
            else if (IsValidXamlTextBufferContent(pSource, cSource))
            {
                // if passed valid text content, use the text xaml
                IFC_RETURN(GetXamlTextReader(spXamlSchemaContext, textReaderSettings, cSource, pSource, spNodeStreamCacheEntry, spXamlReader));
            }
        }
    }

    return S_OK;
}

// Returns a XamlReader from the input binary stream
_Check_return_ HRESULT
XamlNodeStreamCacheManager::CreateXamlReaderFromBinaryBuffer(
    _In_ IPALMemory* pBinaryXamlBuffer,
    _In_ Parser::XamlBufferType bufferType,
    _In_ const std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext,
    _Out_ std::shared_ptr<XamlReader>& spXamlReader,
    _Out_ std::shared_ptr<XamlBinaryFormatReader2>& spVersion2Reader)
{
    // If the buffer is memory mapped XBFv2, we need to hold a reference to
    // the created XBFv2 reader so it remains alive as long as the core and 
    // this manager. We need to keep a reference to the reader in order to 
    // prevent XBFv2-origin strings finding themselves holding dangling 
    // pointers (the xstring_ptr_storage buffer wrappers that the strings 
    // reference to are actually owned by the reader; the strings only keep
    // a weak reference to the wrappers).
    //
    // In order to prevent monotonically increasing memory usage, we utilize a 
    // key-value store for the cache to avoid creating multiple XBFv2 readers 
    // for the same memory-mapped resource.
    auto cacheKey = static_cast<const void*>(pBinaryXamlBuffer->GetAddress());
    if (bufferType == Parser::XamlBufferType::MemoryMappedResource)
    {
        auto result = m_XBFv2ReaderCache.find(cacheKey);
        if (result != m_XBFv2ReaderCache.end())
        {
            spVersion2Reader = result->second;

            return S_OK;
        }
    }

    IFC_RETURN(CreateXamlBinaryFileReader(
        spXamlSchemaContext,
        pBinaryXamlBuffer,
        bufferType,
        spXamlReader,
        spVersion2Reader));

    if (spVersion2Reader && bufferType == Parser::XamlBufferType::MemoryMappedResource)
    {
        m_XBFv2ReaderCache.emplace(cacheKey, spVersion2Reader);
    }

    #if DBG
    // The Binary Format Validator has some overhead, so do this only for DBG builds
    if (spXamlReader)
    {
        std::shared_ptr<XamlBinaryFormatValidator> spXamlBinaryFormatValidator;
        IFC_RETURN(XamlBinaryFormatValidator::Create(spXamlSchemaContext, spXamlReader, spXamlBinaryFormatValidator));
        spXamlReader = spXamlBinaryFormatValidator;
    }
    #endif

    return S_OK;
}

// Get or create a NodeStreamCacheEntry for the Uri
_Check_return_ HRESULT
XamlNodeStreamCacheManager::EnsureCacheEntry(
    _In_ const xstring_ptr& spUniqueName,
    std::shared_ptr<NodeStreamCacheEntry>& spNodeStreamCacheEntry)
{
    auto itNodeStreamCacheEntry = m_UriToNodelistMap.find(spUniqueName);
    if (itNodeStreamCacheEntry == m_UriToNodelistMap.end())
    {
        spNodeStreamCacheEntry = std::make_shared<NodeStreamCacheEntry>();
        m_UriToNodelistMap.insert({ spUniqueName, spNodeStreamCacheEntry });
    }
    else{
        spNodeStreamCacheEntry = itNodeStreamCacheEntry->second;
    }

    return S_OK;
}
