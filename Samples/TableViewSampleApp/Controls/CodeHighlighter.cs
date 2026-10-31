// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace TableViewSampleApp.Controls;

public enum CodeTokenKind
{
    Plain,
    Keyword,
    String,
    Comment,
    Type,
    Element,
    Attribute,
    AttributeValue,
    Punctuation,
}

/// <summary>
/// A small lexical highlighter for the sample's XAML and C# snippets. It colours tokens, it does not
/// parse: a token it does not recognise is plain text.
/// </summary>
public static partial class CodeHighlighter
{
    public const string Xaml = "XAML";
    public const string CSharp = "C#";

    private static readonly HashSet<string> s_keywords = new(StringComparer.Ordinal)
    {
        "abstract", "and", "as", "async", "await", "base", "bool", "break", "byte", "case", "catch", "char",
        "checked", "class", "const", "continue", "decimal", "default", "delegate", "do", "double", "dynamic",
        "else", "enum", "event", "explicit", "extern", "false", "finally", "fixed", "float", "for", "foreach",
        "get", "global", "goto", "if", "implicit", "in", "init", "int", "interface", "internal", "is", "lock",
        "long", "nameof", "namespace", "new", "not", "null", "object", "operator", "or", "out", "override",
        "params", "partial", "private", "protected", "public", "readonly", "record", "ref", "return", "sbyte",
        "sealed", "set", "short", "sizeof", "stackalloc", "static", "string", "struct", "switch", "this",
        "throw", "true", "try", "typeof", "uint", "ulong", "unchecked", "unsafe", "ushort", "using", "value",
        "var", "virtual", "void", "volatile", "when", "where", "while", "with", "yield",
    };

    private static readonly HashSet<string> s_typePrefixKeywords = new(StringComparer.Ordinal)
    {
        "new", "is", "as", "typeof", "class", "struct", "record", "interface", "enum", "not",
    };

    [GeneratedRegex(@"(?<comment>//[^\n]*|/\*[\s\S]*?\*/)|(?<string>@""(?:""""|[^""])*""|\$?@?""(?:\\.|[^""\\\n])*""|'(?:\\.|[^'\\\n])+')|(?<ident>@?[A-Za-z_][A-Za-z0-9_]*)")]
    private static partial Regex CSharpTokenRegex();

    [GeneratedRegex(@"^\??\s+@?[A-Za-z_]|^<|^\[\]")]
    private static partial Regex TypeFollowerRegex();

    public static IEnumerable<(string Text, CodeTokenKind Kind)> Tokenize(string code, string? language) => language switch
    {
        Xaml => TokenizeXaml(code),
        CSharp => TokenizeCSharp(code),
        _ => new[] { (code, CodeTokenKind.Plain) },
    };

    private static IEnumerable<(string, CodeTokenKind)> TokenizeCSharp(string code)
    {
        var position = 0;
        string? previousWord = null;
        foreach (Match match in CSharpTokenRegex().Matches(code))
        {
            if (match.Index > position)
            {
                yield return (code.Substring(position, match.Index - position), CodeTokenKind.Plain);
            }

            var text = match.Value;
            var kind = CodeTokenKind.Plain;
            if (match.Groups["comment"].Success)
            {
                kind = CodeTokenKind.Comment;
            }
            else if (match.Groups["string"].Success)
            {
                kind = CodeTokenKind.String;
            }
            else if (s_keywords.Contains(text))
            {
                kind = CodeTokenKind.Keyword;
            }
            else if (IsTypeName(code, match, previousWord))
            {
                kind = CodeTokenKind.Type;
            }

            if (match.Groups["ident"].Success)
            {
                previousWord = text;
            }

            yield return (text, kind);
            position = match.Index + match.Length;
        }

        if (position < code.Length)
        {
            yield return (code.Substring(position), CodeTokenKind.Plain);
        }
    }

    // A PascalCase name in a type position: after new/is/as/typeof, a base-type colon or a generic
    // '<', or followed by a declared name, a type argument list or [].
    private static bool IsTypeName(string code, Match match, string? previousWord)
    {
        if (!char.IsUpper(match.Value[0]))
        {
            return false;
        }

        var before = match.Index - 1;
        while (before >= 0 && code[before] is ' ' or '\t')
        {
            before--;
        }

        var previousChar = before >= 0 ? code[before] : '\n';
        if (previousChar is '<' or ':' || (previousChar == '(' && previousWord == "typeof"))
        {
            return true;
        }

        if (previousWord is not null && s_typePrefixKeywords.Contains(previousWord)
            && before >= previousWord.Length - 1
            && string.CompareOrdinal(code, before - previousWord.Length + 1, previousWord, 0, previousWord.Length) == 0)
        {
            return true;
        }

        var end = match.Index + match.Length;
        return TypeFollowerRegex().IsMatch(code.AsSpan(end, Math.Min(64, code.Length - end)));
    }

    private static IEnumerable<(string, CodeTokenKind)> TokenizeXaml(string code)
    {
        var tokens = new List<(string, CodeTokenKind)>();
        var i = 0;
        var n = code.Length;

        void Emit(int start, int end, CodeTokenKind kind)
        {
            if (end > start)
            {
                tokens.Add((code.Substring(start, end - start), kind));
            }
        }

        int ReadWhile(int start, Func<char, bool> predicate)
        {
            var j = start;
            while (j < n && predicate(code[j]))
            {
                j++;
            }

            return j;
        }

        while (i < n)
        {
            if (string.CompareOrdinal(code, i, "<!--", 0, 4) == 0)
            {
                var close = code.IndexOf("-->", i + 4, StringComparison.Ordinal);
                var end = close < 0 ? n : close + 3;
                Emit(i, end, CodeTokenKind.Comment);
                i = end;
                continue;
            }

            if (code[i] != '<')
            {
                var next = code.IndexOf('<', i);
                var end = next < 0 ? n : next;
                Emit(i, end, CodeTokenKind.Plain);
                i = end;
                continue;
            }

            var nameStart = i + (i + 1 < n && code[i + 1] is '/' or '?' ? 2 : 1);
            Emit(i, nameStart, CodeTokenKind.Punctuation);
            i = ReadWhile(nameStart, c => !char.IsWhiteSpace(c) && c is not '/' and not '>' and not '?');
            Emit(nameStart, i, CodeTokenKind.Element);

            while (i < n)
            {
                var c = code[i];
                if (char.IsWhiteSpace(c))
                {
                    var end = ReadWhile(i, char.IsWhiteSpace);
                    Emit(i, end, CodeTokenKind.Plain);
                    i = end;
                }
                else if (c == '>' || ((c == '/' || c == '?') && i + 1 < n && code[i + 1] == '>'))
                {
                    var end = c == '>' ? i + 1 : i + 2;
                    Emit(i, end, CodeTokenKind.Punctuation);
                    i = end;
                    break;
                }
                else if (c == '=')
                {
                    Emit(i, i + 1, CodeTokenKind.Punctuation);
                    i++;
                }
                else if (c is '"' or '\'')
                {
                    var close = code.IndexOf(c, i + 1);
                    var end = close < 0 ? n : close + 1;
                    Emit(i, end, CodeTokenKind.AttributeValue);
                    i = end;
                }
                else
                {
                    var end = ReadWhile(i, ch => !char.IsWhiteSpace(ch) && ch is not '=' and not '>' and not '/' and not '"' and not '\'');
                    if (end == i)
                    {
                        end = i + 1;
                    }

                    Emit(i, end, CodeTokenKind.Attribute);
                    i = end;
                }
            }
        }

        return tokens;
    }
}
