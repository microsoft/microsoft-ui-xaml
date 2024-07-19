// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "LineInfo.h"

class XamlNamespace;
struct XamlQualifiedObject;
class XamlProperty;
class XamlType;

namespace Parser
{
struct XamlPredicateAndArgs;
}

// Represents the interface that a custom writer uses to communicate
// with its owning CustomWriterManager.
class ICustomWriter
{
public:
    virtual ~ICustomWriter() {}
    virtual _Check_return_ HRESULT WriteObject(const std::shared_ptr<XamlType>& inType, bool bIsObjectFromMember, _Out_ bool* pResult) = 0;
    virtual _Check_return_ HRESULT WriteEndObject(_Out_ bool* pResult) = 0;
    virtual _Check_return_ HRESULT WriteMember(const std::shared_ptr<XamlProperty>& inProperty, _Out_ bool* pResult) = 0;
    virtual _Check_return_ HRESULT WriteEndMember(_Out_ bool* pResult) = 0;
    virtual _Check_return_ HRESULT WriteConditionalScope(const std::shared_ptr<Parser::XamlPredicateAndArgs>& xamlPredicateAndArgs, _Out_ bool* pResult) = 0;
    virtual _Check_return_ HRESULT WriteEndConditionalScope(_Out_ bool* pResult) = 0;
    virtual _Check_return_ HRESULT WriteValue(const std::shared_ptr<XamlQualifiedObject>& value, _Out_ bool* pResult) = 0;
    virtual _Check_return_ HRESULT WriteNamespace(_In_ const xstring_ptr& inssPrefix, const std::shared_ptr<XamlNamespace>& inssXamlNamespace, _Out_ bool* pResult) = 0;
    virtual bool IsCustomWriterFinished() const = 0;

    // A custom writer may override this to allow or disallow the CustomWriterManager to create 
    // new custom writers depending on the current writer's state.  When the manager calls this
    // at the start of its WriteObject method, the parameter spXamlType is the incoming type.
    // Otherwise, it is null, such as when called from the manager's WriteEndMember method.
    virtual bool ShouldAllowNewCustomWriter(_In_ const std::shared_ptr<XamlType>& spXamlType)
    {
        return true;
    }

    void SetLineInfo(const XamlLineInfo& lineInfo)
    {
        m_lineInfo = lineInfo;
    }

    XamlLineInfo GetLineInfo() const
    {
        return m_lineInfo;
    }

private:
    XamlLineInfo m_lineInfo;
};
