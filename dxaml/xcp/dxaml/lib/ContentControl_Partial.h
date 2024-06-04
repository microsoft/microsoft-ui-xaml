// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ContentControl.g.h"

namespace DirectUI
{
    /// <summary>
    /// Represents a control with a single piece of content.
    /// </summary>
    /// <remarks>
    /// A ContentControl has a limited default style.  If you want to enhance
    /// the appearance of the control, you can create a new control template
    /// that uses &lt;ContentPresenter Content="{TemplateBinding Content}" /&gt;
    /// to display the value of the Content property.
    /// </remarks>
    PARTIAL_CLASS(ContentControl)
    {
    public:
        // Provides the behavior for the Measure pass of layout. Classes can
        // override this method to define their own Measure pass behavior.
        IFACEMETHOD(MeasureOverride)(
            // Measurement constraints, a control cannot return a size
            // larger than the constraint.
            _In_ wf::Size availableSize,
            // The desired size of the control.
            _Out_ wf::Size* pReturnValue) override;

        // Provides the behavior for the Arrange pass of layout.  Classes
        // can override this method to define their own Arrange pass
        // behavior.
        IFACEMETHOD(ArrangeOverride)(
            // The computed size that is used to arrange the content.
            _In_ wf::Size arrangeSize,
            // The size of the control.
            _Out_ wf::Size* returnValue) override;

       // Virtual methods.
        _Check_return_ HRESULT OnContentChangedImpl(
            _In_ IInspectable* oldContent,
            _In_ IInspectable* newContent);

        _Check_return_ HRESULT OnContentTemplateChangedImpl(
            _In_ xaml::IDataTemplate* oldContentTemplate,
            _In_ xaml::IDataTemplate* newContentTemplate);

        _Check_return_ HRESULT OnContentTemplateSelectorChangedImpl(
            _In_ xaml_controls::IDataTemplateSelector* oldContentTemplateSelector,
            _In_ xaml_controls::IDataTemplateSelector* newContentTemplateSelector);

        // Protected methods.
        _Check_return_ HRESULT SetContentIsNotLogical();

        _Check_return_ HRESULT GetPlainText(_Out_ HSTRING* strPlainText) override;

        using ContentControlGenerated::OnContentChanged;

        static _Check_return_ HRESULT OnContentChangedCallback(
            _In_ CDependencyObject* nativeTarget,
            _In_ CValue* oldContentValue,
            _In_ CValue* newContentValue,
            _In_opt_ IInspectable* pValueOuter);

        _Check_return_ HRESULT get_ContentTemplateRootImpl(_Outptr_ xaml::IUIElement** pValue);

        _Check_return_ HRESULT GetGlobalBoundsImpl(
            _Out_ wf::Rect* pReturnValue);

        _Check_return_ HRESULT GetRasterizationScaleImpl(
            _Out_ FLOAT* returnValue);

    protected:
        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;


        IFACEMETHOD(OnDisconnectVisualChildren)() override;

    private:
        _Check_return_ HRESULT OnContentTemplateChanged(
            _In_ IInspectable* pOldValue,
            _In_ IInspectable* pNewValue);

        _Check_return_ HRESULT OnContentTemplateSelectorChanged(
            _In_ IInspectable* pOldValue,
            _In_ IInspectable* pNewValue);

        _Check_return_ HRESULT RefreshSelectedTemplate(
            _In_ xaml_controls::IDataTemplateSelector* pContentTemplateSelector,
            _In_opt_ IInspectable* pContent,
            _In_ BOOLEAN reloadContent,
            _Outptr_ xaml::IDataTemplate** ppContentTemplate);
    };
}
