// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;

namespace ItemsRepeaterDemos
{
    public class Data
    {
        static int _next = 0;

        public static ObservableCollection<object> CreateNested(int levels = 3, int groupsAtLevel = 5, int countAtLeaf = 10, string prefix="0")
        {
            var data = new NamedObservableCollection<object>() { Name = prefix };
            if (levels != 0)
            {
                for (int i = 0; i < groupsAtLevel; i++)
                {
                    data.Add(CreateNested(levels - 1, groupsAtLevel, countAtLeaf, prefix + "." + i.ToString()));
                }
            }
            else
            {
                for (int i = 0; i < countAtLeaf; i++)
                {
                    data.Add(prefix + "." + _next++.ToString());
                }
            }

            return data;
        }
    }

    public class NamedObservableCollection<T> : ObservableCollection<T>
    {
        public string Name { get; set; }

        public override string ToString()
        {
            return Name;
        }
    }
}
