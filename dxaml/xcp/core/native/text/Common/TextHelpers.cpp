// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextHelpers.h"

#include "Glyphs.h"

//---------------------------------------------------------------------------
//
//  Initializes ID2D1SolidColorBrush from CBrush.
//
//---------------------------------------------------------------------------
void InitializeD2DBrush(
    _In_ CBrush *pBrush,
    _Inout_ ID2D1SolidColorBrush *pD2DSolidColorBrush
    )
{
    if (pBrush->GetTypeIndex() == KnownTypeIndex::SolidColorBrush)
    {
        CSolidColorBrush *pSolidColorBrush = static_cast<CSolidColorBrush *>(pBrush);
        pD2DSolidColorBrush->SetColor(&D2D1::ColorF(pSolidColorBrush->m_rgb));
    }
    else
    {
        pD2DSolidColorBrush->SetColor(&D2D1::ColorF(D2D1::ColorF::Black));
    }
}

//---------------------------------------------------------------------------
//
//  Get Foreground brush from the property source.
//
//---------------------------------------------------------------------------
CBrush * GetForegroundBrush(
    _In_ const xref::weakref_ptr<CDependencyObject>& pBrushSource,
    _In_opt_ CBrush *pBrush,
    bool controlEnabled
    )
{
    CBrush *pBrushNoRef = nullptr;

    if (pBrush == nullptr)
    {
        ASSERT(pBrushSource);

        CDependencyObject* pDOBrushSource = pBrushSource.lock();
        if (pDOBrushSource != nullptr)
        {
            if (pDOBrushSource->OfTypeByIndex<KnownTypeIndex::Glyphs>())
            {
                pBrushNoRef = static_cast<CGlyphs*>(pDOBrushSource)->m_pFill;
            }
            else
            {
                const TextFormatting *pTextFormatting;
                HRESULT hr = pDOBrushSource->GetTextFormatting(&pTextFormatting);
                if (SUCCEEDED(hr) && pTextFormatting != nullptr)
                {
                    pBrushNoRef = pTextFormatting->m_pForeground;
                }
            }
        }
    }
    else
    {
        // Brush is only set when BackPlate is enabled. This brush is set to a color that ensures HighContrast with the BackPlate. Due to the override
        // we need to provide the SystemColorDisabled brush if the control is not enabled, otherwise it will render as an active control.
        if (controlEnabled)
        {
            pBrushNoRef = pBrush;
        }
        else
        {
            ASSERT(pBrushSource);

            CDependencyObject* pDOBrushSource = pBrushSource.lock();

            if (pDOBrushSource != nullptr)
            {
                pBrushNoRef = pDOBrushSource->GetContext()->GetSystemColorDisabledTextBrushNoRef();
            }
            else
            {
                pBrushNoRef = pBrush;
            }
        }
    }

    return pBrushNoRef;
}

//---------------------------------------------------------------------------
//
//  Determines whether brush is explicitly set or inherited from the parent FE.
//
//---------------------------------------------------------------------------
bool HasExplicitForegroundBrush(
    _In_ const xref::weakref_ptr<CDependencyObject>& pBrushSource,
    _In_opt_ CBrush *pBrush
    )
{
    bool hasExplicitBrush = (pBrush != nullptr);

    if (!hasExplicitBrush && pBrushSource)
    {
        CDependencyObject* pDOBrushSource = pBrushSource.lock();
        CDependencyObject* pInheritanceParent;
        TextFormatting **ppLocalTextFormatting = NULL;
        TextFormatting **ppParentTextFormatting;

        if (S_OK == pDOBrushSource->EnsureTextFormattingForRead())
        {
            // We can assume TextFormattingMember is a valid pointer since TextElements and
            // their hosts have it always non-NULL.
            ppLocalTextFormatting = pDOBrushSource->GetTextFormattingMember();
            ASSERT(ppLocalTextFormatting != NULL && *ppLocalTextFormatting != NULL && !(*ppLocalTextFormatting)->IsOld());

            // If the brush source is provided and it is a TextElement, walk the parent chain up to
            // the first UIElement (TextBlock or RichTextBlock) and check if the foreground property is inherited from there.
            // If not, we are dealing with explicit brush set inside TextBlock�s or RichTextBlock�s content.
            while (pDOBrushSource != NULL && pDOBrushSource->OfTypeByIndex<KnownTypeIndex::TextElement>())
            {
                pInheritanceParent = pDOBrushSource->GetParentInternal(false);
                ASSERT(pInheritanceParent != NULL);

                // Parent of TextElements is TextElementCollection, so need to skip those during the walk,
                // since collection itself cannot have any properties set.
                if (pInheritanceParent->OfTypeByIndex<KnownTypeIndex::TextElementCollection>())
                {
                    pInheritanceParent = pInheritanceParent->GetParentInternal(false);
                    ASSERT(pInheritanceParent != NULL);
                }

                ppParentTextFormatting = pInheritanceParent->GetTextFormattingMember();
                if (ppParentTextFormatting != NULL && *ppParentTextFormatting != NULL)
                {
                    // We can assume that TextFormatting is up to date since Foreground property
                    // was retrieved before we got to this point and it guarantees that TextFormatting is up to date.
                    ASSERT(!(*ppParentTextFormatting)->IsOld());

                    if ((*ppLocalTextFormatting)->m_pForeground != (*ppParentTextFormatting)->m_pForeground)
                    {
                        hasExplicitBrush = TRUE;
                        break;
                    }
                }

                pDOBrushSource = pInheritanceParent;
            }
        }
    }

    return hasExplicitBrush;
}

