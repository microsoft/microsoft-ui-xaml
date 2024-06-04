// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using System;

    internal class CompiledBindingException : Exception
    {
        public int StartCharacterPosition { get; }

        public CompiledBindingException(string exceptionMessage, int startCharacterPosition)
            : base(exceptionMessage)
        {
            this.StartCharacterPosition = startCharacterPosition;
        }
    }

    internal class CompiledBindingParseException : CompiledBindingException
    {
        public string ExpressionBeingParsed { get; }

        public CompiledBindingParseException(string expressionBeingParsed, string exceptionMessage, int startCharacterPosition)
            : base(exceptionMessage, startCharacterPosition)
        {
            ExpressionBeingParsed = expressionBeingParsed;
        }
    }
}
