// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ImageCacheIdentifierUnitTests.h"
#include <XStringUtilities.h>
#include <PixelFormat.h>

#define LOG_OUTPUT(fmt, ...) WEX::Logging::Log::Comment(WEX::Common::String().Format(fmt, __VA_ARGS__))

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Xaml {

    void ImageCacheIdentifierUnitTests::ValidateSimpleUrl()
    {
        DECLARE_CONST_XSTRING_PTR_STORAGE(cnn, L"http://cnn.com");
        xstring_ptr str;

        HRESULT hr = GenerateCacheIdentifier(
            xstring_ptr(cnn),
            100,
            150,
            static_cast<PixelFormat>(3),
            FALSE,
            0,
            &str);

        VERIFY_IS_TRUE(SUCCEEDED(hr));
        VERIFY_IS_TRUE(wcscmp(L"http://cnn.com / width:[ 100 ] height:[ 150 ] format:[ 3 ]", str.GetBuffer()) == 0);
    }

    void ImageCacheIdentifierUnitTests::ValidateInvalidationId()
    {
        DECLARE_CONST_XSTRING_PTR_STORAGE(url, L"this url isn't actually validated by the function");
        xstring_ptr str;

        HRESULT hr = GenerateCacheIdentifier(
            xstring_ptr(url),
            0,
            1234,
            static_cast<PixelFormat>(42),
            TRUE,
            99999,
            &str);

        VERIFY_IS_TRUE(SUCCEEDED(hr));
        VERIFY_IS_TRUE(wcscmp(L"this url isn't actually validated by the function / width:[ 0 ] height:[ 1234 ] format:[ 42 ] / invalidationid:[ 99999 ]", str.GetBuffer()) == 0);
    }

    void ImageCacheIdentifierUnitTests::ValidateLongUrl()
    {
        DECLARE_CONST_XSTRING_PTR_STORAGE(url,
            L"http://hits.theguardian.com/b/ss/guardiangu-feeds/1/H.25.5/5930?ns=guardian&pageName=Article%3Atop-10-crime-movies%3A1982172&ch=Film&c3=GU.co.uk&"
            L"c4=Crime+%28Film+genre%29%2CFilm%2CMartin+Scorsese+%28Film%29%2CRay+Liotta%2CRobert+De+Niro+%28Film%29%2CGangs+%28Society%29%2CMichael+Haneke+%28"
            L"film%29%2CThriller+%28Film+genre%29%2CWorld+cinema+%28Film+genre%29%2CJuliette+Binoche%2CQuentin+Tarantino+%28Film%29%2CJohn+Travolta%2CSamuel+L+"
            L"Jackson%2CBruce+Willis+%28Film%29%2CMichael+Caine%2CJohn+Osborne+%28Playwright%29%2CJames+M+Cain+%28author+kw%29%2CFilm+adaptations+%28Film%29%2C"
            L"Billy+Wilder+%28Film%29%2CRaymond+Chandler+%28Author%29%2CAkira+Kurosawa+%28Film%29%2CTerrence+Malick%2CMartin+Sheen%2CCrime+%28US%29%2CAlfred+Hi"
            L"tchcock+%28Film%29%2CJames+Stewart+%28Film%29%2COrson+Welles+%28Film%29%2CCharlton+Heston+%28Film%29%2CRoman+Polanski+%28Film%29%2CJack+Nicholson"
            L"+%28Film%29%2CFaye+Dunaway%2CJohn+Huston+%28Film%29&c5=Unclassified%2CFilm+Awards%2CNot+commercially+useful%2CCommunities+Society%2CFilm+Reviews%"
            L"2CTheatre&c6=&c7=2013%2F10%2F18+04%3A44&c8=1982172&c9=Blog&c10=Blogpost&c13=Top+10...&c19=GUK&c25=Film+blog&c47=UK&c64=UK&c65=Top+10+crime+movies"
            L"&c66=Culture&c67=nextgen-compatible&c72=&c73=&c74=&c75=&h2=GU%2FCulture%2FFilm%2FCrime");

        xstring_ptr str;

        HRESULT hr = GenerateCacheIdentifier(
            xstring_ptr(url),
            100,
            150,
            pixelUnknown,
            FALSE,
            0,
            &str);

        VERIFY_IS_TRUE(SUCCEEDED(hr));
        VERIFY_IS_TRUE(str.GetCount() > url.Count + 40); // make sure it didn't get truncated
    }

    }
}}}}
