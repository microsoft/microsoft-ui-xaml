// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    static class PropertyPathParser
    {
        // Not a complete parser.  I throw out the Brackets because I am not interested in them.
        static public bool Parse(String input, out List<String> qualifiedProperties, out List<String> names)
        {
            qualifiedProperties = null;
            names = null;

            if (!RemoveBrackets(input, out input))
            {
                return false;
            }

            while(input != null)
            {
                string before = null;
                string inside = null;
                string after = null;

                if (!SplitOnParens(input, out before, out inside, out after))
                {
                    return false;
                }
                if (before != null)
                {
                    String[] pathNames = before.Split('.');
                    if (names == null)
                    {
                        names = new List<string>();
                    }
                    foreach(String n in pathNames)
                    {
                        if(!String.IsNullOrWhiteSpace(n))
                        {
                            names.Add(n);
                        }
                    }
                }
                if (inside != null)
                {
                    if (qualifiedProperties == null)
                    {
                        qualifiedProperties = new List<string>();
                    }
                    qualifiedProperties.Add(inside);
                }
                input = after;
            }
            return true;
        }

        // Split a string on the first set of ()s.
        // before - the string before the first ( or all of it if there are no Parens.
        // inside - the string inside the first set of ().   The parens are eliminated.
        // after - the string after the first set of ().  Which may have more parens.
        static bool SplitOnParens(String input, out String before, out String inside, out String after)
        {
            before = null;
            inside = null;
            after = null;

            int openIdx = input.IndexOf('(');
            if (openIdx == -1)
            {
                before = input;
                return true;
            }

            int length = openIdx;
            if (length > 0)
            {
                before = input.Substring(0, length);
            }
            int closeIdx = input.IndexOf(')');
            if (closeIdx == -1)
            {
                // this is an error, but don't crash
                return false;
            }
            
            length = closeIdx - (openIdx + 1);
            if (length > 0)
            {
                inside = input.Substring(openIdx + 1, length);

                // must have one and only one '.'
                int dotIdx = inside.IndexOf('.');
                if (dotIdx == -1 || dotIdx != inside.LastIndexOf('.'))
                {
                    return false;
                }
            }

            length = input.Length - (closeIdx + 1);
            if (length > 0)
            {
                after = input.Substring(closeIdx + 1, length);
            }
            return true;
        }

        static bool RemoveBrackets(String input, out String result)
        {
            string remaining = input;
            result = String.Empty;

            while (true)
            {
                int openIdx = remaining.IndexOf('[');
                int closeIdx = remaining.IndexOf(']');
                if (openIdx == -1)
                {
                    if (closeIdx == -1)     // Bad string, don't crash
                    {
                        result += remaining;
                        return true;
                    }
                    return false;
                }
                else if (closeIdx == -1)
                {
                    // Opening but no closing bracket, a bad string - we should error
                    return false;
                }
                result += remaining.Substring(0, openIdx);
                remaining = remaining.Substring(closeIdx + 1);
            }
        }
    }
}
