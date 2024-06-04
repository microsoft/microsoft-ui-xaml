// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ListViewItemPresenter.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

using namespace Microsoft::WRL;

_Check_return_ HRESULT ListViewItemPresenter::GoToElementStateCoreImpl(
    _In_ HSTRING stateName,
    _In_ BOOLEAN useTransitions,
    _Out_ BOOLEAN* returnValue)
{
    CListViewBaseItemChrome* pChrome = static_cast<CListViewBaseItemChrome*>(GetHandle());
    bool wentToState = false;

    *returnValue = FALSE;

#ifdef DBG
    if (!m_roundedListViewBaseItemChromeFallbackColorsSet)
    {
        m_roundedListViewBaseItemChromeFallbackColorsSet = true;
        SetRoundedListViewBaseItemChromeFallbackColors();
    }
#endif // DBG

    IFC_RETURN(pChrome->GoToChromedState(
        HStringUtil::GetRawBuffer(stateName, nullptr),
        useTransitions,
        &wentToState));

    IFC_RETURN(ProcessAnimationCommands());

    *returnValue = !!wentToState;

    IFC_RETURN(__super::GoToElementStateCoreImpl(stateName, useTransitions, returnValue));

    return S_OK;
}

_Check_return_ HRESULT ListViewItemPresenter::get_ListViewItemPresenterPaddingImpl(_Out_ xaml::Thickness* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::ContentPresenter_Padding, pValue));
}
_Check_return_ HRESULT ListViewItemPresenter::put_ListViewItemPresenterPaddingImpl(_In_ xaml::Thickness value)
{
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::ContentPresenter_Padding, value));
}
_Check_return_ HRESULT ListViewItemPresenter::get_ListViewItemPresenterHorizontalContentAlignmentImpl(_Out_ xaml::HorizontalAlignment* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::ContentPresenter_HorizontalContentAlignment, pValue));
}
_Check_return_ HRESULT ListViewItemPresenter::put_ListViewItemPresenterHorizontalContentAlignmentImpl(_In_ xaml::HorizontalAlignment value)
{
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::ContentPresenter_HorizontalContentAlignment, value));
}
_Check_return_ HRESULT ListViewItemPresenter::get_ListViewItemPresenterVerticalContentAlignmentImpl(_Out_ xaml::VerticalAlignment* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::ContentPresenter_VerticalContentAlignment, pValue));
}
_Check_return_ HRESULT ListViewItemPresenter::put_ListViewItemPresenterVerticalContentAlignmentImpl(_In_ xaml::VerticalAlignment value)
{
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::ContentPresenter_VerticalContentAlignment, value));
}

#ifdef DBG
void ListViewItemPresenter::SetRoundedListViewBaseItemChromeFallbackColor(KnownPropertyIndex propertyIndex, XUINT32 color)
{
    DXamlCore* core = DXamlCore::GetCurrent();
    CREATEPARAMETERS cp(core->GetHandle());
    xref_ptr<CSolidColorBrush> fallbackSolidColorBrush = nullptr;
    CValue valColor, valBrush;

    IGNOREHR(GetValueByKnownIndex(propertyIndex, valBrush));
    if (valBrush.IsNull())
    {
        IGNOREHR(CSolidColorBrush::Create(reinterpret_cast<CDependencyObject**>(fallbackSolidColorBrush.ReleaseAndGetAddressOf()), &cp));
        if (fallbackSolidColorBrush)
        {
            valColor.SetColor(color);
            IGNOREHR(fallbackSolidColorBrush->SetValueByKnownIndex(KnownPropertyIndex::SolidColorBrush_Color, valColor));
            valBrush.WrapObjectNoRef(fallbackSolidColorBrush.get());
            IGNOREHR(SetValueByKnownIndex(propertyIndex, valBrush));
        }
    }
}

// Sets default fallback light theme colors for testing purposes, for cases where the properties are not set in markup and
// the rounded corner style is forced.
void ListViewItemPresenter::SetRoundedListViewBaseItemChromeFallbackColors()
{
    if (CListViewBaseItemChrome::IsRoundedListViewBaseItemChromeForced())
    {
        SetRoundedListViewBaseItemChromeFallbackColor(KnownPropertyIndex::ListViewItemPresenter_PointerOverBorderBrush, 0x37000000);
        SetRoundedListViewBaseItemChromeFallbackColor(KnownPropertyIndex::ListViewItemPresenter_CheckPressedBrush, 0xB2FFFFFF);
        SetRoundedListViewBaseItemChromeFallbackColor(KnownPropertyIndex::ListViewItemPresenter_CheckDisabledBrush, 0xFFFFFFFF);
        SetRoundedListViewBaseItemChromeFallbackColor(KnownPropertyIndex::ListViewItemPresenter_CheckBoxPointerOverBrush, 0xFFF3F3F3);
        SetRoundedListViewBaseItemChromeFallbackColor(KnownPropertyIndex::ListViewItemPresenter_CheckBoxPressedBrush, 0xFFEBEBEB);
        SetRoundedListViewBaseItemChromeFallbackColor(KnownPropertyIndex::ListViewItemPresenter_CheckBoxDisabledBrush, 0x06000000);
        SetRoundedListViewBaseItemChromeFallbackColor(KnownPropertyIndex::ListViewItemPresenter_CheckBoxSelectedBrush, 0xFF0070CB);
        SetRoundedListViewBaseItemChromeFallbackColor(KnownPropertyIndex::ListViewItemPresenter_CheckBoxSelectedPointerOverBrush, 0xFF0065BD);
        SetRoundedListViewBaseItemChromeFallbackColor(KnownPropertyIndex::ListViewItemPresenter_CheckBoxSelectedPressedBrush, 0xFF007DD5);
        SetRoundedListViewBaseItemChromeFallbackColor(KnownPropertyIndex::ListViewItemPresenter_CheckBoxSelectedDisabledBrush, 0x37000000);
        SetRoundedListViewBaseItemChromeFallbackColor(KnownPropertyIndex::ListViewItemPresenter_CheckBoxBorderBrush, 0x71000000);
        SetRoundedListViewBaseItemChromeFallbackColor(KnownPropertyIndex::ListViewItemPresenter_CheckBoxPointerOverBorderBrush, 0x71000000);
        SetRoundedListViewBaseItemChromeFallbackColor(KnownPropertyIndex::ListViewItemPresenter_CheckBoxPressedBorderBrush, 0x37000000);
        SetRoundedListViewBaseItemChromeFallbackColor(KnownPropertyIndex::ListViewItemPresenter_CheckBoxDisabledBorderBrush, 0x37000000);
        SetRoundedListViewBaseItemChromeFallbackColor(KnownPropertyIndex::ListViewItemPresenter_SelectedDisabledBackground, 0x06000000);
        SetRoundedListViewBaseItemChromeFallbackColor(KnownPropertyIndex::ListViewItemPresenter_SelectedBorderBrush, 0xFF0070CB);
        SetRoundedListViewBaseItemChromeFallbackColor(KnownPropertyIndex::ListViewItemPresenter_SelectedPressedBorderBrush, 0xFF007DD5);
        SetRoundedListViewBaseItemChromeFallbackColor(KnownPropertyIndex::ListViewItemPresenter_SelectedDisabledBorderBrush, 0x37000000);
        SetRoundedListViewBaseItemChromeFallbackColor(KnownPropertyIndex::ListViewItemPresenter_SelectedInnerBorderBrush, 0xFFFFFFFF);
    }

    if (CListViewBaseItemChrome::IsSelectionIndicatorVisualForced())
    {
        SetRoundedListViewBaseItemChromeFallbackColor(KnownPropertyIndex::ListViewItemPresenter_SelectionIndicatorBrush, 0xFF0070CB);
        SetRoundedListViewBaseItemChromeFallbackColor(KnownPropertyIndex::ListViewItemPresenter_SelectionIndicatorPointerOverBrush, 0xFF0070CB);
        SetRoundedListViewBaseItemChromeFallbackColor(KnownPropertyIndex::ListViewItemPresenter_SelectionIndicatorPressedBrush, 0xFF0070CB);
        SetRoundedListViewBaseItemChromeFallbackColor(KnownPropertyIndex::ListViewItemPresenter_SelectionIndicatorDisabledBrush, 0x37000000);
    }
}
#endif // DBG

