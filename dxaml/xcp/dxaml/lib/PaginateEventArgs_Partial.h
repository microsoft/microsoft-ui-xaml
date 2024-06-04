// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PaginateEventArgs.g.h"
#include <fwd/windows.graphics.h>

namespace DirectUI
{
    PARTIAL_CLASS(PaginateEventArgs)
    {
        friend class PrintDocument;

        protected:
            PaginateEventArgs();
            ~PaginateEventArgs() override;

        public:
            _Check_return_ HRESULT get_PrintTaskOptionsImpl(_Outptr_ wgr::Printing::IPrintTaskOptionsCore** pValue);
            _Check_return_ HRESULT put_PrintTaskOptionsImpl(_In_ wgr::Printing::IPrintTaskOptionsCore* value);
            _Check_return_ HRESULT get_CurrentPreviewPageNumberImpl(_Out_ INT* pValue);
            _Check_return_ HRESULT put_CurrentPreviewPageNumberImpl(_In_ INT value);

        private:
            wgr::Printing::IPrintTaskOptionsCore* m_pPrintTaskOptionsCore;
            INT m_pageNumber;
    };
}
