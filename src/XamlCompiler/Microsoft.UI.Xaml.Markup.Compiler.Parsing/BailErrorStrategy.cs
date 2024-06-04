// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Antlr4.Runtime;

namespace Microsoft.UI.Xaml.Markup.Compiler.Parsing
{
    /// <summary>
    /// BailErrorStrategy
    /// Disables Antlr's automatic error recovery mechanisms following the example in
    /// The Definitive Antlr 4 Reference - section 9.5 on page 172.
    /// </summary>
    internal class BailErrorStrategy : DefaultErrorStrategy
    {
        public override void Recover(Parser recognizer, RecognitionException e) { throw e; }

        public override IToken RecoverInline(Parser recognizer)
        {
            throw new InputMismatchException(recognizer);
        }

        public override void Sync(Parser recognizer) { }
    }
}
