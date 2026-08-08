// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Win8Xaml.CompilerProxies
{
    public class CompilerDomRootToken
    {
        Object _instance;

        public CompilerDomRootToken(object instance)
        {
            _instance = instance;
        }

        public object Instance
        {
            get { return _instance; }
        }

        public DirectUISchemaContext Schema { get; set; }
    }
}
