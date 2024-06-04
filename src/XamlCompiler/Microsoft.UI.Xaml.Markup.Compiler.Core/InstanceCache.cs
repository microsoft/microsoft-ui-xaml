// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;

namespace Microsoft.UI.Xaml.Markup.Compiler.Core
{
    public class InstanceCacheManager
    {
        private static List<Action> clearActions = new List<Action>();

        public static void Register(Action action)
        {
            clearActions.Add(action);
        }

        public static void ClearCache()
        {
            foreach (var clear in clearActions)
            {
                clear.Invoke();
            }
        }
    }

    public class InstanceCache<TKey, TValue> : Dictionary<TKey, TValue>
    {
        public InstanceCache()
        {
            InstanceCacheManager.Register(() => this.Clear());
        }

        public InstanceCache(Action clearAction)
        {
            InstanceCacheManager.Register(clearAction);
        }
    }
}
