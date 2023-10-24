// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class XamlNamespace;
class ObjectWriterFrame;

class ObjectWriterStack
{
    typedef std::vector<ObjectWriterFrame> stack_type;
public:
    typedef stack_type::reverse_iterator iterator;
    typedef stack_type::const_reverse_iterator const_iterator;

    ObjectWriterStack();

    // An ObjectWriterStack can potentially manage a lot of memory.
    // And shouldn't be copied often. We'll provide an explicit method for copying.
    ObjectWriterStack(const ObjectWriterStack&) = delete;
    ObjectWriterStack& operator=(const ObjectWriterStack&) = delete;

    ObjectWriterStack(ObjectWriterStack&& other) WI_NOEXCEPT
        : m_Stack(std::move(other.m_Stack))
    {
        UpdateIterators();
        other.UpdateIterators();
    }

    ObjectWriterStack& operator=(ObjectWriterStack&& other) WI_NOEXCEPT
    {
        if (this != &other) {
            m_Stack = std::move(other.m_Stack);
            UpdateIterators();
            other.UpdateIterators();
        }
        return *this;
    }

    ObjectWriterStack CopyStack() const;

    void PushScope();
    void PopScope();

    _Check_return_ HRESULT AddNamespacePrefix(
        _In_ const xstring_ptr& inPrefix,
        _In_ const std::shared_ptr<XamlNamespace>& spXamlNamespace);

    std::shared_ptr<XamlNamespace> FindNamespaceByPrefix(
        _In_ const xstring_ptr& inPrefix);

    bool IsInConditionalScope(bool ignoreInactiveScopes) const;

    bool empty() const
    {
        return m_Stack.empty();
    }

    const ObjectWriterFrame& Current() const
    {
        ASSERT(!empty());
        return *m_itCurrent;
    }

    ObjectWriterFrame& Current()
    {
        ASSERT(!empty());
        return *m_itCurrent;
    }

    const ObjectWriterFrame& Bottom() const
    {
        ASSERT(!empty());
        return *m_Stack.begin();
    }

    ObjectWriterFrame& Bottom()
    {
        ASSERT(!empty());
        return *m_Stack.begin();
    }

    const ObjectWriterFrame& Parent() const
    {
        ASSERT(size() > 1);
        return *m_itParent;
    }

    ObjectWriterFrame& Parent()
    {
        ASSERT(size() > 1);
        return *m_itParent;
    }

    UINT32 size() const
    {
        return static_cast<UINT32>(m_Stack.size());
    }

    iterator begin()
    {
        return m_Stack.rbegin();
    }

    const_iterator begin() const
    {
        return m_Stack.rbegin();
    }

    iterator end()
    {
        return m_Stack.rend();
    }

    const_iterator end() const
    {
        return m_Stack.rend();
    }

private:
    void UpdateIterators();

    stack_type m_Stack;
    iterator m_itCurrent;
    iterator m_itParent;
};


