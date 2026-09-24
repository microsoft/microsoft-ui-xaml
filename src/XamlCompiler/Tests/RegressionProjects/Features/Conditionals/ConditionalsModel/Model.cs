// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ConditionalControls
{
    public sealed class Model
    {
        public Model()
        {
            this.Org = new Organization();
        }

        public Organization Org { get; }
    }
}
