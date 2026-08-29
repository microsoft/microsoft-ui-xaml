// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System.Collections.ObjectModel;
using System.ComponentModel;

using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Enterprise.Moco.MaintainViewport
{
    public class MoCoGroupedDataModel : INotifyPropertyChanged
    {
        string _header = string.Empty;
        public string Header
        {
            get
            {
                return _header;
            }
            private set
            {
                _header = value;
                RaisePropertyChanged("Header");
            }
        }

        ObservableCollection<MoCoFlatDataModel> _items = new ObservableCollection<MoCoFlatDataModel> ();
        public ObservableCollection<MoCoFlatDataModel> Items
        {
            get
            {
                return _items;
            }
            private set
            {
                _items = value;
                RaisePropertyChanged("Items");
            }
        }

        public MoCoGroupedDataModel()
        {
            Header = "#";
        }

        public MoCoGroupedDataModel(string key)
        {
            Header = key;
        }

        #region INotifyPropertyChanged
        public event PropertyChangedEventHandler PropertyChanged;
        protected void RaisePropertyChanged(string propertyName)
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
            }
        }
        #endregion
    }
}