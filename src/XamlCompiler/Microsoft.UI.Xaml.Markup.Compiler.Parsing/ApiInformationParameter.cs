// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    public class ApiInformationParameter
    {
        public string ParameterValue { get; }
        public Type ParameterType { get; set; }

        public ApiInformationParameter(string value)
        {
            ParameterValue = value;
        }

        public ApiInformationParameter(Type type)
        {
            ParameterType = type;
        }

        public string UniqueName => ParameterValue;
    }
}
