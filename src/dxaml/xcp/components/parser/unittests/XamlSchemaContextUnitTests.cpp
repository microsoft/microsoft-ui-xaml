// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XamlSchemaContextUnitTests.h"
#include "ParserUtilities.h"
#include "ParserUnitTestIncludes.h"
#include "MockParserCoreServices.h"
#include <XamlLogging.h>
#include <CStaticLock.h>
#include <ThreadLocalStorage.h>
#include <MockDynamicMetadataStorage.h>

using namespace DirectUI;
using namespace xaml_interop;
using namespace ::Windows::Internal;
using namespace Microsoft::UI::Xaml::Tests::Metadata;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    #pragma region Test Class Initialization & Cleanup
    bool XamlSchemaContextUnitTests::ClassSetup()
    {
        THROW_IF_FAILED(StaticLockGlobalInit());
        return true;
    }

    bool XamlSchemaContextUnitTests::ClassCleanup()
    {
        StaticLockGlobalDeinit();
        return true;
    }
    #pragma endregion

    void XamlSchemaContextUnitTests::Create()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        std::shared_ptr<ParserErrorReporter> spCustomErrorReporter;

        // construct schema context with no custom error reporter
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);
        VERIFY_IS_NULL(spNullErrorReporter);
        VERIFY_IS_NOT_NULL(spSchemaContext);

        // ensure default error reporter was set
        std::shared_ptr<ParserErrorReporter> outErrorReporter;
        VERIFY_SUCCEEDED(spSchemaContext->GetErrorService(outErrorReporter));
        VERIFY_IS_NOT_NULL(outErrorReporter);

        // construct schema context with a specific custom error reporter
        spCustomErrorReporter = std::make_shared<ParserErrorService>();
        auto spSchemaContextWithCustomReporter = GetSchemaContext(spCustomErrorReporter);
        VERIFY_IS_NOT_NULL(spSchemaContextWithCustomReporter);

        // ensure custom error reporter was set
        VERIFY_SUCCEEDED(spSchemaContextWithCustomReporter->GetErrorService(outErrorReporter));
        VERIFY_ARE_EQUAL(outErrorReporter, std::static_pointer_cast<ParserErrorReporter>(spCustomErrorReporter));
    }

    void XamlSchemaContextUnitTests::SpecialNamespacesInitialized()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);

        {
            std::shared_ptr<XamlNamespace> spXamlNamespace;
            std::shared_ptr<DirectiveProperty> spXamlProperty;

            // verify xml namespace
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strXmlUri, L"http://www.w3.org/XML/1998/namespace");
            VERIFY_SUCCEEDED(spSchemaContext->GetXamlXmlNamespace(c_strXmlUri, spXamlNamespace));
            VERIFY_IS_NOT_NULL(spXamlNamespace);

            // verify namespace contains xml:space directive
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strXmlSpaceDirective, L"space");
            VERIFY_SUCCEEDED(spXamlNamespace->GetDirectiveProperty(c_strXmlSpaceDirective, spXamlProperty));
            VERIFY_IS_NOT_NULL(spXamlProperty);

            // verify namespace doesn't contain xml:foo directive
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strXmlFooDirective, L"foo");
            VERIFY_SUCCEEDED(spXamlNamespace->GetDirectiveProperty(c_strXmlFooDirective, spXamlProperty));
            VERIFY_IS_NULL(spXamlProperty);
        }

        {
            std::shared_ptr<XamlNamespace> spXamlNamespace;
            std::shared_ptr<DirectiveProperty> spXamlProperty;
            std::shared_ptr<XamlType> spXamlType;

            // verify xaml namespace
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strMarkupNamespace, L"http://schemas.microsoft.com/winfx/2006/xaml");
            VERIFY_SUCCEEDED(spSchemaContext->GetXamlXmlNamespace(c_strMarkupNamespace, spXamlNamespace));
            VERIFY_IS_NOT_NULL(spXamlNamespace);

            // verify namespace contains x:Class directive
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strXamlClassDirective, L"Class");
            VERIFY_SUCCEEDED(spXamlNamespace->GetDirectiveProperty(c_strXamlClassDirective, spXamlProperty));
            VERIFY_IS_NOT_NULL(spXamlProperty);

            // verify namespace contains x:String type
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strXamlStringType, L"String");
            VERIFY_SUCCEEDED(spXamlNamespace->GetXamlType(c_strXamlStringType, spXamlType));
            VERIFY_IS_NOT_NULL(spXamlType);

            // verify namespace contains x:DeferLoadStrategy type
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strXamlDeferLoadStrategyDirective, L"DeferLoadStrategy");
            VERIFY_SUCCEEDED(spXamlNamespace->GetDirectiveProperty(c_strXamlDeferLoadStrategyDirective, spXamlProperty));
            VERIFY_IS_NOT_NULL(spXamlProperty);

            // verify namespace contains x:Load type
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strXamlLoadDirective, L"Load");
            VERIFY_SUCCEEDED(spXamlNamespace->GetDirectiveProperty(c_strXamlLoadDirective, spXamlProperty));
            VERIFY_IS_NOT_NULL(spXamlProperty);

            // verify namespace doesn't contain x:foo directive
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strXmlFooDirective, L"foo");
            VERIFY_SUCCEEDED(spXamlNamespace->GetDirectiveProperty(c_strXmlFooDirective, spXamlProperty));
            VERIFY_IS_NULL(spXamlProperty);
        }
    }

    void XamlSchemaContextUnitTests::SourceAssemblies()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);

        // verify default source assembly is "Microsoft.UI.Xaml"
        xstring_ptr strAssembly;
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strDefaultAssembly, L"Microsoft.UI.Xaml");
        strAssembly = spSchemaContext->GetSourceAssembly();
        VERIFY_ARE_EQUAL(true, !!strAssembly.Equals(c_strDefaultAssembly, xstrCompareCaseSensitive));

        // verify push source assembly
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strNewAssembly, L"Microsoft.UI.Xaml.phone");
        VERIFY_SUCCEEDED(spSchemaContext->PushSourceAssembly(c_strNewAssembly));
        strAssembly = spSchemaContext->GetSourceAssembly();
        VERIFY_ARE_EQUAL(true, !!strAssembly.Equals(c_strNewAssembly, xstrCompareCaseSensitive));

        // verify pop source assembly
        spSchemaContext->PopSourceAssembly();
        strAssembly = spSchemaContext->GetSourceAssembly();
        VERIFY_ARE_EQUAL(true, !!strAssembly.Equals(c_strDefaultAssembly, xstrCompareCaseSensitive));
    }

    void XamlSchemaContextUnitTests::XamlAsssemblyInSchema()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);

        // verify custom assembly is added to schema and can be retrieved
        {
            XamlAssemblyToken myAssemblyToken(tpkNative, 5);
            xstring_ptr strOutAssembly;
            std::shared_ptr<XamlAssembly> spOutAssembly;
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strMyAssemblyName, L"Foobar");

            VERIFY_SUCCEEDED(spSchemaContext->AddAssembly(myAssemblyToken, c_strMyAssemblyName));
            VERIFY_SUCCEEDED(spSchemaContext->GetXamlAssembly(myAssemblyToken, spOutAssembly));
            VERIFY_ARE_EQUAL(spOutAssembly->get_AssemblyToken(), myAssemblyToken);
            VERIFY_SUCCEEDED(spOutAssembly->get_Name(&strOutAssembly));
            VERIFY_ARE_EQUAL(true, !!strOutAssembly.Equals(c_strMyAssemblyName, xstrCompareCaseInsensitive));

            // TODO: Adding a different XamlAssembly by name using the same token will silently fail.
        }

        // verify custom assembly cannot be retrieved if it was not added.
        {
            XamlAssemblyToken myAssemblyToken(tpkNative, 6);
            std::shared_ptr<XamlAssembly> spOutAssembly;

            VERIFY_FAILED(spSchemaContext->GetXamlAssembly(myAssemblyToken, spOutAssembly));
        }

        // verify GetXamlAssembly will add the assembly to schema if not present
        {
            XamlAssemblyToken myAssemblyToken(tpkNative, 7);
            xstring_ptr strOutAssembly;
            std::shared_ptr<XamlAssembly> spOutAssembly;
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strMyAssemblyName, L"Foobar2");

            VERIFY_SUCCEEDED(spSchemaContext->GetXamlAssembly(myAssemblyToken, c_strMyAssemblyName, spOutAssembly));
            VERIFY_ARE_EQUAL(spOutAssembly->get_AssemblyToken(), myAssemblyToken);
            VERIFY_SUCCEEDED(spOutAssembly->get_Name(&strOutAssembly));
            VERIFY_ARE_EQUAL(true, !!strOutAssembly.Equals(c_strMyAssemblyName, xstrCompareCaseSensitive));
        }
    }

    void XamlSchemaContextUnitTests::CrackXmlns()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);
        xstring_ptr strOutNamespace;
        bool isValidXmlns = false;

        // verify positive case
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strFullNamespace, L"using:Application1");
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strNamespace, L"Application1");
        VERIFY_SUCCEEDED(spSchemaContext->CrackXmlns(c_strFullNamespace, strOutNamespace, isValidXmlns));
        VERIFY_IS_TRUE(isValidXmlns);
        VERIFY_ARE_EQUAL(true, !!strOutNamespace.Equals(c_strNamespace, xstrCompareCaseSensitive));

        // verify null and empty strings cannot be cracked
        VERIFY_SUCCEEDED(spSchemaContext->CrackXmlns(xstring_ptr::EmptyString(), strOutNamespace, isValidXmlns));
        VERIFY_IS_FALSE(isValidXmlns);
        VERIFY_SUCCEEDED(spSchemaContext->CrackXmlns(xstring_ptr::NullString(), strOutNamespace, isValidXmlns));
        VERIFY_IS_FALSE(isValidXmlns);

        // verify leading spaces fail
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strNamespaceWithLeadingSpace, L"   using:foo");
        VERIFY_SUCCEEDED(spSchemaContext->CrackXmlns(c_strNamespaceWithLeadingSpace, strOutNamespace, isValidXmlns));
        VERIFY_IS_FALSE(isValidXmlns);

        // verify namespace no ':' delimeter fails
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strNamespaceNoDelimeter, L"using foo");
        VERIFY_SUCCEEDED(spSchemaContext->CrackXmlns(c_strNamespaceNoDelimeter, strOutNamespace, isValidXmlns));
        VERIFY_IS_FALSE(isValidXmlns);

        // verify namespace with extra ':' delimeter fails
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strNamespaceExtraDelimeter, L"using:foo:two");
        VERIFY_SUCCEEDED(spSchemaContext->CrackXmlns(c_strNamespaceExtraDelimeter, strOutNamespace, isValidXmlns));
        VERIFY_IS_FALSE(isValidXmlns);

        // verify namespace without 'using' prefix fails
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strNamespaceBadPrefix, L"clr-namespace:foo");
        VERIFY_SUCCEEDED(spSchemaContext->CrackXmlns(c_strNamespaceBadPrefix, strOutNamespace, isValidXmlns));
        VERIFY_IS_FALSE(isValidXmlns);

        // verify namespace with empty namespace after 'using' prefix fails
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strNoNamespace, L"using:");
        VERIFY_SUCCEEDED(spSchemaContext->CrackXmlns(c_strNoNamespace, strOutNamespace, isValidXmlns));
        VERIFY_IS_FALSE(isValidXmlns);
    }

    void XamlSchemaContextUnitTests::XamlNamespacesInSchema()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);
        auto mock = TlsProvider<DynamicMetadataStorageMock>::CreateWrappedObject();

        // verify internal xaml namespace
        {
            std::shared_ptr<XamlNamespace> spXamlNamespace;

            DECLARE_CONST_STRING_IN_TEST_CODE(c_strMarkupNamespace, L"http://schemas.microsoft.com/winfx/2006/xaml");
            VERIFY_SUCCEEDED(spSchemaContext->GetXamlXmlNamespace(c_strMarkupNamespace, spXamlNamespace));
            VERIFY_IS_NOT_NULL(spXamlNamespace);
        }

        // verify custom xaml namespace gets added automatically
        {
            std::shared_ptr<XamlNamespace> spXamlNamespace;

            DECLARE_CONST_STRING_IN_TEST_CODE(c_strCustomUsingNamespace, L"using:Application1");
            VERIFY_SUCCEEDED(spSchemaContext->GetXamlXmlNamespace(c_strCustomUsingNamespace, spXamlNamespace));
            VERIFY_IS_NOT_NULL(spXamlNamespace);
            xstring_ptr strOutNamespace = spXamlNamespace->get_TargetNamespace();
            VERIFY_ARE_EQUAL(true, !!strOutNamespace.Equals(c_strCustomUsingNamespace));
        }

        // verify xml namespaces can lookup types in registered type namespaces
        {
            XamlAssemblyToken myAssemblyToken(tpkNative, 2);
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strMyAssemblyName, L"Microsoft.UI.Xaml");

            XamlTypeNamespaceToken myTypeNamespaceToken;
            myTypeNamespaceToken.SetProviderKind(tpkNative);
            myTypeNamespaceToken.SetHandle(KnownNamespaceIndex::Microsoft_UI_Xaml_Controls);
            myTypeNamespaceToken.AssemblyToken = myAssemblyToken;

            DECLARE_CONST_STRING_IN_TEST_CODE(c_strPresentationNamespace, L"http://schemas.microsoft.com/winfx/2006/xaml/presentation");
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strControlsNamespace, L"Microsoft.UI.Xaml.Controls");

            VERIFY_FAILED(spSchemaContext->AddAssemblyXmlnsDefinition(myAssemblyToken, c_strPresentationNamespace, myTypeNamespaceToken, c_strControlsNamespace));
            VERIFY_SUCCEEDED(spSchemaContext->AddAssembly(myAssemblyToken, c_strMyAssemblyName));
            VERIFY_SUCCEEDED(spSchemaContext->AddAssemblyXmlnsDefinition(myAssemblyToken, c_strPresentationNamespace, myTypeNamespaceToken, c_strControlsNamespace));

            std::shared_ptr<XamlType> spXamlType;
            std::shared_ptr<XamlNamespace> spOutNamespace;
            VERIFY_SUCCEEDED(spSchemaContext->GetXamlXmlNamespace(c_strPresentationNamespace, spOutNamespace));

            DECLARE_CONST_STRING_IN_TEST_CODE(c_strGridType, L"Grid");
            VERIFY_SUCCEEDED(spOutNamespace->GetXamlType(c_strGridType, spXamlType));
            VERIFY_IS_NOT_NULL(spXamlType);

            // ensure we can retrieve namespace by runtime index
            std::shared_ptr<XamlNamespace> spOutXamlNamespace2 = spSchemaContext->GetXamlNamespaceFromRuntimeIndex(spOutNamespace->get_RuntimeIndex());
            VERIFY_ARE_EQUAL(spOutXamlNamespace2, spOutNamespace);
        }
    }

    void XamlSchemaContextUnitTests::XamlTypeNamespacesInSchema()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);

        XamlAssemblyToken myAssemblyToken(tpkNative, 2);
        std::shared_ptr<XamlAssembly> spAssembly;
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strMyAssemblyName, L"Microsoft.UI.Xaml");
        VERIFY_SUCCEEDED(spSchemaContext->AddAssembly(myAssemblyToken, c_strMyAssemblyName));
        VERIFY_SUCCEEDED(spSchemaContext->GetXamlAssembly(myAssemblyToken, spAssembly));

        XamlTypeNamespaceToken myTypeNamespaceToken;
        myTypeNamespaceToken.SetProviderKind(tpkNative);
        myTypeNamespaceToken.SetHandle(KnownNamespaceIndex::Microsoft_UI_Xaml_Controls);
        myTypeNamespaceToken.AssemblyToken = myAssemblyToken;

        // verify type namespace is added and can be retrieved
        std::shared_ptr<XamlTypeNamespace> spOutTypeNamespace;
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strControlsNamespace, L"Microsoft.UI.Xaml.Controls");
        VERIFY_SUCCEEDED(spSchemaContext->GetXamlTypeNamespace(myTypeNamespaceToken, c_strControlsNamespace, spAssembly, std::shared_ptr<XamlXmlNamespace>(), spOutTypeNamespace));
        VERIFY_IS_NOT_NULL(spOutTypeNamespace);
        VERIFY_ARE_EQUAL(spOutTypeNamespace->get_TypeNamespaceToken(), myTypeNamespaceToken);

        // verify we get back the same entry on repeated requested
        std::shared_ptr<XamlTypeNamespace> spOutTypeNamespace2;
        VERIFY_SUCCEEDED(spSchemaContext->GetXamlTypeNamespace(myTypeNamespaceToken, c_strControlsNamespace, spAssembly, std::shared_ptr<XamlXmlNamespace>(), spOutTypeNamespace2));
        VERIFY_ARE_EQUAL(spOutTypeNamespace, spOutTypeNamespace2);
    }

    void XamlSchemaContextUnitTests::XamlTypesInSchema()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);

        // verify GetXamlType can retrieve type from known providers
        std::shared_ptr<XamlType> spXamlType;
        XamlTypeToken gridToken(tpkNative, KnownTypeIndex::Grid);
        VERIFY_SUCCEEDED(spSchemaContext->GetXamlType(gridToken, spXamlType));
        VERIFY_IS_NOT_NULL(spXamlType);

        std::shared_ptr<XamlTypeNamespace> spNamespace;
        VERIFY_SUCCEEDED(spSchemaContext->GetXamlType(gridToken, spNamespace, spXamlType));
        VERIFY_IS_NOT_NULL(spXamlType);

        // verify we retrieved the same object.
        std::shared_ptr<XamlType> spXamlType2;
        VERIFY_SUCCEEDED(spSchemaContext->GetXamlType(gridToken, spXamlType2));
        VERIFY_ARE_EQUAL(spXamlType, spXamlType2);

        // verify we can retrieve by runtime index
        spXamlType2 = spSchemaContext->GetXamlTypeFromRuntimeIndex(spXamlType->get_RuntimeIndex());
        VERIFY_ARE_EQUAL(spXamlType, spXamlType2);

        // Try to cache an unknown type
        VERIFY_SUCCEEDED(spSchemaContext->GetXamlType(XamlTypeToken(tpkNative, static_cast<KnownTypeIndex>(-1)), spXamlType));
        spXamlType2 = spSchemaContext->GetXamlTypeFromRuntimeIndex(spXamlType->get_RuntimeIndex());
        VERIFY_ARE_EQUAL(spXamlType, spXamlType2);
    }

    void XamlSchemaContextUnitTests::XamlPropertiesInSchema()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);
        auto mock = TlsProvider<DynamicMetadataStorageMock>::CreateWrappedObject();

        // verify GetXamlProperty fully qualified property
        {
            std::shared_ptr<XamlProperty> spXamlProperty;
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strBorderThicknessProperty, L"[using:Microsoft.UI.Xaml.Controls]Border.BorderThickness");
            VERIFY_SUCCEEDED(spSchemaContext->GetXamlProperty(c_strBorderThicknessProperty, spXamlProperty));
            VERIFY_IS_NOT_NULL(spXamlProperty);
            VERIFY_ARE_EQUAL(spXamlProperty->get_PropertyToken(), XamlPropertyToken(tpkNative, KnownPropertyIndex::Border_BorderThickness));
        }

        // verify GetXamlProperty fully qualified property failure cases
        {
            std::shared_ptr<XamlProperty> spXamlProperty;
            VERIFY_FAILED(spSchemaContext->GetXamlProperty(xstring_ptr::EmptyString(), spXamlProperty));
            VERIFY_FAILED(spSchemaContext->GetXamlProperty(xstring_ptr::NullString(), spXamlProperty));

            DECLARE_CONST_STRING_IN_TEST_CODE(c_strNoPrefixQualifier, L"using:Microsoft.UI.Xaml.Controls]Border.BorderThickness");
            VERIFY_FAILED(spSchemaContext->GetXamlProperty(c_strNoPrefixQualifier, spXamlProperty));

            DECLARE_CONST_STRING_IN_TEST_CODE(c_strMissingEndQualifier, L"[using:Microsoft.UI.Xaml.ControlsBorder.BorderThickness");
            VERIFY_FAILED(spSchemaContext->GetXamlProperty(c_strMissingEndQualifier, spXamlProperty));

            DECLARE_CONST_STRING_IN_TEST_CODE(c_strMissingProperty, L"[using:Microsoft.UI.Xaml.Controls]Border");
            VERIFY_FAILED(spSchemaContext->GetXamlProperty(c_strMissingProperty, spXamlProperty));

            DECLARE_CONST_STRING_IN_TEST_CODE(c_strWrongProperty, L"[using:Microsoft.UI.Xaml.Controls]Border.Foobar");
            VERIFY_FAILED(spSchemaContext->GetXamlProperty(c_strWrongProperty, spXamlProperty));
        }

        // verify GetXamlProperty by token
        {
            std::shared_ptr<XamlProperty> spXamlProperty;
            std::shared_ptr<XamlProperty> spXamlProperty2;
            VERIFY_SUCCEEDED(spSchemaContext->GetXamlProperty(XamlPropertyToken(tpkNative, KnownPropertyIndex::Border_BorderThickness), XamlTypeToken(tpkNative, KnownTypeIndex::Thickness), spXamlProperty));
            VERIFY_SUCCEEDED(spSchemaContext->GetXamlProperty(XamlPropertyToken(tpkNative, KnownPropertyIndex::Border_BorderThickness), spXamlProperty2));
            VERIFY_ARE_EQUAL(spXamlProperty, spXamlProperty2);

            spXamlProperty2 = spSchemaContext->GetXamlPropertyFromRuntimeIndex(spXamlProperty->get_RuntimeIndex());
            VERIFY_ARE_EQUAL(spXamlProperty, spXamlProperty2);

            // Known properties can be retrieved "on the fly" without needing to specify a type (since the system really knows the type anyway)
            spXamlProperty.reset();
            VERIFY_SUCCEEDED(spSchemaContext->GetXamlProperty(XamlPropertyToken(tpkNative, KnownPropertyIndex::Border_BorderBrush), spXamlProperty));

            spXamlProperty2 = spSchemaContext->GetXamlPropertyFromRuntimeIndex(spXamlProperty->get_RuntimeIndex());
            VERIFY_ARE_EQUAL(spXamlProperty, spXamlProperty2);

            // However, custom properties can not.
            spXamlProperty.reset();
            VERIFY_FAILED(spSchemaContext->GetXamlProperty(XamlPropertyToken(tpkManaged, static_cast<KnownPropertyIndex>(-1)), spXamlProperty));

            // Attempt to cache a custom property with a custom type
            auto oldTestHookValue = spSchemaContext->get_TestHook_UseEmptyXamlTypeNamespaceInGetXamlProperty();
            spSchemaContext->set_TestHook_UseEmptyXamlTypeNamespaceInGetXamlProperty(true);
            auto guard = wil::scope_exit([&spSchemaContext, &oldTestHookValue]()
            {
                spSchemaContext->set_TestHook_UseEmptyXamlTypeNamespaceInGetXamlProperty(oldTestHookValue);
            });
            VERIFY_SUCCEEDED(spSchemaContext->GetXamlProperty(XamlPropertyToken(tpkNative, static_cast<KnownPropertyIndex>(-1)), XamlTypeToken(tpkNative, static_cast<KnownTypeIndex>(-1)), spXamlProperty));

            spXamlProperty2 = spSchemaContext->GetXamlPropertyFromRuntimeIndex(spXamlProperty->get_RuntimeIndex());
            VERIFY_ARE_EQUAL(spXamlProperty, spXamlProperty2);
        }
    }

    void XamlSchemaContextUnitTests::XamlTypeInfoProviderInSchema()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);

        // verify tpkNative provider
        {
            std::shared_ptr<XamlTypeInfoProvider> spNativeProvider;
            xstring_ptr strFullName;
            VERIFY_SUCCEEDED(spSchemaContext->GetTypeInfoProvider(tpkNative, spNativeProvider));
            VERIFY_SUCCEEDED(spNativeProvider->GetTypeFullName(XamlTypeToken(tpkNative, KnownTypeIndex::Grid), &strFullName));
            VERIFY_ARE_EQUAL(true, !!strFullName.Equals(L"Microsoft.UI.Xaml.Controls.Grid", xstrCompareCaseSensitive));

            // TODO : This is broken - the managed types can be retrieved from the  type provider because the type
            // provider will just call the Metadata API and retrieve the type. Either consolidate the providers or validate before
            // calling the Metadata API.
            //
            // VERIFY_SUCCEEDED(spNativeProvider->GetTypeFullName(XamlTypeToken(tpkManaged, KnownTypeIndex::ItemsControl), &strFullName));
            // VERIFY_ARE_EQUAL(true, strFullName.IsNullOrEmpty());
        }

        // verify tpkManaged provider
        {
            std::shared_ptr<XamlTypeInfoProvider> spManagedProvider;
            xstring_ptr strFullName;
            VERIFY_SUCCEEDED(spSchemaContext->GetTypeInfoProvider(tpkManaged, spManagedProvider));
            VERIFY_SUCCEEDED(spManagedProvider->GetTypeFullName(XamlTypeToken(tpkNative, KnownTypeIndex::ItemsControl), &strFullName));
            VERIFY_ARE_EQUAL(true, !!strFullName.Equals(L"Microsoft.UI.Xaml.Controls.ItemsControl", xstrCompareCaseSensitive));

            // TODO : This is broken - the managed types can be retrieved from the  type provider because the type
            // provider will just call the Metadata API and retrieve the type. Either consolidate the providers or validate before
            // calling the Metadata API.
            //
            // VERIFY_SUCCEEDED(spManagedProvider->GetTypeFullName(XamlTypeToken(tpkNative, KnownTypeIndex::Grid), &strFullName));
            // VERIFY_ARE_EQUAL(true, strFullName.IsNullOrEmpty());
        }

        // verify unknown provider
        {
            std::shared_ptr<XamlTypeInfoProvider> spProvider;
            VERIFY_FAILED(spSchemaContext->GetTypeInfoProvider(tpkUnknown, spProvider));
        }
    }

    void XamlSchemaContextUnitTests::CachedTypesAndProperties()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);

        // verify x:Key
        {
            std::shared_ptr<DirectiveProperty> spProperty;

            VERIFY_SUCCEEDED(spSchemaContext->get_X_KeyProperty(spProperty));
            VERIFY_ARE_EQUAL(true, spProperty->IsDirective());
            VERIFY_ARE_EQUAL(xdKey, spProperty->get_DirectiveKind());

            std::shared_ptr<DirectiveProperty> spProperty2;
            VERIFY_SUCCEEDED(spSchemaContext->get_X_KeyProperty(spProperty2));
            VERIFY_ARE_EQUAL(spProperty, spProperty2);
        }

        // verify x:Name
        {
            std::shared_ptr<DirectiveProperty> spProperty;
            VERIFY_SUCCEEDED(spSchemaContext->get_X_NameProperty(spProperty));
            VERIFY_ARE_EQUAL(true, spProperty->IsDirective());
            VERIFY_ARE_EQUAL(xdName, spProperty->get_DirectiveKind());

            std::shared_ptr<DirectiveProperty> spProperty2;
            VERIFY_SUCCEEDED(spSchemaContext->get_X_NameProperty(spProperty2));
            VERIFY_ARE_EQUAL(spProperty, spProperty2);
        }

        // verify x:String Type
        {
            std::shared_ptr<XamlType> spType;
            VERIFY_SUCCEEDED(spSchemaContext->get_StringXamlType(spType));
            VERIFY_ARE_EQUAL(spType->get_TypeToken(), XamlTypeToken(tpkNative, KnownTypeIndex::String));

            std::shared_ptr<XamlType> spType2;
            VERIFY_SUCCEEDED(spSchemaContext->get_StringXamlType(spType2));
            VERIFY_ARE_EQUAL(spType, spType2);

            std::shared_ptr<XamlTextSyntax> spTextSyntax;
            VERIFY_SUCCEEDED(spSchemaContext->get_StringSyntax(spTextSyntax));
            VERIFY_ARE_EQUAL(spTextSyntax->get_TextSyntaxToken(), XamlTypeToken(tpkNative, KnownTypeIndex::String));

            std::shared_ptr<XamlTextSyntax> spTextSyntax2;
            VERIFY_SUCCEEDED(spSchemaContext->get_StringSyntax(spTextSyntax2));
            VERIFY_ARE_EQUAL(spTextSyntax, spTextSyntax2);
        }

        // verify x:Int32 Type
        {
            std::shared_ptr<XamlType> spType;
            VERIFY_SUCCEEDED(spSchemaContext->get_Int32XamlType(spType));
            VERIFY_ARE_EQUAL(spType->get_TypeToken(), XamlTypeToken(tpkNative, KnownTypeIndex::Int32));

            std::shared_ptr<XamlType> spType2;
            VERIFY_SUCCEEDED(spSchemaContext->get_Int32XamlType(spType2));
            VERIFY_ARE_EQUAL(spType, spType2);

            std::shared_ptr<XamlTextSyntax> spTextSyntax;
            VERIFY_SUCCEEDED(spSchemaContext->get_Int32Syntax(spTextSyntax));
            VERIFY_ARE_EQUAL(spTextSyntax->get_TextSyntaxToken(), XamlTypeToken(tpkNative, KnownTypeIndex::Int32));

            std::shared_ptr<XamlTextSyntax> spTextSyntax2;
            VERIFY_SUCCEEDED(spSchemaContext->get_Int32Syntax(spTextSyntax2));
            VERIFY_ARE_EQUAL(spTextSyntax, spTextSyntax2);
        }

        // verify implicit initialization property
        {
            std::shared_ptr<ImplicitProperty> spProperty;
            VERIFY_SUCCEEDED(spSchemaContext->get_InitializationProperty(spProperty));
            VERIFY_ARE_EQUAL(true, spProperty->IsImplicit());
            VERIFY_ARE_EQUAL(iptInitialization, spProperty->get_ImplicitPropertyType());

            std::shared_ptr<ImplicitProperty> spProperty2;
            VERIFY_SUCCEEDED(spSchemaContext->get_InitializationProperty(spProperty2));
            VERIFY_ARE_EQUAL(spProperty, spProperty2);
        }

        // verify implicit items property
        {
            std::shared_ptr<ImplicitProperty> spProperty;
            std::shared_ptr<XamlType> spType;

            // TODO: spType doesn't do anything here
            VERIFY_SUCCEEDED(spSchemaContext->get_ItemsProperty(spType, spProperty));
            VERIFY_ARE_EQUAL(true, spProperty->IsImplicit());
            VERIFY_ARE_EQUAL(iptItems, spProperty->get_ImplicitPropertyType());

            std::shared_ptr<ImplicitProperty> spProperty2;
            VERIFY_SUCCEEDED(spSchemaContext->get_ItemsProperty(spType, spProperty2));
            VERIFY_ARE_EQUAL(spProperty, spProperty2);
        }
    }

    void XamlSchemaContextUnitTests::SetErrorService()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        std::shared_ptr<ParserErrorReporter> spCustomErrorReporter;

        // construct schema context with no custom error reporter
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);
        VERIFY_IS_NULL(spNullErrorReporter);
        VERIFY_IS_NOT_NULL(spSchemaContext);

        // ensure default error reporter was set
        std::shared_ptr<ParserErrorReporter> outErrorReporter;
        VERIFY_SUCCEEDED(spSchemaContext->GetErrorService(outErrorReporter));
        VERIFY_IS_NOT_NULL(outErrorReporter);

        // validate SetErrorService works
        spCustomErrorReporter = std::make_shared<ParserErrorService>();
        VERIFY_SUCCEEDED(spSchemaContext->SetErrorService(spCustomErrorReporter));
        VERIFY_SUCCEEDED(spSchemaContext->GetErrorService(outErrorReporter));
        VERIFY_ARE_EQUAL(outErrorReporter, spCustomErrorReporter);
    }

    void XamlSchemaContextUnitTests::GetXamlTextSyntax()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        std::shared_ptr<XamlTextSyntax> spTextSyntax;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);

        VERIFY_SUCCEEDED(spSchemaContext->GetXamlTextSyntax(XamlTypeToken(tpkNative, KnownTypeIndex::String), spTextSyntax));
        VERIFY_ARE_EQUAL(spTextSyntax->get_TextSyntaxToken(), XamlTypeToken(tpkNative, KnownTypeIndex::String));

        VERIFY_SUCCEEDED(spSchemaContext->GetXamlTextSyntax(XamlTypeToken(tpkNative, KnownTypeIndex::Int32), spTextSyntax));
        VERIFY_ARE_EQUAL(spTextSyntax->get_TextSyntaxToken(), XamlTypeToken(tpkNative, KnownTypeIndex::Int32));
    }

    void XamlSchemaContextUnitTests::XamlTokenToProviderMapping()
    {
        VERIFY_ARE_EQUAL(tpkNative, XamlPropertyToken::FromProperty(MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::DependencyObject_Line)).GetProviderKind());
        VERIFY_ARE_EQUAL(tpkNative, XamlPropertyToken::FromProperty(MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ItemsControl_IsGrouping)).GetProviderKind());

        // Interface properties are expected to get routed through the managed runtime still, because they currently work with EORs.
        VERIFY_ARE_EQUAL(tpkManaged, XamlPropertyToken::FromProperty(MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ItemsControl_GroupStyle)).GetProviderKind());
    }

    std::shared_ptr<XamlSchemaContext> XamlSchemaContextUnitTests::GetSchemaContext(const std::shared_ptr<ParserErrorReporter>& customErrorReporter)
    {
        ParserUtilities parserUtils;
        return parserUtils.GetSchemaContext(customErrorReporter);
    }

} } } } }

