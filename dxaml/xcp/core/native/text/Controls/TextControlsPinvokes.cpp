// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

namespace CoreImports
{
    //---------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Returns extent, viewport, and scrollbar offsets required by the fx
    //      ScrollViewer.
    //  
    //---------------------------------------------------------------------------
    void TextBoxView_GetScrollData(
        _In_ CTextBoxView *pTextBoxView, 
        _Outptr_ const CTextBoxView_ScrollData **ppScrollData
        )
    {
        *ppScrollData = pTextBoxView->GetScrollData();
    }

    //---------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Enables or disables scrolling.
    //  
    //---------------------------------------------------------------------------
    void TextBoxView_SetScrollEnabled(
        _In_ CTextBoxView *pTextBoxView,
        _In_ bool canHorizontallScroll,
        _In_ bool canVerticallyScroll
        )
    {
        pTextBoxView->SetScrollEnabled(canHorizontallScroll, canVerticallyScroll);
    }

    //---------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Scrolls the viewport by a particular unit in a particular direction.
    //  
    //---------------------------------------------------------------------------
    _Check_return_ HRESULT TextBoxView_Scroll(
        _In_ CTextBoxView *pTextBoxView,
        _In_ CTextBoxView_ScrollCommand command,
        _In_ XUINT32 mouseWheelDelta,
        _Out_opt_ bool *pScrolled)
    {
        return pTextBoxView->Scroll(command, FALSE /*moveCaret*/, FALSE /*expandSelection*/, mouseWheelDelta, pScrolled);
    }

    //---------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Scrolls the viewport to a given offset.
    //  
    //---------------------------------------------------------------------------
    _Check_return_ HRESULT TextBoxView_SetScrollOffsets(
        _In_ CTextBoxView *pTextBoxView,
        _In_ XDOUBLE horizontalOffset,
        _In_ XDOUBLE verticalOffset
        )
    {
        return pTextBoxView->SetScrollOffsets(static_cast<XFLOAT>(horizontalOffset), static_cast<XFLOAT>(verticalOffset));
    }

    //---------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Direct manipulation started event.
    //
    //---------------------------------------------------------------------------
    void TextBoxView_DirectManipulationStarted(
        _In_ CTextBoxView *pTextBoxView
        )
    {
        pTextBoxView->OnDirectManipulationStarted();
    }

    //---------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Direct manipulation started event.
    //
    //---------------------------------------------------------------------------
    void TextBoxView_DirectManipulationCompleted(
        _In_ CTextBoxView *pTextBoxView
        )
    {
        pTextBoxView->OnDirectManipulationCompleted();
    }

    //---------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Direct manipulation delta event.
    //
    //---------------------------------------------------------------------------
    void TextBoxView_DirectManipulationDelta(
        _In_ CTextBoxView *pTextBoxView,
        FLOAT xCumulativeTranslation,
        FLOAT yCumulativeTranslation,
        FLOAT zCumulativeFactor
        )
    {
        pTextBoxView->OnDirectManipulationDelta(xCumulativeTranslation, yCumulativeTranslation, zCumulativeFactor);
    }

    //---------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Dispatcher callback to update the caret element
    //
    //---------------------------------------------------------------------------
    void TextBoxView_CaretChanged(
        _In_ CTextBoxView* pTextBoxView)
    {
        pTextBoxView->CaretChanged();
    }

    //------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Dispatcher callback to update the caret element visibility
    //
    //------------------------------------------------------------------------
    void TextBoxView_CaretVisibilityChanged(
        _In_ CTextBoxView* pTextBoxView)
    {
        pTextBoxView->CaretVisibilityChanged();
    }

    //---------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Dispatcher callback to invalidate the view
    //
    //---------------------------------------------------------------------------
    void TextBoxView_InvalidateView(
        _In_ CTextBoxView* pTextBoxView)
    {
        pTextBoxView->InvalidateViewDispatcherCallback();
    }

    //---------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Notifies a CTextBox that its templated DeleteButton has been clicked.
    //  
    //---------------------------------------------------------------------------
    _Check_return_ HRESULT TextBox_OnDeleteButtonClick(_In_ CTextBox *pTextBox)
    {
        return pTextBox->OnDeleteButtonClick();
    }

    //---------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Gets the inner ITextDocument for a RichEditBox.
    //  
    //---------------------------------------------------------------------------
    _Check_return_ HRESULT RichEditBox_GetDocument(
        _In_ void *pObject,
        _In_ REFIID iid,
        _Outptr_ IUnknown **ppInterface)
    {
        CRichEditBox *pRichEditBox = reinterpret_cast<CRichEditBox *>(pObject);
        return pRichEditBox->GetDocument(iid, ppInterface);
    }
}