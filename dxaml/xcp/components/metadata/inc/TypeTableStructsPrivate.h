// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TypeTableStructs.h"

struct MetaDataNamespace
{
    KnownNamespaceIndex m_nIndex;
    xstring_ptr_storage m_strNameStorage;

    inline xstring_ptr GetName() const { return xstring_ptr(m_strNameStorage); }
};

struct MetaDataType
{
    KnownTypeIndex            m_nIndex;
    KnownTypeIndex            m_nBaseTypeIndex;
    MetaDataTypeInfoFlags     m_flags;
};

struct MetaDataTypeActivation
{
    CREATEPFN                 m_pfnCreate;
    CREATEPFNFX               m_pfnCreateFramework;
};

struct MetaDataTypeNameInfo
{
    xstring_ptr_storage       m_strNameStorage;
    xstring_ptr_storage       m_strFullNameStorage;
    KnownNamespaceIndex       m_nNamespaceIndex;
};

struct MetaDataTypeProperties
{
    KnownPropertyIndex        m_nFirstPropertyIndex;
    KnownPropertyIndex        m_nContentPropertyIndex;
    UINT16                    m_nFirstEnterPropertyIndex;
    UINT16                    m_nFirstRenderPropertyIndex;
    UINT16                    m_nFirstObjectPropertyIndex;
    UINT8                     m_nPropertySlotCount;
};

struct MetaDataTypeName
{
    KnownTypeIndex            m_nTypeIndex;
};

struct MetaDataProperty
{
    KnownPropertyIndex        m_nIndex;
    KnownTypeIndex            m_nPropertyTypeIndex;
    KnownTypeIndex            m_nDeclaringTypeIndex;
    KnownTypeIndex            m_nTargetTypeIndex;
    MetaDataPropertyInfoFlags m_flags;
};

struct MetaDataDependencyPropertyRuntimeData
{
    union
    {
        // A property either has a backing property method or field.
        METHODPFN             m_pfn;
        UINT16                m_nOffset;
    };
    RENDERCHANGEDPFN          m_pfnNWRenderChangedHandler;
    CREATEGROUPPFN            m_cgpfn;
    UINT16                    m_nGroupOffset;
};

struct MetaDataEnterProperty
{
    UINT16                            m_nOffset;
    UINT16                            m_nGroupOffset;
    MetaDataEnterPropertyInfoFlags    m_flags;
    KnownPropertyIndex                m_nPropertyIndex;
    UINT16                            m_nNextEnterPropertyIndex;
};

struct MetaDataObjectProperty
{
    UINT16                            m_nOffset;
    UINT16                            m_nGroupOffset;
    KnownPropertyIndex                m_nPropertyIndex;
    UINT16                            m_nNextObjectPropertyIndex;
};