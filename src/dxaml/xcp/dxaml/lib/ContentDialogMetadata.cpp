// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ContentDialogMetadata.h"
#include "ContentDialog.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT ContentDialogMetadata::AddOpenDialog(_In_ xaml_controls::IContentDialog* dialog)
{
#if DBG
    {
        auto contentDialog = static_cast<ContentDialog*>(dialog);

        ctl::ComPtr<xaml::IDependencyObject> parent;
        if (contentDialog->m_placementMode == ContentDialog::PlacementMode::InPlace)
        {
            IFC_RETURN(contentDialog->get_Parent(&parent));
        }

        bool hasOpenDialog = false;
        IFC_RETURN(HasOpenDialog(parent.Get(), &hasOpenDialog));

        // Should not be adding a dialog if one is already open under a given parent.
        ASSERT(!hasOpenDialog);
    }
#endif

    if (!m_openDialogs)
    {
        ctl::ComPtr<TrackerCollection<xaml_controls::ContentDialog*>> openDialogs;
        IFC_RETURN(ctl::make(&openDialogs));
        SetPtrValue(m_openDialogs, openDialogs.Get());
    }

    IFC_RETURN(m_openDialogs->Append(dialog));

    return S_OK;
}

_Check_return_ HRESULT ContentDialogMetadata::RemoveOpenDialog(_In_ xaml_controls::IContentDialog* dialog)
{
    ASSERT(m_openDialogs);

    unsigned int itemIndex = 0;
    BOOLEAN itemFound = FALSE;
    IFC_RETURN(m_openDialogs->IndexOf(dialog, &itemIndex, &itemFound));

    if (itemFound)
    {
        IFC_RETURN(m_openDialogs->RemoveAt(itemIndex));
    }
    else
    {
        // Calls to RemoveOpenDialog should be paired with a call to AddOpenDialog, so we expect it to be found.
        ASSERT(false);
    }

    return S_OK;
}

_Check_return_ HRESULT ContentDialogMetadata::IsOpen(_In_ xaml_controls::IContentDialog* dialog, _Out_ bool* isOpenDialog)
{
    ASSERT(m_openDialogs);

    *isOpenDialog = false;

    unsigned int itemIndex = 0;
    BOOLEAN itemFound = FALSE;
    IFC_RETURN(m_openDialogs->IndexOf(dialog, &itemIndex, &itemFound));

    *isOpenDialog = !!itemFound;

    return S_OK;
}

_Check_return_ HRESULT ContentDialogMetadata::HasOpenDialog(_In_opt_ xaml::IDependencyObject* parent, _Out_ bool* hasOpenDialog)
{
    *hasOpenDialog = false;

    IFC_RETURN(ForEachOpenDialog([&parent, &hasOpenDialog](xaml_controls::IContentDialog* openDialog, bool& stopIterating)
    {
        auto openContentDialog = static_cast<ContentDialog*>(openDialog);

        // If parent is not null, then we're looking for any open in-place dialogs with that same parent.
        // If it is null, then we're looking for any open dialogs in the popup root.
        if (parent != nullptr)
        {
            if (openContentDialog->m_placementMode == ContentDialog::PlacementMode::InPlace)
            {
                BOOLEAN isAncestorOf = FALSE;
                IFC_RETURN(static_cast<DependencyObject*>(parent)->IsAncestorOf(openContentDialog, &isAncestorOf));
                if (isAncestorOf)
                {
                    *hasOpenDialog = true;
                    stopIterating = true;
                }
            }
        }
        else if (openContentDialog->m_placementMode != ContentDialog::PlacementMode::InPlace)
        {
            *hasOpenDialog = true;
            stopIterating = true;
        }

        return S_OK;
    }));

    return S_OK;
}

_Check_return_ HRESULT ContentDialogMetadata::ForEachOpenDialog(std::function<HRESULT(xaml_controls::IContentDialog*, bool&)> action)
{
    if (m_openDialogs)
    {
        ctl::ComPtr<wfc::IIterator<xaml_controls::ContentDialog*>> iter;
        IFC_RETURN(m_openDialogs->First(&iter));

        BOOLEAN hasCurrent = FALSE;
        IFC_RETURN(iter->get_HasCurrent(&hasCurrent));

        while (hasCurrent)
        {
            ctl::ComPtr<xaml_controls::IContentDialog> current;
            IFC_RETURN(iter->get_Current(&current));

            bool stopIterating = false;
            IFC_RETURN(action(current.Get(), stopIterating));
            if (stopIterating)
            {
                break;
            }

            IFC_RETURN(iter->MoveNext(&hasCurrent));
        }
    }

    return S_OK;
}
