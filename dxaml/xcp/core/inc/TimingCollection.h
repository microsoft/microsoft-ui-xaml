// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DOCollection.h"

class CTimeline;

class CTimingCollection : public CDOCollection
{
public:
    CTimingCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
    {}

    void SetTimingOwner( _In_ CTimeline* pTimingOwner );

    // CDOCollection overrides to ensure that Timing tree can be modified.
    _Check_return_ HRESULT Clear() final;
    _Check_return_ HRESULT Append(_In_ CDependencyObject *pObject, _Out_opt_ XUINT32 *pnIndex = NULL) override;
    _Check_return_ HRESULT Insert(_In_ XUINT32 nIndex, _In_ CDependencyObject *pObject) final;
    _Check_return_ void *RemoveAt(_In_ XUINT32 nIndex) final;

private:
    _Check_return_ HRESULT CheckCanBeModified();

protected:
    CTimeline* m_pTimingOwner = nullptr;
};
