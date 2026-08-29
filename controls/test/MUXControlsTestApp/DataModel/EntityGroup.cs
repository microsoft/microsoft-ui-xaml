// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;
using System.Collections.Generic;
using MUXControlsTestApp.Utils;

namespace MUXControlsTestApp.Samples.Model
{
    public class EntityGroup : ResetableCollection<Entity>
    {
        public string Name { get; set; }

        public EntityGroup(IEnumerable<Entity> entities)
            : base(entities)
        { }

        public override string ToString()
        {
            return Name;
        }
    }
}
