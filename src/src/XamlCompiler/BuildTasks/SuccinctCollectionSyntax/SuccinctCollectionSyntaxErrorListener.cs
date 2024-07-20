// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Antlr4.Runtime;
using Microsoft.UI.Xaml.Markup.Compiler;
using System;
using System.Collections.Generic;
using System.Text;

namespace SuccinctCollectionSyntax
{
    public class SuccinctCollectionSyntaxErrorListener : BaseErrorListener
    {
        public SuccinctCollectionSyntaxErrorListener() { }

        public override void SyntaxError(IRecognizer recognizer, IToken offendingSymbol, int line, int charPositionInLine, string msg, RecognitionException e)
        {

            throw new SuccinctCollectionSyntaxException("Offending Symbol Text: {0}, at line Number {1} , character position : {2}" , offendingSymbol.Text, line, charPositionInLine); 
        }
        

    }
}
