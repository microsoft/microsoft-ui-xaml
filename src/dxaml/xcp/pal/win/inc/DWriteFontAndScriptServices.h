// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      DWrite based implementaiton of IPALFontAndScriptServices.

#include "xmap.h"
#include "values.h"

#pragma once
class DXamlFontCollectionLoader;
//---------------------------------------------------------------------------
//
//  DWriteFontAndScriptServices
//
//  The root factory interface for all Font and Script Services objects based on DWrite.
//
//---------------------------------------------------------------------------

// Digit substituion mode
// DIGITS_CTX - context (digit shape follows preceding run or CHARFORMAT's  charset)
// DIGITS_NONE - None (digits always show as European digit shape)
// DIGITS_NATIONAL (digits always show as user locale's native shape)
enum DIGITSHAPE
{
    DIGITS_NOTIMPL = 0,
    DIGITS_CTX,
    DIGITS_NONE,
    DIGITS_NATIONAL
};

class DWriteFontAndScriptServices : public CXcpObjectBase<IPALFontAndScriptServices, CXcpObjectAddRefPolicy>
{
public:
    // Creates and initializes a new instance of the DWriteFontAndScriptServices class.
    static HRESULT Create(
        _Outptr_ DWriteFontAndScriptServices** ppDWriteFontAndScriptServices
        );

    // Gets a font collection representing the set of installed fonts.
    HRESULT GetSystemFontCollection(
        _Outptr_ PALText::IFontCollection **ppFontCollection
        ) override;

    // Creates a font collection from a given uri.
    HRESULT CreateCustomFontCollection(
        _In_ IPALResource* pResource,
        _Outptr_ PALText::IFontCollection **ppFontCollection
        ) override;

    // Create a custom font face.
    HRESULT CreateCustomFontFace(
        _In_ IPALResource* pResource,
        _In_ XUINT32 faceIndex,
        _In_ PALText::FontSimulations::Enum fontSimulations,
        _Outptr_ PALText::IFontFace **ppFontFace
        ) override;

    // Return an interface to perform text analysis with.
    HRESULT CreateTextAnalyzer(
        _Outptr_ PALText::ITextAnalyzer **ppTextAnalyzer
        ) override;

    // Return an interface to perform glyph analysis with.
    HRESULT CreateGlyphAnalyzer(
        _Outptr_ PALText::IGlyphAnalyzer **ppGlyphAnalyzer
        ) override;

    // Return an interface to perform script analysis with.
    HRESULT CreateScriptAnalyzer(
        _Outptr_ PALText::IScriptAnalyzer **ppScriptAnalyzer
        ) override;

    HRESULT GetSystemFontFallback(
        _Outptr_ PALText::IFontFallback **ppFontFallback
        ) override;

    HRESULT CreateFontFallbackBuilder(
        _Outptr_ PALText::IFontFallbackBuilder **ppFontFallbackBuilder
        ) override;

    _Check_return_ HRESULT SetSystemFontCollectionOverride(_In_opt_ IDWriteFontCollection* pFontCollection);

    // Clear the cached IDWriteNumberSubstitution objects when system settings
    // changed. This is triggered only when locale is changed in
    // WM_SETTINGCHANGE message.
    void ClearNumberSubstitutionList();

    // Get the cached IDWriteNumberSubstitution object based on the given
    // locale, if the given locale does not exist, create a new one.
    HRESULT GetNumberSubstitution(_In_ const xstring_ptr& strLocaleName,  _Out_opt_ NumberSubstitutionData** ppData, _Out_opt_ IDWriteNumberSubstitution** ppNumberSubstitution);

    _Check_return_ HRESULT IsFontNameValid(_In_ const xstring_ptr& strFontName, _Out_ BOOL* pIsValid);

    _Check_return_ HRESULT GetDWriteFactory(_COM_Outptr_ IDWriteFactory** ppIDWriteFactory) const;
    _Check_return_ HRESULT GetDWriteFactory(_COM_Outptr_ IDWriteFactory2** ppIDWriteFactory2) const;
    _Check_return_ HRESULT GetDWriteFactory(_COM_Outptr_ IDWriteFactory3** ppIDWriteFactory3) const;
    _Check_return_ HRESULT GetDWriteFactory(_COM_Outptr_ IDWriteFactory4** ppIDWriteFactory4) const;

    // For fast path TextBlock, we need to determine whether the current font collection is the static one,
    // if so, we need to create a IDWriteTextFormat object with the static font collection, so that the static font will be picked up.
    // if not, we need to keep passing nullptr when we create IDWriteTextFormat for perf reasons.
    bool IsSystemFontCollectionOverride() const {return s_customSystemFontCollection != nullptr;}

    _Check_return_ HRESULT GetDefaultDWriteTextFormat(_COM_Outptr_ IDWriteTextFormat** ppIDWriteTextFormat);
    _Check_return_ HRESULT GetDWriteTextAnalyzer(_COM_Outptr_ IDWriteTextAnalyzer** ppIDWriteTextAnalyzer) const;

private:
    static void ClearNumberSubstitutionListCallback(
        _In_ const xstring_ptr& strKey,
        std::pair<IDWriteNumberSubstitution*, NumberSubstitutionData>* data,
        XHANDLE extraData
        );

private:

    containers::vector_map<xstring_ptr, std::pair<IDWriteNumberSubstitution*, NumberSubstitutionData>*> m_numberSubstitutionList;
    containers::vector_map<xstring_ptr, BOOL> m_fontNameList;

    // The root factory interface for all DWrite objects.
    Microsoft::WRL::ComPtr<IDWriteFactory4> m_dwriteFactory;

    // DXaml custom font collection loader.
    DXamlFontCollectionLoader *m_pDXamlFontCollectionLoader;

    // Caches the System FssFontCollection to avoid creating multiple DWriteFontCollection.
    xref_ptr<PALText::IFontCollection> m_systemFontCollection;

    // For our TAEF tests, we set the static (checked in) font collection to override the default system font collection.
    // So that our tests will not be impacted by font changes from FI.
    static wrl::ComPtr<IUnknown> s_customSystemFontCollection;

    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_defaultTextFormat;
    Microsoft::WRL::ComPtr<IDWriteTextAnalyzer> m_dwriteTextAnalyzer;

    HRESULT EnsureSystemFontCollection();

    // Initializes a new instance of the DWriteFontAndScriptServices class.
    DWriteFontAndScriptServices();

    // Release resources associated with the DWriteFontAndScriptServices.
    ~DWriteFontAndScriptServices() override;

    // Initialize DWrite wrapper.
    HRESULT Initialize();
};
