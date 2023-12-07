// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Microsoft.UI.Xaml.Markup.Compiler.CodeGen
{
    internal static class StringExtensions
    {
        internal static string AsNamespaceDeclarationBegin(this string instance)
        {
            return "namespace " + instance.Replace(".", " { namespace ");
        }

        internal static string AsNamespaceDeclarationEnd(this string instance)
        {
            return instance.Where(c => c == '.').Aggregate("", (current, c) => current + "}");
        }


        internal static string Quotenate(this string value)
        {
            return string.Format("\"{0}\"", value);
        }

        internal static string ToTitleCase(this string value)
        {
            StringBuilder titleCased = new StringBuilder();
            bool capitalizeNext = true;
            foreach (char c in value)
            {
                titleCased.Append(capitalizeNext ? char.ToUpper(c) : c);
                capitalizeNext = char.IsWhiteSpace(c);
            }
            return titleCased.ToString();
        }

        internal static string ToLocalCppWinRTTypeName(this string fullName)
        {
            var typeSeparatorIndex = fullName.LastIndexOf("::");
            if (typeSeparatorIndex > 0)
            {
                return fullName.Insert(typeSeparatorIndex, "::implementation");
            }
            else
            {
                return fullName;
            }
        }

        internal static string ToCommaSeparatedValues(this IEnumerable<object> list)
        {
            int position = 0;
            string result = "";
            foreach (var item in list)
            {
                result += (position++ == 0) ? $"{item}" : $", {item}";
            }
            return result;
        }
    }
}
