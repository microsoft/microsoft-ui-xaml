// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Antlr4.Runtime;

namespace Microsoft.UI.Xaml.Markup.Compiler.Parsing
{
    internal class ParseErrorListener : BaseErrorListener
    {
        public ParseErrorListener()
        { }

        public override void SyntaxError(IRecognizer recognizer, IToken offendingSymbol, int line,
            int charPositionInLine, string msg, RecognitionException e)
        {
            throw new ParseException(ErrorMessages.SyntaxError, offendingSymbol.Text);
        }
    }
}
