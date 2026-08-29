// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <filesystem>

// AppX-based apps expect resources to be located in a file called resources.pri
// at the root of the AppX. Our MUX tests don't actually use an AppX - they just treat
// the Test directory in the test payload as though it were the root of an AppX,
// which is used as the root for both the managed and the native apps.
// As such, since both the managed and the native apps expect a single file called
// resources.pri, we can't put just one file at that location, so we instead put
// two resources.pri files in subfolders and then configure the correct one
// right before we launch the test app.

// Note: Because the appx server will open the ResourcesPri it is possible that it will not
//       get it closed in time for the the next test to configure it.  This, in the past, 
//       has caused intermittent test failure as an access violation has been triggered as
//       we tried to copy the new file to the target location.  To get around this we
//       are doing two things.  First, we are only going do any work, if the current file
//       is not already the correct file.  And second, we will use a symbolic link so when
//       the Resources.pri file is opened, the link will be followed and the actual file
//       will be open.  This leave the actual link available to be changed.
STDAPI ConfigureResourcesPri(bool configureManaged)
{
    // The expected structure of the test directory:
    //
    //     [Root]                      - Directory containing main executable (e.g. te.exe)
    //       Test                      - Directory containing Test executables
    //         managed                 - Directory containing Managed resources.pri file
    //         native                  - Directory containing Native resources.pri file

    WCHAR modulePathStr[MAX_PATH] = { 0 };
    GetModuleFileName(NULL, modulePathStr, MAX_PATH);

    auto moduleDir = std::filesystem::path(modulePathStr).remove_filename();
    auto resourcesPriSourcePath = moduleDir / "Test" / (configureManaged ? "managed" : "native") / "resources.pri";
    auto resourcesPriDestinationPath = moduleDir / "Test" / "resources.pri";

    bool createLink = true;
    if (std::filesystem::exists(resourcesPriDestinationPath))
    {
        if (std::filesystem::equivalent(resourcesPriSourcePath, resourcesPriDestinationPath))
        {
            LOG_OUTPUT(L"Using existing symbolic link from %s to %s...", resourcesPriSourcePath.c_str(), resourcesPriDestinationPath.c_str());
            createLink = false;
        }
        else
        {
            std::filesystem::remove(resourcesPriDestinationPath);
        }
    }
    
    if (createLink)
    {
        std::error_code errorCode;
        std::filesystem::create_symlink(resourcesPriSourcePath, resourcesPriDestinationPath, errorCode);

        if (errorCode) {
            LOG_ERROR(L"Failed to createsymbolic link from  %s to %s: %d - %s", resourcesPriSourcePath.c_str(), resourcesPriDestinationPath.c_str(), errorCode.value(), errorCode.message().c_str());
            return E_FAIL;
        }
        LOG_OUTPUT(L"Created symbolic link from  %s to %s...", resourcesPriSourcePath.c_str(), resourcesPriDestinationPath.c_str());
    }
 
    return S_OK;
}
