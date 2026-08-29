// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Globalization;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    public class FunctionNumberParam : FunctionParam
    {
        public string Value { get; }

        public FunctionNumberParam(string value)
        {
            Value = value;
        }

        protected override void ValidateParameter(Parameter paramInfo)
        {
            bool success = false;

            if (paramInfo.ParameterType.FullName == typeof(short).FullName)
            {
                short result;
                success = short.TryParse(Value, out result);
            }
            else if (paramInfo.ParameterType.FullName == typeof(ushort).FullName)
            {
                ushort result;
                success = ushort.TryParse(Value, out result);
            }
            else if (paramInfo.ParameterType.FullName == typeof(int).FullName)
            {
                int result;
                success = int.TryParse(Value, out result);
            }
            else if (paramInfo.ParameterType.FullName == typeof(uint).FullName)
            {
                uint result;
                success = uint.TryParse(Value, out result);
            }
            else if (paramInfo.ParameterType.FullName == typeof(long).FullName)
            {
                long result;
                success = long.TryParse(Value, out result);
            }
            else if (paramInfo.ParameterType.FullName == typeof(ulong).FullName)
            {
                ulong result;
                success = ulong.TryParse(Value, out result);
            }
            else if (paramInfo.ParameterType.FullName == typeof(float).FullName)
            {
                float result;
                success = float.TryParse(Value, NumberStyles.Float, CultureInfo.InvariantCulture, out result);
            }
            else if (paramInfo.ParameterType.FullName == typeof(double).FullName)
            {
                double result;
                success = double.TryParse(Value, NumberStyles.Float, CultureInfo.InvariantCulture, out result);
            }
            else
            {
                System.Diagnostics.Debug.Assert(false, "Implement this case");
            }

            if (!success)
            {
                throw new ArgumentException();
            }
        }

        public override string UniqueName => Value;
    }
}
