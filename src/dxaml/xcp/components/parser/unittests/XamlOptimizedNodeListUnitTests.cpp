// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XamlOptimizedNodeListUnitTests.h"
#include "ParserUtilities.h"
#include "ParserUnitTestIncludes.h"
#include "MockParserCoreServices.h"
#include <XamlOptimizedNodeList.h>
#include <XamlLogging.h>

using namespace DirectUI;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    void XamlOptimizedNodeListUnitTests::Create()
    {
        ParserUtilities parserUtils;
        std::shared_ptr<ParserErrorReporter> nullErrorReporter;
        std::shared_ptr<XamlOptimizedNodeList> nodeList;
        std::shared_ptr<XamlWriter> nodeWriter;
        std::shared_ptr<XamlType> gridXamlType;

        // construct schema context with no custom error reporter
        auto schemaContext = parserUtils.GetSchemaContext(nullErrorReporter);
        nodeList = std::make_shared<XamlOptimizedNodeList>(schemaContext);

        // get xaml type for grid
        XamlTypeToken gridToken = XamlTypeToken::FromType(DirectUI::MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::Grid));
        VERIFY_SUCCEEDED(schemaContext->GetXamlType(gridToken, gridXamlType));

        // construct a simple optimized node list with single WriteObject node
        VERIFY_SUCCEEDED(nodeList->get_Writer(nodeWriter));
        VERIFY_SUCCEEDED(nodeWriter->WriteObject(gridXamlType, false));
        VERIFY_SUCCEEDED(nodeWriter->Close());

        // ensure node list was created
        VERIFY_IS_TRUE(nodeList->get_Count() > 0);
    }

    // this is a unit test to ensure that old style resource dictionary deferral (xbf1)
    // can use a consistent logic for counting the elements in an optimized node list
    // Previously, the element count was basically the bytes written to the list but
    // this can vary by releases because the index assigned to a type could get encoded
    // as multiple bytes. CResourceDictionary::ShouldDeferNodeStream assumed the count
    // was independent of the size of these indexes and would not vary across releases.
    // Unfortunately, this is not true and the indexes assigned are different
    // primarily because the schema context index allocates new indexes after the known indexes
    //
    // this is tricky issue to quirk so the best possible solution is to err on the side of
    // preventing deferring assuming a lower bound case where property indexes/type indexes
    // are treated as 1 element count. Since this is not a perfect solution, old apps may
    // end up not deferring some content if this heuristic is incorrect. However, that
    // is a safer behavior than ending up deferring and causing creation dependent issues.
    //
    // this unit test makes sure that get_CountForDictionaryCount added to XamlOptimizedNodeList
    // returns a index value independent count of the node list.
    void XamlOptimizedNodeListUnitTests::GetNormalizedCountForOldDictionary()
    {
        ParserUtilities parserUtils;
        std::shared_ptr<ParserErrorReporter> nullErrorReporter;
        std::shared_ptr<XamlOptimizedNodeList> nodeList;
        std::shared_ptr<XamlWriter> nodeWriter;
        std::shared_ptr<XamlType> newXamlType;
        std::shared_ptr<XamlProperty> newXamlProperty;

        // construct schema context with no custom error reporter
        auto schemaContext = parserUtils.GetSchemaContext(nullErrorReporter);
        auto oldTestHookValue = schemaContext->get_TestHook_UseEmptyXamlTypeNamespaceInGetXamlProperty();
        schemaContext->set_TestHook_UseEmptyXamlTypeNamespaceInGetXamlProperty(true);
        auto guard = wil::scope_exit([&schemaContext, &oldTestHookValue]()
        {
            schemaContext->set_TestHook_UseEmptyXamlTypeNamespaceInGetXamlProperty(oldTestHookValue);
        });

        nodeList = std::make_shared<XamlOptimizedNodeList>(schemaContext);

        // generate custom type and property in schema context cache which causes a runtime index to be populated.
        VERIFY_SUCCEEDED(schemaContext->GetXamlType(XamlTypeToken(tpkNative, static_cast<KnownTypeIndex>(-1)), newXamlType));
        VERIFY_SUCCEEDED(schemaContext->GetXamlProperty(XamlPropertyToken(tpkNative, static_cast<KnownPropertyIndex>(-1)), XamlTypeToken(tpkNative, static_cast<KnownTypeIndex>(-1)), newXamlProperty));

        // ensure these have runtime indexes that are > 128 so that when they get encoded in
        // the optimized node list they use more than 1 element count when counting using the old
        // style get_Count()
        VERIFY_IS_TRUE(newXamlType->get_RuntimeIndex() > 128);
        VERIFY_IS_TRUE(newXamlProperty->get_RuntimeIndex() > 128);

        // consturct a node list with the new xaml type and property
        VERIFY_SUCCEEDED(nodeList->get_Writer(nodeWriter));
        VERIFY_SUCCEEDED(nodeWriter->WriteObject(newXamlType, false));
        VERIFY_SUCCEEDED(nodeWriter->WriteMember(newXamlProperty));
        VERIFY_SUCCEEDED(nodeWriter->WriteEndMember());
        VERIFY_SUCCEEDED(nodeWriter->WriteEndObject());
        VERIFY_SUCCEEDED(nodeWriter->Close());

        // ensure that the count for dictionary deferral stays constant at 12
        VERIFY_IS_TRUE(nodeList->get_CountForOldDictionaryDeferral() == 12);

        // ensure that get_Count() and get_CountForOldDictionaryDeferral are different
        VERIFY_IS_TRUE(nodeList->get_Count() >= nodeList->get_CountForOldDictionaryDeferral());
    }

} } } } }
