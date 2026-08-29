// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace PathVisualizer
{
    public partial class MainWindow
    {
        public MainWindow()
        {
            var viewModel = new ObservableModel();
            viewModel.PropertyChanged += (s, e) => {
                if (e.PropertyName == "PathTree") { Visualizer.Tree = viewModel.PathTree; }
                };

            DataContext = viewModel;

            InitializeComponent();
        }
    }
}