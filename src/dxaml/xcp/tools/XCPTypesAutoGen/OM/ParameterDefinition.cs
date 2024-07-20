// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM
{
    public class ParameterDefinition
    {
        public TypeReference ParameterType
        {
            get;
            set;
        }

        public bool RequiresNullCheck
        {
            get
            {
                return (!ParameterType.IsValueType || ParameterType.Type.IsStringType) && !ParameterType.IsOptional && !ParameterType.IsNullable;
            }
        }
    }
}
