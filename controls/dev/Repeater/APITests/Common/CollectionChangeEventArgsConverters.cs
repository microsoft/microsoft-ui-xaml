// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Linq;
using Windows.Foundation.Collections;
using MUXControlsTestApp.Utilities;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common
{
    public static class CollectionChangeEventArgsConverters
    {
        public static NotifyCollectionChangedEventArgs ConvertToDataSourceChangedEventArgs(this IVectorChangedEventArgs args)
        {
            List<object> addedItems = new List<object>();
            List<object> removedItems = new List<object>();

            NotifyCollectionChangedEventArgs newArgs;

            switch (args.CollectionChange)
            {
                case CollectionChange.ItemInserted:
                    addedItems.Add(null);
                    newArgs = new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Add, addedItems, (int)args.Index);
                    break;
                case CollectionChange.ItemRemoved:
                    removedItems.Add(null);
                    newArgs = new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Remove, removedItems, (int)args.Index);
                    break;
                case CollectionChange.ItemChanged:
                    addedItems.Add(null);
                    removedItems.Add(null);
                    newArgs = new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Replace, addedItems, removedItems, (int)args.Index);
                    break;
                case CollectionChange.Reset:
                    newArgs = new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Reset);
                    break;
                default:
                    throw new InvalidOperationException();
            }

            return newArgs;
        }

        public static NotifyCollectionChangedEventArgs CreateNotifyArgs(
            NotifyCollectionChangedAction action,
            int oldStartingIndex,
            int oldItemsCount,
            int newStartingIndex,
            int newItemsCount)
        {
            NotifyCollectionChangedEventArgs newArgs = null;

            List<object> addedItems = new List<object>();
            List<object> removedItems = new List<object>();

            switch (action)
            {
                case NotifyCollectionChangedAction.Add:
                    {
                        for (int i = 0; i < newItemsCount; i++)
                        {
                            addedItems.Add(null);
                        }

                        newArgs = new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Add, addedItems, newStartingIndex);
                    }
                    break;
                case NotifyCollectionChangedAction.Remove:
                    {
                        for (int i = 0; i < oldItemsCount; i++)
                        {
                            removedItems.Add(null);
                        }
                        newArgs = new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Remove, removedItems, oldStartingIndex);
                    }
                    break;
                case NotifyCollectionChangedAction.Replace:
                    {
                        for (int i = 0; i < newItemsCount; i++)
                        {
                            addedItems.Add(null);
                        }
                        for (int i = 0; i < oldItemsCount; i++)
                        {
                            removedItems.Add(null);
                        }
                        newArgs = new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Replace, addedItems, removedItems, newStartingIndex);
                    }
                    break;
                case NotifyCollectionChangedAction.Move:
                    {
                        List<object> movedItems = new List<object>();
                        for (int i = 0; i < oldItemsCount; i++)
                        {
                            movedItems.Add(null);
                        }
                        newArgs = new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Move, movedItems, newStartingIndex, oldStartingIndex);
                    }
                  break;
                case NotifyCollectionChangedAction.Reset:
                    newArgs = new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Reset);
                    break;
                default:
                    throw new InvalidOperationException();
            }

            return newArgs;
        }
    }
}
