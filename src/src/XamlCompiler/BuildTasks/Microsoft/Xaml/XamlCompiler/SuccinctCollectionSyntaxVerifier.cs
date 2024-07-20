// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Antlr4.Runtime;
using Antlr4.Runtime.Tree;
using Microsoft.UI.Xaml.Markup.Compiler.Parsing;
using Microsoft.UI.Xaml.Markup.Compiler.XamlDom;
using SuccinctCollectionSyntax;
using System;
using System.Collections.Generic;
using System.Text;
using System.Xaml;
using BailErrorStrategy = Microsoft.UI.Xaml.Markup.Compiler.Parsing.BailErrorStrategy;

namespace Microsoft.UI.Xaml.Markup.Compiler

{
    internal static class SuccinctSyntaxLexerExtensions
    {
        public static void ConfirmInputFullyConsumed(this SuccinctCollectionSyntaxLexer lexer)
        {
            if (!lexer.HitEOF)
            {
                var input = lexer.InputStream.ToString();
                var unconsumed = lexer.Token == null ? input : input.Substring(lexer.Token.StartIndex);
                throw new SuccinctCollectionSyntaxException(ErrorMessages.SyntaxError, unconsumed);
            }
        }
    }

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

                // locationforErrors only contains start of XAML property assignment, in order to find the position in markup it must be combined with the Antlr Line info from the exception object
                
                
                int markupLine = locationForErrors.StartLineNumber;
                int exceptionLine = e.Line;
                int row = markupLine + exceptionLine - 1;

                int markupColumn = locationForErrors.EndLinePosition;
                int exceptionColumn = e.Col;
                int column = markupColumn + exceptionColumn;

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
        public static void Parse(string bindPath)
        {
            // Antlr parsing engine setup
            var input = new AntlrInputStream(bindPath);
            var lexer = new SuccinctCollectionSyntaxLexer(input);
            var tokens = new CommonTokenStream(lexer);
            var parser = new SuccinctCollectionSyntaxParser(tokens);

            // redirect parsing errors as VS exceptions instead of console output
            // and disable the automatic error recovery handler
            parser.RemoveErrorListeners();
            parser.AddErrorListener(new SuccinctCollectionSyntaxErrorListener());
            parser.ErrorHandler = new BailErrorStrategy();

            // set our custom callback override to build up the SuccinctSyntax during parse
            var listener = new SuccinctCollectionSyntaxBaseListener();

            // parse the path input
            var walker = new ParseTreeWalker();

            walker.Walk(listener, parser.items());

            // confirm nothing left unparsed at end of path string
            lexer.ConfirmInputFullyConsumed();
        }
    }
}
