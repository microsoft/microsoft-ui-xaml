// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Win8Xaml.CompilerProxies
{
    public class XbfFilenameInfoArrayToken
    {
        Object _instance;

        public XbfFilenameInfoArrayToken(object instance)
        {
            _instance = instance;
        }

        public object Instance
        {
            get { return _instance; }
        }

        // I think this could be implemented with an indexer override and Property Invokes
        // to make this a live list of filenames.  But we don't need that.
    }
}
