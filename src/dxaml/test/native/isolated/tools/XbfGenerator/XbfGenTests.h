// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools { namespace XbfGenerator {

    class XbfGenerationTests : public WEX::TestClass<XbfGenerationTests>
    {
    public:
        BEGIN_TEST_CLASS(XbfGenerationTests)
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e22a917c-ad18-4a09-bff9-d3ca3e5ee0b8")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        //
        // Platform:Desktop
        //
        BEGIN_TEST_METHOD(ValidateWrite)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully write XBF to output streams.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateErrorMessage)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we get the correct error message using the write API.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(ValidateChecksumPlacement)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the checksum is correctly placed in the header.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateXbfGenerationOfLargeDouble)
            TEST_METHOD_PROPERTY(L"Description", L"Validates GenXbf can encode a Dictionary with a large x:Double value")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateXbfGenerationOfPhonePivot)
            TEST_METHOD_PROPERTY(L"Description", L"Validates GenXbf can encode the Phone Pivot Type which lives in the default namespace but exists in an external metadata provider.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateXbfGenerationMetadataReset)
            TEST_METHOD_PROPERTY(L"Description", L"Validates GenXbf resets the Metadata cache between runs.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateXbfGenerationOfLooseXamlEvents)
            TEST_METHOD_PROPERTY(L"Description", L"Validates GenXbf can generate XBF for a Xaml file with Events without crashing.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(ValidateEmptyXbfHash)
            TEST_METHOD_PROPERTY(L"Description", L"Validates GenXbf creates empty hash when not using WriteMethod.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDisableLineInfoFlag)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the disable line info flag works")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateXbfGenerationOfResourceDictionary)
            TEST_METHOD_PROPERTY(L"Description", L"Validates XBF generation for a XAML file with Resource Dictionaries")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateXbfGenerationOfVSM)
            TEST_METHOD_PROPERTY(L"Description", L"Validates XBF generation for a XAML file with VSM")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateXbfGenerationOfStyles)
            TEST_METHOD_PROPERTY(L"Description", L"Validates XBF generation for a XAML file with Styles")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateXbfDump)
            TEST_METHOD_PROPERTY(L"Description", L"Validates XBF dump tool works")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Enable ValidateXbfDump Test
                                                     // The test is still valid, but it's not properly hooking up the metadata provider to understand WinUI2.x types. It should be using the SampleXbfMetadataProvider
                                                     // Then a new Dump export need to be created, which takes one of these providers, and instead of creating a DumpMetadataProvider, create the appropriate MetadataProviderAdapter
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SanityTest)
            // Generally, this test fails after a change to controls that requires a change to genxbf. Failures should be assigned to them first
            // for investigation.
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the genxbf.dll that builds Windows creates the same output as the one in the sdk")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

    };

} } } } } }
