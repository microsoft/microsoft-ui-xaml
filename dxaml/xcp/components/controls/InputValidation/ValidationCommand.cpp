// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ValidationCommand.h"
#include "Indexes.g.h"
#include "CControl.h"
#include "TypeTableStructs.h"
#include "DOPointerCast.h"

CInputValidationCommand::CInputValidationCommand(_In_ CCoreServices* core)
    : CMultiParentShareableDependencyObject(core)
{
}

_Check_return_ HRESULT CInputValidationCommand::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    switch(args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::InputValidationCommand_InputValidationMode:
        {
            if (args.m_pNewValue->AsEnum() == static_cast<uint32_t>(DirectUI::InputValidationMode::Default))
            {
                EnsureAllErrors();
            }
            else
            {
                DeferAllErrors();
            }
            break;
        }
        case KnownPropertyIndex::InputValidationCommand_InputValidationKind:
        {
            UpdateAllVisuals();
            break;
        }
    }

    return S_OK;
}

void CInputValidationCommand::EnsureAllErrors()
{
    const std::size_t parentCount = GetParentCount();
    for (std::size_t index = 0; index < parentCount; index++)
    {
        if (auto parent = do_pointer_cast<CControl>(GetParentItem(index)))
        {
            parent->EnsureErrors();
        }
    }
}

void CInputValidationCommand::DeferAllErrors()
{
    const std::size_t parentCount = GetParentCount();
    for (std::size_t index = 0; index < parentCount; index++)
    {
        if (auto parent = do_pointer_cast<CControl>(GetParentItem(index)))
        {
            parent->DeferErrors();
        }
    }
}

void CInputValidationCommand::UpdateAllVisuals()
{
    const std::size_t parentCount = GetParentCount();
    for (std::size_t index = 0; index < parentCount; index++)
    {
        if (auto parent = do_pointer_cast<CControl>(GetParentItem(index)))
        {
            parent->EnsureValidationVisuals();
        }
    }
}