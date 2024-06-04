// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TextBoxView.g.h"
#include "DirectManipulationStateChangeHandler.h"
#include "IScrollOwner.h"
#include "Callback.h"

namespace DirectUI
{
    //---------------------------------------------------------------------------
    //
    //  The FrameworkElement responsible for measuring and rendering and scrolling
    //  the content of a TextBox, RichTextBox, or PasswordBox.
    //
    //---------------------------------------------------------------------------
    PARTIAL_CLASS(TextBoxView)
        , private DirectManipulationStateChangeHandler
    {
    public:
        TextBoxView();

        // IScrollInfo implementation.
        _Check_return_ HRESULT get_CanVerticallyScrollImpl(_Out_ BOOLEAN *pCanVerticallyScroll);
        _Check_return_ HRESULT put_CanVerticallyScrollImpl(_In_ BOOLEAN canVerticallyScroll);
        _Check_return_ HRESULT get_CanHorizontallyScrollImpl(_Out_ BOOLEAN *pCanHorizontallyScroll);
        _Check_return_ HRESULT put_CanHorizontallyScrollImpl(_In_ BOOLEAN canHorizontallyScroll);
        _Check_return_ HRESULT get_ExtentWidthImpl(_Out_ DOUBLE *pExtentWidth);
        _Check_return_ HRESULT get_ExtentHeightImpl(_Out_ DOUBLE *pExtentHeight);
        _Check_return_ HRESULT get_ViewportWidthImpl(_Out_ DOUBLE *pViewportWidth);
        _Check_return_ HRESULT get_ViewportHeightImpl(_Out_ DOUBLE *pViewportHeight);
        _Check_return_ HRESULT get_HorizontalOffsetImpl(_Out_ DOUBLE *pHorizontalOffset);
        _Check_return_ HRESULT get_VerticalOffsetImpl(_Out_ DOUBLE *pVerticalOffset);
        _Check_return_ HRESULT get_MinHorizontalOffsetImpl(_Out_ DOUBLE *pMinHorizontalOffset);
        _Check_return_ HRESULT get_MinVerticalOffsetImpl(_Out_ DOUBLE *pMinVerticalOffset);
        _Check_return_ HRESULT get_ScrollOwnerImpl(_Outptr_ IInspectable **ppScrollOwner);
        _Check_return_ HRESULT put_ScrollOwnerImpl(_In_opt_ IInspectable *pScrollOwner);
        _Check_return_ HRESULT LineUpImpl();
        _Check_return_ HRESULT LineDownImpl();
        _Check_return_ HRESULT LineLeftImpl();
        _Check_return_ HRESULT LineRightImpl();
        _Check_return_ HRESULT PageUpImpl();
        _Check_return_ HRESULT PageDownImpl();
        _Check_return_ HRESULT PageLeftImpl();
        _Check_return_ HRESULT PageRightImpl();
        _Check_return_ HRESULT SetHorizontalOffsetImpl(_In_ DOUBLE offset);
        _Check_return_ HRESULT SetVerticalOffsetImpl(_In_ DOUBLE offset);
        _Check_return_ HRESULT MakeVisibleImpl(
            _In_ IUIElement *visual, 
            wf::Rect targetRect, 
            BOOLEAN useAnimation,
            DOUBLE horizontalAlignmentRatio,
            DOUBLE verticalAlignmentRatio,
            DOUBLE offsetX,
            DOUBLE offsetY,
            _Out_ wf::Rect *resultRectangle,
            _Out_opt_ DOUBLE* appliedOffsetX = nullptr,
            _Out_opt_ DOUBLE* appliedOffsetY = nullptr);

        IFACEMETHOD(MouseWheelUp)(_In_ UINT mouseWheelDelta) override;
        IFACEMETHOD(MouseWheelDown)(_In_ UINT mouseWheelDelta) override;
        IFACEMETHOD(MouseWheelLeft)(_In_ UINT mouseWheelDelta) override;
        IFACEMETHOD(MouseWheelRight)(_In_ UINT mouseWheelDelta) override;

        // DirectManipulationStateChangeHandler overrides
        _Check_return_ HRESULT NotifyStateChange(
            _In_ DMManipulationState state,
            _In_ FLOAT xCumulativeTranslation,
            _In_ FLOAT yCumulativeTranslation,
            _In_ FLOAT zCumulativeFactor,
            _In_ FLOAT xCenter,
            _In_ FLOAT yCenter,
            _In_ BOOLEAN isInertial,
            _In_ BOOLEAN isTouchConfigurationActivated,
            _In_ BOOLEAN isBringIntoViewportConfigurationActivated
            ) override;

        static _Check_return_ HRESULT NotifyOffsetsChanging(
            _In_ CDependencyObject* textBoxView,
            XDOUBLE oldHorizontalOffset,
            XDOUBLE newHorizontalOffset,
            XDOUBLE oldVerticalOffset,
            XDOUBLE newVerticalOffset);
        // Callback from core requesting an InvalidateScrollInfo call on m_pScrollOwnerWeakRef.
        static _Check_return_ HRESULT InvalidateScrollInfo(_In_ CDependencyObject *pTextBoxView);
        static _Check_return_ HRESULT CaretChanged(_In_ CDependencyObject* pNativeTextBoxView);
        _Check_return_ HRESULT CaretChangedImpl();

        static _Check_return_ HRESULT CaretVisibilityChanged(_In_ CDependencyObject* pNativeTextBoxView);
        _Check_return_ HRESULT CaretVisibilityChangedImpl();

        static _Check_return_ HRESULT InvalidateView(_In_ CDependencyObject* pNativeTextBoxView);
        _Check_return_ HRESULT InvalidateViewImpl();

    protected:
        ~TextBoxView() override;

    private:
        _Check_return_ HRESULT GetScrollOwnerFlowDirection(xaml::FlowDirection* pValue);
        
        // Check whether we should defer our scroll wheel handling to scroll owner.
        _Check_return_ HRESULT get_ShouldPassWheelMessageToScrollOwner(_Out_ BOOLEAN &shouldPass);
            
        // Allow our scroll owner to handle the last mouse wheel message.
        _Check_return_ HRESULT PassWheelMessageToScrollOwner(_In_ ZoomDirection zoomDirection);

        _Check_return_ HRESULT TextBoxViewScroll(
            _In_ CTextBoxView_ScrollCommand command,
            _In_ XUINT32 mouseWheelDelta = 0,
            _Out_opt_ bool *pScrolled = NULL);

        typedef _Check_return_ HRESULT (TextBoxView::*CoreDispatcherTextBoxViewCallbackHandler)();
        static _Check_return_ HRESULT PostDispatcherCallback(
            _In_ CDependencyObject* pNativeTextBoxView,
            _In_ CoreDispatcherTextBoxViewCallbackHandler pfnEventHandler);

        // The containing scroll owner, or NULL if there is none.
        IWeakReference* m_pScrollOwnerWeakRef;
    };

    template <>
    struct WeakRefTypeTraits<TextBoxView>
    {
        typedef xaml::IFrameworkElement ResolveType;
    };
}
