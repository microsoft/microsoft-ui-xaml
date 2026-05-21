// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XbfVersioningUnitTests.h"
#include "XbfVersioning.h"

using namespace Parser::Versioning;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    #pragma region Test Class Initialization & Cleanup
    bool XbfVersioningUnitTests::ClassSetup()
    {
        return true;
    }

    bool XbfVersioningUnitTests::ClassCleanup()
    {
        return true;
    }
    #pragma endregion

    void XbfVersioningUnitTests::VerifyTargetOSVersionOperators()
    {
        // operator==
        {
            VERIFY_IS_TRUE(TargetOSVersion(10, 11, 12, 13) == TargetOSVersion(10, 11, 12, 13));
            VERIFY_IS_FALSE(TargetOSVersion(10, 11, 12, 13) == TargetOSVersion(10, 11, 12, 1));
            VERIFY_IS_FALSE(TargetOSVersion(10, 11, 12, 13) == TargetOSVersion(10, 11, 1, 13));
            VERIFY_IS_FALSE(TargetOSVersion(10, 11, 12, 13) == TargetOSVersion(10, 1, 12, 13));
            VERIFY_IS_FALSE(TargetOSVersion(10, 11, 12, 13) == TargetOSVersion(1, 11, 12, 13));
        }

        // operator!=
        {
            VERIFY_IS_FALSE(TargetOSVersion(10, 11, 12, 13) != TargetOSVersion(10, 11, 12, 13));
            VERIFY_IS_TRUE(TargetOSVersion(10, 11, 12, 13) != TargetOSVersion(10, 11, 12, 1));
            VERIFY_IS_TRUE(TargetOSVersion(10, 11, 12, 13) != TargetOSVersion(10, 11, 1, 13));
            VERIFY_IS_TRUE(TargetOSVersion(10, 11, 12, 13) != TargetOSVersion(10, 1, 12, 13));
            VERIFY_IS_TRUE(TargetOSVersion(10, 11, 12, 13) != TargetOSVersion(1, 11, 12, 13));
        }

        // operator<
        {
            VERIFY_IS_FALSE(TargetOSVersion(10, 11, 12, 13) < TargetOSVersion(10, 11, 12, 13));
            VERIFY_IS_TRUE(TargetOSVersion(10, 11, 12, 13) < TargetOSVersion(10, 11, 12, 14));
            VERIFY_IS_TRUE(TargetOSVersion(10, 11, 12, 13) < TargetOSVersion(10, 11, 13, 13));
            VERIFY_IS_TRUE(TargetOSVersion(10, 11, 12, 13) < TargetOSVersion(10, 12, 12, 13));
            VERIFY_IS_TRUE(TargetOSVersion(10, 11, 12, 13) < TargetOSVersion(11, 11, 12, 13));
            VERIFY_IS_TRUE(TargetOSVersion(10, 0, 15063, 0) < TargetOSVersion(99, 0, 0, 0));

            VERIFY_IS_FALSE(TargetOSVersion(10, 11, 12, 14) < TargetOSVersion(10, 11, 12, 13));
            VERIFY_IS_FALSE(TargetOSVersion(10, 11, 13, 13) < TargetOSVersion(10, 11, 12, 13));
            VERIFY_IS_FALSE(TargetOSVersion(10, 12, 12, 13) < TargetOSVersion(10, 11, 12, 13));
            VERIFY_IS_FALSE(TargetOSVersion(11, 11, 12, 13) < TargetOSVersion(10, 11, 12, 13));
            VERIFY_IS_FALSE(TargetOSVersion(99, 0, 0, 0) < TargetOSVersion(10, 0, 15063, 0));
        }

        // operator>
        {
            VERIFY_IS_FALSE(TargetOSVersion(10, 11, 12, 13) > TargetOSVersion(10, 11, 12, 13));
            VERIFY_IS_TRUE(TargetOSVersion(10, 11, 12, 14) > TargetOSVersion(10, 11, 12, 13));
            VERIFY_IS_TRUE(TargetOSVersion(10, 11, 13, 13) > TargetOSVersion(10, 11, 12, 13));
            VERIFY_IS_TRUE(TargetOSVersion(10, 12, 12, 13) > TargetOSVersion(10, 11, 12, 13));
            VERIFY_IS_TRUE(TargetOSVersion(11, 11, 12, 13) > TargetOSVersion(10, 11, 12, 13));
            VERIFY_IS_TRUE(TargetOSVersion(99, 0, 0, 0) > TargetOSVersion(10, 0, 15063, 0));

            VERIFY_IS_FALSE(TargetOSVersion(10, 11, 12, 13) > TargetOSVersion(10, 11, 12, 14));
            VERIFY_IS_FALSE(TargetOSVersion(10, 11, 12, 13) > TargetOSVersion(10, 11, 13, 13));
            VERIFY_IS_FALSE(TargetOSVersion(10, 11, 12, 13) > TargetOSVersion(10, 12, 12, 13));
            VERIFY_IS_FALSE(TargetOSVersion(10, 11, 12, 13) > TargetOSVersion(11, 11, 12, 13));
            VERIFY_IS_FALSE(TargetOSVersion(10, 0, 15063, 0) > TargetOSVersion(99, 0, 0, 0));
        }

        // operator<=
        {
            VERIFY_IS_TRUE(TargetOSVersion(10, 11, 12, 13) <= TargetOSVersion(10, 11, 12, 13));
            VERIFY_IS_TRUE(TargetOSVersion(10, 11, 12, 13) <= TargetOSVersion(10, 11, 12, 14));
            VERIFY_IS_TRUE(TargetOSVersion(10, 11, 12, 13) <= TargetOSVersion(10, 11, 13, 13));
            VERIFY_IS_TRUE(TargetOSVersion(10, 11, 12, 13) <= TargetOSVersion(10, 12, 12, 13));
            VERIFY_IS_TRUE(TargetOSVersion(10, 11, 12, 13) <= TargetOSVersion(11, 11, 12, 13));
            VERIFY_IS_TRUE(TargetOSVersion(10, 0, 15063, 0) <= TargetOSVersion(99, 0, 0, 0));

            VERIFY_IS_FALSE(TargetOSVersion(10, 11, 12, 14) <= TargetOSVersion(10, 11, 12, 13));
            VERIFY_IS_FALSE(TargetOSVersion(10, 11, 13, 13) <= TargetOSVersion(10, 11, 12, 13));
            VERIFY_IS_FALSE(TargetOSVersion(10, 12, 12, 13) <= TargetOSVersion(10, 11, 12, 13));
            VERIFY_IS_FALSE(TargetOSVersion(11, 11, 12, 13) <= TargetOSVersion(10, 11, 12, 13));
            VERIFY_IS_FALSE(TargetOSVersion(99, 0, 0, 0) <= TargetOSVersion(10, 0, 15063, 0));
        }

        // operator>=
        {
            VERIFY_IS_TRUE(TargetOSVersion(10, 11, 12, 13) >= TargetOSVersion(10, 11, 12, 13));
            VERIFY_IS_TRUE(TargetOSVersion(10, 11, 12, 14) >= TargetOSVersion(10, 11, 12, 13));
            VERIFY_IS_TRUE(TargetOSVersion(10, 11, 13, 13) >= TargetOSVersion(10, 11, 12, 13));
            VERIFY_IS_TRUE(TargetOSVersion(10, 12, 12, 13) >= TargetOSVersion(10, 11, 12, 13));
            VERIFY_IS_TRUE(TargetOSVersion(11, 11, 12, 13) >= TargetOSVersion(10, 11, 12, 13));
            VERIFY_IS_TRUE(TargetOSVersion(99, 0, 0, 0) >= TargetOSVersion(10, 0, 15063, 0));

            VERIFY_IS_FALSE(TargetOSVersion(10, 11, 12, 13) >= TargetOSVersion(10, 11, 12, 14));
            VERIFY_IS_FALSE(TargetOSVersion(10, 11, 12, 13) >= TargetOSVersion(10, 11, 13, 13));
            VERIFY_IS_FALSE(TargetOSVersion(10, 11, 12, 13) >= TargetOSVersion(10, 12, 12, 13));
            VERIFY_IS_FALSE(TargetOSVersion(10, 11, 12, 13) >= TargetOSVersion(11, 11, 12, 13));
            VERIFY_IS_FALSE(TargetOSVersion(10, 0, 15063, 0) >= TargetOSVersion(99, 0, 0, 0));
        }
    }

    void XbfVersioningUnitTests::VerifyGetXBFVersion()
    {
        auto rs5plus1 = TargetOSVersion(OSVersions::WIN10_RS5.major, OSVersions::WIN10_RS5.minor, OSVersions::WIN10_RS5.build + 1, OSVersions::WIN10_RS5.revision);
        // Note that adding a new XBF version will necessitate updating this test
        VERIFY_ARE_EQUAL(GetXBFVersion(OSVersions::WINBLUE), ::Parser::XbfV2_1);
        VERIFY_ARE_EQUAL(GetXBFVersion(OSVersions::WIN10_TH1), ::Parser::XbfV2_1);
        VERIFY_ARE_EQUAL(GetXBFVersion(OSVersions::WIN10_TH2), ::Parser::XbfV2_1);
        VERIFY_ARE_EQUAL(GetXBFVersion(OSVersions::WIN10_RS1), ::Parser::XbfV2_1);
        VERIFY_ARE_EQUAL(GetXBFVersion(OSVersions::WIN10_RS2), ::Parser::XbfV2_1);
        VERIFY_ARE_EQUAL(GetXBFVersion(OSVersions::WIN10_RS3), ::Parser::XbfV2_1);
        VERIFY_ARE_EQUAL(GetXBFVersion(OSVersions::WIN10_RS4), ::Parser::XbfV2_1);
        VERIFY_ARE_EQUAL(GetXBFVersion(OSVersions::WIN10_RS5), ::Parser::XbfV2_1);
        VERIFY_ARE_EQUAL(GetXBFVersion(rs5plus1), ::Parser::XbfV2_1);
        VERIFY_ARE_EQUAL(GetXBFVersion(TargetOSVersion(99, 0, 0, 0)), ::Parser::XbfV2_1);
    }

    void XbfVersioningUnitTests::VerifyGetDeferredElementSerializationVersion()
    {
        auto rs5plus1 = TargetOSVersion(OSVersions::WIN10_RS5.major, OSVersions::WIN10_RS5.minor, OSVersions::WIN10_RS5.build + 1, OSVersions::WIN10_RS5.revision);
        // Note that adding a new XBF version will necessitate updating this test
        VERIFY_ARE_EQUAL(GetDeferredElementSerializationVersion(OSVersions::WINBLUE), CustomWriterRuntimeDataTypeIndex::DeferredElement_v3);
        VERIFY_ARE_EQUAL(GetDeferredElementSerializationVersion(OSVersions::WIN10_TH1), CustomWriterRuntimeDataTypeIndex::DeferredElement_v3);
        VERIFY_ARE_EQUAL(GetDeferredElementSerializationVersion(OSVersions::WIN10_TH2), CustomWriterRuntimeDataTypeIndex::DeferredElement_v3);
        VERIFY_ARE_EQUAL(GetDeferredElementSerializationVersion(OSVersions::WIN10_RS1), CustomWriterRuntimeDataTypeIndex::DeferredElement_v3);
        VERIFY_ARE_EQUAL(GetDeferredElementSerializationVersion(OSVersions::WIN10_RS2), CustomWriterRuntimeDataTypeIndex::DeferredElement_v3);
        VERIFY_ARE_EQUAL(GetDeferredElementSerializationVersion(OSVersions::WIN10_RS3), CustomWriterRuntimeDataTypeIndex::DeferredElement_v3);
        VERIFY_ARE_EQUAL(GetDeferredElementSerializationVersion(OSVersions::WIN10_RS4), CustomWriterRuntimeDataTypeIndex::DeferredElement_v3);
        VERIFY_ARE_EQUAL(GetDeferredElementSerializationVersion(OSVersions::WIN10_RS5), CustomWriterRuntimeDataTypeIndex::DeferredElement_v3);
        VERIFY_ARE_EQUAL(GetDeferredElementSerializationVersion(rs5plus1), CustomWriterRuntimeDataTypeIndex::DeferredElement_v3);
        VERIFY_ARE_EQUAL(GetDeferredElementSerializationVersion(TargetOSVersion(99, 0, 0, 0)), CustomWriterRuntimeDataTypeIndex::DeferredElement_v3);
    }

    void XbfVersioningUnitTests::VerifyGetResourceDictionarySerializationVersion()
    {
        auto rs5plus1 = TargetOSVersion(OSVersions::WIN10_RS5.major, OSVersions::WIN10_RS5.minor, OSVersions::WIN10_RS5.build + 1, OSVersions::WIN10_RS5.revision);
        // Note that adding a new XBF version will necessitate updating this test
        VERIFY_ARE_EQUAL(GetResourceDictionarySerializationVersion(OSVersions::WINBLUE), CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v3);
        VERIFY_ARE_EQUAL(GetResourceDictionarySerializationVersion(OSVersions::WIN10_TH1), CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v3);
        VERIFY_ARE_EQUAL(GetResourceDictionarySerializationVersion(OSVersions::WIN10_TH2), CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v3);
        VERIFY_ARE_EQUAL(GetResourceDictionarySerializationVersion(OSVersions::WIN10_RS1), CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v3);
        VERIFY_ARE_EQUAL(GetResourceDictionarySerializationVersion(OSVersions::WIN10_RS2), CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v3);
        VERIFY_ARE_EQUAL(GetResourceDictionarySerializationVersion(OSVersions::WIN10_RS3), CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v3);
        VERIFY_ARE_EQUAL(GetResourceDictionarySerializationVersion(OSVersions::WIN10_RS4), CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v3);
        VERIFY_ARE_EQUAL(GetResourceDictionarySerializationVersion(OSVersions::WIN10_RS5), CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v3);
        VERIFY_ARE_EQUAL(GetResourceDictionarySerializationVersion(rs5plus1), CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v3);
        VERIFY_ARE_EQUAL(GetResourceDictionarySerializationVersion(TargetOSVersion(99, 0, 0, 0)), CustomWriterRuntimeDataTypeIndex::ResourceDictionary_v3);
    }

    void XbfVersioningUnitTests::VerifyGetStyleSerializationVersion()
    {
        auto rs5plus1 = TargetOSVersion(OSVersions::WIN10_RS5.major, OSVersions::WIN10_RS5.minor, OSVersions::WIN10_RS5.build + 1, OSVersions::WIN10_RS5.revision);
        // Note that adding a new XBF version will necessitate updating this test
        VERIFY_ARE_EQUAL(GetStyleSerializationVersion(OSVersions::WINBLUE), CustomWriterRuntimeDataTypeIndex::Style_v3);
        VERIFY_ARE_EQUAL(GetStyleSerializationVersion(OSVersions::WIN10_TH1), CustomWriterRuntimeDataTypeIndex::Style_v3);
        VERIFY_ARE_EQUAL(GetStyleSerializationVersion(OSVersions::WIN10_TH2), CustomWriterRuntimeDataTypeIndex::Style_v3);
        VERIFY_ARE_EQUAL(GetStyleSerializationVersion(OSVersions::WIN10_RS1), CustomWriterRuntimeDataTypeIndex::Style_v3);
        VERIFY_ARE_EQUAL(GetStyleSerializationVersion(OSVersions::WIN10_RS2), CustomWriterRuntimeDataTypeIndex::Style_v3);
        VERIFY_ARE_EQUAL(GetStyleSerializationVersion(OSVersions::WIN10_RS3), CustomWriterRuntimeDataTypeIndex::Style_v3);
        VERIFY_ARE_EQUAL(GetStyleSerializationVersion(OSVersions::WIN10_RS4), CustomWriterRuntimeDataTypeIndex::Style_v3);
        VERIFY_ARE_EQUAL(GetStyleSerializationVersion(OSVersions::WIN10_RS5), CustomWriterRuntimeDataTypeIndex::Style_v3);
        VERIFY_ARE_EQUAL(GetStyleSerializationVersion(rs5plus1), CustomWriterRuntimeDataTypeIndex::Style_v3);
        VERIFY_ARE_EQUAL(GetStyleSerializationVersion(TargetOSVersion(99, 0, 0, 0)), CustomWriterRuntimeDataTypeIndex::Style_v3);
    }

    void XbfVersioningUnitTests::VerifyGetVisualStateGroupCollectionSerializationVersion()
    {
        auto rs5plus1 = TargetOSVersion(OSVersions::WIN10_RS5.major, OSVersions::WIN10_RS5.minor, OSVersions::WIN10_RS5.build + 1, OSVersions::WIN10_RS5.revision);
        // Note that adding a new XBF version will necessitate updating this test
        VERIFY_ARE_EQUAL(GetVisualStateGroupCollectionSerializationVersion(OSVersions::WINBLUE), CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v5);
        VERIFY_ARE_EQUAL(GetVisualStateGroupCollectionSerializationVersion(OSVersions::WIN10_TH1), CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v5);
        VERIFY_ARE_EQUAL(GetVisualStateGroupCollectionSerializationVersion(OSVersions::WIN10_TH2), CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v5);
        VERIFY_ARE_EQUAL(GetVisualStateGroupCollectionSerializationVersion(OSVersions::WIN10_RS1), CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v5);
        VERIFY_ARE_EQUAL(GetVisualStateGroupCollectionSerializationVersion(OSVersions::WIN10_RS2), CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v5);
        VERIFY_ARE_EQUAL(GetVisualStateGroupCollectionSerializationVersion(OSVersions::WIN10_RS3), CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v5);
        VERIFY_ARE_EQUAL(GetVisualStateGroupCollectionSerializationVersion(OSVersions::WIN10_RS4), CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v5);
        VERIFY_ARE_EQUAL(GetVisualStateGroupCollectionSerializationVersion(OSVersions::WIN10_RS5), CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v5);
        VERIFY_ARE_EQUAL(GetVisualStateGroupCollectionSerializationVersion(rs5plus1), CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v5);
        VERIFY_ARE_EQUAL(GetVisualStateGroupCollectionSerializationVersion(TargetOSVersion(99, 0, 0, 0)), CustomWriterRuntimeDataTypeIndex::VisualStateGroupCollection_v5);
    }

} } } } }