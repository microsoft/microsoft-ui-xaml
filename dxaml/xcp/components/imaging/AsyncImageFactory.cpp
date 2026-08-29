// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ImageProviderInterfaces.h"
#include "palgfx.h"
#include "corep.h"
#include "PalWorkItem.h"
#include "ImageProviderInterfaces.h"
#include "SurfaceDecodeParams.h"
#include "ImageCopyParams.h"
#include "ImageTaskDispatcher.h"
#include "AsyncImageFactory.h"
#include "AsyncImageWorkData.h"
#include "AsyncCopyToSurfaceTask.h"

// TODO: Modernize everything in this file.

_Check_return_ HRESULT AsyncImageFactory::CopyAsync(
    _In_ const xref_ptr<ImageCopyParams>& spCopyParams,
    _In_ const xref_ptr<IImageDecodeCallback>& spCallback,
    _Outptr_ IPALWorkItem** ppWorkOutParam
    )
{
    HRESULT hr = S_OK;
    AsyncCopyToSurfaceTask* pTask = NULL;
    AsyncImageWorkData *pData = NULL;
    IPALWorkItem *pWorkItem = NULL;

    IFC(AsyncCopyToSurfaceTask::Create(m_pDispatcher, spCopyParams, spCallback, &pTask));

    pData = new AsyncImageWorkData(pTask, this);

    // If successful, CreateWorkItem will ref pData and hold it in the workitem until that is destroyed
    IFC(m_pWorkItemFactory->CreateWorkItem(&pWorkItem, &AsyncImageFactory::WorkCallback, pData));

    // Submit will add the work item to a queue and ref it again
    IFC(pWorkItem->Submit());

    if (ppWorkOutParam != nullptr)
    {
        SetInterface(*ppWorkOutParam, pWorkItem);
    }

Cleanup:
    ReleaseInterface(pWorkItem);
    ReleaseInterface(pData);
    ReleaseInterface(pTask);

    RRETURN(hr);
}
