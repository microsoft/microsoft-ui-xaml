// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlToken.h"
#include "XamlDirectives.h"

class CPropertyBase;

enum class KnownPropertyIndex : UINT16;

struct XamlPropertyToken : public XamlToken
{
    XamlPropertyToken()
        : XamlToken()
    {}

    XamlPropertyToken(const XamlTypeInfoProviderKind& inKind, const KnownPropertyIndex& inHandle)
        : XamlToken(inKind, static_cast<const XUINT32>(inHandle))
    {}

    static XamlPropertyToken FromProperty(_In_ const CPropertyBase* pProperty);

    XamlDirectives GetDirectiveHandle() const
    {
        ASSERT(GetProviderKind() == tpkParser);
        return static_cast<XamlDirectives>(XamlToken::GetHandle());
    }

    void SetDirectiveHandle(XamlDirectives inHandle)
    {
        ASSERT(GetProviderKind() == tpkParser);
        XamlToken::SetHandle(inHandle);
    }

    KnownPropertyIndex GetHandle() const
    {
        ASSERT(GetProviderKind() != tpkParser);
        return static_cast<KnownPropertyIndex>(XamlToken::GetHandle());
    }
    void SetHandle(KnownPropertyIndex inHandle)
    {
        ASSERT(GetProviderKind() != tpkParser);
        XamlToken::SetHandle(static_cast<XUINT32>(inHandle));
    }

    bool IsUnknown() const
    {
        switch (GetProviderKind())
        {
        case tpkUnknown:
            return true;
        case tpkParser:
            return GetDirectiveHandle() == xdNone;
        default:
            return GetHandle() == KnownPropertyIndex::UnknownType_UnknownProperty;
        }
    }

    bool Equals(XamlPropertyToken propertyToken) const
    {
        return (XUINT32)(*this) == (XUINT32)propertyToken;
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
