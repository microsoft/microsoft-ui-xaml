// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <CDependencyObject.h>
#include <UIElement.h>
#include <CControl.h>

#include <corep.h>

#include <GeneralTransform.h>
#include <Framework.h>
#include <RichTextBlockView.h>
#include <DOCollection.h>
#include <Panel.h>
#include <RootVisual.h>

// Text
#include <Inline.h>
#include <FocusableHelper.h>
#include <TextBlock.h>
#include <RichTextBlock.h>
#include <RichTextBlockOverflow.h>

#include "Focusable.h"

// Attach Focusable wrapper to given DO
void Focusable::Attach(_In_ CDependencyObject* obj)
{
    Object = obj;
    if (!obj)
    {
        m_isFrameworkElement = false;
        m_isFocusableElement = false;
        return;
    }

    // A focusable MUST be either a UIElement or a hyperlink.
    // Enforce this here with FAIL_FAST so calling code doesn't have to.
    m_isFrameworkElement = obj->OfTypeByIndex<KnownTypeIndex::FrameworkElement>();
    if (m_isFrameworkElement)
    {
        m_isFocusableElement = false;
    }
    else
    {
        m_isFocusableElement = CFocusableHelper::IsFocusableDO(obj);
        if (!m_isFocusableElement)
        {
            FAIL_FAST_ASSERT(obj->OfTypeByIndex<KnownTypeIndex::UIElement>());
        }
    }
}


// Get bounds for this object.
// Return value is the union of all the bounds
// If multiple bounds are required, multipleBounds is populated with all the bounds rectangles
XRECTF Focusable::GetBounds(_Inout_opt_ std::vector<XRECTF>* multipleBounds) const
{
    if (CFocusableHelper::IsFocusableDO(Object))
    {
        XRECTF totalBounds = {};
        GetBoundsCollectionForFocusable(GuaranteedFocusable(), &totalBounds, multipleBounds);
        return totalBounds;
    }
    else
    {
        return {
            0.0f,
            0.0f,
            AsFrameworkElement()->GetActualWidth(),
            AsFrameworkElement()->GetActualHeight()};
    }
}

// Get the element that's going to draw this element during the renderwalk
CUIElement* Focusable::GetElementResponsibleForDrawingThisElement() const
{
    // This involves some tree-walking, should we cache it?
    if (IsNull())
    {
        return nullptr;
    }

    if (IsFrameworkElement())
    {
        return AsFrameworkElement();
    }

    return  CFocusableHelper::GetContainingFrameworkElementIfFocusable(GuaranteedFocusable());
}

CDependencyObject* Focusable::GuaranteedFocusable() const
{
    FAIL_FAST_ASSERT(IsFocusableElement());
    return Object;
}

CFrameworkElement* Focusable::AsFrameworkElement() const
{
    FAIL_FAST_ASSERT(IsFrameworkElement());
    return static_cast<CFrameworkElement*>(Object);
}

// Hyperlink  may need multiple focus rects.  Return the focus rects in boundsVector.
// totalBounds will be the rectangle that contains all the rects in boundsVector.
/*static*/ void
Focusable::GetBoundsCollectionForFocusable(
    _In_ CDependencyObject* focusable,
    _Out_ XRECTF* totalBounds,
    _Inout_opt_ std::vector<XRECTF>* boundsVector)
{
    ITextView* textView = nullptr;
    CFrameworkElement* container = CFocusableHelper::GetContainingFrameworkElementIfFocusable(focusable);
    XUINT32 focusRectCount = 0;
    XRECTF* focusRects = nullptr;

    if (container->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
    {
        CRichTextBlock* richTextBlock = static_cast<CRichTextBlock*>(container);
        textView = richTextBlock->GetSingleElementTextView();
    }
    else if (container->OfTypeByIndex<KnownTypeIndex::TextBlock>())
    {
        CTextBlock* textBlock = static_cast<CTextBlock*>(container);
        textView = textBlock->GetTextView();
    }
    else if (container->OfTypeByIndex<KnownTypeIndex::RichTextBlockOverflow>())
    {
        CRichTextBlockOverflow* richTextBlockOverflow = static_cast<CRichTextBlockOverflow*>(container);
        textView = richTextBlockOverflow->GetSingleElementTextView();
    }
    else
    {
        IFCFAILFAST(E_FAIL);
    }

    // If the CTextBlock is dirty, it may not have a valid textView yet.  In that case, don't worry about it for now, we'll
    // measure/arrange the CTextBlock in the pending NWDrawTree, and then update the focus rect again.
    // This is inefficient, we should just always wait until the consistent frame to update the focus rect.  (bug 10628954)
    if (textView)
    {
        // If we include non-TextElement IFocusables, this cast will need to change
        IFCFAILFAST(RichTextBlockView::GetBoundsCollectionForElement(textView, static_cast<CTextElement*>(focusable), &focusRectCount, &focusRects));
    }

    if (focusRectCount > 0)
    {
        // The normal case is to have just one focus rect.  If there's just one, the caller doesn't
        // need the vector, it can just use *totalBounds.
        if (focusRectCount == 1)
        {
            *totalBounds = focusRects[0];
        }
        else
        {
            XPOINTF topLeft = { focusRects[0].X, focusRects[0].Y };
            XPOINTF bottomRight = { focusRects[0].Right(), focusRects[0].Bottom() };

            if (boundsVector)
            {
                boundsVector->reserve(focusRectCount);
            }

            for (XUINT32 i = 0; i < focusRectCount; i++)
            {
                const XRECTF& rect = focusRects[i];
                if (!IsEmptyRectF(rect))
                {
                    if (boundsVector)
                    {
                        boundsVector->push_back(rect);
                    }

                    topLeft.x = MIN(topLeft.x, rect.X);
                    topLeft.y = MIN(topLeft.y, rect.Y);
                    bottomRight.x = MAX(bottomRight.x, rect.Right());
                    bottomRight.y = MAX(bottomRight.y, rect.Bottom());
                }
            }
            *totalBounds = { topLeft.x, topLeft.y, bottomRight.x-topLeft.x, bottomRight.y-topLeft.y };
        }
    }
    else
    {
        *totalBounds = {};
    }
    delete[] focusRects;
}

