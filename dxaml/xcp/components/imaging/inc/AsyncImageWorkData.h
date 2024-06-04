// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <AsyncImageFactory.h>

struct IImageTask;

class AsyncImageWorkData : public CXcpObjectBase < IObject >
{
public:
    AsyncImageWorkData(
        _In_ IImageTask* pTask,
        _In_ AsyncImageFactory *pImageFactory
        )
    {
        SetInterface(m_pTask, pTask);
        SetInterface(m_pImageFactory, pImageFactory);
    }

    ~AsyncImageWorkData() override
    {
        ReleaseInterface(m_pTask);
        ReleaseInterface(m_pImageFactory);
    }

    IImageTask *m_pTask;
    AsyncImageFactory *m_pImageFactory;
};