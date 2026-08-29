// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;

using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Enterprise.Moco.FastGrouping
{
    public class ObservableGroup : ObservableCollection<object>
    {
        public string GroupName { get; set; }

        public ObservableGroup()
        {

        }

        public void ResetGroup(List<object> newContents)
        {
            this.Items.Clear();
            foreach (var obj in newContents)
            {
                this.Items.Add(obj);
            }

            var collectionChangedArgs = new global::System.Collections.Specialized.NotifyCollectionChangedEventArgs(
                NotifyCollectionChangedAction.Reset);
            OnCollectionChanged(collectionChangedArgs);
        }

        public IEnumerable<object> GetIEnumerable()
        {
            return this as IEnumerable<object>;
        }

        public Collection<object> GetCollection()
        {
            return this.Items as Collection<object>;
        }

        public override string ToString()
        {
            return GroupName;
        }
    }
}
