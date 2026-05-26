// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Data;
using System.ComponentModel;

namespace Microsoft.UI.Xaml.Tests.Enterprise.Moco.MaintainViewport
{
    public class MoCoFlatDataModel : INotifyPropertyChanged
    {
        public MoCoFlatDataModel() {}

        public MoCoFlatDataModel(string name)
        {
            Name = name;
        }

        string _name;
        public string Name
        {
            get
            {
                return _name;
            }
            set
            {
                _name = value;
                RaisePropertyChanged("Name");
            }
        }

        Visibility _isExpanded = Visibility.Collapsed;
        public Visibility IsExpanded
        {
            get
            {
                return _isExpanded;
            }
            set
            {
                _isExpanded = value;
                RaisePropertyChanged("IsExpanded");
            }
        }

        public override string ToString()
        {
            return Name;
        }

        public override bool Equals(object obj)
        {
            return (obj != null && obj is MoCoFlatDataModel && (obj as MoCoFlatDataModel).Name == this.Name);
        }

        public override int GetHashCode()
        {
            return Name.GetHashCode();
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