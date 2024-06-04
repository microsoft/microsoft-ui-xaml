// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Build.Framework;
using System;
using System.Collections;
using System.Collections.Generic;

namespace Microsoft.UI.Xaml.Markup.Compiler.MSBuildInterop
{
    public sealed class MSBuildItem : IFileItem, IAssemblyItem
    {
        public MSBuildItem()
        {

        }
        public MSBuildItem(ITaskItem item)
        {
            PopulateItem(item);
        }

        private void PopulateItem(ITaskItem item)
        {
            DependentUpon = item?.GetMetadata("DependentUpon");
            FullPath = item?.GetMetadata("FullPath");
            ItemSpec = item?.ItemSpec;

            string isSystemReference = item?.GetMetadata("IsSystemReference");
            bool isSystem = !String.IsNullOrEmpty(isSystemReference) &&
                isSystemReference.Equals("True", StringComparison.OrdinalIgnoreCase);

            string resolvedFrom = item?.GetMetadata("ResolvedFrom");
            bool isFrameworkMetadata = resolvedFrom != null && (resolvedFrom == "{TargetFrameworkDirectory}");

            IsSystemReference = isSystem || isFrameworkMetadata;

            IsNuGetReference = !String.IsNullOrEmpty(item?.GetMetadata("NuGetPackageId"));

            IsStaticLibraryReference = item?.GetMetadata("ProjectType") == "StaticLibrary";
            MSBuild_Link = item?.GetMetadata("Link");
            MSBuild_TargetPath = item?.GetMetadata("TargetPath");
            MSBuild_XamlResourceMapName = item?.GetMetadata("XamlResourceMapName");
            MSBuild_XamlComponentResourceLocation = item?.GetMetadata("XamlComponentResourceLocation");
        }

        public string DependentUpon { get; set; }

        public string FullPath { get; set; }

        public string ItemSpec { get; set; }

        public bool IsSystemReference { get; set; }

        public bool IsNuGetReference { get; set; }

        public bool IsStaticLibraryReference { get; set; }

        public string MSBuild_Link { get; set; }

        public string MSBuild_TargetPath { get; set; }

        public string MSBuild_XamlResourceMapName { get; set; }

        public string MSBuild_XamlComponentResourceLocation { get; set; }
    }
}
