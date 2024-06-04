// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Linq;
using System.Text.RegularExpressions;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    public static class StringExtensions
    {
        // Matches letters or numbers at the begining of a string, equivalent to a-zA-Z_0-9
        private static readonly Regex MemberFriendlyNameRegex = new Regex(@"[^\w]");

        /// <summary>
        /// Returns a name that could be used in code.
        /// </summary>
        public static string GetMemberFriendlyName(this string instance)
        {
            return MemberFriendlyNameRegex.Replace(instance, "_");
        }

        public static bool IsConditionalNamespace(this string instance)
        {
            return instance.Contains("?");
        }
        
        internal static bool HasAtLeastTwo(this string instance, char character)
        {
            return instance.Contains(character) && instance.IndexOf(character) != instance.LastIndexOf(character);
        }

        public static bool HasUsingPrefix(this string instance)
        {
            return instance.StartsWith(KnownStrings.UsingPrefix);
        }

        public static string StripUsingPrefix(this string instance)
        {
            if (instance.HasUsingPrefix())
            {
                return instance.Substring(KnownStrings.UsingPrefix.Length);
            }
            else
            {
                return instance;
            }
        }
    }
}
