// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <core.h>
#include <DOPointerCast.h>
#include <depends.h>
#include <TypeTableStructs.h>
#include <brush.h>
#include <CreateParameters.h>
#include <ErrorHelper.h>
#include "TextRangeCollection.h"
#include "TextHighlighterCollection.h"
#include "TextHighlighter.h"
#include "xcperrorresource.h"

CTextHighlighter::~CTextHighlighter()
{
    if (m_ranges)
    {
        IGNOREHR(m_ranges->Clear());
        VERIFYHR(m_ranges->RemoveParent(this));
    }

    ReleaseInterface(m_ranges);
    ReleaseInterface(m_foreground);
    ReleaseInterface(m_background);
}

_Check_return_ HRESULT
CTextHighlighter::InitInstance()
{
    ASSERT(m_ranges == nullptr);

    xref_ptr<CTextRangeCollection> textRangeCollection;

    CREATEPARAMETERS createParameters(GetContext());
    IFC_RETURN(CreateDO(textRangeCollection.ReleaseAndGetAddressOf(), &createParameters));
    IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::TextHighlighter_Ranges, textRangeCollection.get()));

    return S_OK;
}
_Check_return_ HRESULT
CTextHighlighter::SetValue(
    _In_ const SetValueParams& args
    )
{
    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::TextHighlighter_Foreground:
    case KnownPropertyIndex::TextHighlighter_Background:
        {
            auto dependencyObject = args.m_value.As<valueObject>();

            // Validate that it is a CSolidColorBrush, otherwise fail back to the app with a helpful error
            if ((dependencyObject != nullptr) &&
                !dependencyObject->OfTypeByIndex<KnownTypeIndex::SolidColorBrush>())
            {
                IFC_RETURN(DirectUI::ErrorHelper::OriginateErrorUsingResourceID(
                    E_INVALIDARG,
                    ERROR_TEXTHIGHLIGHTER_NOSOLIDCOLORBRUSH));
            }
        }
        break;
    }

    return __super::SetValue(args);
}

void
CTextHighlighter::InvalidateTextRanges()
{
    auto parent = GetParent();

    if (parent != nullptr)
    {
        if (auto parentCollection = do_pointer_cast<CTextHighlighterCollection>(parent))
        {
            parentCollection->OnCollectionChanged();
        }
        else
        {
            // CTextRangeCollection has unsupported owner.  If CTextRangeCollection is added
            // to new types in the future, it must be appropriate cast here to notify the parent type
            // of collection changes.
            ASSERT(FALSE);
        }
    }
}