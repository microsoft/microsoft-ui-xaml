// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace Microsoft.UI.Xaml.Markup.Compiler.MSBuildInterop
{
    public class FileItem
    {
        public FileItem()
        {
        }

        public string ItemSpec { get; set; }
        public string DependentUpon { get; set; }
        public string FullPath { get; set; }

        public string MSBuild_Link
        {
            // We don't support Link.
            get { return null; }
        }

        public string MSBuild_TargetPath
        {
            // Only used for SDK Xaml Files. Not implemented for now.
            get { return null; }
        }

        public string MSBuild_XamlResourceMapName
        {
            // Used only for supplying a per-xaml-page override. The plan is to not support it.
            get { return null; }
        }

        public string MSBuild_XamlComponentResourceLocation
        {
            // Used only for supplying a per-xaml-page override. The plan is to not support it.
            get { return null; }
        }
    }
}
