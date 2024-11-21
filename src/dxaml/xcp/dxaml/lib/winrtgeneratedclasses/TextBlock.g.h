// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//------------------------------------------------------------------------
//
//  Abstract:
//
//      XAML types.
//      NOTE: This file was generated by a tool.
//
//------------------------------------------------------------------------

#pragma once

#include "FrameworkElement.g.h"
#include "Thickness.g.h"

#define __TextBlock_GUID "084b23b7-41a9-44c6-a55d-f16b809e0dcf"

namespace DirectUI
{
    class TextBlock;
    class Brush;
    class FlyoutBase;
    class FontFamily;
    class InlineCollection;
    class SolidColorBrush;
    class TextPointer;

    class __declspec(novtable) TextBlockGenerated:
        public DirectUI::FrameworkElement
        , public ABI::Microsoft::UI::Xaml::Controls::ITextBlock
    {
        friend class DirectUI::TextBlock;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Controls.TextBlock");

        BEGIN_INTERFACE_MAP(TextBlockGenerated, DirectUI::FrameworkElement)
            INTERFACE_ENTRY(TextBlockGenerated, ABI::Microsoft::UI::Xaml::Controls::ITextBlock)
        END_INTERFACE_MAP(TextBlockGenerated, DirectUI::FrameworkElement)

    public:
        TextBlockGenerated();
        ~TextBlockGenerated() override;

        // Event source typedefs.
        typedef CRoutedEventSource<ABI::Microsoft::UI::Xaml::IRoutedEventHandler, IInspectable, ABI::Microsoft::UI::Xaml::IRoutedEventArgs> SelectionChangedEventSourceType;
        typedef CRoutedEventSource<ABI::Microsoft::UI::Xaml::Controls::IContextMenuOpeningEventHandler, IInspectable, ABI::Microsoft::UI::Xaml::Controls::IContextMenuEventArgs> ContextMenuOpeningEventSourceType;
        typedef CEventSource<ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Controls::TextBlock*, ABI::Microsoft::UI::Xaml::Controls::IsTextTrimmedChangedEventArgs*>, ABI::Microsoft::UI::Xaml::Controls::ITextBlock, ABI::Microsoft::UI::Xaml::Controls::IIsTextTrimmedChangedEventArgs> IsTextTrimmedChangedEventSourceType;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::TextBlock;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::TextBlock;
        }

        // Properties.
        IFACEMETHOD(get_BaselineOffset)(_Out_ DOUBLE* pValue) override;
        IFACEMETHOD(get_CharacterSpacing)(_Out_ INT* pValue) override;
        IFACEMETHOD(put_CharacterSpacing)(INT value) override;
        IFACEMETHOD(get_ContentEnd)(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Documents::ITextPointer** ppValue) override;
        IFACEMETHOD(get_ContentStart)(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Documents::ITextPointer** ppValue) override;
        IFACEMETHOD(get_FontFamily)(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Media::IFontFamily** ppValue) override;
        IFACEMETHOD(put_FontFamily)(_In_opt_ ABI::Microsoft::UI::Xaml::Media::IFontFamily* pValue) override;
        IFACEMETHOD(get_FontSize)(_Out_ DOUBLE* pValue) override;
        IFACEMETHOD(put_FontSize)(DOUBLE value) override;
        IFACEMETHOD(get_FontStretch)(_Out_ ABI::Windows::UI::Text::FontStretch* pValue) override;
        IFACEMETHOD(put_FontStretch)(ABI::Windows::UI::Text::FontStretch value) override;
        IFACEMETHOD(get_FontStyle)(_Out_ ABI::Windows::UI::Text::FontStyle* pValue) override;
        IFACEMETHOD(put_FontStyle)(ABI::Windows::UI::Text::FontStyle value) override;
        IFACEMETHOD(get_FontWeight)(_Out_ ABI::Windows::UI::Text::FontWeight* pValue) override;
        IFACEMETHOD(put_FontWeight)(ABI::Windows::UI::Text::FontWeight value) override;
        IFACEMETHOD(get_Foreground)(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Media::IBrush** ppValue) override;
        IFACEMETHOD(put_Foreground)(_In_opt_ ABI::Microsoft::UI::Xaml::Media::IBrush* pValue) override;
        IFACEMETHOD(get_HorizontalTextAlignment)(_Out_ ABI::Microsoft::UI::Xaml::TextAlignment* pValue) override;
        IFACEMETHOD(put_HorizontalTextAlignment)(ABI::Microsoft::UI::Xaml::TextAlignment value) override;
        IFACEMETHOD(get_Inlines)(_Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IVector<ABI::Microsoft::UI::Xaml::Documents::Inline*>** ppValue) override;
        _Check_return_ HRESULT put_Inlines(_In_opt_ ABI::Windows::Foundation::Collections::IVector<ABI::Microsoft::UI::Xaml::Documents::Inline*>* pValue);
        IFACEMETHOD(get_IsColorFontEnabled)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_IsColorFontEnabled)(BOOLEAN value) override;
        IFACEMETHOD(get_IsTextScaleFactorEnabled)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_IsTextScaleFactorEnabled)(BOOLEAN value) override;
        IFACEMETHOD(get_IsTextSelectionEnabled)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_IsTextSelectionEnabled)(BOOLEAN value) override;
        IFACEMETHOD(get_IsTextTrimmed)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(get_LineHeight)(_Out_ DOUBLE* pValue) override;
        IFACEMETHOD(put_LineHeight)(DOUBLE value) override;
        IFACEMETHOD(get_LineStackingStrategy)(_Out_ ABI::Microsoft::UI::Xaml::LineStackingStrategy* pValue) override;
        IFACEMETHOD(put_LineStackingStrategy)(ABI::Microsoft::UI::Xaml::LineStackingStrategy value) override;
        IFACEMETHOD(get_MaxLines)(_Out_ INT* pValue) override;
        IFACEMETHOD(put_MaxLines)(INT value) override;
        IFACEMETHOD(get_OpticalMarginAlignment)(_Out_ ABI::Microsoft::UI::Xaml::OpticalMarginAlignment* pValue) override;
        IFACEMETHOD(put_OpticalMarginAlignment)(ABI::Microsoft::UI::Xaml::OpticalMarginAlignment value) override;
        IFACEMETHOD(get_Padding)(_Out_ ABI::Microsoft::UI::Xaml::Thickness* pValue) override;
        IFACEMETHOD(put_Padding)(ABI::Microsoft::UI::Xaml::Thickness value) override;
        IFACEMETHOD(get_SelectedText)(_Out_ HSTRING* pValue) override;
        IFACEMETHOD(get_SelectionEnd)(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Documents::ITextPointer** ppValue) override;
        IFACEMETHOD(get_SelectionFlyout)(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Controls::Primitives::IFlyoutBase** ppValue) override;
        IFACEMETHOD(put_SelectionFlyout)(_In_opt_ ABI::Microsoft::UI::Xaml::Controls::Primitives::IFlyoutBase* pValue) override;
        IFACEMETHOD(get_SelectionHighlightColor)(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Media::ISolidColorBrush** ppValue) override;
        IFACEMETHOD(put_SelectionHighlightColor)(_In_opt_ ABI::Microsoft::UI::Xaml::Media::ISolidColorBrush* pValue) override;
        IFACEMETHOD(get_SelectionStart)(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Documents::ITextPointer** ppValue) override;
        IFACEMETHOD(get_Text)(_Out_ HSTRING* pValue) override;
        IFACEMETHOD(put_Text)(_In_opt_ HSTRING value) override;
        IFACEMETHOD(get_TextAlignment)(_Out_ ABI::Microsoft::UI::Xaml::TextAlignment* pValue) override;
        IFACEMETHOD(put_TextAlignment)(ABI::Microsoft::UI::Xaml::TextAlignment value) override;
        IFACEMETHOD(get_TextDecorations)(_Out_ ABI::Windows::UI::Text::TextDecorations* pValue) override;
        IFACEMETHOD(put_TextDecorations)(ABI::Windows::UI::Text::TextDecorations value) override;
        IFACEMETHOD(get_TextHighlighters)(_Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IVector<ABI::Microsoft::UI::Xaml::Documents::TextHighlighter*>** ppValue) override;
        IFACEMETHOD(get_TextLineBounds)(_Out_ ABI::Microsoft::UI::Xaml::TextLineBounds* pValue) override;
        IFACEMETHOD(put_TextLineBounds)(ABI::Microsoft::UI::Xaml::TextLineBounds value) override;
        IFACEMETHOD(get_TextReadingOrder)(_Out_ ABI::Microsoft::UI::Xaml::TextReadingOrder* pValue) override;
        IFACEMETHOD(put_TextReadingOrder)(ABI::Microsoft::UI::Xaml::TextReadingOrder value) override;
        IFACEMETHOD(get_TextTrimming)(_Out_ ABI::Microsoft::UI::Xaml::TextTrimming* pValue) override;
        IFACEMETHOD(put_TextTrimming)(ABI::Microsoft::UI::Xaml::TextTrimming value) override;
        IFACEMETHOD(get_TextWrapping)(_Out_ ABI::Microsoft::UI::Xaml::TextWrapping* pValue) override;
        IFACEMETHOD(put_TextWrapping)(ABI::Microsoft::UI::Xaml::TextWrapping value) override;

        // Events.
        _Check_return_ HRESULT GetContextMenuOpeningEventSourceNoRef(_Outptr_ ContextMenuOpeningEventSourceType** ppEventSource);
        IFACEMETHOD(add_ContextMenuOpening)(_In_ ABI::Microsoft::UI::Xaml::Controls::IContextMenuOpeningEventHandler* pValue, _Out_ EventRegistrationToken* pToken) override;
        IFACEMETHOD(remove_ContextMenuOpening)(EventRegistrationToken token) override;
        _Check_return_ HRESULT GetIsTextTrimmedChangedEventSourceNoRef(_Outptr_ IsTextTrimmedChangedEventSourceType** ppEventSource);
        IFACEMETHOD(add_IsTextTrimmedChanged)(_In_ ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Controls::TextBlock*, ABI::Microsoft::UI::Xaml::Controls::IsTextTrimmedChangedEventArgs*>* pValue, _Out_ EventRegistrationToken* pToken) override;
        IFACEMETHOD(remove_IsTextTrimmedChanged)(EventRegistrationToken token) override;
        _Check_return_ HRESULT GetSelectionChangedEventSourceNoRef(_Outptr_ SelectionChangedEventSourceType** ppEventSource);
        IFACEMETHOD(add_SelectionChanged)(_In_ ABI::Microsoft::UI::Xaml::IRoutedEventHandler* pValue, _Out_ EventRegistrationToken* pToken) override;
        IFACEMETHOD(remove_SelectionChanged)(EventRegistrationToken token) override;

        // Methods.
        IFACEMETHOD(CopySelectionToClipboard)() override;
        IFACEMETHOD(GetAlphaMask)(_Outptr_ ABI::Microsoft::UI::Composition::ICompositionBrush** ppReturnValue) override;
        IFACEMETHOD(Select)(_In_ ABI::Microsoft::UI::Xaml::Documents::ITextPointer* pStart, _In_ ABI::Microsoft::UI::Xaml::Documents::ITextPointer* pEnd) override;
        IFACEMETHOD(SelectAll)() override;


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "TextBlock_Partial.h"

namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) TextBlockFactory:
       public ctl::BetterCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::Controls::ITextBlockStatics
    {
        BEGIN_INTERFACE_MAP(TextBlockFactory, ctl::BetterCoreObjectActivationFactory)
            INTERFACE_ENTRY(TextBlockFactory, ABI::Microsoft::UI::Xaml::Controls::ITextBlockStatics)
        END_INTERFACE_MAP(TextBlockFactory, ctl::BetterCoreObjectActivationFactory)

    public:
        // Factory methods.

        // Static properties.

        // Dependency properties.
        IFACEMETHOD(get_FontSizeProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_FontFamilyProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_FontWeightProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_FontStyleProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_FontStretchProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_CharacterSpacingProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_ForegroundProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_TextWrappingProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_TextTrimmingProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_TextAlignmentProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_TextProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        
        IFACEMETHOD(get_PaddingProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_LineHeightProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_LineStackingStrategyProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsTextSelectionEnabledProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_SelectedTextProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_SelectionHighlightColorProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_MaxLinesProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_TextLineBoundsProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_OpticalMarginAlignmentProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsColorFontEnabledProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_TextReadingOrderProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsTextScaleFactorEnabledProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_TextDecorationsProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_IsTextTrimmedProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_HorizontalTextAlignmentProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        
        IFACEMETHOD(get_SelectionFlyoutProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::TextBlock;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
    };
}
