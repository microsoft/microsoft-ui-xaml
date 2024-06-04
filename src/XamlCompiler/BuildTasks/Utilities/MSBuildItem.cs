// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Build.Framework;
using System;

namespace Microsoft.UI.Xaml.Markup.Compiler.Tasks
{
    internal class MSBuildItem : IFileItem, IAssemblyItem
    {
        private ITaskItem item;
        public MSBuildItem(ITaskItem item)
        {
            this.item = item;
        }

        public string DependentUpon
        {
            get { return item?.GetMetadata("DependentUpon"); }
            set { throw new NotImplementedException(); }
        }

        public string FullPath
        {
            get { return item?.GetMetadata("FullPath"); }
        }

        public string ItemSpec
        {
            get { return item?.ItemSpec; }
        }

        public bool IsSystemReference
        {
            get
            {
                // This is checking 2 properties for MSBuild.
                string isSystemReference = item?.GetMetadata("IsSystemReference");
                bool isSystem = !String.IsNullOrEmpty(isSystemReference) && 
                    isSystemReference.Equals("True", StringComparison.OrdinalIgnoreCase);

                string resolvedFrom = item?.GetMetadata("ResolvedFrom");
                bool isFrameworkMetadata = resolvedFrom != null && (resolvedFrom == "{TargetFrameworkDirectory}");

                return isSystem || isFrameworkMetadata;

            }
            set { throw new NotImplementedException(); }
        }

        public bool IsNuGetReference
        {
            get { return !String.IsNullOrEmpty(item?.GetMetadata("NuGetPackageId")); }
            set { throw new NotImplementedException(); }
        }

        public bool IsStaticLibraryReference
        {
            get {
                var projectType = item?.GetMetadata("ProjectType");
                return projectType == "StaticLibrary";
            }
            set { throw new NotImplementedException(); }
        }

        public string MSBuild_Link
        {
            get { return item?.GetMetadata("Link"); }
        }

        public string MSBuild_TargetPath
        {
            get { return item?.GetMetadata("TargetPath"); }
        }

        public string MSBuild_XamlResourceMapName
        {
            get { return item?.GetMetadata("XamlResourceMapName"); }
        }

        public string MSBuild_XamlComponentResourceLocation
        {
            get { return item?.GetMetadata("XamlComponentResourceLocation"); }
        }
    }
}
