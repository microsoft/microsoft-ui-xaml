// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <elements.h>

class CSemanticZoom final : public CControl
{
protected:
    CSemanticZoom(_In_ CCoreServices *pCore)
        : CControl(pCore)
    {
        SetIsCustomType();
    }

    ~CSemanticZoom() override
    {
    }

public:
    DECLARE_CREATE(CSemanticZoom);

    KnownTypeIndex GetTypeIndex() const final
    {
        return KnownTypeIndex::SemanticZoom;
    }

    XUINT32 ParticipatesInManagedTreeInternal() final
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }

protected:
    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params) final;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) final;

private:

    CDependencyObject* GetSparseValue(KnownPropertyIndex propertyIndex) const
    {
        if (m_pValueTable)
        {
            auto& value = (*m_pValueTable)[propertyIndex];
            return value.value.AsObject();
        }

        return nullptr;
    }

    CDependencyObject* GetZoomOutView() const
    {
        return GetSparseValue(KnownPropertyIndex::SemanticZoom_ZoomedOutView);
    }

    CDependencyObject* GetZoomInView() const
    {
        return GetSparseValue(KnownPropertyIndex::SemanticZoom_ZoomedInView);
    }
};

