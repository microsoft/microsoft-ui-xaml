// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TrackerCollections.h"

namespace DirectUI
{
    class ContentDialogMetadata
        : public ctl::WeakReferenceSource
    {
    public:
        _Check_return_ HRESULT AddOpenDialog(_In_ xaml_controls::IContentDialog* dialog);

        _Check_return_ HRESULT RemoveOpenDialog(_In_ xaml_controls::IContentDialog* dialog);

        _Check_return_ HRESULT IsOpen(_In_ xaml_controls::IContentDialog* dialog, _Out_ bool* isOpenDialog);

        // Passing in nullptr for parent will query whether there is an open dialog under the popup root.
        _Check_return_ HRESULT HasOpenDialog(_In_opt_ xaml::IDependencyObject* parent, _Out_ bool* hasOpenDialog);

        _Check_return_ HRESULT ForEachOpenDialog(std::function<HRESULT(xaml_controls::IContentDialog*, bool&)> action);

    private:
        TrackerPtr<TrackerCollection<xaml_controls::ContentDialog*>> m_openDialogs;
    };
}
