// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Resources;
using System.Diagnostics;
using System.Globalization;
using System.Text.RegularExpressions;

namespace Microsoft.UI.Xaml.Markup.Compiler.Utilities
{
    /// <summary>
    /// This class contains utility methods for dealing with resources.
    /// </summary>
    /// <owner>SumedhK</owner>
    internal static class ResourceUtilities
    {
        // used to find MSBuild message code prefixes
        private static readonly Regex msbuildMessageCodePattern = new Regex(@"^\s*(?<CODE>MSB\d\d\d\d):\s*(?<MESSAGE>.*)$", RegexOptions.Singleline);

        /// <summary>
        /// Extracts the message code (if any) prefixed to the given string. If a message code pattern is not supplied, the
        /// MSBuild message code pattern is used by default. The message code pattern must contain two named capturing groups
        /// called "CODE" and "MESSAGE" that identify the message code and the message respectively.
        /// </summary>
        /// <remarks>This method is thread-safe.</remarks>
        /// <owner>SumedhK</owner>
        /// <param name="messageCodePattern">The Regex used to find the message code (can be null).</param>
        /// <param name="messageWithCode">The string to parse.</param>
        /// <param name="code">[out] The message code, or null if there was no code.</param>
        /// <returns>The string without its message code prefix.</returns>
        internal static string ExtractMessageCode(Regex messageCodePattern, string messageWithCode, out string code)
        {
            code = null;
            string messageOnly = messageWithCode;

            if (messageCodePattern == null)
            {
                messageCodePattern = msbuildMessageCodePattern;
            }

            // NOTE: the Regex class is thread-safe (see MSDN)
            Match messageCode = messageCodePattern.Match(messageWithCode);

            if (messageCode.Success)
            {
                code = messageCode.Groups["CODE"].Value;
                messageOnly = messageCode.Groups["MESSAGE"].Value;
            }

            return messageOnly;
        }

        /// <summary>
        /// Formats the given string using the variable arguments passed in.
        /// 
        /// PERF WARNING: calling a method that takes a variable number of arguments is expensive, because memory is allocated for
        /// the array of arguments -- do not call this method repeatedly in performance-critical scenarios
        /// </summary>
        /// <remarks>This method is thread-safe.</remarks>
        /// <owner>SumedhK</owner>
        /// <param name="unformatted">The string to format.</param>
        /// <param name="args">Optional arguments for formatting the given string.</param>
        /// <returns>The formatted string.</returns>
        internal static string FormatString(string unformatted, params object[] args)
        {
            string formatted = unformatted;

            // NOTE: String.Format() does not allow a null arguments array
            if ((args != null) && (args.Length > 0))
            {
                // Format the string, using the variable arguments passed in.
                // NOTE: all String methods are thread-safe
                formatted = String.Format(CultureInfo.CurrentCulture, unformatted, args);
            }

            return formatted;
        }

    }
}
