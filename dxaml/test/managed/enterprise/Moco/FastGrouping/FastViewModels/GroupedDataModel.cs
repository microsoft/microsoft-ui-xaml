// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;

namespace Microsoft.UI.Xaml.Tests.Enterprise.Moco.FastGrouping
{
    public abstract class GroupedDataModel
    {
        public abstract object GetGroupedDataSource();

        public abstract void AddEmptyGroup(string groupName);

        public abstract void AddGroupItem(string groupName, Object item);

        public abstract void RemoveSelectedGroupItems(IList<object> items);

        public abstract void ReplaceGroup(string groupName);

        public abstract void ResetGroup(string groupName);

        public abstract void DeleteGroup(string groupName);

        public abstract void ClearGroup(string groupName);

        public abstract void ReplaceFirstGroupItem(string groupName, object item);

        public abstract object GetGroup(string groupName);
    }
}
