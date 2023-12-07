// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace XamlGen.Templates.Metadata
{
    public abstract class XbfMetadataCodeGenerator<TModel> : XamlCodeGenerator<TModel>
    {
        public StableXbfIndexGenerator StableIndexes
        {
            get;
            set;
        }
    }
}
