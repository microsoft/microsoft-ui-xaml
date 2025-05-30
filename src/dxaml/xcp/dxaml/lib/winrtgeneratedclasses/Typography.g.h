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




namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) TypographyFactory:
       public ctl::AbstractActivationFactory
        , public ABI::Microsoft::UI::Xaml::Documents::ITypographyStatics
    {
        BEGIN_INTERFACE_MAP(TypographyFactory, ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(TypographyFactory, ABI::Microsoft::UI::Xaml::Documents::ITypographyStatics)
        END_INTERFACE_MAP(TypographyFactory, ctl::AbstractActivationFactory)

    public:
        // Factory methods.

        // Static properties.

        // Dependency properties.

        // Attached properties.
        static _Check_return_ HRESULT GetAnnotationAlternatesStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ INT* pValue);
        static _Check_return_ HRESULT SetAnnotationAlternatesStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, INT value);
        IFACEMETHOD(get_AnnotationAlternatesProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetAnnotationAlternates)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ INT* pValue);
        IFACEMETHOD(SetAnnotationAlternates)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, INT value);
        static _Check_return_ HRESULT GetEastAsianExpertFormsStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetEastAsianExpertFormsStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_EastAsianExpertFormsProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetEastAsianExpertForms)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetEastAsianExpertForms)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetEastAsianLanguageStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ ABI::Microsoft::UI::Xaml::FontEastAsianLanguage* pValue);
        static _Check_return_ HRESULT SetEastAsianLanguageStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, ABI::Microsoft::UI::Xaml::FontEastAsianLanguage value);
        IFACEMETHOD(get_EastAsianLanguageProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetEastAsianLanguage)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ ABI::Microsoft::UI::Xaml::FontEastAsianLanguage* pValue);
        IFACEMETHOD(SetEastAsianLanguage)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, ABI::Microsoft::UI::Xaml::FontEastAsianLanguage value);
        static _Check_return_ HRESULT GetEastAsianWidthsStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ ABI::Microsoft::UI::Xaml::FontEastAsianWidths* pValue);
        static _Check_return_ HRESULT SetEastAsianWidthsStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, ABI::Microsoft::UI::Xaml::FontEastAsianWidths value);
        IFACEMETHOD(get_EastAsianWidthsProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetEastAsianWidths)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ ABI::Microsoft::UI::Xaml::FontEastAsianWidths* pValue);
        IFACEMETHOD(SetEastAsianWidths)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, ABI::Microsoft::UI::Xaml::FontEastAsianWidths value);
        static _Check_return_ HRESULT GetStandardLigaturesStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetStandardLigaturesStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_StandardLigaturesProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetStandardLigatures)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetStandardLigatures)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetContextualLigaturesStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetContextualLigaturesStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_ContextualLigaturesProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetContextualLigatures)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetContextualLigatures)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetDiscretionaryLigaturesStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetDiscretionaryLigaturesStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_DiscretionaryLigaturesProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetDiscretionaryLigatures)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetDiscretionaryLigatures)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetHistoricalLigaturesStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetHistoricalLigaturesStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_HistoricalLigaturesProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetHistoricalLigatures)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetHistoricalLigatures)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetStandardSwashesStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ INT* pValue);
        static _Check_return_ HRESULT SetStandardSwashesStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, INT value);
        IFACEMETHOD(get_StandardSwashesProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetStandardSwashes)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ INT* pValue);
        IFACEMETHOD(SetStandardSwashes)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, INT value);
        static _Check_return_ HRESULT GetContextualSwashesStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ INT* pValue);
        static _Check_return_ HRESULT SetContextualSwashesStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, INT value);
        IFACEMETHOD(get_ContextualSwashesProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetContextualSwashes)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ INT* pValue);
        IFACEMETHOD(SetContextualSwashes)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, INT value);
        static _Check_return_ HRESULT GetContextualAlternatesStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetContextualAlternatesStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_ContextualAlternatesProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetContextualAlternates)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetContextualAlternates)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetStylisticAlternatesStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ INT* pValue);
        static _Check_return_ HRESULT SetStylisticAlternatesStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, INT value);
        IFACEMETHOD(get_StylisticAlternatesProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetStylisticAlternates)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ INT* pValue);
        IFACEMETHOD(SetStylisticAlternates)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, INT value);
        static _Check_return_ HRESULT GetStylisticSet1Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetStylisticSet1Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_StylisticSet1Property)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetStylisticSet1)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetStylisticSet1)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetStylisticSet2Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetStylisticSet2Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_StylisticSet2Property)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetStylisticSet2)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetStylisticSet2)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetStylisticSet3Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetStylisticSet3Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_StylisticSet3Property)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetStylisticSet3)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetStylisticSet3)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetStylisticSet4Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetStylisticSet4Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_StylisticSet4Property)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetStylisticSet4)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetStylisticSet4)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetStylisticSet5Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetStylisticSet5Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_StylisticSet5Property)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetStylisticSet5)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetStylisticSet5)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetStylisticSet6Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetStylisticSet6Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_StylisticSet6Property)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetStylisticSet6)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetStylisticSet6)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetStylisticSet7Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetStylisticSet7Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_StylisticSet7Property)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetStylisticSet7)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetStylisticSet7)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetStylisticSet8Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetStylisticSet8Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_StylisticSet8Property)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetStylisticSet8)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetStylisticSet8)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetStylisticSet9Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetStylisticSet9Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_StylisticSet9Property)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetStylisticSet9)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetStylisticSet9)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetStylisticSet10Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetStylisticSet10Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_StylisticSet10Property)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetStylisticSet10)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetStylisticSet10)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetStylisticSet11Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetStylisticSet11Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_StylisticSet11Property)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetStylisticSet11)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetStylisticSet11)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetStylisticSet12Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetStylisticSet12Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_StylisticSet12Property)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetStylisticSet12)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetStylisticSet12)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetStylisticSet13Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetStylisticSet13Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_StylisticSet13Property)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetStylisticSet13)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetStylisticSet13)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetStylisticSet14Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetStylisticSet14Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_StylisticSet14Property)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetStylisticSet14)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetStylisticSet14)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetStylisticSet15Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetStylisticSet15Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_StylisticSet15Property)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetStylisticSet15)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetStylisticSet15)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetStylisticSet16Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetStylisticSet16Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_StylisticSet16Property)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetStylisticSet16)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetStylisticSet16)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetStylisticSet17Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetStylisticSet17Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_StylisticSet17Property)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetStylisticSet17)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetStylisticSet17)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetStylisticSet18Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetStylisticSet18Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_StylisticSet18Property)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetStylisticSet18)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetStylisticSet18)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetStylisticSet19Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetStylisticSet19Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_StylisticSet19Property)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetStylisticSet19)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetStylisticSet19)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetStylisticSet20Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetStylisticSet20Static(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_StylisticSet20Property)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetStylisticSet20)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetStylisticSet20)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetCapitalsStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ ABI::Microsoft::UI::Xaml::FontCapitals* pValue);
        static _Check_return_ HRESULT SetCapitalsStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, ABI::Microsoft::UI::Xaml::FontCapitals value);
        IFACEMETHOD(get_CapitalsProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetCapitals)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ ABI::Microsoft::UI::Xaml::FontCapitals* pValue);
        IFACEMETHOD(SetCapitals)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, ABI::Microsoft::UI::Xaml::FontCapitals value);
        static _Check_return_ HRESULT GetCapitalSpacingStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetCapitalSpacingStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_CapitalSpacingProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetCapitalSpacing)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetCapitalSpacing)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetKerningStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetKerningStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_KerningProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetKerning)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetKerning)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetCaseSensitiveFormsStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetCaseSensitiveFormsStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_CaseSensitiveFormsProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetCaseSensitiveForms)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetCaseSensitiveForms)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetHistoricalFormsStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetHistoricalFormsStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_HistoricalFormsProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetHistoricalForms)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetHistoricalForms)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetFractionStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ ABI::Microsoft::UI::Xaml::FontFraction* pValue);
        static _Check_return_ HRESULT SetFractionStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, ABI::Microsoft::UI::Xaml::FontFraction value);
        IFACEMETHOD(get_FractionProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetFraction)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ ABI::Microsoft::UI::Xaml::FontFraction* pValue);
        IFACEMETHOD(SetFraction)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, ABI::Microsoft::UI::Xaml::FontFraction value);
        static _Check_return_ HRESULT GetNumeralStyleStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ ABI::Microsoft::UI::Xaml::FontNumeralStyle* pValue);
        static _Check_return_ HRESULT SetNumeralStyleStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, ABI::Microsoft::UI::Xaml::FontNumeralStyle value);
        IFACEMETHOD(get_NumeralStyleProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetNumeralStyle)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ ABI::Microsoft::UI::Xaml::FontNumeralStyle* pValue);
        IFACEMETHOD(SetNumeralStyle)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, ABI::Microsoft::UI::Xaml::FontNumeralStyle value);
        static _Check_return_ HRESULT GetNumeralAlignmentStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ ABI::Microsoft::UI::Xaml::FontNumeralAlignment* pValue);
        static _Check_return_ HRESULT SetNumeralAlignmentStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, ABI::Microsoft::UI::Xaml::FontNumeralAlignment value);
        IFACEMETHOD(get_NumeralAlignmentProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetNumeralAlignment)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ ABI::Microsoft::UI::Xaml::FontNumeralAlignment* pValue);
        IFACEMETHOD(SetNumeralAlignment)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, ABI::Microsoft::UI::Xaml::FontNumeralAlignment value);
        static _Check_return_ HRESULT GetSlashedZeroStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetSlashedZeroStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_SlashedZeroProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetSlashedZero)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetSlashedZero)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetMathematicalGreekStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        static _Check_return_ HRESULT SetMathematicalGreekStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        IFACEMETHOD(get_MathematicalGreekProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetMathematicalGreek)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetMathematicalGreek)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN value);
        static _Check_return_ HRESULT GetVariantsStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ ABI::Microsoft::UI::Xaml::FontVariants* pValue);
        static _Check_return_ HRESULT SetVariantsStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, ABI::Microsoft::UI::Xaml::FontVariants value);
        IFACEMETHOD(get_VariantsProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetVariants)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ ABI::Microsoft::UI::Xaml::FontVariants* pValue);
        IFACEMETHOD(SetVariants)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, ABI::Microsoft::UI::Xaml::FontVariants value);

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::Typography;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
    };
}
