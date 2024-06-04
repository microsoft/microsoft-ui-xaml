// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "MultiParentShareableDependencyObject.h"
#include "DependencyObjectTraits.h"
#include "DependencyObjectTraits.g.h"
#include "xref_ptr.h"
#include "EnumDefs.g.h"
#include "Template.h"

class CInputValidationCommand : public CMultiParentShareableDependencyObject
{
public:

    CInputValidationCommand(_In_ CCoreServices* core);

    DECLARE_CREATE(CInputValidationCommand);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CInputValidationCommand>::Index;
    }

    DirectUI::InputValidationKind m_inputValidationKind = DirectUI::InputValidationKind::Auto;
    DirectUI::InputValidationMode m_inputValidationMode = DirectUI::InputValidationMode::Default;

    _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) override;

private:
    void EnsureAllErrors();
    void DeferAllErrors();
    void UpdateAllVisuals();
};