// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class TypeNamePtr
{
public:
    TypeNamePtr() = default;

    TypeNamePtr(HSTRING name, wxaml_interop::TypeKind kind)
    {
        m_typeName.Kind = kind;
        WindowsDuplicateString(name, &m_typeName.Name);
    }

    TypeNamePtr(const TypeNamePtr& other)
    {
        m_typeName.Kind = other.m_typeName.Kind;
        WindowsDuplicateString(other.m_typeName.Name, &m_typeName.Name);
    }

    TypeNamePtr(const wxaml_interop::TypeName& other)
    {
        m_typeName.Kind = other.Kind;
        WindowsDuplicateString(other.Name, &m_typeName.Name);
    }

    TypeNamePtr(TypeNamePtr&& other)
    {
        m_typeName = other.m_typeName;
        other.m_typeName = { nullptr, wxaml_interop::TypeKind_Primitive };
    }

    TypeNamePtr& operator=(const TypeNamePtr& other)
    {
        if (this != &other)
        {
            Reset();
            m_typeName.Kind = other.m_typeName.Kind;
            WindowsDuplicateString(other.m_typeName.Name, &m_typeName.Name);
        }

        return *this;
    }

    TypeNamePtr& operator=(TypeNamePtr&& other)
    {
        if (this != &other)
        {
            Reset();
            m_typeName = other.m_typeName;
            other.m_typeName = { nullptr, wxaml_interop::TypeKind_Primitive };
        }

        return *this;
    }

    ~TypeNamePtr()
    {
        Reset();
    }

    // do not allow non-const references to fields as it defeats lifetime management provided by this class.
    const wxaml_interop::TypeName* operator->() const
    {
        return &m_typeName;
    }

    const wxaml_interop::TypeName& Get() const
    {
        return m_typeName;
    }

    // ...unless we clear out previous state and take ownership.
    wxaml_interop::TypeName* ReleaseAndGetAddressOf()
    {
        Reset();
        return &m_typeName;
    }

    HRESULT CopyTo(wxaml_interop::TypeName* typeName) const
    {
        typeName->Kind = m_typeName.Kind;
        return WindowsDuplicateString(m_typeName.Name, &typeName->Name);
    }

    void Reset()
    {
        WindowsDeleteString(m_typeName.Name);
        m_typeName = { nullptr, wxaml_interop::TypeKind_Primitive };
    }

    explicit operator bool() const
    {
        return m_typeName.Name != nullptr;
    }

private:
    wxaml_interop::TypeName m_typeName { nullptr, wxaml_interop::TypeKind_Primitive };
};
