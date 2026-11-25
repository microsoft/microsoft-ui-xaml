// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <DocumentSource.h>
#include <thread>
#include "PrintDocumentLifetimeTests.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>

using namespace Platform;
using namespace Microsoft::UI::Xaml::Printing;
using namespace test_infra;
using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Printing {

bool PrintDocumentLifetimeTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool PrintDocumentLifetimeTests::TestCleanup()
{
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void PrintDocumentLifetimeTests::MakeDocumentAfterOwnerReleaseDoesNotCrash()
{
    TestCleanupWrapper cleanup;
    Microsoft::WRL::ComPtr<IPrintDocumentPageSource> documentSource;
    Microsoft::WRL::ComPtr<IPrintPreviewPageCollection> previewPageCollection;
    Platform::WeakReference printDocumentWeakReference;

    struct CallResults
    {
        HRESULT initializeResult = E_FAIL;
        HRESULT makeDocumentResult = E_FAIL;
        HRESULT getPreviewCollectionResult = E_FAIL;
        HRESULT paginateResult = E_FAIL;
        HRESULT makePageResult = E_FAIL;
        bool previewCollectionReturned = false;
    };

    auto callPrintMethods = [&]()
    {
        CallResults results;
        std::thread worker([&]()
        {
            results.initializeResult = ::RoInitialize(RO_INIT_MULTITHREADED);
            if (SUCCEEDED(results.initializeResult))
            {
                results.makeDocumentResult = documentSource->MakeDocument(nullptr, nullptr);

                Microsoft::WRL::ComPtr<IPrintPreviewPageCollection> returnedPreviewCollection;
                results.getPreviewCollectionResult = documentSource->GetPreviewPageCollection(nullptr, &returnedPreviewCollection);
                results.previewCollectionReturned = returnedPreviewCollection != nullptr;
                if (returnedPreviewCollection)
                {
                    previewPageCollection = returnedPreviewCollection;
                }
                if (previewPageCollection)
                {
                    results.paginateResult = previewPageCollection->Paginate(0, nullptr);
                    results.makePageResult = previewPageCollection->MakePage(1, 100.0f, 100.0f);
                }

                ::RoUninitialize();
            }
        });
        worker.join();
        return results;
    };

    CallResults liveOwnerResults;
    RunOnUIThread([&]()
    {
        PrintDocument^ printDocument = ref new PrintDocument();
        Object^ managedDocumentSource = printDocument->DocumentSource;
        auto documentSourceInspectable = reinterpret_cast<IInspectable*>(managedDocumentSource);

        VERIFY_SUCCEEDED(documentSourceInspectable->QueryInterface(IID_PPV_ARGS(&documentSource)));
        printDocumentWeakReference = Platform::WeakReference(printDocument);

        liveOwnerResults = callPrintMethods();
        printDocument = nullptr;
        VERIFY_IS_NULL(printDocumentWeakReference.Resolve<PrintDocument>());
    });

    VERIFY_SUCCEEDED(liveOwnerResults.initializeResult);
    VERIFY_ARE_EQUAL(S_OK, liveOwnerResults.makeDocumentResult);
    VERIFY_ARE_EQUAL(S_OK, liveOwnerResults.getPreviewCollectionResult);
    VERIFY_ARE_EQUAL(S_OK, liveOwnerResults.paginateResult);
    VERIFY_ARE_EQUAL(S_OK, liveOwnerResults.makePageResult);
    VERIFY_IS_TRUE(liveOwnerResults.previewCollectionReturned);
    VERIFY_IS_NOT_NULL(previewPageCollection.Get());
    TestServices::WindowHelper->WaitForIdle();

    const auto expiredOwnerResults = callPrintMethods();
    VERIFY_SUCCEEDED(expiredOwnerResults.initializeResult);
    VERIFY_ARE_EQUAL(E_UNEXPECTED, expiredOwnerResults.makeDocumentResult);
    VERIFY_ARE_EQUAL(E_UNEXPECTED, expiredOwnerResults.getPreviewCollectionResult);
    VERIFY_ARE_EQUAL(E_UNEXPECTED, expiredOwnerResults.paginateResult);
    VERIFY_ARE_EQUAL(E_UNEXPECTED, expiredOwnerResults.makePageResult);
    VERIFY_IS_FALSE(expiredOwnerResults.previewCollectionReturned);
}

} } } } } }
