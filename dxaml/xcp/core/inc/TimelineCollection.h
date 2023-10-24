// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TimingCollection.h"

class CTimelineCollection final : public CTimingCollection
{
private:
    CTimelineCollection(_In_ CCoreServices *pCore)
        : CTimingCollection(pCore)
    {}

public:
#if defined(__XAML_UNITTESTS__)
    CTimelineCollection() // !!! FOR UNIT TESTING ONLY !!!
        : CTimelineCollection(nullptr)
    {}
#endif

    DECLARE_CREATE(CTimelineCollection);

    KnownTypeIndex GetTypeIndex() const override;

    CDependencyObject* GetStandardNameScopeParent() override;

    bool ShouldEnsureNameResolution() override { return true; }

    _Check_return_ HRESULT Append(_In_ CDependencyObject *pObject, _Out_opt_ XUINT32 *pnIndex = NULL) override;

    _Check_return_ HRESULT OnAddToCollection(CDependencyObject *pDO) override;

    _Check_return_ HRESULT CycleCheck(_In_ CDependencyObject *pObject) override;
};
