// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <XamlTailored.h>

#include "FontHelper.h"
#include "RpcClient.h"
#include "Utilities.h"
#include <DWrite_3.h>
#include "WindowHelper.h"
#include "IXamlTestHooks-win.h"
#include <RuntimeEnabledFeaturesEnum.h>

using namespace WEX::Common;
using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Private { namespace Infrastructure {

BOOLEAN FontHelper::s_isCustomSystemFontCollectionInUse = false;

float FontHelper::s_fontScale = 1.0f;

bool DoesFontFamilyExist(
    IDWriteFontCollection* fontCollection,
    _In_z_ wchar_t const* fontFamilyName
    )
{
    uint32_t familyListIndex;
    BOOL exists;
    if SUCCEEDED(fontCollection->FindFamilyName(fontFamilyName, OUT &familyListIndex, OUT &exists))
    {
        return !!exists;
    }
    else
    {
        return false;
    }
}

HRESULT FontHelper::RuntimeClassInitialize()
{
    s_isCustomSystemFontCollectionInUse = false;
    return S_OK;
}

HRESULT FontHelper::GetCustomSystemFontCollection(_Out_ IUnknown** fontCollection)
{
    COM_START
    {
        if (m_customSystemFontCollection == nullptr)
        {
            static wchar_t const* fontNames[] = { L"Segoe Fluent Icons", L"Segoe UI", L"Nirmala UI", L"Yu Gothic UI", L"Microsoft YaHei UI", L"Arial", L"Malgun Gothic", L"Consolas", L"Segoe MDL2 Assets", L"Segoe UI Symbol", L"Marlett", L"Algerian", L"Bitmap Test", L"Gecko Emoji", L"Palatino Linotype"};
            static wchar_t const* fontFileNames[] = { L"SegoeIcons.ttf", L"segoeui.ttf", L"YuGothM.ttc", L"Nirmala.ttf", L"msyh.ttc", L"arial.ttf", L"segoeuii.ttf", L"seguibl.ttf", L"malgun.ttf", L"consola.ttf", L"segmdl2.ttf", L"seguisym.ttf", L"marlett.ttf", L"alger.ttf", L"BitmapFont.ttf", L"GeckoEmoji(SVG).ttf", L"Pala.ttf"};

            LogThrow_IfFailed(CreateCustomFontCollection(fontFileNames, ARRAYSIZE(fontFileNames), fontNames, ARRAYSIZE(fontNames), &m_customSystemFontCollection));
        }
        LogThrow_IfFailed(m_customSystemFontCollection.CopyTo(fontCollection));
    }
    COM_END
}

HRESULT FontHelper::CreateCustomFontCollection(
    wchar_t const* fontFileNames[], unsigned int cFontFileNames,
    wchar_t const* fontNames[], unsigned int cFontNames,
    _Out_ IUnknown** ppFontCollection)
{
    COM_START
    {
        wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
        bool useTypographicModel;
        LogThrow_IfFailed(windowTestHooks->ShouldUseTypographicFontModel(&useTypographicModel));
        DWRITE_FONT_FAMILY_MODEL dwriteFontFamilyModel = useTypographicModel ? DWRITE_FONT_FAMILY_MODEL_TYPOGRAPHIC : DWRITE_FONT_FAMILY_MODEL_WEIGHT_STRETCH_STYLE;
               
        wrl::ComPtr<IDWriteFactory6> pDWriteFactory;
        LogThrow_IfFailed(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &pDWriteFactory));
               
        Microsoft::WRL::ComPtr<IDWriteFontSetBuilder2> fontSetBuilder;
        LogThrow_IfFailed(pDWriteFactory->CreateFontSetBuilder(fontSetBuilder.ReleaseAndGetAddressOf()));

        String deploymentDir;
        LogThrow_IfFailed(WEX::TestExecution::RuntimeParameters::TryGetValue(WEX::TestExecution::RuntimeParameterConstants::c_szTestDeploymentDir, deploymentDir));
        String resourcesPath = deploymentDir + L"resources\\native\\external\\foundation\\graphics\\rendering\\";
        for (uint32_t fontIndex = 0; fontIndex < cFontFileNames; ++fontIndex)
        {
            std::wstring filePath = resourcesPath + fontFileNames[fontIndex];
            LogThrow_IfFailed(fontSetBuilder->AddFontFile(filePath.c_str()));
        }

        Microsoft::WRL::ComPtr<IDWriteFontSet> fontSet;
        LogThrow_IfFailed(fontSetBuilder->CreateFontSet(&fontSet));
        wrl::ComPtr<IDWriteFontCollection2> dwriteFontCollection;
        LogThrow_IfFailed(pDWriteFactory->CreateFontCollectionFromFontSet(fontSet.Get(), dwriteFontFamilyModel, dwriteFontCollection.ReleaseAndGetAddressOf()));

        // Confirm the collection has all the fonts we are interested in. Consider the test
        // blocked if any are missing.
        for (uint32_t fontIndex = 0; fontIndex < cFontNames; ++fontIndex)
        {
            if (!DoesFontFamilyExist(dwriteFontCollection.Get(), fontNames[fontIndex]))
            {
                WEX::Logging::Log::Result(
                    WEX::Logging::TestResults::Failed,
                    String().Format(
                    L"The font file '%s' was missing from the custom font collection.",
                    fontNames[fontIndex])
                );
            }
        }

        *ppFontCollection = dwriteFontCollection.Detach();
    }
    COM_END
}

HRESULT FontHelper::SetSystemFontCollectionOverride(_In_ IUnknown* pFontCollection)
{
    COM_START
    {
        wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
        IDWriteFontCollection* fontCollection = nullptr;
        fontCollection = static_cast<IDWriteFontCollection*>(pFontCollection);
        windowTestHooks->SetSystemFontCollectionOverride(fontCollection);
        if (pFontCollection == m_customSystemFontCollection.Get() && m_customSystemFontCollection != nullptr)
        {
            s_isCustomSystemFontCollectionInUse = true;
        }
        else
        {
            s_isCustomSystemFontCollectionInUse = false;
        }
    }
    COM_END
}

HRESULT FontHelper::ClearSystemFontCollectionOverride()
{
    COM_START
    {
        wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
 
        windowTestHooks->SetSystemFontCollectionOverride(nullptr);
        s_isCustomSystemFontCollectionInUse = false;
    }
    COM_END
}

HRESULT FontHelper::UpdateFontScale(_In_ float scale)
{
    COM_START
    {
        wrl::ComPtr<IXamlTestHooks> windowTestHooks = WindowHelper::GetTestHooks();
        windowTestHooks->UpdateFontScale(scale);
        s_fontScale = scale;
    }
    COM_END
}

HRESULT FontHelper::get_IsCustomSystemFontCollectionInUse(_Out_ BOOLEAN* isCustomSystemFontCollectionInUse)
{
    FontHelper::IsCustomSystemFontCollectionInUseStatic(isCustomSystemFontCollectionInUse);
    return S_OK;
}

void FontHelper::IsCustomSystemFontCollectionInUseStatic(_Out_ BOOLEAN* isCustomSystemFontCollectionInUse)
{
    *isCustomSystemFontCollectionInUse = s_isCustomSystemFontCollectionInUse;
}

} }

