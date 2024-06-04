// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.Parsing
{
    internal static class ConditionalNamespaceLexerExtensions
    {
        public static void ConfirmInputFullyConsumed(this ConditionalNamespaceLexer lexer)
        {
            if (!lexer.HitEOF)
            {
                var input = lexer.InputStream.ToString();
                var unconsumed = lexer.Token == null ? input : input.Substring(lexer.Token.StartIndex);
                throw new ParseException(ErrorMessages.SyntaxError, unconsumed);
            }
        }
    }
}
