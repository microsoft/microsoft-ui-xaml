// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    static class ClrNamespaceParser
    {
        internal static bool TryParseUri(string uriInput, out string clrNs, out string assemblyName,
            out string error, bool returnErrors)
        {
            clrNs = null;
            assemblyName = null;
            error = null;

            // xmlns:foo="clr-namespace:System.Windows;assembly=myassemblyname"
            // xmlns:bar="clr-namespace:MyAppsNs"
            // xmlns:spam="clr-namespace:MyAppsNs;assembly="  

            int colonIdx = KS.IndexOf(uriInput, ":");
            if (-1 == colonIdx)
            {
                //if (returnErrors)
                //{
                //    error = SR.Get(SRID.MissingTagInNamespace, ":", uriInput);
                //}
                return false;
            }

            string keyword = uriInput.Substring(0, colonIdx);
            if (!KS.Eq(keyword, KnownStrings.UriClrNamespace))
            {
                //if (returnErrors)
                //{
                //    error = SR.Get(SRID.MissingTagInNamespace, KnownStrings.UriClrNamespace, uriInput);
                //}
                return false;
            }

            int clrNsStartIdx = colonIdx + 1;
            int semicolonIdx = KS.IndexOf(uriInput, ";");
            if (-1 == semicolonIdx)
            {
                clrNs = uriInput.Substring(clrNsStartIdx);
                assemblyName = null;
                return true;
            }
            else
            {
                int clrnsLength = semicolonIdx - clrNsStartIdx;
                clrNs = uriInput.Substring(clrNsStartIdx, clrnsLength);
            }

            int assemblyKeywordStartIdx = semicolonIdx + 1;
            int equalIdx = KS.IndexOf(uriInput, "=");
            if (-1 == equalIdx)
            {
                //if (returnErrors)
                //{
                //    error = SR.Get(SRID.MissingTagInNamespace, "=", uriInput);
                //}
                return false;
            }
            keyword = uriInput.Substring(assemblyKeywordStartIdx, equalIdx - assemblyKeywordStartIdx);
            if (!KS.Eq(keyword, KnownStrings.UriAssembly))
            {
                //if (returnErrors)
                //{
                //    error = SR.Get(SRID.AssemblyTagMissing, KnownStrings.UriAssembly, uriInput);
                //}
                return false;
            }
            assemblyName = uriInput.Substring(equalIdx + 1);
            return true;
        }
    }
}
