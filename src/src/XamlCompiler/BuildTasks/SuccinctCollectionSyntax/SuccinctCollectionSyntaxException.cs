// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Text;
using System.Globalization;
using System.Security.Cryptography;

namespace SuccinctCollectionSyntax
{
    public class SuccinctCollectionSyntaxException : Exception
    {
        public int Line { get; }
        public int Col { get; }
        public string OffendingToken { get; }
       
        public SuccinctCollectionSyntaxException(string syntaxError, string offendingSymbolText,int line = 0,  int charPositionInLine = 0)
            : base(FormatMessage(syntaxError, offendingSymbolText, line, charPositionInLine))
        {
            Line = line;
            Col = charPositionInLine;
            OffendingToken = offendingSymbolText;
        }
        


        private static string FormatMessage(string errorMessage, string offendingSymbolText, int line, int charPositionInLine)
        {

            return string.Format(errorMessage, offendingSymbolText, line, charPositionInLine);

        }


    }
}
