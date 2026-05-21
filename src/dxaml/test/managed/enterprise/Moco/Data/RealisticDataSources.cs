// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Threading.Tasks;
using Windows.Foundation;
using Microsoft.UI.Xaml.Data;

using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Enterprise.Moco.Data
{
    public class RealisticIncrementalLoadingDataSource<T> : ObservableCollection<T>, ISupportIncrementalLoading
    {
        private Func<T> _createItemFunction;

        public RealisticIncrementalLoadingDataSource(IEnumerable<T> list, Func<T> createItemFunction)
        {
            _createItemFunction = createItemFunction;

            foreach (T item in list)
            {
                this.Add(item);
            }
        }

        #region ISupportIncrementalLoading Members

        // The ISIL Interface requires a getter, whereas for testability I expose a setter also.
        bool hasMoreItems;
        public bool HasMoreItems 
        { 
            get
            {
                return this.hasMoreItems;   
            }

            set
            {
                this.hasMoreItems = value;
            }
        }

        public global::Windows.Foundation.IAsyncOperation<LoadMoreItemsResult> LoadMoreItemsAsync(uint count)
        {
            var asyncOperation = new RealisticLoadMoreItemsAsync<T>(this, count, _createItemFunction);
            asyncOperation.TimeDelay = 1000;
            asyncOperation.TrickleItems = false;
            asyncOperation.CreatePlaceholders = false;
            asyncOperation.Start();

            return asyncOperation;
        }

        #endregion
    }

    public partial class RealisticLoadMoreItemsAsync<T> : IAsyncOperation<LoadMoreItemsResult>
    {
        private IList<T> _internalCollection;
        private uint _numItemsToLoad;
        private Func<T> _createItemFunction;

        public RealisticLoadMoreItemsAsync(IList<T> collection, uint count, Func<T> createItemFunction)
        {
            _internalCollection = collection;
            _numItemsToLoad = count;
            _createItemFunction = createItemFunction;
        }

        public bool CreatePlaceholders { get; set; }
        public int TimeDelay { get; set; }
        public bool TrickleItems { get; set; }

        #region IAsyncOperation<LoadMoreItemsResult> Members

        public AsyncOperationCompletedHandler<LoadMoreItemsResult> Completed { get; set; }

        public LoadMoreItemsResult GetResults()
        {
            return new LoadMoreItemsResult { Count = _numItemsToLoad };
        }

        #endregion

        #region IAsyncInfo Members

        public void Cancel()
        {
            throw new NotImplementedException();
        }

        public void Close()
        {
            // called after results have been consumed but before Release
        }

        public Exception ErrorCode
        {
            get { throw new NotImplementedException(); }
        }

        public uint Id
        {
            get { throw new NotImplementedException(); }
        }

        public AsyncStatus Status { get; set; }

        #endregion

        async public void Start()
        {
            Status = AsyncStatus.Started;

            int startingIndex = _internalCollection.Count;

            if (CreatePlaceholders)
            {
                for (int i = 0; i < _numItemsToLoad; i++)
                {
                    _internalCollection.Add(default(T));
                }
            }

            if (!TrickleItems)
            {
                await Task.Delay(TimeDelay);
            }

            for (int i = 0; i < _numItemsToLoad; i++)
            {
                if (TrickleItems)
                {
                    await Task.Delay(TimeDelay);
                }
                if (CreatePlaceholders)
                {
                    _internalCollection[startingIndex + i] = _createItemFunction();
                }
                else
                {
                    _internalCollection.Add(_createItemFunction());
                }
            }

            Status = AsyncStatus.Completed;

            if (Completed != null)
            {
                Completed(this, AsyncStatus.Completed);
            }
        }
    }
}
