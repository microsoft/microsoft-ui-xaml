// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ElementStateChangedBuilderTests.h"
#include "ElementStateChangedBuilder.h"
#include <MetadataAPI.h>
#include <metadata\inc\TypeTableStructs.h>
#include <CustomClassInfo.h>
#include <ThreadLocalStorage.h>
#include <criticalsection\inc\CStaticLock.h>

using namespace Diagnostics;
using namespace DirectUI;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace XamlDiagnostics {

    DECLARE_CONST_STRING_IN_TEST_CODE(c_ButtonName, L"Button");
    bool ElementStateChangedBuilderUnitTests::ClassSetup()
    {
        THROW_IF_FAILED(StaticLockGlobalInit());
        return true;
    }

    bool ElementStateChangedBuilderUnitTests::ClassCleanup()
    {
        THROW_IF_FAILED(StaticLockGlobalInit());
        return true;
    }

    void ElementStateChangedBuilderUnitTests::TestDictionaryAccessor()
    {
        ElementStateChangedBuilder builder;

        CDependencyProperty dp;
        dp.SetIndex(KnownPropertyIndex::FrameworkElement_Resources);
        dp.SetPropertyTypeIndex(KnownTypeIndex::ResourceDictionary);
        dp.SetDeclaringTypeIndex(KnownTypeIndex::FrameworkElement);
        VERIFY_SUCCEEDED(builder.AddAccessor(L"myResource"));
        VERIFY_SUCCEEDED(builder.AddProperty(&dp));
        VERIFY_ARE_EQUAL(std::wstring(L"Resources:Microsoft.UI.Xaml.FrameworkElement['myResource']"), builder.GetContext().PathToError);
    }

    void ElementStateChangedBuilderUnitTests::TestArrayAccessor()
    {
        ElementStateChangedBuilder builder;

        CDependencyProperty dp;
        dp.SetIndex(KnownPropertyIndex::Panel_Children);
        dp.SetPropertyTypeIndex(KnownTypeIndex::UIElementCollection);
        dp.SetDeclaringTypeIndex(KnownTypeIndex::Panel);
        VERIFY_SUCCEEDED(builder.AddAccessor(3));
        VERIFY_SUCCEEDED(builder.AddProperty(&dp));
        VERIFY_ARE_EQUAL(std::wstring(L"Children:Microsoft.UI.Xaml.Controls.Panel[3]"), builder.GetContext().PathToError);
    }

    void ElementStateChangedBuilderUnitTests::TestImplicitStyleAccessor()
    {
        ElementStateChangedBuilder builder;

        CDependencyProperty dp;
        dp.SetIndex(KnownPropertyIndex::FrameworkElement_Resources);
        dp.SetPropertyTypeIndex(KnownTypeIndex::ResourceDictionary);
        dp.SetDeclaringTypeIndex(KnownTypeIndex::FrameworkElement);
        const CClassInfo* buttonType = MetadataAPI::GetBuiltinClassInfoByName(c_ButtonName);
        VERIFY_SUCCEEDED(builder.AddAccessor(buttonType));
        VERIFY_SUCCEEDED(builder.AddProperty(&dp));
        VERIFY_ARE_EQUAL(std::wstring(L"Resources:Microsoft.UI.Xaml.FrameworkElement[{x:Type Microsoft.UI.Xaml.Controls.Button}]"), builder.GetContext().PathToError);
    }

    void ElementStateChangedBuilderUnitTests::TestMultiStepAccessor()
    {
        ElementStateChangedBuilder builder;
        
        CDependencyProperty dp;
        dp.SetIndex(KnownPropertyIndex::Setter_Value);
        dp.SetDeclaringTypeIndex(KnownTypeIndex::Setter);
        VERIFY_SUCCEEDED(builder.AddProperty(&dp));

        dp.SetIndex(KnownPropertyIndex::VisualState_Setters);
        dp.SetDeclaringTypeIndex(KnownTypeIndex::VisualState);
        dp.SetPropertyTypeIndex(KnownTypeIndex::SetterBaseCollection);
        VERIFY_SUCCEEDED(builder.AddAccessor(3));
        VERIFY_SUCCEEDED(builder.AddProperty(&dp));

        dp.SetIndex(KnownPropertyIndex::FrameworkElement_Resources);
        dp.SetDeclaringTypeIndex(KnownTypeIndex::FrameworkElement);
        dp.SetPropertyTypeIndex(KnownTypeIndex::ResourceDictionary);
        const CClassInfo* buttonType = MetadataAPI::GetBuiltinClassInfoByName(c_ButtonName);
        VERIFY_SUCCEEDED(builder.AddAccessor(buttonType));
        VERIFY_SUCCEEDED(builder.AddProperty(&dp));
        VERIFY_ARE_EQUAL(std::wstring(L"Resources:Microsoft.UI.Xaml.FrameworkElement[{x:Type Microsoft.UI.Xaml.Controls.Button}]/Setters:Microsoft.UI.Xaml.VisualState[3]/Value:Microsoft.UI.Xaml.Setter"), builder.GetContext().PathToError);
    }
} } } } }
