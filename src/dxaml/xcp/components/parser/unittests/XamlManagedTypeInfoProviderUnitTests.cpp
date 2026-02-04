// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XamlManagedTypeInfoProviderUnitTests.h"
#include "ParserUtilities.h"
#include "ParserUnitTestIncludes.h"
#include "MockParserCoreServices.h"
#include <XamlLogging.h>
#include <ThreadLocalStorage.h>
#include <MockDynamicMetadataStorage.h>

using namespace DirectUI;
using namespace Microsoft::UI::Xaml::Tests::Metadata;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    void XamlManagedTypeInfoProviderUnitTests::Create()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);
        auto spManagedProvider = GetManagedTypeInfoProvider(spSchemaContext);

        VERIFY_IS_NOT_NULL(spManagedProvider);
    }

    void XamlManagedTypeInfoProviderUnitTests::ResolveAssembly()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);
        auto spManagedProvider = GetManagedTypeInfoProvider(spSchemaContext);
        XamlAssemblyToken assemblyToken;
        DECLARE_CONST_STRING_IN_TEST_CODE(c_strMyAssemblyName, L"Foobar");

        // TODO: Remove assembly concept from parser because it is redundant. Currently
        // the managed provider has no resolution capability, so this is expected to fail.
        VERIFY_FAILED(spManagedProvider->ResolveAssembly(c_strMyAssemblyName, assemblyToken));
    }

    void XamlManagedTypeInfoProviderUnitTests::GetTypeNamespace()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);
        auto spManagedProvider = GetManagedTypeInfoProvider(spSchemaContext);

        // verify we can resolve known type namespaces
        {
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strKnownTypenamespace, L"using:Microsoft.UI.Xaml.Controls");
            XamlTypeNamespaceToken typeNamespaceToken;
            XamlTypeNamespaceToken controlsNamespaceToken(tpkNative, KnownNamespaceIndex::Microsoft_UI_Xaml_Controls);

            VERIFY_SUCCEEDED(spManagedProvider->GetTypeNamespace(c_strKnownTypenamespace, typeNamespaceToken));
            VERIFY_IS_TRUE(typeNamespaceToken.GetHandle() == controlsNamespaceToken.GetHandle());
        }

        // verify we succeed but return S_FALSE for badly formed typenamespaces
        {
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strKnownTypenamespace, L"Foobar");
            XamlTypeNamespaceToken typeNamespaceToken;

            VERIFY_SUCCEEDED(spManagedProvider->GetTypeNamespace(c_strKnownTypenamespace, typeNamespaceToken));
        }
    }

    void XamlManagedTypeInfoProviderUnitTests::GetTypeNamespaceForType()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);
        auto spManagedProvider = GetManagedTypeInfoProvider(spSchemaContext);

        // verify we can resolve known types
        {
            xstring_ptr typeNamespaceName;
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strControlsNamespace, L"Microsoft.UI.Xaml.Controls");
            XamlTypeNamespaceToken typeNamespaceToken;
            XamlTypeNamespaceToken controlsTypeNamespaceToken(tpkNative, KnownNamespaceIndex::Microsoft_UI_Xaml_Controls);
            XamlTypeToken gridTypeToken(tpkManaged, KnownTypeIndex::Button);

            VERIFY_SUCCEEDED(spManagedProvider->GetTypeNamespaceForType(gridTypeToken, typeNamespaceToken, &typeNamespaceName));
            VERIFY_IS_TRUE(typeNamespaceToken.GetHandle() == controlsTypeNamespaceToken.GetHandle());
            VERIFY_ARE_EQUAL(true, !!typeNamespaceName.Equals(c_strControlsNamespace, xstrCompareCaseSensitive));
        }
    }

    void XamlManagedTypeInfoProviderUnitTests::LookupTypeFlags()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);
        auto spManagedProvider = GetManagedTypeInfoProvider(spSchemaContext);

        // verify we can retrieve type flags
        {
            XamlBitSet<BoolTypeBits> bitsChecked;
            XamlBitSet<BoolTypeBits> bitsReturned;
            XamlBitSet<BoolTypeBits> bitsToRequest;
            bitsToRequest.SetAllBits();

            XamlTypeToken gridTypeToken(tpkManaged, KnownTypeIndex::Button);

            VERIFY_SUCCEEDED(spManagedProvider->LookupTypeFlags(gridTypeToken, bitsToRequest, bitsChecked, bitsReturned));
            VERIFY_IS_TRUE(bitsChecked.IsBitSet(btbConstructible) && bitsReturned.IsBitSet(btbConstructible));
            VERIFY_IS_TRUE(bitsChecked.IsBitSet(btbIsPublic) && bitsReturned.IsBitSet(btbIsPublic));
        }
    }


    void XamlManagedTypeInfoProviderUnitTests::LookupPropertyFlags()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);
        auto spManagedProvider = GetManagedTypeInfoProvider(spSchemaContext);

        // verify we can retrieve type flags
        {
            XamlBitSet<BoolPropertyBits> bitsChecked;
            XamlBitSet<BoolPropertyBits> bitsReturned;
            XamlBitSet<BoolPropertyBits> bitsToRequest;
            bitsToRequest.SetAllBits();

            XamlPropertyToken buttonClickModePropertyToken(tpkManaged, KnownPropertyIndex::ButtonBase_ClickMode);

            VERIFY_SUCCEEDED(spManagedProvider->LookupPropertyFlags(buttonClickModePropertyToken, bitsToRequest, bitsChecked, bitsReturned));
            VERIFY_IS_TRUE(bitsChecked.IsBitSet(bpbIsPublic) && bitsReturned.IsBitSet(bpbIsPublic));
        }
    }

    void XamlManagedTypeInfoProviderUnitTests::ResolveTypeName()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);
        auto spManagedProvider = GetManagedTypeInfoProvider(spSchemaContext);

        // verify we can resolve type name against default namespaces.
        {
            XamlTypeNamespaceToken defaultNamespaceToken(tpkNative, KnownNamespaceIndex::UnknownNamespace);
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strButtonType, L"Button");
            XamlTypeToken typeToken;
            XamlTypeToken buttonTypeToken(tpkManaged, KnownTypeIndex::Button);

            VERIFY_SUCCEEDED(spManagedProvider->ResolveTypeName(defaultNamespaceToken, c_strButtonType, typeToken));
            VERIFY_IS_TRUE(typeToken == buttonTypeToken);
        }

        // verify we can resolve type name against specified namespaces.
        {
            XamlTypeNamespaceToken controlsNamespaceToken(tpkNative, KnownNamespaceIndex::Microsoft_UI_Xaml_Controls);
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strButtonType, L"Button");
            XamlTypeToken typeToken;
            XamlTypeToken buttonTypeToken(tpkManaged, KnownTypeIndex::Button);

            VERIFY_SUCCEEDED(spManagedProvider->ResolveTypeName(controlsNamespaceToken, c_strButtonType, typeToken));
            VERIFY_IS_TRUE(typeToken == buttonTypeToken);
        }
    }

    void XamlManagedTypeInfoProviderUnitTests::ResolvePropertyName()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);
        auto spManagedProvider = GetManagedTypeInfoProvider(spSchemaContext);

        {
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strButtonClickModeProperty, L"ClickMode");
            XamlTypeToken buttonTypeToken(tpkManaged, KnownTypeIndex::Button);
            XamlPropertyToken propertyToken;
            XamlPropertyToken buttonClickModePropertyToken(tpkNative, KnownPropertyIndex::ButtonBase_ClickMode);
            XamlTypeToken propertyTypeToken;
            XamlTypeToken buttonClickModePropertyTypeToken(tpkNative, KnownTypeIndex::ClickMode);

            VERIFY_SUCCEEDED(spManagedProvider->ResolvePropertyName(buttonTypeToken, c_strButtonClickModeProperty, propertyToken, propertyTypeToken));
            VERIFY_IS_TRUE(propertyToken == buttonClickModePropertyToken);
            VERIFY_IS_TRUE(propertyTypeToken == buttonClickModePropertyTypeToken);
        }
    }

    void XamlManagedTypeInfoProviderUnitTests::ResolveDependencyPropertyName()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);
        auto spManagedProvider = GetManagedTypeInfoProvider(spSchemaContext);
    }

    void XamlManagedTypeInfoProviderUnitTests::ProviderMetadataQueryTests()
    {
        std::shared_ptr<ParserErrorReporter> spNullErrorReporter;
        auto spSchemaContext = GetSchemaContext(spNullErrorReporter);
        auto spManagedProvider = GetManagedTypeInfoProvider(spSchemaContext);

        // verify type name can be retrieved given a type token
        {
            xstring_ptr strTypeName;
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strButtonTypeName, L"Button");
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strButtonTypeFullName, L"Microsoft.UI.Xaml.Controls.Button");
            XamlTypeToken buttonType(tpkManaged, KnownTypeIndex::Button);

            VERIFY_SUCCEEDED(spManagedProvider->GetTypeName(buttonType, &strTypeName));
            VERIFY_ARE_EQUAL(true, !!strTypeName.Equals(c_strButtonTypeName, xstrCompareCaseSensitive));

            VERIFY_SUCCEEDED(spManagedProvider->GetTypeFullName(buttonType, &strTypeName));
            VERIFY_ARE_EQUAL(true, !!strTypeName.Equals(c_strButtonTypeFullName, xstrCompareCaseSensitive));
        }

        // verify property name can be retrieved given a property token
        {
            xstring_ptr strPropertyName;
            DECLARE_CONST_STRING_IN_TEST_CODE(c_strButtonClickModePropertyName, L"ClickMode");
            XamlPropertyToken buttonClickModePropertyToken(tpkNative, KnownPropertyIndex::ButtonBase_ClickMode);

            VERIFY_SUCCEEDED(spManagedProvider->GetPropertyName(buttonClickModePropertyToken, &strPropertyName));
            VERIFY_ARE_EQUAL(true, !!strPropertyName.Equals(c_strButtonClickModePropertyName, xstrCompareCaseSensitive));
        }

        // verify base type can be retrieved given a type token
        {
            XamlTypeToken buttonTypeToken(tpkManaged, KnownTypeIndex::Button);
            XamlTypeToken buttonBaseTypeToken(tpkManaged, KnownTypeIndex::ButtonBase);
            XamlTypeToken baseTypeToken;

            VERIFY_SUCCEEDED(spManagedProvider->GetBaseType(buttonTypeToken, baseTypeToken));
            VERIFY_IS_TRUE(baseTypeToken == buttonBaseTypeToken);
        }

        // verify declaring type can be retrieved given a property token
        {
            XamlPropertyToken buttonClickModePropertyToken(tpkNative, KnownPropertyIndex::ButtonBase_ClickMode);
            XamlTypeToken buttonBaseTypeToken(tpkManaged, KnownTypeIndex::ButtonBase);
            XamlTypeToken declaringPropertyTypeToken;

            VERIFY_SUCCEEDED(spManagedProvider->GetDeclaringType(buttonClickModePropertyToken, declaringPropertyTypeToken));
            VERIFY_IS_TRUE(declaringPropertyTypeToken == buttonBaseTypeToken);
        }

        // verify assignable from
        {
            XamlTypeToken solidColorBrushType(tpkNative, KnownTypeIndex::SolidColorBrush);
            XamlTypeToken brushType(tpkNative, KnownTypeIndex::Brush);
            bool isAssignable;

            VERIFY_SUCCEEDED(spManagedProvider->IsAssignableFrom(solidColorBrushType, brushType, isAssignable));
            VERIFY_IS_TRUE(isAssignable == TRUE);
        }

        // verify content property
        {
            XamlTypeToken buttonType(tpkManaged, KnownTypeIndex::Button);
            XamlPropertyToken buttonContentProperty(tpkNative, KnownPropertyIndex::ContentControl_Content);
            XamlTypeToken buttonContentPropertyType(tpkNative, KnownTypeIndex::Object);
            XamlPropertyToken contentPropertyToken;
            XamlTypeToken contentPropertyTypeToken;

            VERIFY_SUCCEEDED(spManagedProvider->GetContentProperty(buttonType, contentPropertyToken, contentPropertyTypeToken));
            VERIFY_IS_TRUE(contentPropertyToken == buttonContentProperty);
            VERIFY_IS_TRUE(contentPropertyTypeToken == buttonContentPropertyType);
        }

        // verify runtime name property
        {
            XamlTypeToken buttonType(tpkManaged, KnownTypeIndex::Button);
            XamlTypeToken namePropertyType;
            XamlPropertyToken nameProperty;
            XamlPropertyToken dependencyObjectNameProperty(tpkNative, KnownPropertyIndex::DependencyObject_Name);
            XamlTypeToken stringType(tpkNative, KnownTypeIndex::String);

            VERIFY_SUCCEEDED(spManagedProvider->GetRuntimeNameProperty(buttonType, nameProperty, namePropertyType));
            VERIFY_IS_TRUE(nameProperty == dependencyObjectNameProperty);
            VERIFY_IS_TRUE(namePropertyType == stringType);
        }

        // verify text syntax type converter
        {
            XamlTypeToken objectType(tpkNative, KnownTypeIndex::Object);
            XamlTypeToken stringType(tpkNative, KnownTypeIndex::String);
            XamlTypeToken textSyntaxConverterType;

            VERIFY_SUCCEEDED(spManagedProvider->GetTextSyntaxForType(objectType, textSyntaxConverterType));
            VERIFY_IS_TRUE(textSyntaxConverterType == stringType);
        }

        // no-op stuff
        {
            XamlTypeToken buttonType(tpkManaged, KnownTypeIndex::Button);
            XamlPropertyToken buttonContentProperty(tpkNative, KnownPropertyIndex::ContentControl_Content);
            XamlTypeToken contentWrapperToken;
            XamlPropertyToken xamlProperty;
            XamlTypeToken xamlType;

            VERIFY_SUCCEEDED(spManagedProvider->GetContentWrapper(buttonType, contentWrapperToken));
            VERIFY_SUCCEEDED(spManagedProvider->GetDictionaryKeyProperty(buttonType, xamlProperty, xamlType));
            VERIFY_SUCCEEDED(spManagedProvider->GetXmlLangProperty(buttonType, xamlProperty, xamlType));
            VERIFY_SUCCEEDED(spManagedProvider->GetCollectionItemType(buttonType, xamlType));
            VERIFY_SUCCEEDED(spManagedProvider->GetTextSyntaxForProperty(buttonContentProperty, xamlType));
        }
    }

    std::shared_ptr<XamlSchemaContext> XamlManagedTypeInfoProviderUnitTests::GetSchemaContext(const std::shared_ptr<ParserErrorReporter>& customErrorReporter)
    {
        ParserUtilities parserUtils;
        return parserUtils.GetSchemaContext(customErrorReporter);
    }

    std::shared_ptr<XamlTypeInfoProvider> XamlManagedTypeInfoProviderUnitTests::GetManagedTypeInfoProvider(const std::shared_ptr<XamlSchemaContext>& schemaContext)
    {
        std::shared_ptr<XamlTypeInfoProvider> spManagedProvider;
        VERIFY_SUCCEEDED(schemaContext->GetTypeInfoProvider(tpkManaged, spManagedProvider));
        return spManagedProvider;
    }

} } } } }
