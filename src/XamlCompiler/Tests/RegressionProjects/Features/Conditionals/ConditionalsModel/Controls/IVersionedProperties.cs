// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ConditionalControls
{
    public interface IVersionedProperties
    {
        string V1Property { get; set; }
        string V2Property { get; set; }
        string V3Property { get; set; }
    }
}
