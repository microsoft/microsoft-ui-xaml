// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Globalization;
using System.Text;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    /// <summary>
    /// Writes a string as a C# or Visual Basic string literal, exactly like generating a <c>CodePrimitiveExpression</c> with
    /// <c>CSharpCodeProvider</c>/<c>VBCodeProvider</c> of System.CodeDom 8.0 (with the default <c>CodeGeneratorOptions</c>), which
    /// is what the compiler used to do.
    /// </summary>
    /// <remarks>
    /// The compiler doesn't use the CodeDom providers: the static constructor of <c>CodeDomProvider</c>
    /// registers <c>CompilerInfo</c>s, which load the provider types by name (<c>Type.GetType</c>, not supported by trimming).
    /// The code below is a port of <c>QuoteSnippetString</c> of <c>Microsoft.CSharp.CSharpCodeGenerator</c> and
    /// <c>Microsoft.VisualBasic.VBCodeGenerator</c> in dotnet/runtime v8.0.0 (src/libraries/System.CodeDom, MIT license).
    /// </remarks>
    internal static class CodeDomStringLiteral
    {
        // CodeGeneratorOptions.IndentString, once (the expression is generated at indentation level 0, and continuation lines are indented by one more level)
        private const string ContinuationIndentation = "    ";
        private const int MaxLineLength = 80;

        public static string ToCSharp(string value)
        {
            // If the string is short, use C style quoting (e.g "\r\n")
            // Also do it if it is too long to fit in one line
            // If the string contains '\0', verbatim style won't work.
            if (value.Length < 256 || value.Length > 1500 || (value.IndexOf('\0') != -1))
            {
                return ToCSharpCStyle(value);
            }

            // Otherwise, use 'verbatim' style quoting (e.g. @"foo")
            return ToCSharpVerbatimStyle(value);
        }

        private static string ToCSharpCStyle(string value)
        {
            var b = new StringBuilder(value.Length + 5);

            b.Append('\"');

            int i = 0;
            while (i < value.Length)
            {
                switch (value[i])
                {
                    case '\r':
                        b.Append("\\r");
                        break;
                    case '\t':
                        b.Append("\\t");
                        break;
                    case '\"':
                        b.Append("\\\"");
                        break;
                    case '\'':
                        b.Append("\\\'");
                        break;
                    case '\\':
                        b.Append("\\\\");
                        break;
                    case '\0':
                        b.Append("\\0");
                        break;
                    case '\n':
                        b.Append("\\n");
                        break;
                    case '\u2028':
                    case '\u2029':
                    case '\u0085':
                        b.Append("\\u");
                        b.Append(((int)value[i]).ToString("X4", CultureInfo.InvariantCulture));
                        break;

                    default:
                        b.Append(value[i]);
                        break;
                }

                if (i > 0 && i % MaxLineLength == 0)
                {
                    // If current character is a high surrogate and the following
                    // character is a low surrogate, don't break them.
                    // Otherwise when we write the string to a file, we might lose
                    // the characters.
                    if (char.IsHighSurrogate(value[i]) && (i < value.Length - 1) && char.IsLowSurrogate(value[i + 1]))
                    {
                        b.Append(value[++i]);
                    }

                    b.Append("\" +");
                    b.Append(Environment.NewLine);
                    b.Append(ContinuationIndentation);
                    b.Append('\"');
                }
                ++i;
            }

            b.Append('\"');

            return b.ToString();
        }

        private static string ToCSharpVerbatimStyle(string value)
        {
            var b = new StringBuilder(value.Length + 5);

            b.Append("@\"");

            for (int i = 0; i < value.Length; i++)
            {
                if (value[i] == '\"')
                    b.Append("\"\"");
                else
                    b.Append(value[i]);
            }

            b.Append('\"');

            return b.ToString();
        }

        public static string ToVisualBasic(string value)
        {
            StringBuilder b = new StringBuilder(value.Length + 5);

            bool fInDoubleQuotes = true;

            b.Append('\"');

            int i = 0;
            while (i < value.Length)
            {
                char ch = value[i];
                switch (ch)
                {
                    case '\"':
                    // These are the inward sloping quotes used by default in some cultures like CHS.
                    // VBC.EXE does a mapping ANSI that results in it treating these as syntactically equivalent to a
                    // regular double quote.
                    case '\u201C':
                    case '\u201D':
                    case '\uFF02':
                        EnsureInDoubleQuotes(ref fInDoubleQuotes, b);
                        b.Append(ch);
                        b.Append(ch);
                        break;
                    case '\r':
                        EnsureNotInDoubleQuotes(ref fInDoubleQuotes, b);
                        if (i < value.Length - 1 && value[i + 1] == '\n')
                        {
                            b.Append("&Global.Microsoft.VisualBasic.ChrW(13)&Global.Microsoft.VisualBasic.ChrW(10)");
                            i++;
                        }
                        else
                        {
                            b.Append("&Global.Microsoft.VisualBasic.ChrW(13)");
                        }
                        break;
                    case '\t':
                        EnsureNotInDoubleQuotes(ref fInDoubleQuotes, b);
                        b.Append("&Global.Microsoft.VisualBasic.ChrW(9)");
                        break;
                    case '\0':
                        EnsureNotInDoubleQuotes(ref fInDoubleQuotes, b);
                        b.Append("&Global.Microsoft.VisualBasic.ChrW(0)");
                        break;
                    case '\n':
                        EnsureNotInDoubleQuotes(ref fInDoubleQuotes, b);
                        b.Append("&Global.Microsoft.VisualBasic.ChrW(10)");
                        break;
                    case '\u2028':
                    case '\u2029':
                        EnsureNotInDoubleQuotes(ref fInDoubleQuotes, b);
                        b.Append("&Global.Microsoft.VisualBasic.ChrW(");
                        b.Append(((int)ch).ToString(CultureInfo.InvariantCulture));
                        b.Append(')');
                        break;
                    default:
                        EnsureInDoubleQuotes(ref fInDoubleQuotes, b);
                        b.Append(value[i]);
                        break;
                }

                if (i > 0 && i % MaxLineLength == 0)
                {
                    // If current character is a high surrogate and the following
                    // character is a low surrogate, don't break them.
                    // Otherwise when we write the string to a file, we might lose
                    // the characters.
                    if (char.IsHighSurrogate(value[i])
                        && (i < value.Length - 1)
                        && char.IsLowSurrogate(value[i + 1]))
                    {
                        b.Append(value[++i]);
                    }

                    if (fInDoubleQuotes)
                        b.Append('\"');
                    fInDoubleQuotes = true;

                    b.Append("& _ ");
                    b.Append(Environment.NewLine);
                    b.Append(ContinuationIndentation);
                    b.Append('\"');
                }
                ++i;
            }

            if (fInDoubleQuotes)
                b.Append('\"');

            return b.ToString();
        }

        private static void EnsureInDoubleQuotes(ref bool fInDoubleQuotes, StringBuilder b)
        {
            if (fInDoubleQuotes)
            {
                return;
            }

            b.Append("&\"");
            fInDoubleQuotes = true;
        }

        private static void EnsureNotInDoubleQuotes(ref bool fInDoubleQuotes, StringBuilder b)
        {
            if (!fInDoubleQuotes)
            {
                return;
            }

            b.Append('\"');
            fInDoubleQuotes = false;
        }
    }
}
