// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    public class FunctionNullValueParam : FunctionParam
    {
        public FunctionNullValueParam()
        {
        }

        protected override void ValidateParameter(Parameter paramInfo)
        {
            if (paramInfo.ParameterType.IsValueType)
            {
                throw new ArgumentException("Argument must be nullable");
            }
        }

        public override string UniqueName => "Null";
    }
}
