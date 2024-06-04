// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM
{
    public class AttributeInstantiation
    {
        private List<object> _arguments = new List<object>();

        public TypeDefinition ArgumentType
        {
            get;
            set;
        }

        public List<object> Arguments
        {
            get
            {
                return _arguments;
            }
        }
    }
}
