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
    Platform::WeakReference printDocumentWeakReference;

    RunOnUIThread([&]()
    {
        PrintDocument^ printDocument = ref new PrintDocument();
        Object^ managedDocumentSource = printDocument->DocumentSource;
        auto documentSourceInspectable = reinterpret_cast<IInspectable*>(managedDocumentSource);

        VERIFY_SUCCEEDED(documentSourceInspectable->QueryInterface(IID_PPV_ARGS(&documentSource)));
        printDocumentWeakReference = Platform::WeakReference(printDocument);

        printDocument = nullptr;
        managedDocumentSource = nullptr;

        VERIFY_IS_NULL(printDocumentWeakReference.Resolve<PrintDocument>());
    });

    HRESULT initializeResult = E_FAIL;
    HRESULT makeDocumentResult = E_FAIL;
    std::thread worker([&]()
    {
        initializeResult = ::RoInitialize(RO_INIT_MULTITHREADED);
        if (SUCCEEDED(initializeResult))
        {
            makeDocumentResult = documentSource->MakeDocument(nullptr, nullptr);
            ::RoUninitialize();
        }
    });
    worker.join();

    VERIFY_SUCCEEDED(initializeResult);
    LOG_OUTPUT(L"MakeDocument returned 0x%08X after owner expiry.", makeDocumentResult);
}

} } } } } }
