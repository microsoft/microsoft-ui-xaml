// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ContentPresenter.g.h"
#include "FrameworkElementAutomationPeer.g.h"
#include "FaceplateContentPresenterAutomationPeer.g.h"
#include "TextBlock.g.h"
#include "PropertyChangedParamsHelper.h"
#include "Binding.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT ContentPresenter::OnContentTemplateChangedImpl(
    _In_ xaml::IDataTemplate* oldContentTemplate,
    _In_ xaml::IDataTemplate* newContentTemplate)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    ctl::ComPtr<IDataTemplate> spContentTemplate;
    ctl::ComPtr<IDataTemplateSelector> spContentTemplateSelector;
    ctl::ComPtr<IInspectable> spContent;

    if (!newContentTemplate)
    {
        IFC(get_ContentTemplateSelector(&spContentTemplateSelector));

        if (spContentTemplateSelector)
        {
            IFC(get_Content(&spContent));
            IFC(spContentTemplateSelector->SelectTemplate(spContent.Get(), this, &spContentTemplate));
        }

        IFC(put_SelectedContentTemplate(spContentTemplate.Get()));
    }

Cleanup:
    RRETURN(S_OK);
}

_Check_return_ HRESULT
ContentPresenter::OnContentTemplateChangedCallback(
    _In_ CDependencyObject* pTarget,
    _In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<ContentPresenter> spContentPresenter;
    ctl::ComPtr<IInspectable> spOldValue, spNewValue;
    ctl::ComPtr<IDataTemplate> spOldContentTemplate;
    ctl::ComPtr<IDataTemplate> spNewContentTemplate;

    IFC(DXamlCore::GetCurrent()->GetPeer(pTarget, &spTarget));
    IFC(spTarget.As(&spContentPresenter));

    IFC(PropertyChangedParamsHelper::GetObjects(args, &spOldValue, &spNewValue));
    IFC(spOldValue.As(&spOldContentTemplate));
    IFC(spNewValue.As(&spNewContentTemplate));

    IFC(spContentPresenter->OnContentTemplateChangedProtected(spOldContentTemplate.Get(), spNewContentTemplate.Get()));

Cleanup:
    return hr;
}

_Check_return_ HRESULT ContentPresenter::OnContentTemplateSelectorChangedImpl(
    _In_ xaml_controls::IDataTemplateSelector* oldContentTemplateSelector,
    _In_ xaml_controls::IDataTemplateSelector* newContentTemplateSelector)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    ctl::ComPtr<IDataTemplate> spContentTemplate;
    ctl::ComPtr<IInspectable> spContent;

    IFC(get_ContentTemplate(&spContentTemplate));
    if (!spContentTemplate)
    {
        if (newContentTemplateSelector)
        {
            IFC(get_Content(&spContent));
            IFC(newContentTemplateSelector->SelectTemplate(spContent.Get(), this, &spContentTemplate));
        }

        IFC(put_SelectedContentTemplate(spContentTemplate.Get()));
    }

Cleanup:
    RRETURN(S_OK);
}

_Check_return_ HRESULT
ContentPresenter::OnContentTemplateSelectorChangedCallback(
    _In_ CDependencyObject* pTarget,
    _In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<ContentPresenter> spContentPresenter;
    ctl::ComPtr<IInspectable> spOldValue, spNewValue;
    ctl::ComPtr<IDataTemplateSelector> spOldContentTemplateSelector;
    ctl::ComPtr<IDataTemplateSelector> spNewContentTemplateSelector;

    IFC(DXamlCore::GetCurrent()->GetPeer(pTarget, &spTarget));
    IFC(spTarget.As(&spContentPresenter));

    IFC(PropertyChangedParamsHelper::GetObjects(args, &spOldValue, &spNewValue));
    IFC(spOldValue.As(&spOldContentTemplateSelector));
    IFC(spNewValue.As(&spNewContentTemplateSelector));

    IFC(spContentPresenter->OnContentTemplateSelectorChangedProtected(spOldContentTemplateSelector.Get(), spNewContentTemplateSelector.Get()));

Cleanup:
    return hr;
}

_Check_return_
HRESULT
ContentPresenter::OnChildrenCleared(
    _In_ CDependencyObject* nativeTarget)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<xaml_controls::IContentPresenter> spContentPresenter;

    IFCPTR(nativeTarget);

    IFC(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &spTarget));
    IFC(spTarget.As(&spContentPresenter));

    IFC(spContentPresenter.Cast<ContentPresenter>()->OnChildrenCleared());

Cleanup:
    RRETURN(hr);
}

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
_Check_return_
HRESULT
ContentPresenter::BindDefaultTextBlock(
    _In_ CTextBlock* pTextBlock,
    _In_opt_ const xstring_ptr *pstrBindingPath)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spTextBlockAsDO;
    ctl::ComPtr<ITextBlock> spTextBlock;
    const CDependencyProperty* pTextProperty = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::TextBlock_Text);
    ctl::ComPtr<Binding> spTextBinding;

    ASSERT(pTextBlock != NULL);
    IFC(DXamlCore::GetCurrent()->GetPeer(pTextBlock, &spTextBlockAsDO));
    IFC(spTextBlockAsDO.As(&spTextBlock));

#if DBG
    {
        ctl::ComPtr<IBindingExpression> spBindingExpression;
        IFC(spTextBlock.Cast<TextBlock>()->GetBindingExpression(pTextProperty, &spBindingExpression));
        ASSERT(!spBindingExpression);
    }
#endif

    IFC(ctl::make<Binding>(&spTextBinding));

    if (pstrBindingPath)
    {
        xruntime_string_ptr strRuntimeBindingPath;
        IFC(pstrBindingPath->Promote(&strRuntimeBindingPath));
        IFC(spTextBinding->SetPathString(strRuntimeBindingPath.GetHSTRING()));
    }

    IFC(spTextBinding->put_Mode(xaml_data::BindingMode_OneWay));
    IFC(spTextBlock.Cast<TextBlock>()->SetBinding(pTextProperty, spTextBinding.Get()));
Cleanup:
    RRETURN(hr);
}

// Create FaceplateContentPresenterAutomationPeer to represent the ContentPresenter when used as Faceplate for ComboBox.
IFACEMETHODIMP ContentPresenter::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
   HRESULT hr = S_OK;

   IFCPTR(ppAutomationPeer);
   *ppAutomationPeer = nullptr;

   if (m_fIsFaceplateForComboBox)
   {
       ctl::ComPtr<FaceplateContentPresenterAutomationPeer> spAutomationPeer;
       IFC(ActivationAPI::ActivateAutomationInstance(KnownTypeIndex::FaceplateContentPresenterAutomationPeer, GetHandle(), spAutomationPeer.GetAddressOf()));
       IFC(spAutomationPeer->put_Owner(this));
       *ppAutomationPeer = spAutomationPeer.Detach();
   }

Cleanup:
    RRETURN(hr);
}
