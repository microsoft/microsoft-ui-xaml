// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM
{
    public abstract class MethodBaseDefinition : MemberDefinition
    {
        private List<ParameterDefinition> _parameters = new List<ParameterDefinition>();

        public List<ParameterDefinition> Parameters
        {
            get
            {
                return _parameters;
            }
        }
    }
}
