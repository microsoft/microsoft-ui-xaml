// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
namespace Microsoft.UI.Xaml.Tests.Common
{
    // this file defines the versions for managed test.
    // please make a corresponding change to dxaml\test\native\inc\Versioning.h when you update this file.
    public static class AppxManifests
    {
        public const string WINDOWS_VERSION_CURRENT                 = "AppXManifest.managed.current.xml";   // Windows 19H1
        public const string WINDOWS_VERSION_CURRENT_DM              = "AppxManifest.DesignMode.xml";        // Windows 19H1 with DesignMode support
        public const string WINDOWS_VERSION_CURRENT_CENTENNIAL      = "AppXManifest.Centennial.xml";        // Windows Centennial Latest
    }

    public static class WindowsOSVersion
    {
        public const string RS4 = "17134";
        public const string RS5 = "17763";
        public const string _19H1 = "18362";
        public const string _20H2 = "19042";
        public const string _22H2 = "22621";
    }
}
