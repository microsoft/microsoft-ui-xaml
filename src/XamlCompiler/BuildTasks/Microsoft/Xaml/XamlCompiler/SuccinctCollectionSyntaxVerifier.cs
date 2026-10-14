// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Markup.Compiler.XamlDom;
using SuccinctCollectionSyntax;
using System;
using System.Collections.Generic;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    class SuccinctCollectionSyntaxVerifier
    {

        public static bool TryParse(string collectionItem, XamlDomNode locationForErrors, List<XamlCompileError> Errors, XamlMember property)
        {
            try
            {
                Parse(collectionItem);
            }
            catch (SuccinctCollectionSyntaxException e)
            {

                // Only the first value line shares the property's column offset.
                // Later lines start at their own column in the original input.
                int markupLine = locationForErrors.StartLineNumber;
                int exceptionLine = e.Line;
                int row = markupLine + exceptionLine - 1;

                int markupColumn = locationForErrors.EndLinePosition;
                int exceptionColumn = e.Col;
                int column = exceptionLine == 1 ? markupColumn + exceptionColumn : exceptionColumn + 1;

                Errors.Add(new XamlSuccinctSyntaxError(row, column, e.OffendingToken, locationForErrors.SourceFilePath));

                return false;
            }
            catch (Exception)
            {
                Errors.Add(new XamlValidationErrorCannotAssignTextToProperty(locationForErrors, property, collectionItem));
                return false;
            }
            return true;
        }

        private static void ThrowSyntaxError(string input, int position)
        {
            int line = 1;
            int column = 0;
            for (int i = 0; i < position; i++)
            {
                if (input[i] == '\r')
                {
                    if (i + 1 < position && input[i + 1] == '\n')
                    {
                        i++;
                    }
                    line++;
                    column = 0;
                }
                else if (input[i] == '\n')
                {
                    line++;
                    column = 0;
                }
                else
                {
                    column++;
                }
            }

            string token = position < input.Length ? input[position].ToString() : "<EOF>";
            throw new SuccinctCollectionSyntaxException("Offending Symbol Text: {0}, at line Number {1} , character position : {2}", token, line, column);
        }

        public static void Parse(string bindPath)
        {
            if (String.IsNullOrWhiteSpace(bindPath))
            {
                ThrowSyntaxError(bindPath ?? String.Empty, 0);
            }

            int position = 0;
            while (position < bindPath.Length)
            {
                while (position < bindPath.Length && Char.IsWhiteSpace(bindPath[position]))
                {
                    position++;
                }

                if (position == bindPath.Length)
                {
                    ThrowSyntaxError(bindPath, position);
                }

                char quote = bindPath[position] == '\'' || bindPath[position] == '"'
                    ? bindPath[position++]
                    : '\0';
                bool quoteClosed = false;
                bool hasUnquotedCharacter = false;

                while (position < bindPath.Length)
                {
                    char current = bindPath[position];

                    if (current == '\\')
                    {
                        if (++position == bindPath.Length)
                        {
                            ThrowSyntaxError(bindPath, position);
                        }
                        position++;
                        hasUnquotedCharacter = true;
                        continue;
                    }

                    if (quote != '\0')
                    {
                        if (current == quote)
                        {
                            position++;
                            quoteClosed = true;
                            break;
                        }
                        if (current == '\'' || current == '"')
                        {
                            ThrowSyntaxError(bindPath, position);
                        }
                        position++;
                        continue;
                    }

                    if (current == ',')
                    {
                        break;
                    }
                    if (current == '[' || current == ']' || current == '\'' || current == '"')
                    {
                        ThrowSyntaxError(bindPath, position);
                    }

                    hasUnquotedCharacter |= !Char.IsWhiteSpace(current);
                    position++;
                }

                if (quote != '\0')
                {
                    if (!quoteClosed)
                    {
                        ThrowSyntaxError(bindPath, position);
                    }

                    while (position < bindPath.Length && Char.IsWhiteSpace(bindPath[position]))
                    {
                        position++;
                    }
                    if (position < bindPath.Length && bindPath[position] != ',')
                    {
                        ThrowSyntaxError(bindPath, position);
                    }
                }
                else if (!hasUnquotedCharacter)
                {
                    ThrowSyntaxError(bindPath, position);
                }

                if (position == bindPath.Length)
                {
                    return;
                }

                position++;
                if (position == bindPath.Length)
                {
                    ThrowSyntaxError(bindPath, position);
                }
            }
        }
    }
}

