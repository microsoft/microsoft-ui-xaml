// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.Serialization.Json;
using System.Text;
using Microsoft.Build.Framework;
using Microsoft.UI.Xaml.Markup.Compiler.MSBuildInterop;

namespace Microsoft.UI.Xaml.Markup.Compiler.Tasks
{
    /// <summary>
    /// Reads and writes the JSON files of the contract with the XAML compiler (see CompilerContract), with the serializer of the
    /// framework: the tasks have no dependency, so that they never conflict with the assemblies of the MSBuild that loads them.
    /// </summary>
    internal static class CompilerJson
    {
        public static void Write<T>(Stream stream, T value)
        {
            new DataContractJsonSerializer(typeof(T)).WriteObject(stream, value);
        }

        public static void Write<T>(string path, T value)
        {
            using (FileStream stream = new FileStream(path, FileMode.Create, FileAccess.Write, FileShare.None))
            {
                Write(stream, value);
            }
        }

        public static T Read<T>(Stream stream)
        {
            return (T)new DataContractJsonSerializer(typeof(T)).ReadObject(stream);
        }

        public static T Read<T>(string path)
        {
            using (FileStream stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read))
            {
                return Read<T>(stream);
            }
        }
    }

    /// <summary>Converts MSBuild items to and from the items of the contract with the XAML compiler.</summary>
    internal static class TaskItems
    {
        public static MSBuildItem ToMSBuildItem(ITaskItem item)
        {
            string isSystemReference = item.GetMetadata("IsSystemReference");
            bool isSystem = !String.IsNullOrEmpty(isSystemReference) && isSystemReference.Equals("True", StringComparison.OrdinalIgnoreCase);
            bool isFrameworkMetadata = item.GetMetadata("ResolvedFrom") == "{TargetFrameworkDirectory}";

            return new MSBuildItem
            {
                DependentUpon = item.GetMetadata("DependentUpon"),
                FullPath = item.GetMetadata("FullPath"),
                ItemSpec = item.ItemSpec,
                IsSystemReference = isSystem || isFrameworkMetadata,
                IsNuGetReference = !String.IsNullOrEmpty(item.GetMetadata("NuGetPackageId")),
                IsStaticLibraryReference = item.GetMetadata("ProjectType") == "StaticLibrary",
                MSBuild_Link = item.GetMetadata("Link"),
                MSBuild_TargetPath = item.GetMetadata("TargetPath"),
                MSBuild_XamlResourceMapName = item.GetMetadata("XamlResourceMapName"),
                MSBuild_XamlComponentResourceLocation = item.GetMetadata("XamlComponentResourceLocation"),
            };
        }

        /// <summary>Converts the items that aren't excluded from the build, or returns null for a null array.</summary>
        public static List<MSBuildItem> ToMSBuildItems(ITaskItem[] items)
        {
            if (items == null)
            {
                return null;
            }

            List<MSBuildItem> result = new List<MSBuildItem>(items.Length);
            foreach (ITaskItem item in items)
            {
                if (!IsExcludedFromBuild(item))
                {
                    result.Add(ToMSBuildItem(item));
                }
            }
            return result;
        }

        public static bool IsExcludedFromBuild(ITaskItem item)
        {
            return string.Equals(item.GetMetadata("ExcludedFromBuild"), "true", StringComparison.Ordinal);
        }

        /// <summary>
        /// Escapes the MSBuild special characters of a file name (what ProjectCollection.Escape does), so that names like
        /// "%23.g.cs" survive as item specs.
        /// </summary>
        public static string Escape(string unescaped)
        {
            if (string.IsNullOrEmpty(unescaped) || unescaped.IndexOfAny(s_specialCharacters) < 0)
            {
                return unescaped;
            }

            StringBuilder builder = new StringBuilder(unescaped.Length + 8);
            foreach (char c in unescaped)
            {
                if (Array.IndexOf(s_specialCharacters, c) >= 0)
                {
                    builder.Append('%').Append(((int)c).ToString("x2"));
                }
                else
                {
                    builder.Append(c);
                }
            }
            return builder.ToString();
        }

        private static readonly char[] s_specialCharacters = { '%', '*', '?', '@', '$', '(', ')', ';', '\'' };
    }
}
