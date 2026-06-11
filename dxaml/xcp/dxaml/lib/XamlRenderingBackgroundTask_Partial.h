// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Provides a base class for background tasks which 
//      need XAML framework.

#pragma once

#include "XamlRenderingBackgroundTask.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(XamlRenderingBackgroundTask),
          public wab::IBackgroundTask
    {
        friend class XamlRenderingBackgroundTaskFactory;

        // Implement the IBackgroundTask type. This is cloaked
        // from the code generation so that app implementations of 
        // XamlRenderingBackgroundTasks don't find it by default.
        BEGIN_INTERFACE_MAP(XamlRenderingBackgroundTask, XamlRenderingBackgroundTaskGenerated)
            INTERFACE_ENTRY(XamlRenderingBackgroundTask, wab::IBackgroundTask)
        END_INTERFACE_MAP(XamlRenderingBackgroundTask, XamlRenderingBackgroundTaskGenerated)

    protected:
        XamlRenderingBackgroundTask();
        ~XamlRenderingBackgroundTask() override;

        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    public:
        IFACEMETHOD(Run)(
            _In_ wab::IBackgroundTaskInstance* pTaskInstance) override;
        _Check_return_ HRESULT OnRunImpl(
            _In_ wab::IBackgroundTaskInstance* pTaskInstance);
    };
}
