// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ParserTypedefs.h"
#include "CompressedStackCacheHint.h"

struct XamlQualifiedObject;
class XamlType;

class CompressedObjectWriterStack
{
private:
    std::shared_ptr<XamlQualifiedObject> m_qoTargetType;
    std::shared_ptr<XamlQualifiedObject> m_qoResources;
    std::shared_ptr<XamlType> m_spTargetTypeForEncoder;
    
public:
    const std::shared_ptr<XamlQualifiedObject>& get_TargetType() const
    {
        return m_qoTargetType;
    }

    void set_TargetType(const std::shared_ptr<XamlQualifiedObject>& qoTargetType)
    {
        m_qoTargetType = qoTargetType;
    }

    const std::shared_ptr<XamlType>& get_TargetTypeForEncoder() const
    {
        return m_spTargetTypeForEncoder;
    }

    void set_TargetTypeForEncoder(const std::shared_ptr<XamlType>& spTargetType)
    {
        m_spTargetTypeForEncoder = spTargetType;
    }

    const std::shared_ptr<XamlQualifiedObject>& get_Resources() const
    {
        return m_qoResources;
    }

    void set_Resources(const std::shared_ptr<XamlQualifiedObject>& qoResources)
    {
        m_qoResources = qoResources;
    }


    HRESULT get_CachedItem(_In_ CompressedStackCacheHint cacheHint, _Out_ std::shared_ptr<XamlQualifiedObject>& qoItem) const
    {
        switch (cacheHint)
        {
        case CompressedStackCacheHint::TargetType:
            qoItem = get_TargetType();
            break;
        case CompressedStackCacheHint::Resources:
            qoItem = get_Resources();
            break;
        case CompressedStackCacheHint::None:
        default:
            qoItem.reset();
            break;
        }

        RRETURN(S_OK);
    }
};

