// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <FontHelperPrivate.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {

        class CustomSystemFontCollectionOverride
        {
        public:

            CustomSystemFontCollectionOverride(IUnknown* dwriteFontCollection)
            {
                RunOnUIThread([&]()
                {
                   wrl::ComPtr<IFontHelperNative> fontHelperNative;
                   IUnknown* pfontHelperUnk = reinterpret_cast<IUnknown*>(test_infra::TestServices::FontHelper);
                   LogThrow_IfFailed(pfontHelperUnk->QueryInterface(IID_PPV_ARGS(&fontHelperNative)));
                   LogThrow_IfFailed(fontHelperNative->SetSystemFontCollectionOverride(dwriteFontCollection));
                });
            }

            CustomSystemFontCollectionOverride(wchar_t const* fontFileNames[], unsigned int cFontFileNames, wchar_t const* fontNames[], unsigned int cFontNames)
            {
                RunOnUIThread([&]()
                {
                    wrl::ComPtr<IFontHelperNative> fontHelperNative;
                    wrl::ComPtr<IUnknown> dwriteFontCollection;;
                    IUnknown* pfontHelperUnk = reinterpret_cast<IUnknown*>(Private::Infrastructure::TestServices::FontHelper);
                    LogThrow_IfFailed(pfontHelperUnk->QueryInterface(IID_PPV_ARGS(&fontHelperNative)));
                    fontHelperNative->CreateCustomFontCollection(fontFileNames, cFontFileNames, fontNames, cFontNames, dwriteFontCollection.ReleaseAndGetAddressOf());
                    LogThrow_IfFailed(fontHelperNative->SetSystemFontCollectionOverride(dwriteFontCollection.Get()));
                });
            }

            ~CustomSystemFontCollectionOverride()
            {
                RunOnUIThread([&]()
                {
                    wrl::ComPtr<IFontHelperNative> fontHelperNative;
                    IUnknown* pfontHelperUnk = reinterpret_cast<IUnknown*>(test_infra::TestServices::FontHelper);
                    LogThrow_IfFailed(pfontHelperUnk->QueryInterface(IID_PPV_ARGS(&fontHelperNative)));
                    wrl::ComPtr<IUnknown> fontCollection;
                    LogThrow_IfFailed(fontHelperNative->GetCustomSystemFontCollection(&fontCollection));
                    LogThrow_IfFailed(fontHelperNative->SetSystemFontCollectionOverride(fontCollection.Get()));
                });
            }
        };

        class FontScaleOverride
        {
        public:

            FontScaleOverride(float fontScale)
            {
                RunOnUIThread([&]()
                {
                   m_fontScale = fontScale;
                   if (m_fontScale != 1.0f)
                   {
                       test_infra::TestServices::FontHelper->UpdateFontScale(fontScale);
                   }
                });
            }

            ~FontScaleOverride()
            {
                RunOnUIThread([&]()
                {
                    if (m_fontScale != 1.0f)
                    {
                        test_infra::TestServices::FontHelper->UpdateFontScale(1.0f);
                    }
                });
            }
         private:
            float m_fontScale;
        };
    }
} } } }
