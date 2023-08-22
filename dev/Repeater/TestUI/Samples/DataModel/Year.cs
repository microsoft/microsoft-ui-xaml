// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Collections.ObjectModel;

namespace MUXControlsTestApp.Samples.Model
{
    public class Year : ObservableCollection<RecipeGroup>
    {
        public int Value { get; set; }

        public Year(IEnumerable<RecipeGroup> recipeGroups)
            : base(recipeGroups)
        {

        }

        public override string ToString()
        {
            return string.Format("Year {0}", Value);
        }
    }
}
