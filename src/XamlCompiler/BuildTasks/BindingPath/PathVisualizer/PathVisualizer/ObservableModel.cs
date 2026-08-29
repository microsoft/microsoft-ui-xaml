// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows.Input;

namespace PathVisualizer
{
    internal class ObservableBase : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;

        protected void RaisePropertyChanged([CallerMemberName]string propertyName = "")
        {
            if (null != PropertyChanged)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
            }
        }
    }

    internal class Command : ICommand
    {
        public Command(ExecuteHandler onExecute)
        {
            OnExecute = onExecute;
        }

        private bool _canExecute = true;
        public bool CanExecuteValue
        {
            get { return _canExecute; }
            set
            {
                if (value != _canExecute)
                {
                    _canExecute = value;
                    if (null != CanExecuteChanged)
                    {
                        CanExecuteChanged(this, new EventArgs());
                    }
                }
            }
        }
        public event EventHandler CanExecuteChanged;

        public bool CanExecute(object parameter)
        {
            return CanExecuteValue;
        }

        public delegate void ExecuteHandler(object parameter);
        private event ExecuteHandler OnExecute;

        public void Execute(object parameter)
        {
            if (null != OnExecute) OnExecute(parameter);
        }
    }

    internal class ObservableModel : ObservableBase
    {
        public ObservableModel()
        {
            BuildTree = new Command((p) => {
                PathTree = PathTreeBuilder.ParsePath(SourcePath);
                });
        }

        public string SourcePath
        {
            get { return _sourcePath; }
            set
            {
                if (_sourcePath != value)
                {
                    _sourcePath = value;
                    RaisePropertyChanged();
                }
            }
        }
        private string _sourcePath;

        public Command BuildTree { get; }

        public PathTree PathTree
        {
            get { return _pathTree; }
            private set
            {
                if (_pathTree != value)
                {
                    _pathTree = value;
                    RaisePropertyChanged();
                }
            }
        }
        private PathTree _pathTree;
    }
}