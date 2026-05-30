// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlBinaryMetadata.h"
#include "XamlBinaryMetadataReader2.h"
#include "ObjectWriterNode.h"
#include "ObjectWriterNodeType.h"
#include <type_traits>
#include <cstdint>

class XamlBinaryFormatReader2;
class XamlSchemaContext;
class XamlTypeNamspace;

// A client of a XamlBinaryFormatReader2. This class is responsible for
// maintaining a small amount of state related to its current position in
// the node stream as well as maintaining shared ownership of a XamlBinaryFormatReader2.
// It does not directly own the byte buffers passed into it, relying on the owning XBFR2
// to keep them alive for the duration of its lifetime.
//
// This class will communicate errors that indicate XBF file corruption or unexpected
// failures during ObjectWriterNode construction using C++EH, unless explicitly marked
// otherwise, while expected endOfStream conditions are returned using bools.
class XamlBinaryFormatSubReader2 final
{
public:
    XamlBinaryFormatSubReader2(
        std::shared_ptr<XamlBinaryFormatReader2> xamlBinaryFormatMasterReader,
        unsigned int nodeStreamMasterOffset, unsigned int nodeStreamLength,
        unsigned int lineStreamLength,
        _In_ uint8_t* nodeStream,
        _In_opt_ uint8_t* lineStream);

    const Parser::XamlBinaryFileVersion& GetVersion() const;

    void set_NextIndex(unsigned int idx)
    {
        ASSERT(idx < m_nodeStreamLength);
        m_currentNodeStreamOffset = idx;
    }
    unsigned int get_NextIndex() const { return m_currentNodeStreamOffset; }
    void Reset() { m_currentNodeStreamOffset = 0;  }

    // The XamlBinaryFormatSubreader is used for two main purposes today:

    // Pulling ObjectWriterNodes out of a stream and using those nodes to create
    // objects. In this scenario consumers will often call Read until the stream runs
    // out. An HRESULT return code is provided for unexpected errors while a bool is
    // provided to indicate end of stream.
    bool TryRead(ObjectWriterNode& currentXamlNode);

    // This interface is kept around for simple compatibility with existing consumers
    // that aren't exception-oriented yet. It simply wraps TryRead above with an exception
    // catching block.
    _Check_return_ HRESULT TryReadHRESULT(ObjectWriterNode& currentXamlNode, _Out_ bool* endOfStream);


    // Pulling smaller chunks of data out in the case of CustomRuntimeData and using
    // that data to deserialize a CustomRuntimeData instance from the nodestream. In this
    // case the stream format is always encoded with the lengths of fields.
#pragma region Public Primitive Type Decoders
    CValue ReadCValue();
    unsigned int ReadUInt();
    std::uint64_t ReadUInt64();
    // Reads a fixed-width (8-byte) uint64_t from the node stream, bypassing LEB128 encoding.
    // Used for values like high-quality hashes where variable-length encoding is counterproductive.
    std::uint64_t ReadFixedWidthUInt64();
    xstring_ptr ReadSharedString();
    std::shared_ptr<XamlType> ReadXamlType();
    std::shared_ptr<XamlProperty> ReadXamlProperty();
#pragma endregion

    const XamlLineInfo ResolveLineInfo(_In_ const unsigned int streamOffset);

private:
#pragma region Raw Stream Accessors
    // Generic buffer read for variable-size data (arrays, strings).
    // For fixed-size type reads, prefer TryReadFromNodeStreamBuffer<T> which
    // avoids the overhead of function call indirection and void* type erasure.
    bool TryReadFromBuffer(
        _In_ XUINT8 *pBuffer,
        _In_ XUINT32 cbBufferTotalSize,
        _In_ XUINT32 cbBufferReadSize,
        _Out_writes_bytes_(cbBufferReadSize) void* pbTarget,
        _Inout_ XUINT32 *pcbBufferOffset);

    // Optimized fixed-size read from the node stream. Uses memcpy for
    // type-punning safety; the compiler optimizes this to a single load
    // instruction for types that fit in a register (1, 2, 4, or 8 bytes).
    // This eliminates the overhead of the generic TryReadFromBuffer path
    // (function call, void* indirection, std::copy_n) for the most common
    // read sizes in the hot deserialization loop.
    template <typename T>
    bool TryReadFromNodeStreamBuffer(_Out_ T* pTarget)
    {
        static_assert(std::is_trivially_copyable_v<T>,
            "TryReadFromNodeStreamBuffer only supports trivially copyable types");
        // Overflow-safe bounds check: verify offset is within buffer, then check
        // remaining bytes >= sizeof(T). The subtraction form avoids unsigned wrap.
        if (m_currentNodeStreamOffset > m_nodeStreamLength ||
            sizeof(T) > m_nodeStreamLength - m_currentNodeStreamOffset)
        {
            return false;
        }
        memcpy(pTarget, m_nodeStream + m_currentNodeStreamOffset, sizeof(T));
        m_currentNodeStreamOffset += sizeof(T);
        return true;
    }

    template <typename T>
    bool TryReadArrayFromNodeStreamBuffer(_Out_writes_all_(count) T* pTarget, _In_ UINT32 count)
    {
        return TryReadFromBuffer(m_nodeStream, m_nodeStreamLength, sizeof(T)*count, pTarget, &m_currentNodeStreamOffset);
    }
#pragma endregion

#pragma region Primitive Type Decoders
    // ReadNodeType is a 'try' method. Clients of this method that are using this
    // stream to create runtime object instances will often read to endOfStream. This
    // method is always read before the ObjectWriterNode is decoded and will be the
    // point in the stream where the end is hit.
    bool TryReadNodeType(_Out_ ObjectWriterNodeType& nodeType);

    xstring_ptr ReadPersistedString();
    PersistedXamlNode2 ReadPersistedXamlNode();

    // Branchless fast path for decoding 7-bit encoded uint32 values.
    // Uses architecture-specific bit scanning intrinsics to decode without
    // data-dependent branches, improving instruction-level parallelism.
    // Falls back to TryRead7BitEncodedIntSlow when near end of buffer.
    // Implemented in xamlbinaryformatsubreader2.cpp.
    _Success_(return != false)
    bool TryRead7BitEncodedIntFast32(
        _In_ XUINT8* pBuffer,
        _In_ XUINT32 cbBufferTotalSize,
        _Inout_ XUINT32* pcbBufferOffset,
        _Out_ unsigned int* pValue);

    // Slow-path loop for decoding 7-bit encoded integers of any width.
    // Reads bytes directly from the buffer without TryReadFromBuffer overhead.
    // Used as fallback when near end of buffer, and for uint64_t decoding
    // (where the branchless approach would be overly complex for 10-byte max).
    template <typename T,
        std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T>, int> = 0>
    _Success_(return != false)
    bool TryRead7BitEncodedIntSlow(
        _In_ XUINT8* pBuffer,
        _In_ XUINT32 cbBufferTotalSize,
        _Inout_ XUINT32* pcbBufferOffset,
        _Out_ T* pValue)
    {
        static constexpr unsigned int ContinuationBit = 0x80;  // High bit signals "more bytes follow"
        static constexpr unsigned int PayloadMask = 0x7F;      // Low 7 bits carry data payload
        static constexpr unsigned int BitsPerEncodedByte = 7;  // Each encoded byte carries 7 payload bits
        // Maximum encoded byte count: ceil(sizeof(T) * 8 / BitsPerEncodedByte).
        // A uint32 (32 bits) requires at most 5 bytes; a uint64 (64 bits) at most 10.
        static constexpr unsigned int MaxEncodedBytes = (sizeof(T) * 8 + 6) / BitsPerEncodedByte;

        T value = 0;
        unsigned int shift = 0;
        XUINT32 offset = *pcbBufferOffset;

        for (unsigned int bytesRead = 0; bytesRead < MaxEncodedBytes; ++bytesRead)
        {
            if (offset >= cbBufferTotalSize)
            {
                return false;
            }

            unsigned char currentByte = pBuffer[offset++];
            value |= static_cast<T>(currentByte & PayloadMask) << shift;
            shift += BitsPerEncodedByte;

            if (!(currentByte & ContinuationBit))
            {
                *pcbBufferOffset = offset;
                *pValue = value;

                return true;
            }
        }

        // Overlong encoding: more continuation bytes than T can represent.
        return false;
    }

    // Dispatcher for 7-bit encoded integer decoding.
    // Routes uint32 to the branchless fast path (TryRead7BitEncodedIntFast32)
    // when possible; uint64 and other widths use the generic loop.
    template <typename T,
        std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T>, int> = 0>
    _Success_(return != false)
    bool TryRead7BitEncodedInt(
        _In_ XUINT8 *pBuffer,
        _In_ XUINT32 cbBufferTotalSize,
        _Inout_ XUINT32 *pcbBufferOffset,
        _Out_ T *pValue)
    {
        if constexpr (sizeof(T) == sizeof(unsigned int))
        {
            // uint32: use branchless fast path with architecture-specific intrinsics
            return TryRead7BitEncodedIntFast32(
                pBuffer, cbBufferTotalSize, pcbBufferOffset,
                reinterpret_cast<unsigned int*>(pValue));
        }
        else
        {
            // uint64 and other widths: use the direct-buffer loop
            return TryRead7BitEncodedIntSlow(pBuffer, cbBufferTotalSize, pcbBufferOffset, pValue);
        }
    }

    PersistedConstantType ReadConstantNodeType();
    void ReadNamespace(_Out_ std::shared_ptr<XamlNamespace>& spNamespace, _Out_ xstring_ptr* pStrPrefixValue);

    std::shared_ptr<XamlQualifiedObject> ReadConstantAsQO();
#pragma endregion

    const XamlLineInfo GetLineInfo();

#pragma region ObjectWriterNode Decoders
    // These could likely be pulled out and made static.

    ObjectWriterNode ReadPushScopeAddNamespaceNode();
    ObjectWriterNode ReadAddNamespaceNode();
    ObjectWriterNode ReadSetValueNode();
    ObjectWriterNode ReadSetValueFromMarkupExtensionNode();
    ObjectWriterNode ReadPushConstantNode();
    ObjectWriterNode ReadMakeCheckPeerTypeNode();
    ObjectWriterNode ReadSetConnectionIdNode();
    ObjectWriterNode ReadSetNameNode();
    ObjectWriterNode ReadGetResourcePropertyBagNode();
    ObjectWriterNode ReadPushScopeGetValueNode();
    ObjectWriterNode ReadPushScopeCreateTypeBeginInitNode();
    ObjectWriterNode ReadPushScopeCreateTypeWithConstantBeginInitNode();
    ObjectWriterNode ReadPushScopeCreateTypeWithTypeConvertedConstantBeginInitNode();
    ObjectWriterNode ReadCreateTypeWithConstantBeginInitNode();
    ObjectWriterNode ReadCreateTypeWithTypeConvertedConstantBeginInitNode();
    ObjectWriterNode ReadSetValueConstantNode();
    ObjectWriterNode ReadSetValueTypeConvertedConstantNode();
    ObjectWriterNode ReadSetValueTypeConvertedResolvedTypeNode();
    ObjectWriterNode ReadSetValueTypeConvertedResolvedPropertyNode();
    ObjectWriterNode ReadProvideStaticResourceValueNode();
    ObjectWriterNode ReadProvideThemeResourceValueNode();
    ObjectWriterNode ReadSetValueFromStaticResourceNode();
    ObjectWriterNode ReadSetValueFromThemeResourceNode();
    ObjectWriterNode ReadSetValueFromTemplateBindingNode();
    ObjectWriterNode ReadSetDeferredPropertyNode();
    ObjectWriterNode ReadSetCustomRuntimeDataNode();
    ObjectWriterNode ReadCreateTypeBeginInitNode();
    ObjectWriterNode ReadAddToDictionaryWithKeyNode();
    ObjectWriterNode ReadBeginConditionalScopeNode();

#pragma endregion

    std::shared_ptr<XamlBinaryFormatReader2> m_spXamlBinaryFormatMasterReader;

    // The master offset is used for CustomRuntimeData lookup, when we ask
    // the owning XamlBinaryFormatReader for any preivously created CustomRuntimeData
    // instance at the given XBF stream offset.
    unsigned int m_nodeStreamMasterOffset;

    unsigned int m_currentNodeStreamOffset;
    unsigned int m_lastNodeStreamOffset;

    // The length and pointers to the underlying buffer is information
    // that could easily be read from XamlBinaryFormatReader2 using
    // only the stream index. On the other hand all this data fits
    // neatly in a cache line and prevents additional indirection in what
    // really is a quite common operation.
    unsigned int m_nodeStreamLength;
    unsigned int m_lineStreamLength;
    uint8_t* m_nodeStream;
    uint8_t* m_lineStream;

    // This is a little subtle: When you receive an ObjectWriterNode from a
    // SubReader it will be holding on to shared resources that will be repurposed
    // when the next call to Read occurs, chiefly this XQO instance is reused over
    // and over again.
    std::shared_ptr<XamlQualifiedObject> m_spTemporaryValue;
};

