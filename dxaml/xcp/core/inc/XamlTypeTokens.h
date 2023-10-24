// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Indexes.g.h>

class CNamespaceInfo;
class CClassInfo;
class CDependencyProperty;

enum XamlSpecialXmlNamespaceKind
{
    sxkNone,
    sxkXml,
    sxkXmlns
};

#include "XamlDirectives.h"

#include "XamlTypeInfoProviderKind.h"
#include "XamlFactoryKind.h"

#include "XamlToken.h"
#include <DataStructureFunctionProvider.h>

struct XamlAssemblyToken : public XamlToken
{
    XamlAssemblyToken()
        : XamlToken()
    {}

    XamlAssemblyToken(const XamlTypeInfoProviderKind& inKind, const XUINT32& inHandle)
        : XamlToken(inKind, inHandle)
    {}

    XUINT32 GetHandle() const
    {
        return XamlToken::GetHandle();
    }
    void SetHandle(XUINT32 inHandle)
    {
        XamlToken::SetHandle(inHandle);
    }

    size_t hash() const
    {
        return static_cast<size_t>(XamlToken::GetHandle());
    }

    // DEPRECATED. Do not directly cast a XamlToken to an integer. Perform
    // comparisons with the relevent fields. If equality is needed please
    // implement a comparison operator.
    operator XUINT32() const
    {
        return *(reinterpret_cast<const XUINT32*>(this));
    }
};

struct XamlTypeNamespaceToken : public XamlToken
{
    XamlTypeNamespaceToken()
        : XamlToken()
    {}

    XamlTypeNamespaceToken(const XamlTypeInfoProviderKind& inKind, const KnownNamespaceIndex& inHandle)
        : XamlToken(inKind, static_cast<const XUINT32>(inHandle))
    {}

    static XamlTypeNamespaceToken FromNamespace(_In_ const CNamespaceInfo* pNamespace);

    KnownNamespaceIndex GetHandle() const
    {
        return static_cast<KnownNamespaceIndex>(XamlToken::GetHandle());
    }

    void SetHandle(KnownNamespaceIndex inHandle)
    {
        XamlToken::SetHandle(static_cast<XUINT32>(inHandle));
    }

    size_t hash() const
    {
        return static_cast<size_t>(XamlToken::GetHandle()) ^ AssemblyToken.hash();
    }

    // DEPRECATED. Do not directly cast a XamlToken to an integer. Perform
    // comparisons with the relevent fields. If equality is needed please
    // implement a comparison operator. This is evil and depends implicitly
    // on the binary layout of this struct which is totally unnecessary.
    operator XUINT64() const
    {
        return *(reinterpret_cast<const XUINT64*>(this));
    }

    // This evil field is not part of the constructor. It's not clear how it's
    // used or how the idea of an asssembly has translated into Jupiter. Dragons
    // abound.
    XamlAssemblyToken AssemblyToken;
};

struct XamlTypeToken : public XamlToken
{
    XamlTypeToken(const XamlTypeInfoProviderKind& inKind, const KnownTypeIndex& inHandle)
        : XamlToken(inKind, static_cast<const XUINT32>(inHandle))
    {
    }

    XamlTypeToken()
    {
    }

    static XamlTypeToken FromType(_In_ const CClassInfo* pType);

    KnownTypeIndex GetHandle() const
    {
        return static_cast<KnownTypeIndex>(XamlToken::GetHandle());
    }
    void SetHandle(KnownTypeIndex inHandle)
    {
        XamlToken::SetHandle(static_cast<XUINT32>(inHandle));
    }

    bool Equals(XamlTypeToken typeToken) const
    {
        return (XUINT32)(*this) == (XUINT32)typeToken;
    }

    size_t hash() const
    {
        return static_cast<size_t>(XamlToken::GetHandle());
    }
    // DEPRECATED. Do not directly cast a XamlToken to an integer. Perform
    // comparisons with the relevent fields. This is evil and depends implicitly
    // on the binary layout of this struct which is totally unnecessary.
    operator XUINT32() const
    {
        return *(reinterpret_cast<const XUINT32*>(this));
    }
};

#include "XamlPropertyToken.h"

struct XamlPropertyAndTypeToken
{
    XamlTypeToken m_ttPropertyType;
    XamlPropertyToken m_tpProperty;
};

#pragma region specializations of hash
namespace std
{
    template<>
    struct hash < XamlAssemblyToken >
    {
        typedef XamlAssemblyToken Key;
        typedef size_t result_type;
        result_type operator()(const XamlAssemblyToken& token) const { return token.hash(); }
    };

    template<>
    struct hash < XamlTypeNamespaceToken >
    {
        typedef XamlTypeNamespaceToken Key;
        typedef size_t result_type;
        result_type operator()(const XamlTypeNamespaceToken& token) const { return token.hash(); }
    };

    template<>
    struct hash < XamlTypeToken >
    {
        typedef XamlTypeToken Key;
        typedef size_t result_type;
        result_type operator()(const XamlTypeToken& token) const { return token.hash(); }
    };

    template<>
    struct hash < XamlPropertyToken >
    {
        typedef XamlPropertyToken Key;
        typedef size_t result_type;
        result_type operator()(const XamlPropertyToken& token) const { return token.hash(); }
    };
}
#pragma endregion

#pragma region Depricated x-collection Hash/Equality Providers
// DEPRECATED: New code should move away from x-collections and these
// data providers are hacky and bad. Remove these  as time allows and
// remove the cast to XINT32/64 operators from the XamlToken-derived classes.
template<>
struct DataStructureFunctionProvider<XamlPropertyToken>
{
    static XUINT32 Hash(const XamlPropertyToken& data)
    {
        return (XUINT32)data;
    }

    static bool AreEqual(const XamlPropertyToken& lhs, const XamlPropertyToken& rhs)
    {
        return (XUINT32)lhs == (XUINT32)rhs;
    }
};

// DEPRECATED: New code should move away from x-collections and these
// data providers are hacky and bad. Remove these  as time allows and
// remove the cast to XINT32/64 operators from the XamlToken-derived classes.
template<>
struct DataStructureFunctionProvider<XamlTypeToken>
{
    static XUINT32 Hash(const XamlTypeToken& data)
    {
        return (XUINT32)data;
    }

    static bool AreEqual(const XamlTypeToken& lhs, const XamlTypeToken& rhs)
    {
        return (XUINT32)lhs == (XUINT32)rhs;
    }
};

// DEPRECATED: New code should move away from x-collections and these
// data providers are hacky and bad. Remove these  as time allows and
// remove the cast to XINT32/64 operators from the XamlToken-derived classes.
template<>
struct DataStructureFunctionProvider<XamlTypeNamespaceToken>
{
    static XUINT32 Hash(const XamlTypeNamespaceToken& data)
    {
        return (XUINT32)data;
    }

    static bool AreEqual(const XamlTypeNamespaceToken& lhs, const XamlTypeNamespaceToken& rhs)
    {
        return (XUINT64)lhs == (XUINT64)rhs;
    }
};

// DEPRECATED: New code should move away from x-collections and these
// data providers are hacky and bad. Remove these  as time allows and
// remove the cast to XINT32/64 operators from the XamlToken-derived classes.
template<>
struct DataStructureFunctionProvider<XamlAssemblyToken>
{
    static XUINT32 Hash(const XamlAssemblyToken& data)
    {
        return (XUINT32)data;
    }

    static bool AreEqual(const XamlAssemblyToken& lhs, const XamlAssemblyToken& rhs)
    {
        return (XUINT32)lhs == (XUINT32)rhs;
    }
};
#pragma endregion
