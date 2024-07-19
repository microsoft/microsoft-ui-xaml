﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    public sealed partial class TreeViewItemTemplateSelectorTestPage : TestPage
    {
        public TreeViewItemTemplateSelectorTestPage()
        {
            this.InitializeComponent();
            TestTreeView.RootNodes.Add(new TreeViewNode() { Content = 1 });
            TestTreeView.RootNodes.Add(new TreeViewNode() { Content = 2 });
        }
    }

    public class TreeViewItemTemplateSelector : DataTemplateSelector
    {
        public DataTemplate Template1 { get; set; }
        public DataTemplate Template2 { get; set; }

        protected override DataTemplate SelectTemplateCore(object item)
        {
            var node = (TreeViewNode)item;
            int content = (int)node.Content;
            if (content % 2 == 0) return Template2;
            return Template1;
        }
    }

    public class TreeViewItemStyleSelector : StyleSelector
    {
        public Style Style1 { get; set; }
        public Style Style2 { get; set; }

        protected override Style SelectStyleCore(object item, DependencyObject container)
        {
            var node = (TreeViewNode)item;
            int content = (int)node.Content;
            if (content % 2 == 0) return Style2;
            return Style1;
        }
    }
}
