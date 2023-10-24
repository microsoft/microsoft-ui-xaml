// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DWriteFontAndScriptServices.h"
#include "DWriteTextAnalyzer.h"
#include "DWriteFontCollection.h"
#include "DWriteFontFace.h"
#include "DXamlFontCollectionLoader.h"
#include "corep.h"

wrl::ComPtr<IUnknown> DWriteFontAndScriptServices::s_customSystemFontCollection;

//---------------------------------------------------------------------------
//
//  Initializes a new instance of the DWriteFontAndScriptServices class.
//
//---------------------------------------------------------------------------
DWriteFontAndScriptServices::DWriteFontAndScriptServices() :
    m_pDXamlFontCollectionLoader(NULL)
{
}

//---------------------------------------------------------------------------
//
//  Release resources associated with the DWriteFontAndScriptServices.
//
//---------------------------------------------------------------------------
DWriteFontAndScriptServices::~DWriteFontAndScriptServices()
{
    // Need to call IDWriteFactory::UnregisterFontCollectionLoader to release the ref
    // added by IDWriteFactory::RegisterFontCollectionLoader.
    if (m_dwriteFactory && m_pDXamlFontCollectionLoader)
    {
        VERIFYHR(m_dwriteFactory->UnregisterFontCollectionLoader(m_pDXamlFontCollectionLoader));
    }

    ReleaseInterface(m_pDXamlFontCollectionLoader);
    
    ClearNumberSubstitutionList();
    m_fontNameList.clear();
}

//---------------------------------------------------------------------------
//
//  Creates and initializes a new instance of the DWriteFontAndScriptServices class.
//
//---------------------------------------------------------------------------
HRESULT DWriteFontAndScriptServices::Create(
    _Outptr_ DWriteFontAndScriptServices** ppDWriteFontAndScriptServices
    )
{
    HRESULT hr = S_OK;
    DWriteFontAndScriptServices* pDWriteFontAndScriptServices = NULL;

    IFCPTR(ppDWriteFontAndScriptServices);

    pDWriteFontAndScriptServices = new DWriteFontAndScriptServices();
    IFC(pDWriteFontAndScriptServices->Initialize());

    *ppDWriteFontAndScriptServices = pDWriteFontAndScriptServices;
    pDWriteFontAndScriptServices = NULL;

Cleanup:
    ReleaseInterface(pDWriteFontAndScriptServices);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Initialize DWrite wrapper.
//
//---------------------------------------------------------------------------
HRESULT DWriteFontAndScriptServices::Initialize()
{
    HRESULT hr = S_OK;
    DXamlFontCollectionLoader *pDXamlFontCollectionLoader = NULL;

    if (m_dwriteFactory == nullptr)
    {
        IFC(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &m_dwriteFactory));

        pDXamlFontCollectionLoader = new DXamlFontCollectionLoader();
        IFC(m_dwriteFactory->RegisterFontCollectionLoader(pDXamlFontCollectionLoader));
        m_pDXamlFontCollectionLoader = pDXamlFontCollectionLoader;
        pDXamlFontCollectionLoader = NULL;
        
        IFC(m_dwriteFactory->CreateTextAnalyzer(&m_dwriteTextAnalyzer));
    }

Cleanup:
    ReleaseInterface(pDXamlFontCollectionLoader);
    RRETURN(hr);
}

_Check_return_ HRESULT DWriteFontAndScriptServices::GetDWriteFactory(_COM_Outptr_ IDWriteFactory** ppIDWriteFactory) const
{
    IFC_RETURN(m_dwriteFactory.CopyTo(ppIDWriteFactory));
    return S_OK;
}

_Check_return_ HRESULT DWriteFontAndScriptServices::GetDWriteFactory(_COM_Outptr_ IDWriteFactory2** ppIDWriteFactory2) const
{
    IFC_RETURN(m_dwriteFactory.CopyTo(ppIDWriteFactory2));
    return S_OK;
}

_Check_return_ HRESULT DWriteFontAndScriptServices::GetDWriteFactory(_COM_Outptr_ IDWriteFactory3** ppIDWriteFactory3) const
{
    IFC_RETURN(m_dwriteFactory.CopyTo(ppIDWriteFactory3));
    return S_OK;
}

_Check_return_ HRESULT DWriteFontAndScriptServices::GetDWriteFactory(_COM_Outptr_ IDWriteFactory4** ppIDWriteFactory4) const
{
    IFC_RETURN(m_dwriteFactory.CopyTo(ppIDWriteFactory4));
    return S_OK;
}

HRESULT DWriteFontAndScriptServices::EnsureSystemFontCollection()
{
    if (m_systemFontCollection == nullptr)
    {
        Microsoft::WRL::ComPtr<IDWriteFontCollection1> dWriteFontCollection;
        if (s_customSystemFontCollection != nullptr)
        {
            IFC_RETURN(s_customSystemFontCollection.As(&dWriteFontCollection));
        }
        else
        {
            IFC_RETURN(m_dwriteFactory->GetSystemFontCollection(/*includeDownloadableFonts*/ true, dWriteFontCollection.GetAddressOf()));
        }
        IFC_RETURN(DWriteFontCollection::Create(dWriteFontCollection.Get(), m_systemFontCollection.ReleaseAndGetAddressOf()));
    }
    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Gets a font collection representing the set of both locally installed
//  fonts and any downloadable fonts that are accessible to the device.
//  All text controls that use this font collection, including TextBlock,
//  RichTextBlock, and any other controls that include a TextBlock in them
//  such as Button, will transparently be able to use downloadable fonts in
//  their layout. The main core services loop must react to accumulated
//  download requests by calling IDWriteFontDownloadQueue::BeginDownload and
//  listening for a DownloadCompleted callback to invalidate control's layout.
//
//---------------------------------------------------------------------------
HRESULT DWriteFontAndScriptServices::GetSystemFontCollection(
    _Outptr_ PALText::IFontCollection **ppFontCollection
    )
{
    IFC_RETURN(EnsureSystemFontCollection());
    m_systemFontCollection.CopyTo(ppFontCollection);

    return S_OK;
}

_Check_return_ HRESULT DWriteFontAndScriptServices::GetDefaultDWriteTextFormat(_COM_Outptr_ IDWriteTextFormat** ppIDWriteTextFormat)
{
    if (m_defaultTextFormat == nullptr)
    {
        IFC_RETURN(m_dwriteFactory->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            15.0f,
            L"en-US",
            &m_defaultTextFormat
            ));
    }
    IFC_RETURN(m_defaultTextFormat.CopyTo(ppIDWriteTextFormat));
    return S_OK;
}

_Check_return_ HRESULT DWriteFontAndScriptServices::GetDWriteTextAnalyzer(_COM_Outptr_ IDWriteTextAnalyzer** ppIDWriteTextAnalyzer) const
{
    IFC_RETURN(m_dwriteTextAnalyzer.CopyTo(ppIDWriteTextAnalyzer));
    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Sets a specific system font collection for test purposes.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT DWriteFontAndScriptServices::SetSystemFontCollectionOverride(_In_opt_ IDWriteFontCollection* pFontCollection)
{
    s_customSystemFontCollection = pFontCollection;
    
    // Release the old font collection, the next call to GetSystemFontCollection will retrieve a fresh collection.
    m_systemFontCollection.reset();
    m_fontNameList.clear();
    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Return an interface to perform text analysis with.
//  Provide a locale name to get/create a number substitution object.
// 
//---------------------------------------------------------------------------
HRESULT DWriteFontAndScriptServices::CreateTextAnalyzer(
    _Outptr_ PALText::ITextAnalyzer** ppTextAnalyzer
    )
{
    // Update locale 
    RRETURN(DWriteTextAnalyzer::Create(m_dwriteFactory.Get(), ppTextAnalyzer));
}

//---------------------------------------------------------------------------
//
//  Return an interface to perform glyph analysis with.
//
//---------------------------------------------------------------------------
HRESULT DWriteFontAndScriptServices::CreateGlyphAnalyzer(
    _Outptr_ PALText::IGlyphAnalyzer** ppGlyphAnalyzer
    )
{
    RRETURN(DWriteTextAnalyzer::Create(m_dwriteFactory.Get(), ppGlyphAnalyzer));
}

//---------------------------------------------------------------------------
//
//  Return an interface to perform script analysis with.
//
//---------------------------------------------------------------------------
HRESULT DWriteFontAndScriptServices::CreateScriptAnalyzer(
    _Outptr_ PALText::IScriptAnalyzer** ppScriptAnalyzer
    )
{
    RRETURN(DWriteTextAnalyzer::Create(m_dwriteFactory.Get(), ppScriptAnalyzer));
}

//---------------------------------------------------------------------------
//
//  Creates a font collection from a given uri.
//
//---------------------------------------------------------------------------
HRESULT DWriteFontAndScriptServices::CreateCustomFontCollection(
    _In_ IPALResource* pResource,
    _Outptr_ PALText::IFontCollection **ppFontCollection
    )
{
    HRESULT hr = S_OK;
    xstring_ptr strFilePath;
    IDWriteFontCollection *pDWriteFontCollection = NULL;

    //
    // NOTE: Currently we only support resources that have a file path. 
    // We fail on resources without a file path.
    //
    // See the explanation below, in DWriteFontAndScriptServices::CreateCustomFontFace().
    //
    IFC(pResource->TryGetFilePath(&strFilePath));
    if (strFilePath.IsNullOrEmpty())
    {
        IFC(E_FAIL);
    }
    
    IFC(m_dwriteFactory->CreateCustomFontCollection(
        m_pDXamlFontCollectionLoader,
        reinterpret_cast<void const*>(strFilePath.GetBuffer()),
        (strFilePath.GetCount() + 1) * sizeof(XCHAR),
        &pDWriteFontCollection));

    IFC(DWriteFontCollection::Create(pDWriteFontCollection, ppFontCollection));

Cleanup:
    ReleaseInterface(pDWriteFontCollection);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Creates a custom font face.
//
//---------------------------------------------------------------------------
HRESULT DWriteFontAndScriptServices::CreateCustomFontFace(
    _In_ IPALResource* pResource,
    _In_ XUINT32 faceIndex,
    _In_ FssFontSimulations::Enum fontSimulations,
    _Outptr_ PALText::IFontFace **ppFontFace
    )
{
    HRESULT hr = S_OK;
    xstring_ptr strFilePath;
    DWriteFontFace *pDWriteFontFace;
    IDWriteFontFile* pFontFile = NULL;
    IDWriteFontFace* pFontFace = NULL;
    DWRITE_FONT_FILE_TYPE dwriteFontFileType;
    DWRITE_FONT_FACE_TYPE dwriteFontFaceType;
    XUINT32 numberOfFaces = 0;
    BOOL isSupportedFontType;

    //
    // NOTE: Currently we only support resources that have a file path. 
    // We fail on resources without a file path.
    //
    // For example, MRT embedded data resources won't have a file path 
    // (they're backed by a stream).
    //
    // It would be possible to add support for non-file based resources - this
    // could be done as a future work item. The best option appears to be to use
    // IDWriteFactory::CreateCustomFontFileReference, and write a custom
    // IDWriteFontFileLoader that knows how to read a stream. Another option
    // is to create a temporary file, write the contents of the stream out
    // to disk, and pass that file path to DWrite.
    //
    IFC(pResource->TryGetFilePath(&strFilePath));
    if (strFilePath.IsNullOrEmpty())
    {
        IFC(E_FAIL);
    }

    IFC(m_dwriteFactory->CreateFontFileReference(strFilePath.GetBuffer(), NULL, &pFontFile));

    IFC(pFontFile->Analyze(
        &isSupportedFontType,
        &dwriteFontFileType,
        &dwriteFontFaceType,
        &numberOfFaces));

    IFCEXPECT(faceIndex < numberOfFaces);

    IFC(m_dwriteFactory->CreateFontFace(
        dwriteFontFaceType,
        1,
        &pFontFile,
        faceIndex,
        static_cast<DWRITE_FONT_SIMULATIONS>(fontSimulations),
        &pFontFace));

    pDWriteFontFace = new DWriteFontFace(pFontFace, NULL);
    *ppFontFace = pDWriteFontFace;

Cleanup:
    ReleaseInterface(pFontFace);
    ReleaseInterface(pFontFile);
    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
// Clear the cached IDWriteNumberSubstitution objects. This is triggered only when locale settings is changed in 
// WM_SETTINGCHANGE message from Control Panel.
//
//---------------------------------------------------------------------------

/* static */ void
DWriteFontAndScriptServices::ClearNumberSubstitutionListCallback(
    _In_ const xstring_ptr& /* strKey */,
    std::pair<IDWriteNumberSubstitution*, NumberSubstitutionData>* data,
    XHANDLE /* extraData */
    )
{
    ReleaseInterface(data->first);

    delete data;
}

void DWriteFontAndScriptServices::ClearNumberSubstitutionList()
{
    for (auto it = m_numberSubstitutionList.begin(); it != m_numberSubstitutionList.end(); ++it)
    {
        DWriteFontAndScriptServices::ClearNumberSubstitutionListCallback(it->first, it->second, nullptr);
    }

    m_numberSubstitutionList.clear();
}

HRESULT DWriteFontAndScriptServices::GetNumberSubstitution(_In_ const xstring_ptr& strLocaleName, _Out_opt_ NumberSubstitutionData** ppMappingData, _Out_opt_ IDWriteNumberSubstitution** ppNumberSubstitution)
{
    HRESULT hr = S_OK;
    IDWriteNumberSubstitution* pNumberSubstitution = NULL;
    NumberSubstitutionData data = {L"0",L"1",L"2",L"3",L"4",L"5",L"6",L"7",L"8",L"9",L"%",L".",L","};
    std::pair<IDWriteNumberSubstitution*, NumberSubstitutionData>* pData = NULL;

    // If the locale string is empty, no digit substitution is performed. Return NULL.
    if (!strLocaleName.IsNullOrEmpty())
    {
        auto itData = m_numberSubstitutionList.find(strLocaleName);
        // Lookup the IDWriteNumberSubstitution* object from the map.
        if (itData == m_numberSubstitutionList.end())
        {
            // Read the mode from system settings for the given locale.
            int iDigitSubstitution = 0;
            DIGITSHAPE digitMode = DIGITS_NOTIMPL;

            int res = GetLocaleInfoEx(strLocaleName.GetBuffer(), LOCALE_IDIGITSUBSTITUTION|LOCALE_RETURN_NUMBER, reinterpret_cast<PWSTR>(&iDigitSubstitution), sizeof(iDigitSubstitution) / sizeof(wchar_t));
            if (res == sizeof(iDigitSubstitution) / sizeof(wchar_t))
            {
                digitMode = static_cast<DIGITSHAPE>(iDigitSubstitution + 1);
            }

            // For cultures returns NONE (like en-us), we store the key with a nullptr object, so that next time we can immediately tell without calling GetLocaleinfoEx.
            if (digitMode == DIGITS_NONE)
            {
                pData = new std::pair<IDWriteNumberSubstitution*, NumberSubstitutionData>(nullptr, data);
                VERIFY_COND(m_numberSubstitutionList.insert({ strLocaleName, pData }), .second);
            }
            else if (digitMode != DIGITS_NOTIMPL)
            {
                ASSERT(strLocaleName.GetCount() > 0); // because of the emptiness check above

                if (m_dwriteFactory != nullptr)
                {
                    WCHAR chars[11];

                    res = GetLocaleInfoEx(strLocaleName.GetBuffer(), LOCALE_SNATIVEDIGITS, reinterpret_cast<PWSTR>(chars), ARRAY_SIZE(chars));

                    if (res > 0)
                    {
                        for (XUINT32 i = 0; i < 10; i++)
                        {
                            data.numerals[i][0] = chars[i];
                        }
                    }

                    res = GetLocaleInfoEx(strLocaleName.GetBuffer(), LOCALE_STHOUSAND, reinterpret_cast<PWSTR>(chars), ARRAY_SIZE(chars));
                    if (res > 0)
                    {
                        memcpy_s(data.groupSeparator, ARRAY_SIZE(data.groupSeparator), chars, res);
                    }

                    res = GetLocaleInfoEx(strLocaleName.GetBuffer(), LOCALE_SDECIMAL, reinterpret_cast<PWSTR>(chars), ARRAY_SIZE(chars));
                    if (res > 0)
                    {
                        memcpy_s(data.decimalSeparator, ARRAY_SIZE(data.decimalSeparator), chars, res);
                    }

                    // NLS has no 'percent sign' symbol,
                    // so judge an appropriate glyph from the digits.
                    if (data.numerals[0][0] == L'\x0660'  
                        ||  data.numerals[0][0] == L'\x06F0')  
                    {
                        data.percentSymbol[0] = L'\x066A'; // Arabic percent sign
                    }
                    else
                    {
                        data.percentSymbol[0] = L'%'; // typical Western percent sign 
                    }

                    hr = m_dwriteFactory->CreateNumberSubstitution(  
                         DWRITE_NUMBER_SUBSTITUTION_METHOD_FROM_CULTURE,  
                         strLocaleName.GetBuffer(), false/* ignoreUserOverride*/, &pNumberSubstitution);  

                    // Add this node to the chain list.
                    if (pNumberSubstitution)
                    {
                        pData = new std::pair<IDWriteNumberSubstitution*, NumberSubstitutionData>(pNumberSubstitution, data);

                        pNumberSubstitution = NULL; // ownership transferred to pData

                        VERIFY_COND(m_numberSubstitutionList.insert({ strLocaleName, pData }), .second);
                    }
                }
            }
        }
        else{
            pData = itData->second;
        }
    }

    if (pData)
    {
        if (ppNumberSubstitution)
        {
            SetInterface(*ppNumberSubstitution, pData->first);
        }
        
        if (ppMappingData)
        {
            *ppMappingData = &(pData->second); 
        }
        pData = NULL;
    }

    ReleaseInterface(pNumberSubstitution);
    RRETURN(hr);//RRETURN_REMOVAL
}

_Check_return_ HRESULT DWriteFontAndScriptServices::IsFontNameValid(_In_ const xstring_ptr& strFontName, _Out_ BOOL* pIsValid)
{
    *pIsValid = false;
    if (!strFontName.IsNullOrEmpty())
    {
        auto itIsValid = m_fontNameList.find(strFontName);
        if (itIsValid == m_fontNameList.end())
        {
            UINT32 index = 0;
            IFC_RETURN(EnsureSystemFontCollection());
            IDWriteFontCollection* pDWriteFontCollection = reinterpret_cast<DWriteFontCollection*>(m_systemFontCollection.get())->m_pDWriteFontCollection;
            IFC_RETURN(pDWriteFontCollection->FindFamilyName(strFontName.GetBuffer(), &index, pIsValid));
            VERIFY_COND(m_fontNameList.insert({ strFontName, *pIsValid }), .second);
        }
        else{
            *pIsValid = itIsValid->second;
        }
    }
    return S_OK;
}


