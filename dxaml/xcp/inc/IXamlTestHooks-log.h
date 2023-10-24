// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <memory>
#include <functional>
#include <ErrorHandlerSettings.h>

// Private interface for unit test framework
// References macros defined in <combaseapi.h>, which conflicts with
// code in the core, particularly around GetClassName.
DECLARE_INTERFACE_IID_(IXamlLoggerTestHooks, IUnknown, "0d5fc75b-6ab1-47d3-8190-9a5e2ba52987")
{
    // Using a std::function callback here- however because Jupiter has a custom allocator
    // we must pass the secret recipe on how to delete any internal allocations std::function
    // made between the DLLs as well. We do that using std::shared_ptr's virtual dtor.
    IFACEMETHOD_(void, SetErrorHandlerCallback)(_In_ std::shared_ptr<std::function<void(const ErrorHandling::XamlFailureInfo&)>> callback) = 0;
    IFACEMETHOD_(void, SetLoggerCallback)(_In_ std::shared_ptr<ErrorHandling::LoggerCallback> callback) = 0;
};
