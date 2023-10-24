// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <vector>
#include <memory>
#include "XamlNode.h"
#include "XamlReader.h"
#include "XamlWriter.h"
#include "LineInfo.h"
#include "XamlNodeType.h"

class XamlNamespace;
class XamlNode;
class XamlProperty;
class XamlReader;
class XamlSchemaContext;
class XamlType;
class XamlWriter;
struct XamlQualifiedObject;

namespace Parser
{
struct XamlPredicateAndArgs;
}

XUINT32 XamlOptimizedNodeListGrowthPolicy(XUINT32 oldSize);

// XamlOptimizedNodeList is a container for XamlNode information that
// can be written to via a XamlWriter that can only append nodes to the
// end (accessed via get_Reader()), and read from via a forward only reader
// that can be accessed via get_Writer()
class XamlOptimizedNodeList : public std::enable_shared_from_this<XamlOptimizedNodeList>
{
protected:
    enum NodeListFlags
    {
        nlfNone =                   0x00,
        nlfIsUnknown =              0x01,
        nlfIsRetrieved =            0x02,
        nlfHasLineDelta =           0x04,
        nlfIsLineDeltaNegative =    0x08,
        nlfHasPositionDelta =       0x10,
        nlfIsPositionDeltaNegative = 0x20,
    };

public:
    //---------------------------------------------------------------------------
    //
    //  XamlOptimizedNodeListReader is a forward-only reader of XamlNodes from a
    //  XamlOptimizedNodeList.
    //
    //---------------------------------------------------------------------------
    class XamlOptimizedNodeListReader : public XamlReader
    {
        public:
            XamlOptimizedNodeListReader(
                const std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext,
                const std::shared_ptr<XamlOptimizedNodeList>& spXamlOptimizedNodeList,
                XUINT32 nodeCount
                );
            _Check_return_ HRESULT Read() override;
            const XamlNode& CurrentNode() override;
            _Check_return_ HRESULT GetSchemaContext(std::shared_ptr<XamlSchemaContext>& outSchemaContext) override;

            HRESULT set_NextIndex(XUINT32 uiIndex) override   { m_nextIndex = uiIndex;   RRETURN(S_OK);  }
            HRESULT get_NextIndex(XUINT32 *puiIndex) override { *puiIndex = m_nextIndex; RRETURN(S_OK);  }
        private:

            XamlNodeType ReadCommonInfo();

            void ReadLineInfo();

            std::weak_ptr<XamlSchemaContext> m_spXamlSchemaContext;
            std::shared_ptr<XamlOptimizedNodeList> m_spXamlOptimizedNodeList;

            XUINT32 m_nextIndex;
            XUINT32 m_lastIndex;

            XamlNode m_currentNode;

            XamlOptimizedNodeList::NodeListFlags m_nodeFlags = nlfNone;
            XamlLineInfo m_previousLineInfo;
    };


    //---------------------------------------------------------------------------
    //
    //  XamlOptimizedNodeListWriter the XamlWriter for the XamlOptimizedNodeList.
    //
    //---------------------------------------------------------------------------
    class XamlOptimizedNodeListWriter final : public XamlWriter
    {
        public:
        XamlOptimizedNodeListWriter();
        XamlOptimizedNodeListWriter(
                const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
                const std::shared_ptr<XamlOptimizedNodeList>& spXamlOptimizedNodeList
                );

        #if DEBUG
        _Check_return_ virtual HRESULT WriteNode(_In_ const XamlNode& inNode);
        #endif

        _Check_return_ HRESULT WriteObject(_In_ const std::shared_ptr<XamlType>& inType, bool bIsObjectFromMember) override;
        _Check_return_ HRESULT WriteEndObject() override;
        _Check_return_ HRESULT WriteMember(_In_ const std::shared_ptr<XamlProperty>& inProperty) override;
        _Check_return_ HRESULT WriteEndMember() override;
        _Check_return_ HRESULT WriteConditionalScope(const std::shared_ptr<Parser::XamlPredicateAndArgs>& xamlPredicateAndArgs) override;
        _Check_return_ HRESULT WriteEndConditionalScope() override;
        _Check_return_ HRESULT WriteValue(_In_ const std::shared_ptr<XamlQualifiedObject>& value) override;
        _Check_return_ HRESULT WriteNamespace(_In_ const xstring_ptr& inssPrefix, const std::shared_ptr<XamlNamespace>& inssXamlNamespace) override;

        _Check_return_ HRESULT Close() override;

        _Check_return_ HRESULT GetSchemaContext(std::shared_ptr<XamlSchemaContext>& outSchemaContext) const override
        {
            outSchemaContext = m_spXamlSchemaContext;
            RRETURN(S_OK);
        }

        private:
            void WriteCommonInfo(
                XamlNodeType xamlNodeType,
                NodeListFlags nodeListFlags);

        std::shared_ptr<XamlOptimizedNodeList> m_spXamlOptimizedNodeList;
        std::shared_ptr<XamlSchemaContext> m_spXamlSchemaContext;
        XamlLineInfo m_previousLineInfo;
    };


public:

    XamlOptimizedNodeList(const std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext);

    _Check_return_ HRESULT get_Writer(std::shared_ptr<XamlWriter>& outWriter);
    _Check_return_ HRESULT get_Reader(std::shared_ptr<XamlReader>& outReader);
    UINT32 get_Count() const;
    unsigned int get_CountForOldDictionaryDeferral() const;
    void Clear();
    void ReserveBasedOn(const std::shared_ptr<XamlOptimizedNodeList>& spOther);
    void Close();

protected:
    void AddNodeType(XamlNodeType nodeType);
    void AddFlags(XamlOptimizedNodeList::NodeListFlags flags);
    void AddProperty(const std::shared_ptr<XamlProperty>& spXamlProperty);
    void AddType(const std::shared_ptr<XamlType>& spXamlType);
    void AddXamlNamespace(const std::shared_ptr<XamlNamespace>& spXamlNamespace, bool fIsUnknown);
    void AddValue(const std::shared_ptr<XamlQualifiedObject>& spXqo);
    void AddString(_In_ const xstring_ptr& spString);

    void WriteByte(XBYTE bValue);
    void Write7BitEncodedInt(UINT32 uiValue, bool countAsOneUnit = false);

    XamlNodeType ReadNodeTypeAt(UINT32& ruiStartingIndex) const;
    XamlOptimizedNodeList::NodeListFlags ReadFlagsAt(UINT32& ruiStartingIndex) const;
    std::shared_ptr<XamlProperty> ReadPropertyAt(UINT32& ruiStartingIndex) const;
    std::shared_ptr<XamlType> ReadTypeAt(UINT32& ruiStartingIndex) const;
    std::shared_ptr<XamlNamespace> ReadXamlNamespaceAt(bool fIsUnknown, UINT32& ruiStartingIndex) const;
    std::shared_ptr<XamlQualifiedObject> ReadValueAt(UINT32& ruiStartingIndex) const;
    xstring_ptr ReadStringAt(UINT32& ruiStartingIndex) const;

    XBYTE ReadByteAt(UINT32& ruiStartingIndex) const;
    UINT32 Read7BitEncodedInt(UINT32& ruiStartingIndex) const;


private:
    XamlOptimizedNodeList();

    HRESULT GetSchemaContext(std::shared_ptr<XamlSchemaContext>& outSchemaContext) const
    {
        outSchemaContext = m_spXamlSchemaContext;
        RRETURN(S_OK);
    }

    // Contains the main XamlNode stream
    std::vector<XBYTE> m_bytes;

    // Contains the values that were originally written
    // into the stream.  The values are saved in this vector, and
    // what is written into the stream is an index into this vector.
    std::vector< std::shared_ptr<XamlQualifiedObject> > m_values;

    // Contains the strings that were originally written into the
    // stream.  The values are saved in this vector, and what is
    // written into the stream is an index into this vector.
    std::vector< xstring_ptr > m_strings;

    // Contains the unknown XamlNamespaces that are written into the
    // stream.  While all other Xaml-Schema objects (XamlType, XamlProperty,
    // known XamlNamespace) are expected to have indices that can be
    // saved into the main stream, Unknown namespaces do not, and
    // so must actually be saved in the XamlOptimizedNodeList.
    std::vector< std::shared_ptr<XamlNamespace> > m_unknownNamespaces;

    // Track whether the XamlOptimizedNodeList has gone into "ReadMode".
    // The XamlOptimizedNodeList is expected to be written once through
    // a single XmlWriter, and then read many times.
    bool m_fReadMode = false;

    // Track the node stream count normalizing the sizes for
    // Property Index/Type Index to keep backward compatibility.
    unsigned int m_countForOldDictionaryDeferral = 0;

    std::shared_ptr<XamlSchemaContext> m_spXamlSchemaContext;
    std::weak_ptr<XamlOptimizedNodeListWriter> m_spWriter;
};


