// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <XamlLogging.h>
#include <minerror.h>
#include <minxcptypes.h>
#include <macros.h>
#include <xref_ptr.h>
#include <memory>
#include <minpal.h>
#include "WinUriUnitTests.h"
#include "winuri.h"

struct UriTest {
    const wchar_t* Uri;
    const wchar_t* CanonicalUri;
    const wchar_t* BaseUri;
    const wchar_t* MsResourceUri;
    const wchar_t* Filename;
    const wchar_t* Extension;
    const wchar_t* Path;
    const wchar_t* Scheme;
    const wchar_t* Host;
    const wchar_t* Username;
    const wchar_t* Password;
    const wchar_t* File;
};

static UriTest UriTestList[] = {
    {
        L"ms-appx:///images/SecondaryTileDefault-sdk.png",
        L"ms-appx:///images/SecondaryTileDefault-sdk.png",
        L"ms-appx:///images/",
        L"ms-resource:///Files/images/SecondaryTileDefault-sdk.png",
        L"SecondaryTileDefault-sdk.png",
        L"png",
        L"/images/SecondaryTileDefault-sdk.png",
        L"ms-appx",
        nullptr,
        nullptr,
        nullptr,
        nullptr
    },
    {
        L"ms-appx:///foo/../images/SecondaryTileDefault-sdk.png",
        L"ms-appx:///images/SecondaryTileDefault-sdk.png",
        L"ms-appx:///images/",
        L"ms-resource:///Files/images/SecondaryTileDefault-sdk.png",
        L"SecondaryTileDefault-sdk.png",
        L"png",
        L"/images/SecondaryTileDefault-sdk.png",
        L"ms-appx",
        nullptr,
        nullptr,
        nullptr,
        nullptr
    },
    {
        L"ms-appx:pages/SecondPage.xaml",
        L"ms-appx:pages/SecondPage.xaml",
        L"ms-appx:pages/",
        L"ms-resource:/Files/pages/SecondPage.xaml",
        L"SecondPage.xaml",
        L"xaml",
        L"pages/SecondPage.xaml",
        L"ms-appx",
        nullptr,
        nullptr,
        nullptr,
        nullptr
    },
    {
        L"http://www.microsoft.com/images/mypicture.jpg",
        L"http://www.microsoft.com/images/mypicture.jpg",
        L"http://www.microsoft.com/",
        L"ms-resource://www.microsoft.com/Files/images/mypicture.jpg",
        L"mypicture.jpg",
        L"jpg",
        L"/images/mypicture.jpg",
        L"http",
        L"www.microsoft.com",
        nullptr,
        nullptr,
        nullptr
    },
    {
        L"ftp://user:password@ftp.microsoft.com:8080/path/bin/file.asp?q=query#fragment",
        L"ftp://user:password@ftp.microsoft.com:8080/path/bin/file.asp?q=query#fragment",
        L"ftp://ftp.microsoft.com/",
        L"ms-resource://user:password@ftp.microsoft.com:8080/Files/path/bin/file.asp?q=query#fragment",
        L"file.asp",
        L"asp",
        L"/path/bin/file.asp",
        L"ftp",
        L"ftp.microsoft.com",
        L"user",
        L"password",
        nullptr
    },
    {
        L"ms-appdata:///local/../roaming/logo.png",
        L"ms-appdata:///roaming/logo.png",
        L"ms-appdata:///local/../roaming/logo.png",
        L"ms-resource:///Files/roaming/logo.png",
        L"logo.png",
        L"png",
        L"/roaming/logo.png",
        L"ms-appdata",
        nullptr,
        nullptr,
        nullptr,
        nullptr
    },
    {
        L"file:///MyFiles/picture.png",
        L"file:///MyFiles/picture.png",
        L"file:///MyFiles/picture.png",
        L"ms-resource:/Files/MyFiles/picture.png",
        L"picture.png",
        L"png",
        L"/MyFiles/picture.png",
        L"file",
        nullptr,
        nullptr,
        nullptr,
        L"\\MyFiles\\picture.png"
    }
};

struct CombineUriTest {
    const wchar_t* SourceUri;
    const wchar_t* Added;
    const wchar_t* CombinedUri;
};

static CombineUriTest CombinedTestList[] = {
    {
        L"ms-appx:///images/",
        L"SecondaryTileDefault-sdk.png",
        L"ms-appx:///images/SecondaryTileDefault-sdk.png"
    },
    {
        L"ms-appx:///images/subfolder",
        L"../SecondaryTileDefault-sdk.png",
        L"ms-appx:///SecondaryTileDefault-sdk.png"
    },
    {
        L"ms-appx:///images/subfolder",
        L"/SecondaryTileDefault-sdk.png",
        L"ms-appx:///SecondaryTileDefault-sdk.png"
    }
};


void CreateWinUriHelper(_In_z_ const wchar_t* wsUri, _Out_ xref_ptr<IPALUri>& spUri)
{
    VERIFY_SUCCEEDED(CWinUriFactory::Create(static_cast<XUINT32>(wcslen(wsUri)), wsUri, spUri.ReleaseAndGetAddressOf()));
}

void CheckCanonical(_In_z_ const wchar_t *wsNonCanonicalUrl, _In_z_ const wchar_t *wsCanonicalUrl)
{
    xref_ptr<IPALUri> spUri;
    XUINT32 dwBufferLength;

    CreateWinUriHelper(wsCanonicalUrl, spUri);

    dwBufferLength = 0;
    VERIFY_SUCCEEDED(spUri->GetCanonical(&dwBufferLength, nullptr));
    VERIFY_ARE_EQUAL(dwBufferLength, wcslen(wsCanonicalUrl)+1);

    CreateWinUriHelper(wsNonCanonicalUrl, spUri);
    dwBufferLength = 0;
    VERIFY_SUCCEEDED(spUri->GetCanonical(&dwBufferLength, nullptr));
    VERIFY_ARE_EQUAL(dwBufferLength, wcslen(wsCanonicalUrl)+1);

    std::unique_ptr<wchar_t[]> spBuffer(new wchar_t[dwBufferLength]);
    VERIFY_SUCCEEDED(spUri->GetCanonical(&dwBufferLength, spBuffer.get()));

    VERIFY_IS_TRUE(0 == wcscmp(spBuffer.get(), wsCanonicalUrl));
}

using namespace WEX::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace WinUri {



void WinUriUnitTests::Create()
{
    xref_ptr<IPALUri> spUri;
    const wchar_t *wsUri = UriTestList[0].Uri;
    CreateWinUriHelper(wsUri, spUri);
    VERIFY_IS_TRUE(!!spUri);
}

void WinUriUnitTests::GetCanonical()
{
    int n = ARRAYSIZE(UriTestList);
    for (int i = 0; i < n; i++)
    {
        CheckCanonical(UriTestList[i].Uri, UriTestList[i].CanonicalUri);
    }
}

void WinUriUnitTests::CreateBaseURI()
{
    int n = ARRAYSIZE(UriTestList);
    for (int i = 0; i < n; i++)
    {
        xref_ptr<IPALUri> spUri;
        xref_ptr<IPALUri> spBaseUri;

        const wchar_t *wsUri = UriTestList[i].Uri;
        CreateWinUriHelper(wsUri, spUri);
        VERIFY_SUCCEEDED(spUri->CreateBaseURI(spBaseUri.ReleaseAndGetAddressOf()));
        VERIFY_IS_TRUE(!!spBaseUri);

        XUINT32 dwBufferLength = 0;
        VERIFY_SUCCEEDED(spBaseUri->GetCanonical(&dwBufferLength, nullptr));

        std::unique_ptr<wchar_t[]> spBuffer(new wchar_t[dwBufferLength]);
        VERIFY_SUCCEEDED(spBaseUri->GetCanonical(&dwBufferLength, spBuffer.get()));
    }
}

void WinUriUnitTests::GetElements()
{
    wchar_t wsBuffer[1024];
    XUINT32 dwBufferLength;

    int n = ARRAYSIZE(UriTestList);
    for (int i = 0; i < n; i++)
    {
        xref_ptr<IPALUri> spUri;

        const wchar_t *wsUri = UriTestList[i].Uri;

        CreateWinUriHelper(wsUri, spUri);

        if (UriTestList[i].Extension)
        {
            dwBufferLength = ARRAYSIZE(wsBuffer);
            VERIFY_SUCCEEDED(spUri->GetExtension(&dwBufferLength, wsBuffer));
            VERIFY_ARE_EQUAL(dwBufferLength, wcslen(UriTestList[i].Extension));
            VERIFY_ARE_EQUAL(0, wcscmp(UriTestList[i].Extension, wsBuffer));
        }

        if (UriTestList[i].Filename)
        {
            dwBufferLength = ARRAYSIZE(wsBuffer);
            VERIFY_SUCCEEDED(spUri->GetFileName(&dwBufferLength, wsBuffer));
            VERIFY_ARE_EQUAL(dwBufferLength, wcslen(UriTestList[i].Filename));
            VERIFY_ARE_EQUAL(0, wcscmp(UriTestList[i].Filename, wsBuffer));
        }


        if (UriTestList[i].Path)
        {
            dwBufferLength = ARRAYSIZE(wsBuffer);
            VERIFY_SUCCEEDED(spUri->GetPath(&dwBufferLength, wsBuffer));
            VERIFY_ARE_EQUAL(dwBufferLength, wcslen(UriTestList[i].Path));
            VERIFY_ARE_EQUAL(0, wcscmp(UriTestList[i].Path, wsBuffer));
        }

        if (UriTestList[i].Scheme)
        {
            dwBufferLength = ARRAYSIZE(wsBuffer);
            VERIFY_SUCCEEDED(spUri->GetScheme(&dwBufferLength, wsBuffer));
            VERIFY_ARE_EQUAL(dwBufferLength, wcslen(UriTestList[i].Scheme));
            VERIFY_ARE_EQUAL(0, wcscmp(UriTestList[i].Scheme, wsBuffer));
        }

        if (UriTestList[i].Host)
        {
            dwBufferLength = ARRAYSIZE(wsBuffer);
            VERIFY_SUCCEEDED(spUri->GetHost(&dwBufferLength, wsBuffer));
            VERIFY_ARE_EQUAL(dwBufferLength, wcslen(UriTestList[i].Host));
            VERIFY_ARE_EQUAL(0, wcscmp(UriTestList[i].Host, wsBuffer));
        }

        if (UriTestList[i].Username)
        {
            dwBufferLength = ARRAYSIZE(wsBuffer);
            VERIFY_SUCCEEDED(spUri->GetUsername(&dwBufferLength, wsBuffer));
            VERIFY_ARE_EQUAL(dwBufferLength, wcslen(UriTestList[i].Username));
            VERIFY_ARE_EQUAL(0, wcscmp(UriTestList[i].Username, wsBuffer));
        }

        if (UriTestList[i].Password)
        {
            dwBufferLength = ARRAYSIZE(wsBuffer);
            VERIFY_SUCCEEDED(spUri->GetPassword(&dwBufferLength, wsBuffer));
            VERIFY_ARE_EQUAL(dwBufferLength, wcslen(UriTestList[i].Password));
            VERIFY_ARE_EQUAL(0, wcscmp(UriTestList[i].Password, wsBuffer));
        }
    }

}

void WinUriUnitTests::GetFilePath()
{
    int n = ARRAYSIZE(UriTestList);
    for (int i = 0; i < n; i++)
    {
        if (UriTestList[i].File != nullptr)
        {
            xref_ptr<IPALUri> spUri;

            const wchar_t *wsUri = UriTestList[i].Uri;

            CreateWinUriHelper(wsUri, spUri);

            wchar_t wsBuffer[1024];
            XUINT32 dwBufferLength = ARRAYSIZE(wsBuffer);

            VERIFY_SUCCEEDED(spUri->GetFilePath(&dwBufferLength, wsBuffer));
            VERIFY_ARE_EQUAL(0, wcscmp(wsBuffer, UriTestList[i].File));
        }
    }
}

void WinUriUnitTests::TransformToMsResourceUri()
{
    int n = ARRAYSIZE(UriTestList);
    for (int i = 0; i < n; i++)
    {
        xref_ptr<IPALUri> spUri;
        xref_ptr<IPALUri> spMsResourceUri;

        const wchar_t *wsUri = UriTestList[i].Uri;

        CreateWinUriHelper(wsUri, spUri);

        VERIFY_SUCCEEDED(spUri->TransformToMsResourceUri(spMsResourceUri.ReleaseAndGetAddressOf()));

        wchar_t wsBuffer[1024];
        XUINT32 dwBufferLength = ARRAYSIZE(wsBuffer);

        VERIFY_SUCCEEDED(spMsResourceUri->GetCanonical(&dwBufferLength, wsBuffer));

        VERIFY_ARE_EQUAL(0, wcscmp(wsBuffer, UriTestList[i].MsResourceUri));
    }
}

void WinUriUnitTests::Combine()
{
    int n = ARRAYSIZE(CombinedTestList);
    for (int i = 0; i < n; i++)
    {
        xref_ptr<IPALUri> spUri;
        xref_ptr<IPALUri> spCombinedUri;

        const wchar_t *wsUri = CombinedTestList[i].SourceUri;

        CreateWinUriHelper(wsUri, spUri);
        VERIFY_SUCCEEDED(spUri->Combine(static_cast<XUINT32>(wcslen(CombinedTestList[i].Added)), CombinedTestList[i].Added, spCombinedUri.ReleaseAndGetAddressOf()));

        wchar_t wsBuffer[1024];
        XUINT32 dwBufferLength = ARRAYSIZE(wsBuffer);
        VERIFY_SUCCEEDED(spCombinedUri->GetCanonical(&dwBufferLength, wsBuffer));

        VERIFY_ARE_EQUAL(0, wcscmp(wsBuffer, CombinedTestList[i].CombinedUri));
    }
}

} } } } }