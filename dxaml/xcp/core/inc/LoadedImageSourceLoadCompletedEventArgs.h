// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <EventArgs.h>
#include <EnumDefs.g.h>

class CLoadedImageSourceLoadCompletedEventArgs final : public CEventArgs
{
public:
    _Check_return_ HRESULT get_Status(_Out_ DirectUI::LoadedImageSourceLoadStatus* status)
    {
        IFCPTR_RETURN(status);
        *status = m_status;
        return S_OK;
    }

    _Check_return_ HRESULT put_Status(DirectUI::LoadedImageSourceLoadStatus status)
    {
        m_status = status;
        return S_OK;
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

private:
    DirectUI::LoadedImageSourceLoadStatus m_status = DirectUI::LoadedImageSourceLoadStatus::Success;
};
