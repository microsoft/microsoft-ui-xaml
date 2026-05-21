// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <wrl/wrappers/corewrappers.h>
#include <wil/result_macros.h>

namespace FileNameGenerator {

    inline wrl::Wrappers::HString GetFileName(const mdc::FileNameInfo& fileNameInfo)
    {
        wrl::Wrappers::HString result;
        try
        {
            std::wstring fileName(WindowsGetStringRawBuffer(fileNameInfo.pathAndTestName, nullptr));
            std::wstring variation(WindowsGetStringRawBuffer(fileNameInfo.variation, nullptr));
            if (!variation.empty())
            {
                fileName = fileName + L"." + variation;
            }

            std::wstring extension(WindowsGetStringRawBuffer(fileNameInfo.extension, nullptr));

            // The extension contains the separating dot
            fileName = fileName + extension;

            THROW_IF_FAILED(result.Set(fileName.c_str()));
        }
        CATCH_FAIL_FAST()

        return result;
    }
}
