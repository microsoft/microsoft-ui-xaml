// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ObjectWriterStack.h>
#include <ObjectWriterFrame.h>

ObjectWriterStack::ObjectWriterStack()
{
    UpdateIterators();
}

ObjectWriterStack ObjectWriterStack::CopyStack() const
{
    ObjectWriterStack result;
    result.m_Stack = m_Stack;
    result.UpdateIterators();
    return result;
}

void ObjectWriterStack::PushScope()
{
    // XAML trees are almost always a few layers deep, ObjectWriter trees even more so
    // Let's just bypass the first couple vector reallocations
    m_Stack.reserve(6);
    m_Stack.emplace_back();
    UpdateIterators();
}

void ObjectWriterStack::PopScope()
{
    ASSERT(!empty());
    m_Stack.pop_back();
    UpdateIterators();
}

_Check_return_ HRESULT ObjectWriterStack::AddNamespacePrefix(
    _In_ const xstring_ptr& inPrefix,
    _In_ const std::shared_ptr<XamlNamespace>& spXamlNamespace)
{
    return Current().AddNamespacePrefix(inPrefix, spXamlNamespace);
}

std::shared_ptr<XamlNamespace> ObjectWriterStack::FindNamespaceByPrefix(
    _In_ const xstring_ptr& inPrefix)
{
    std::shared_ptr<XamlNamespace> outNamespace;
    for (const auto& frame : *this) {
        outNamespace = frame.FindNamespaceByPrefix(inPrefix);

        if (outNamespace)
        {
            break;
        }
    }
    return outNamespace;
}

bool ObjectWriterStack::IsInConditionalScope(bool ignoreInactiveScopes) const
{
    for (const auto& frame : *this)
    {
        if (frame.get_HasConditionalScopeToSkip(ignoreInactiveScopes))
        {
            return true;
        }
    }

    return false;
}

void ObjectWriterStack::UpdateIterators()
{
    m_itCurrent = begin();
    m_itParent = (size() > 1)
        ? ++begin()
        : end();
}

