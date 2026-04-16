// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <RpcServer.h>
#include <ResourcesPriHelper.h>
#include <WexTestClass.h>

#include <vector>
#include <string>
#include <sstream>
#include <filesystem>

#include <Versioning.h>

#define LOG_OUTPUT(fmt, ...) WEX::Logging::Log::Comment(WEX::Common::String().Format(fmt, __VA_ARGS__))

// These methods will be called from the out-of-proc loaded instance of
// this DLL. They will register our RPC server.

BEGIN_MODULE()
    MODULE_PROPERTY(L"RunFixtureAs:Module", L"ElevatedUserOrSystem")
    MODULE_PROPERTY(L"EnsureLoggedOnUser:UserCount", L"1")
    MODULE_PROPERTY(L"DeploymentItem", L"..\\EtwProcessor.dll")
    MODULE_PROPERTY(L"ThreadingModel[@HostingMode='WPF']", L"STA")
    MODULE_PROPERTY(L"UAP:Host[@HostingMode='Win32Explicit']", L"PackagedCwa")
    MODULE_PROPERTY(L"UAP:Host[@HostingMode='WPF']", L"PackagedCwa")
    MODULE_PROPERTY(L"UAP:AppXManifest[@HostingMode='Win32Explicit']", APPXMANIFEST_WINDOWS_VERSION_CURRENT_CENTENNIAL)
    MODULE_PROPERTY(L"UAP:AppXManifest[@HostingMode='WPF']", APPXMANIFEST_WINDOWS_VERSION_CURRENT_CENTENNIAL)
    MODULE_PROPERTY(L"UAP:AppXManifest[default]", APPXMANIFEST_WINDOWS_VERSION_CURRENT)
    MODULE_PROPERTY(L"UAP:WaitForXamlWindowActivation", L"false")
    MODULE_PROPERTY(L"Hosting:Mode", L"WPF")
    MODULE_PROPERTY(L"TestPass:MinOSVer", WINDOWS_OS_VERSION_RS4)
    MODULE_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
END_MODULE()

MODULE_SETUP(ModuleSetup);
MODULE_CLEANUP(ModuleCleanup);

// Nice helper functions to split string
// https://stackoverflow.com/questions/236129/most-elegant-way-to-split-a-string
namespace StringHelpers
{
    template<typename Out>
    void Split(const std::string &s, char delim, Out result) {
        std::stringstream ss;
        ss.str(s);
        std::string item;
        while (std::getline(ss, item, delim)) {
            *(result++) = item;
        }
    }

    std::vector<std::string> Split(const std::string &s, char delim) {
        std::vector<std::string> elems;
        Split(s, delim, std::back_inserter(elems));
        return elems;
    }
}

bool ModuleSetup()
{
    // Configure the resources.pri file for native tests
    VERIFY_SUCCEEDED(ConfigureResourcesPri(false /* configureManaged */));
    VERIFY_SUCCEEDED(RpcServerStart());
    return true;
}

bool ModuleCleanup()
{
    VERIFY_SUCCEEDED(RpcServerStop());
    return true;
}
