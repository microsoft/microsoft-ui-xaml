// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

// ShutdownOrderValidation is a mechanism to validate that Xaml's shutdown order is in the right sequence.
namespace ShutdownOrderValidation
{
    // Add a message to the log, the order of the messages will be validated by a test.
    void Log(const wchar_t* message);    

    // If enabled, the app will ensure "SaveLogToTempFile" was called before the process exits.
    void ValidateStateOnProcessExit(bool enable);

    // Save the log to a temp file.  A test will validate the order later.
    void SaveLogToTempFile();
}
