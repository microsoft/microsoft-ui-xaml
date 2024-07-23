// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      XamlOptimizedNodeList is a container for XamlNode information that 
//      can be written to via a XamlWriter that can only append nodes to the 
//      end (accessed via get_Reader()), and read from via a forward only reader 
//      that can be accessed via get_Writer()

#include "precomp.h"

#include "XamlPredicateHelpers.h"

static const XUINT32 Bit8 = 0x00000080;       // 10000000
static const XUINT32 BitMask = 0x0000007F;   // 01111111

XamlOptimizedNodeList::XamlOptimizedNodeListWriter::XamlOptimizedNodeListWriter(
            const std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext,
            const std::shared_ptr<XamlOptimizedNodeList>& spXamlOptimizedNodeList)
    : m_spXamlOptimizedNodeList(spXamlOptimizedNodeList)
    , m_spXamlSchemaContext(spXamlSchemaContext)
{
}

#if DEBUG
HRESULT XamlOptimizedNodeList::XamlOptimizedNodeListWriter::WriteNode( _In_ const XamlNode& xamlNode )
{
    RRETURN(XamlWriter::WriteNode(xamlNode));
}
#endif

//------------------------------------------------------------------------
//
//  Synopsis:
//      Write a StartObject node into the XamlOptimizedNodeList
//
//  Notes:
//      The StartObject node requires the following information:
//          -- NodeType
//          -- Flags
//          -- LineDelta
//          -- PositionDelta
//          -- XamlType ID
//
//------------------------------------------------------------------------
_Check_return_ HRESULT XamlOptimizedNodeList::XamlOptimizedNodeListWriter::WriteObject(_In_ const std::shared_ptr<XamlType>& inType, bool bIsObjectFromMember)
{
    XUINT32 uiFlags = XamlOptimizedNodeList::nlfNone;
    
    if (inType->IsUnknown())
    {
        // REVIEW: Is this flag redundant? Why do we need it for XamlType,
        // but not XamlProperty?
        uiFlags |= XamlOptimizedNodeList::nlfIsUnknown;
    }
    if (bIsObjectFromMember)
    {
        uiFlags |= XamlOptimizedNodeList::nlfIsRetrieved;
    }
    
    WriteCommonInfo(XamlNodeType::xntStartObject, static_cast<XamlOptimizedNodeList::NodeListFlags>(uiFlags));
    m_spXamlOptimizedNodeList->AddType(inType);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Writes the information needed for a XamlNode of type EndObject
//
//  Notes:
//      The EndObject node requires the following information:
//          -- NodeType
//          -- Flags
//          -- LineDelta
//          -- PositionDelta
//
//------------------------------------------------------------------------
_Check_return_ HRESULT XamlOptimizedNodeList::XamlOptimizedNodeListWriter::WriteEndObject()
{
    WriteCommonInfo(XamlNodeType::xntEndObject, XamlOptimizedNodeList::nlfNone);
    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Writes the information needed for a XamlNode of type StartProperty
//
//  Notes:
//      The StartMember node requires the following information:
//          -- NodeType
//          -- Flags
//          -- LineDelta
//          -- PositionDelta
//          -- XamlProperty ID
//
//------------------------------------------------------------------------
_Check_return_ HRESULT XamlOptimizedNodeList::XamlOptimizedNodeListWriter::WriteMember(_In_ const std::shared_ptr<XamlProperty>& inProperty)
{
    WriteCommonInfo(XamlNodeType::xntStartProperty, XamlOptimizedNodeList::nlfNone);
    m_spXamlOptimizedNodeList->AddProperty(inProperty);

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Writes the information needed for a XamlNode of type EndProperty
//
//  Notes:
//      The EndMember node requires the following information:
//          -- NodeType
//          -- Flags
//          -- LineDelta
//          -- PositionDelta
//
//------------------------------------------------------------------------
_Check_return_ HRESULT XamlOptimizedNodeList::XamlOptimizedNodeListWriter::WriteEndMember()
{
    WriteCommonInfo(XamlNodeType::xntEndProperty, XamlOptimizedNodeList::nlfNone);
    return S_OK;
}


_Check_return_ HRESULT XamlOptimizedNodeList::XamlOptimizedNodeListWriter::WriteConditionalScope(const std::shared_ptr<Parser::XamlPredicateAndArgs>& xamlPredicateAndArgs)
{
    WriteCommonInfo(XamlNodeType::xntStartConditionalScope, XamlOptimizedNodeList::nlfNone);
    m_spXamlOptimizedNodeList->AddType(xamlPredicateAndArgs->PredicateType);
    m_spXamlOptimizedNodeList->AddString(xamlPredicateAndArgs->Arguments);

    return S_OK;
}

_Check_return_ HRESULT XamlOptimizedNodeList::XamlOptimizedNodeListWriter::WriteEndConditionalScope()
{
    WriteCommonInfo(XamlNodeType::xntEndConditionalScope, XamlOptimizedNodeList::nlfNone);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Writes the information needed for a XamlNode of type Value
//
//  Notes:
//      The StartObject node requires the following information:
//          -- NodeType
//          -- Flags
//          -- LineDelta
//          -- PositionDelta
//          -- Value
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
XamlOptimizedNodeList::XamlOptimizedNodeListWriter::WriteValue(_In_ const std::shared_ptr<XamlQualifiedObject>& value)
{
    WriteCommonInfo(XamlNodeType::xntValue, XamlOptimizedNodeList::nlfNone);
    m_spXamlOptimizedNodeList->AddValue(value);
    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Writes the information needed for a XamlNode of type Namespace.
//      This node encapsulates the data that would be held in an xmlns attribute
//
//  Notes:
//      The XamlNamespace node requires the following information:
//          -- NodeType
//          -- Flags
//          -- LineDelta
//          -- PositionDelta
//          -- XamlNamespace
//          -- Prefix
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
XamlOptimizedNodeList::XamlOptimizedNodeListWriter::WriteNamespace(_In_ const xstring_ptr& spPrefix, const std::shared_ptr<XamlNamespace>& spXamlNamespace)
{
    XamlOptimizedNodeList::NodeListFlags flags = nlfNone;

    if (!spXamlNamespace->HasValidRuntimeIndex())
    {
        flags = nlfIsUnknown;
    }
    
    WriteCommonInfo(XamlNodeType::xntNamespace, flags);
    m_spXamlOptimizedNodeList->AddXamlNamespace(spXamlNamespace, ((flags & nlfIsUnknown) != 0));
    m_spXamlOptimizedNodeList->AddString(spPrefix);
    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Writes the information that is common to all node types:
//          -- NodeType
//          -- Flags
//          -- Line
//          -- Position
//
//  Notes:
//      Line and Position information are written written as compactly as possible
//      using the following optimizations:
//          --  The values are only written if they have changed from the last written
//              line/position.  Indication as to whether the value has been written or not  
//              is is made in this function, and then provided to future readers
//              by setting the appropriate flag on nodeFlags before writing
//              nodeFlags to the XamlOptimizedNodeList.
//          --  The values that are written are the delta from the last written
//              line/position.  This ensures that the values written are small so that
//              we can take advantage of 7-bit encoding.  There is an additional 
//              flag in nodeFlags to indicate whether this delta is positive or negative.
//          --  The resultant values are 7-bit encoded.  This optimizes space for values
//              that are likely to fit into 1 or 2 bytes.
//
//------------------------------------------------------------------------
void XamlOptimizedNodeList::XamlOptimizedNodeListWriter::WriteCommonInfo(XamlNodeType nodeType, XamlOptimizedNodeList::NodeListFlags nodeFlags)
{
    XUINT32 uiLineDelta = 0;
    XUINT32 uiPositionDelta = 0;
    XUINT32 uiFlags = nodeFlags;
    XamlLineInfo lineInfo = GetLineInfo();
    
    if (lineInfo.LineNumber() > m_previousLineInfo.LineNumber() )
    {
        uiFlags |= XamlOptimizedNodeList::nlfHasLineDelta;
        uiLineDelta = lineInfo.LineNumber() - m_previousLineInfo.LineNumber();
    }
    else if (lineInfo.LineNumber() < m_previousLineInfo.LineNumber() )
    {
        uiFlags |= XamlOptimizedNodeList::nlfHasLineDelta | XamlOptimizedNodeList::nlfIsLineDeltaNegative;
        uiLineDelta =  m_previousLineInfo.LineNumber() - lineInfo.LineNumber();
    }

    if (lineInfo.LinePosition() > m_previousLineInfo.LinePosition())
    {
        uiFlags |= XamlOptimizedNodeList::nlfHasPositionDelta;
        uiPositionDelta = lineInfo.LinePosition() - m_previousLineInfo.LinePosition();
    }
    else if (lineInfo.LinePosition() < m_previousLineInfo.LinePosition())
    {
        uiFlags |= XamlOptimizedNodeList::nlfHasPositionDelta | XamlOptimizedNodeList::nlfIsPositionDeltaNegative;
        uiPositionDelta =  m_previousLineInfo.LinePosition() - lineInfo.LinePosition();
    }

    m_spXamlOptimizedNodeList->AddNodeType(nodeType);
    m_spXamlOptimizedNodeList->AddFlags(static_cast<XamlOptimizedNodeList::NodeListFlags>(uiFlags));

    if (uiLineDelta)
    {
        m_spXamlOptimizedNodeList->Write7BitEncodedInt(uiLineDelta);
    }

    if (uiPositionDelta)
    {
        m_spXamlOptimizedNodeList->Write7BitEncodedInt(uiPositionDelta);
    }

    m_previousLineInfo = lineInfo;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Close this XamlWriter, and Close the XamlOptimizedNodeList that 
//      it it writing to.      
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
XamlOptimizedNodeList::XamlOptimizedNodeListWriter::Close()
{
    WriteCommonInfo(XamlNodeType::xntEndOfStream, XamlOptimizedNodeList::nlfNone);
    m_spXamlOptimizedNodeList->Close();
    return S_OK;
}


XamlOptimizedNodeList::XamlOptimizedNodeListReader::XamlOptimizedNodeListReader(
    const std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext, 
    const std::shared_ptr<XamlOptimizedNodeList>& spXamlOptimizedNodeList, 
    XUINT32 nodeCount)
    : m_spXamlSchemaContext(spXamlSchemaContext)
    , m_spXamlOptimizedNodeList(spXamlOptimizedNodeList)
    , m_lastIndex(nodeCount)
    , m_nextIndex(0)
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Reads a XamlNode from the XamlOptimizedNodeList
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
XamlOptimizedNodeList::XamlOptimizedNodeListReader::Read()
{
    if (m_nextIndex >=  m_lastIndex)
    {
        return S_FALSE;
    }

    switch(ReadCommonInfo())
    {
        case XamlNodeType::xntStartObject:
        {
            auto xamlType = m_spXamlOptimizedNodeList->ReadTypeAt(m_nextIndex);
            m_currentNode.InitStartObjectNode(
                xamlType,
                (m_nodeFlags & XamlOptimizedNodeList::nlfIsRetrieved) != 0, 
                (m_nodeFlags & XamlOptimizedNodeList::nlfIsUnknown) != 0);
            break;
        }
        case XamlNodeType::xntEndObject:
        {
            m_currentNode.InitEndObjectNode();
            break;
        }
        case XamlNodeType::xntStartProperty:
        {
            auto xamlProperty = m_spXamlOptimizedNodeList->ReadPropertyAt(m_nextIndex);
            m_currentNode.InitStartMemberNode(xamlProperty);
            break;
        }
        case XamlNodeType::xntEndProperty:
        {
            m_currentNode.InitEndMemberNode();
            break;
        }
        case XamlNodeType::xntStartConditionalScope:
        {
            auto xamlType = m_spXamlOptimizedNodeList->ReadTypeAt(m_nextIndex);
            auto arguments = m_spXamlOptimizedNodeList->ReadStringAt(m_nextIndex);
            auto xamlPredicateAndArgs = std::make_shared<Parser::XamlPredicateAndArgs>(xamlType, arguments);
            m_currentNode.InitStartConditionalScopeNode(xamlPredicateAndArgs);
            break;
        }
        case XamlNodeType::xntEndConditionalScope:
        {
            m_currentNode.InitEndConditionalScopeNode();
            break;
        }
        case XamlNodeType::xntText:
        {
            IFC_RETURN(E_FAIL);
            break;
        }
        case XamlNodeType::xntValue:
        {
            m_currentNode.InitValueNode(m_spXamlOptimizedNodeList->ReadValueAt(m_nextIndex));
            break;
        }
        case XamlNodeType::xntNamespace:
        {
            auto spNamespace = m_spXamlOptimizedNodeList->ReadXamlNamespaceAt((m_nodeFlags & nlfIsUnknown) != 0, m_nextIndex);
            auto spPrefix = m_spXamlOptimizedNodeList->ReadStringAt(m_nextIndex);
            m_currentNode.InitAddNamespaceNode(spPrefix, spNamespace);
            break;
        }
        case XamlNodeType::xntEndOfStream:
        {
            m_currentNode.InitEndOfStreamNode();
            break;
        }
        case XamlNodeType::xntEndOfAttributes:
        {
            break;
        }
        case XamlNodeType::xntNone:
        {
            ASSERT(FALSE);
            break;
        }
        default:
            IFC_RETURN(E_FAIL);
            // throw new NotImplementedException(SR.Get(SRID.MissingCaseXamlNodes));
    }
    return S_OK;
}

const XamlNode& 
XamlOptimizedNodeList::XamlOptimizedNodeListReader::CurrentNode()
{
    return m_currentNode;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Reads the information that is common to all node types:
//          -- NodeType
//          -- Flags
//          -- Line
//          -- Position
//
//  Notes:
//      Line and Position information are written as compactly as possible
//      using optimizations as spelled out in the comments for
//      XamlOptimizedNodeList::XamlOptimizedNodeListWriter::WriteCommonInfo().
//
//------------------------------------------------------------------------
XamlNodeType XamlOptimizedNodeList::XamlOptimizedNodeListReader::ReadCommonInfo()
{
    auto nodeType = m_spXamlOptimizedNodeList->ReadNodeTypeAt(m_nextIndex);
    m_nodeFlags = m_spXamlOptimizedNodeList->ReadFlagsAt(m_nextIndex);

    if (m_nodeFlags & (nlfHasLineDelta | nlfHasPositionDelta))
    {
        ReadLineInfo();
    }
    return nodeType;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Reads Line/Position from the XamlOptimizedNodeList:
//
//  Notes:
//      Line and Position information are written as compactly as possible
//      using optimizations as spelled out in the comments for
//      XamlOptimizedNodeList::XamlOptimizedNodeListWriter::WriteCommonInfo().
//
//------------------------------------------------------------------------
void XamlOptimizedNodeList::XamlOptimizedNodeListReader::ReadLineInfo()
{
    UINT32 uiLine = m_previousLineInfo.LineNumber();
    UINT32 uiPosition = m_previousLineInfo.LinePosition();
    
    if (m_nodeFlags & nlfHasLineDelta)
    {
        UINT32 uiLineDelta = m_spXamlOptimizedNodeList->Read7BitEncodedInt(m_nextIndex);
        if (m_nodeFlags & nlfIsLineDeltaNegative)
        {
            uiLine -= uiLineDelta;
        }
        else
        {
            uiLine += uiLineDelta;
        }
    }
    if (m_nodeFlags & nlfHasPositionDelta)
    {
        UINT32 uiPositionDelta = m_spXamlOptimizedNodeList->Read7BitEncodedInt(m_nextIndex);
        if (m_nodeFlags & nlfIsPositionDeltaNegative)
        {
            uiPosition -= uiPositionDelta;
        }
        else
        {
            uiPosition += uiPositionDelta;
        }
    }

    m_previousLineInfo = XamlLineInfo(uiLine, uiPosition);
    m_currentNode.set_LineInfo(m_previousLineInfo);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the XamlSchemaContext
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
XamlOptimizedNodeList::XamlOptimizedNodeListReader::GetSchemaContext(std::shared_ptr<XamlSchemaContext>& outSchemaContext)
{
    outSchemaContext = m_spXamlSchemaContext.lock();
    if (!outSchemaContext)
    {
        IFC_RETURN(E_FAIL);
    }
    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      XamlOptimizedNodeList ctor
//
//------------------------------------------------------------------------
XamlOptimizedNodeList::XamlOptimizedNodeList(const std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext)
    : m_spXamlSchemaContext(spXamlSchemaContext)
    , m_fReadMode(false)
    , m_countForOldDictionaryDeferral(0)
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the XamlWriter for this XamlOptimizedNodeList
//
//  Notes:
//      The XamlOptimizedNodeList can only be written to once, and
//      only by one XamlWriter.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
XamlOptimizedNodeList::get_Writer(std::shared_ptr<XamlWriter>& outWriter)
{
    std::shared_ptr<XamlOptimizedNodeListWriter> spWriter;
    // there is only one writer per XamlOptimizedNodeList.
    if (m_spWriter.expired())
    {
        std::shared_ptr<XamlSchemaContext> schemaContext;
        IFC_RETURN(GetSchemaContext(schemaContext));
        spWriter = std::make_shared<XamlOptimizedNodeListWriter>(schemaContext, shared_from_this());
        m_spWriter = spWriter;
    }
    else
    {
        spWriter = m_spWriter.lock();
    }
    outWriter = std::move(spWriter);
    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Get a new forward-only XamlReader that can read the nodes from 
//      this XamlOptimizedNodeList.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
XamlOptimizedNodeList::get_Reader(std::shared_ptr<XamlReader>& outReader)
{
    std::shared_ptr<XamlSchemaContext> schemaContext;

    if (!m_fReadMode)
    {
        IFC_RETURN(E_UNEXPECTED);
    }
    
    IFC_RETURN(GetSchemaContext(schemaContext));
    outReader = std::make_shared<XamlOptimizedNodeListReader>(schemaContext, shared_from_this(), get_Count());
    return S_OK;
}

UINT32 
XamlOptimizedNodeList::get_Count() const
{
    return static_cast<UINT32>(m_bytes.size());
}

// Return a normalized count of the optimized node list
// by assuming property indexes/type indexes are single elements
unsigned int 
XamlOptimizedNodeList::get_CountForOldDictionaryDeferral() const
{
    return m_countForOldDictionaryDeferral;
}

void XamlOptimizedNodeList::Clear()
{
    m_fReadMode = false;
    m_countForOldDictionaryDeferral = 0;
    m_bytes.clear();
    m_values.clear();
    m_strings.clear();
    m_unknownNamespaces.clear();
}




//------------------------------------------------------------------------
//
//  Synopsis:
//      Reserves space in the member vectors based on the sizes of same members
//      in another XamlOptimizedNodeList.
//
//------------------------------------------------------------------------
void
XamlOptimizedNodeList::ReserveBasedOn(const std::shared_ptr<XamlOptimizedNodeList>& spOther)
{
    m_bytes.reserve(spOther->m_bytes.size());
    m_values.reserve(spOther->m_values.size());
    m_unknownNamespaces.reserve(spOther->m_unknownNamespaces.size());
    m_strings.reserve(spOther->m_strings.size());
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Close the XamlOptimizedNodeList for writing.  After Close is called
//      any calls to get_Writer() will fail.
//
//------------------------------------------------------------------------
void XamlOptimizedNodeList::Close()
{
    m_fReadMode = true;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a XamlNodeType to the XamlOptimizedNodeList.
//
//------------------------------------------------------------------------
void XamlOptimizedNodeList::AddNodeType(XamlNodeType nodeType)
{
    ASSERT(nodeType < 256);
    WriteByte(static_cast<XBYTE>(nodeType));
}

void XamlOptimizedNodeList::AddFlags(XamlOptimizedNodeList::NodeListFlags flags)
{
    ASSERT(flags < 256);
    WriteByte(static_cast<XBYTE>(flags));
}

void XamlOptimizedNodeList::AddProperty(const std::shared_ptr<XamlProperty>& spXamlProperty)
{
    Write7BitEncodedInt(static_cast<UINT32>(spXamlProperty->get_RuntimeIndex()), true /* count as one unit */);
}

void XamlOptimizedNodeList::AddType(const std::shared_ptr<XamlType>& spXamlType)
{
    Write7BitEncodedInt(static_cast<UINT32>(spXamlType->get_RuntimeIndex()), true /* count as one unit */);
}

void XamlOptimizedNodeList::AddXamlNamespace(const std::shared_ptr<XamlNamespace>& spXamlNamespace, bool fIsUnknown)
{
    if(fIsUnknown)
    {
        m_unknownNamespaces.push_back(spXamlNamespace);
        Write7BitEncodedInt(static_cast<UINT32>(m_unknownNamespaces.size()) -1);
    }
    else
    {
        Write7BitEncodedInt(static_cast<UINT32>(spXamlNamespace->get_RuntimeIndex()));
    }
}

void XamlOptimizedNodeList::AddValue(const std::shared_ptr<XamlQualifiedObject>& spXqo)
{
    auto index = static_cast<UINT32>(m_values.size());
    m_values.push_back(spXqo);
    Write7BitEncodedInt(index);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a string to the XamlOptimizedNodeList.  This is typically
//      used for the prefix on an xmlns declaration.
//
//------------------------------------------------------------------------
void XamlOptimizedNodeList::AddString(_In_ const xstring_ptr& spString)
{
    auto index = static_cast<UINT32>(m_strings.size());
    m_strings.push_back(spString);
    Write7BitEncodedInt(index);
}



//------------------------------------------------------------------------
//
//  Synopsis:
//      Read a NodeType from the XamlNodeList starting at ruiStartingIndex
//
//  Note:
//      ruiStartingIndex is incremented by the number of bytes read.
//
//------------------------------------------------------------------------
XamlNodeType XamlOptimizedNodeList::ReadNodeTypeAt(UINT32& ruiStartingIndex) const
{
    return static_cast<XamlNodeType>(ReadByteAt(ruiStartingIndex));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Read a NodeListFlags from the XamlNodeList starting at ruiStartingIndex
//
//  Note:
//      ruiStartingIndex is incremented by the number of bytes read.
//
//------------------------------------------------------------------------
XamlOptimizedNodeList::NodeListFlags XamlOptimizedNodeList::ReadFlagsAt(UINT32& ruiStartingIndex) const
{
    return static_cast<NodeListFlags>(ReadByteAt(ruiStartingIndex));
}



//------------------------------------------------------------------------
//
//  Synopsis:
//      Read a XamlProperty from the XamlNodeList starting at ruiStartingIndex
//
//  Note:
//      ruiStartingIndex is incremented by the number of bytes read.
//
//------------------------------------------------------------------------
std::shared_ptr<XamlProperty>
XamlOptimizedNodeList::ReadPropertyAt(UINT32& ruiStartingIndex) const
{
    auto uiIndex = Read7BitEncodedInt(ruiStartingIndex);
    return m_spXamlSchemaContext->GetXamlPropertyFromRuntimeIndex(uiIndex);
}



//------------------------------------------------------------------------
//
//  Synopsis:
//      Read a XamlType from the XamlNodeList starting at ruiStartingIndex
//
//  Note:
//      ruiStartingIndex is incremented by the number of bytes read.
//
//------------------------------------------------------------------------
std::shared_ptr<XamlType>
XamlOptimizedNodeList::ReadTypeAt(UINT32& ruiStartingIndex) const
{
    auto uiIndex = Read7BitEncodedInt(ruiStartingIndex);
    return m_spXamlSchemaContext->GetXamlTypeFromRuntimeIndex(uiIndex);
}



//------------------------------------------------------------------------
//
//  Synopsis:
//      Read a XamlNamespace from the XamlNodeList starting at ruiStartingIndex
//
//  Note:
//      ruiStartingIndex is incremented by the number of bytes read.
//
//------------------------------------------------------------------------
std::shared_ptr<XamlNamespace>
XamlOptimizedNodeList::ReadXamlNamespaceAt(bool fIsUnknown, UINT32& ruiStartingIndex) const
{
    auto uiIndex = Read7BitEncodedInt(ruiStartingIndex);
    if (fIsUnknown)
    {
        ASSERT(uiIndex < m_unknownNamespaces.size());
        return m_unknownNamespaces[uiIndex];
    }
    else
    {
        return m_spXamlSchemaContext->GetXamlNamespaceFromRuntimeIndex(uiIndex);
    }
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Read a NodeType from the XamlNodeList starting at ruiStartingIndex
//
//  Note:
//      ruiStartingIndex is incremented by the number of bytes read.
//
//------------------------------------------------------------------------
std::shared_ptr<XamlQualifiedObject>
XamlOptimizedNodeList::ReadValueAt(UINT32& ruiStartingIndex) const
{    
    auto uiIndex = Read7BitEncodedInt(ruiStartingIndex);
    ASSERT(uiIndex < m_values.size());
    return m_values[uiIndex];
}



//------------------------------------------------------------------------
//
//  Synopsis:
//      Read a String from the XamlNodeList starting at ruiStartingIndex
//
//  Note:
//      ruiStartingIndex is incremented by the number of bytes read.
//
//------------------------------------------------------------------------
xstring_ptr
XamlOptimizedNodeList::ReadStringAt(UINT32& ruiStartingIndex) const
{
    auto uiIndex = Read7BitEncodedInt(ruiStartingIndex);
    ASSERT(uiIndex < m_strings.size());
    return m_strings[uiIndex];
}

void XamlOptimizedNodeList::WriteByte(XBYTE bValue)
{
    m_countForOldDictionaryDeferral++;
    if (m_bytes.empty()) { m_bytes.reserve(128); }
    m_bytes.push_back(bValue);
}

void XamlOptimizedNodeList::Write7BitEncodedInt(UINT32 uiValue, bool countAsOneUnit)
{
    // if the loop ever becomes a perf problem, we can mask
    // the high bits of the 4 bytes, add them up, and have a switch
    // to the optimal un-rolled implementation.
    if (m_bytes.empty()) { m_bytes.reserve(128); }
    while(uiValue >= Bit8) 
    {
        m_bytes.push_back(static_cast<XBYTE>(uiValue | Bit8));
        uiValue >>= 7;
        if (!countAsOneUnit) ++m_countForOldDictionaryDeferral;
    }
    m_bytes.push_back(static_cast<XBYTE>(uiValue));
    ++m_countForOldDictionaryDeferral;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Read a single Byte from the XamlNodeList starting at ruiStartingIndex
//
//  Note:
//      ruiStartingIndex is incremented by the number of bytes read.
//
//------------------------------------------------------------------------
XBYTE XamlOptimizedNodeList::ReadByteAt(UINT32& ruiStartingIndex) const
{
    ASSERT(ruiStartingIndex < m_bytes.size());
    return m_bytes[ruiStartingIndex++];
}



//------------------------------------------------------------------------
//
//  Synopsis:
//      Read an XUINT32 from the XamlNodeList starting at ruiStartingIndex
//      using 7-bit encoding.
//
//  Note:
//      ruiStartingIndex is incremented by the number of bytes read.
//
//------------------------------------------------------------------------
UINT32 XamlOptimizedNodeList::Read7BitEncodedInt(UINT32& ruiStartingIndex) const
{
    UINT32 value = 0;
    UINT32 shift = 0;
    XBYTE currentByte = 0;
    
    do
    {
        ASSERT(shift != 5 * 7);
        ASSERT(ruiStartingIndex < m_bytes.size());
        currentByte = m_bytes[ruiStartingIndex++];
        value |= (currentByte & BitMask) << shift;
        shift += 7;
    } while (currentByte & Bit8);

    return value;
}



