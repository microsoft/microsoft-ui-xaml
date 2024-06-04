// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class XamlReader;
class XamlSchemaContext;
class XamlTextReaderSettings;
class XamlOptimizedNodeList;
class XamlBinaryFormatReader2;

namespace Parser
{
    enum class XamlBufferType : uint8_t;
}

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {
    class XamlNodeStreamCacheManagerUnitTests;
} } } } }

// Caches an optimized representation of a xaml document for use in subsequent parses.
class XamlNodeStreamCacheManager
{
    // Allow unit tests to access everything.
    friend class Microsoft::UI::Xaml::Tests::Parser::XamlNodeStreamCacheManagerUnitTests;

public:
    XamlNodeStreamCacheManager(_In_ IParserCoreServices* pCore)
        : m_pCore(pCore)
    {
        // Memoization should ignore case to avoid creating duplicate entries for the same stream
    }

    ~XamlNodeStreamCacheManager()
    {}

    _Check_return_ static HRESULT Create(
        _In_ IParserCoreServices* pCore,
        _Out_ std::shared_ptr<XamlNodeStreamCacheManager>& spXamlNodeStreamCacheManager);

    _Check_return_ HRESULT HasCacheForUri(
        _In_ const xstring_ptr& strUri,
        _Out_ bool* pfHasCachedNodelist);

    _Check_return_ HRESULT GetXamlReader(
        _In_ std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext,
        _In_ const xstring_ptr& spUniqueName,
        _In_ UINT32 cSource,
        _In_reads_bytes_(cSource) const UINT8* pSource,
        _In_ const XamlTextReaderSettings& textReaderSettings,
        _Out_ std::shared_ptr<XamlReader>& spXamlReader,
        _Out_ std::shared_ptr<XamlBinaryFormatReader2>& spVersion2Reader,
        _Out_ bool* pfBinaryXamlLoaded);

    _Check_return_ HRESULT GetBinaryResourceForXamlUri(
        _In_ const xstring_ptr& strUri,
        _Out_ xref_ptr<IPALResource>& spXbfResourceOut);

    _Check_return_ HRESULT CreateXamlReaderFromBinaryBuffer(
        _In_ IPALMemory* pBinaryXamlBuffer,
        _In_ Parser::XamlBufferType bufferType,
        _In_ const std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext,
        _Out_ std::shared_ptr<XamlReader>& spXamlReader,
        _Out_ std::shared_ptr<XamlBinaryFormatReader2>& spVersion2Reader);

    void Flush();

    // Clears the XBFv2 reader cache. This is distinct from Flush()
    // because we only want to do this if metadata was reset (which leads
    // to our cached readers holding onto stale type information).
    _Check_return_ HRESULT ResetCachedXbfV2Readers();

private:
    class NodeStreamCacheEntry
    {
    public:
        NodeStreamCacheEntry()
            : m_uiAccessCount(0)
        {}

        UINT32 GetAccessCount() const;
        void IncrementAccessCount();

        _Check_return_ HRESULT SetNodeList(
            const std::shared_ptr<XamlOptimizedNodeList>& spXamlNodeList
            );

        _Check_return_ HRESULT GetXamlReader(
            _Out_ std::shared_ptr<XamlReader>& spXamlReader
            );

        bool HasNodeList() const;

    private:
        std::shared_ptr<XamlOptimizedNodeList> m_spNodeList;
        UINT32 m_uiAccessCount;
    };

    _Check_return_ HRESULT GetXamlBinaryReader(
        _In_ std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext,
        _In_ const xstring_ptr& strUniqueName,
        _In_ std::shared_ptr<NodeStreamCacheEntry> spNodeStreamCacheEntry,
        _Out_ std::shared_ptr<XamlReader>& spXamlReader,
        _Out_ std::shared_ptr<XamlBinaryFormatReader2>& spVersion2Reader);

    _Check_return_ HRESULT GetXamlTextReader(
        _In_ std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext,
        _In_ const XamlTextReaderSettings& textReaderSettings,
        _In_ UINT32 cSource,
        _In_reads_bytes_(cSource) const UINT8* pSource,
        _In_ std::shared_ptr<NodeStreamCacheEntry> spNodeStreamCacheEntry,
        _Out_ std::shared_ptr<XamlReader>& spXamlReader);

    _Check_return_ HRESULT EnsureCacheEntry(
        _In_ const xstring_ptr& spUniqueName,
        std::shared_ptr<NodeStreamCacheEntry>& spNodeStreamCacheEntry);

    IParserCoreServices* m_pCore;
    std::unordered_map<xstring_ptr, xref_ptr<IPALResource>> m_UriToXbfResourceMap;
    std::unordered_map<xstring_ptr, std::shared_ptr<NodeStreamCacheEntry>, xstrCaseInsensitiveHasher, xstrCaseInsensitiveEqual> m_UriToNodelistMap;

    // This ensures that memory mapped XBF strings remain valid as long as
    // this manager. We need to hold both the XBFv2 reader (which owns the
    // xstring_ptr_storage wrappers) and the IPALResource that owns the underlying
    // buffers.
    containers::vector_map<const void*, std::shared_ptr<XamlBinaryFormatReader2>> m_XBFv2ReaderCache;
    std::vector<std::shared_ptr<XamlBinaryFormatReader2>> m_staleXBFv2Readers;
    std::vector<xref_ptr<IPALResource>> m_XbfResourceStorage;
};


