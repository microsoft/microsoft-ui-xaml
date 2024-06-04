// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "LineInfo.h"

class XamlNamespace;
class XamlNode;
class XamlProperty;
class XamlSchemaContext;
class XamlType;
struct XamlQualifiedObject;

namespace Parser
{
struct XamlPredicateAndArgs;
}

// Xaml is parsed in a stream-oriented fashion. This interface can be implemented by
// anything that wishes to consume the Xaml node stream, whether it's going to create
// actual XAML objects, generate a new intermediate format, or do some sort of custom
// deferring work.
class IXamlWriter
{
public:
    virtual HRESULT WriteObject(const std::shared_ptr<XamlType>& inType, bool bIsObjectFromMember) = 0;
    virtual HRESULT WriteEndObject() = 0;
    virtual HRESULT WriteMember(const std::shared_ptr<XamlProperty>& inProperty) = 0;
    virtual HRESULT WriteEndMember() = 0;
    virtual HRESULT WriteValue(const std::shared_ptr<XamlQualifiedObject>& value) = 0;
    virtual HRESULT WriteNamespace(const xstring_ptr& inssPrefix, const std::shared_ptr<XamlNamespace>& inssXamlNamespace) = 0;
    virtual HRESULT WriteConditionalScope(const std::shared_ptr<Parser::XamlPredicateAndArgs>& xamlPredicateAndArgs) = 0;
    virtual HRESULT WriteEndConditionalScope() = 0;
    virtual HRESULT Close() = 0;

    virtual HRESULT GetSchemaContext(std::shared_ptr<XamlSchemaContext>& outSchemaContext) const = 0;
    virtual ~IXamlWriter() {}

    const XamlLineInfo& GetLineInfo()
    {
        return m_LineInfo;
    }

    void SetLineInfo(const XamlLineInfo& inLineInfo)
    {
        m_LineInfo = inLineInfo;
    }

private:
    XamlLineInfo m_LineInfo;
};

class XamlWriter
    : public IXamlWriter
{
public:
    // We've added this method here as kind of an awkward way to dispatch to the
    // methods defined on IXamlWriter. Ideally it should be factored out of this class
    // because that's a weird design and it's preventing things from inheriting from
    // IXamlWriter as they should.
    _Check_return_ HRESULT WriteNode(_In_ const XamlNode& inNode);


    // TODO: replace this with something with less loaded meaning
    // this is not an implementation of the Disposing pattern, but is just here
    // satisfy some code in WriterDelegate::Dispose() in System.Xaml 
    _Check_return_ HRESULT Close() override { return S_OK; }
    
protected:
    // This is called by WriteNode when it sees that an error has occurred.  It
    // should not be called by anyone else. It should be moved, along with WriteNode,
    // out of XamlWriter.
    virtual _Check_return_ HRESULT ProcessError()
    {
        return S_OK;
    }
};
