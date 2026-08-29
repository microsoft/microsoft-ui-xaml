// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      The purpose of this class is to wrap a DO. It's use at the time
//      of writing is to work around the single-parent restriction on DOs.
//
//      e.g.: In control templates when we need to place an already-parented DO
//      in a temporary ResourceDictionary used for the template content.

#pragma once
#include "NoParentShareableDependencyObject.h"

class CDependencyObjectWrapper final : public CNoParentShareableDependencyObject
{
private:
    CDependencyObject *m_pdoWrapped = nullptr;

protected:
    CDependencyObjectWrapper(CCoreServices *pCore)
        : CNoParentShareableDependencyObject(pCore)
    {}

public:
    DECLARE_CREATE(CDependencyObjectWrapper);

    ~CDependencyObjectWrapper() override
    {
        ReleaseInterface(m_pdoWrapped);
    }

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CDependencyObjectWrapper>::Index;
    }

    // CDependencyObjectWrapper
    _Check_return_ HRESULT SetWrappedDO(CDependencyObject *pdoWrapped)
    {
        IFCEXPECT_RETURN(m_pdoWrapped == NULL);
        m_pdoWrapped = pdoWrapped;
        AddRefInterface(m_pdoWrapped);

        return S_OK;
    }

    CDependencyObject *WrappedDO()
    {
        return m_pdoWrapped;
    }
};
