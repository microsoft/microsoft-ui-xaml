// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ContentPresenter.g.h"

namespace DirectUI
{
    // ContentPresenter is used within the template of a content control to denote the
    // place in the control's visual tree (control template) where the content
    // is to be added.
    PARTIAL_CLASS(ContentPresenter)
    {
    public:
       // Virtual methods.
        _Check_return_ HRESULT OnContentTemplateChangedImpl(
            _In_ xaml::IDataTemplate* oldContentTemplate, 
            _In_ xaml::IDataTemplate* newContentTemplate);

        _Check_return_ HRESULT OnContentTemplateSelectorChangedImpl(
            _In_ xaml_controls::IDataTemplateSelector* oldContentTemplateSelector, 
            _In_ xaml_controls::IDataTemplateSelector* newContentTemplateSelector);

        static _Check_return_ HRESULT OnChildrenCleared(
            _In_ CDependencyObject* nativeTarget);

        //------------------------------------------------------------------------
        //
        //  Method: BindDefaultTextBlock
        //
        //  Synopsis:
        //      Create the Binding for the TextBlock contained in the default template
        //
        //      The Target Template is
        //
        //      <Grid>
        //        <TextBlock Text="{Binding ZZZ}" HorizontalAlignment="Left" VerticalAlignment="Top" />
        //      </Grid>
        //      or
        //        <TextBlock Text="{Binding ZZZ}" HorizontalAlignment="Left" VerticalAlignment="Top" />
        //
        //      depending on a quirk
        //
        //      where ZZZ can be NULL for "standalone" ContentPresenter
        //      or the DisplayMemberPath when used in a list control
        //
        //------------------------------------------------------------------------
        static _Check_return_ HRESULT BindDefaultTextBlock(_In_ CTextBlock* pTextBlock, _In_opt_ const xstring_ptr *pstrBindingPath);

        static _Check_return_ HRESULT OnContentTemplateChangedCallback(
            _In_ CDependencyObject* pTarget,
            _In_ const PropertyChangedParams& args);

        static _Check_return_ HRESULT OnContentTemplateSelectorChangedCallback(
            _In_ CDependencyObject* pTarget,
            _In_ const PropertyChangedParams& args);

        void SetFaceplateForComboBox()
        {
            m_fIsFaceplateForComboBox = TRUE;
        }

    protected:

        // c~tor
        ContentPresenter():
            m_fIsFaceplateForComboBox(FALSE)
        {}

        virtual _Check_return_ HRESULT OnChildrenCleared()
        {
            RRETURN(S_OK);
        }

        // Override OnCreateAutomationPeer()
        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);

    private:
        BOOLEAN m_fIsFaceplateForComboBox;
    };
}
