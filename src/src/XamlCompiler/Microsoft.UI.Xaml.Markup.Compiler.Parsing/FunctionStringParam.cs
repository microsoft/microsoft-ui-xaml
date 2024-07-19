// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    public class FunctionStringParam : FunctionParam
    {
        public string Value { get; }

        public FunctionStringParam(string value)
        {
            Value = value;
        }
        protected override void ValidateParameter(Parameter paramInfo)
        {
            if (paramInfo.ParameterType.FullName != typeof(string).FullName)
            {
                throw new ArgumentException();
            }
        }

        public override string UniqueName => Value;
    }
}
