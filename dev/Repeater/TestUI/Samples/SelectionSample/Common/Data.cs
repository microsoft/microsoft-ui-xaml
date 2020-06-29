// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;

namespace MUXControlsTestApp.Samples.Selection
{
    public class Data
    {
        static int _next = 0;

        public static ObservableCollection<object> CreateNested(int levels = 3, int groupsAtLevel = 5, int countAtLeaf = 10)
        {
            var data = new ObservableCollection<object>();
            if (levels != 0)
            {
                for (int i = 0; i < groupsAtLevel; i++)
                {
                    data.Add(CreateNested(levels - 1, groupsAtLevel, countAtLeaf));
                }
            }
            else
            {
                for (int i = 0; i < countAtLeaf; i++)
                {
                    data.Add(_next++);
                }
            }

            return data;
        }
    }
}
