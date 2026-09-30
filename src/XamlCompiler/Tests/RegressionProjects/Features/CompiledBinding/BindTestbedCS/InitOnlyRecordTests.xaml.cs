// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.ObjectModel;
using Microsoft.UI.Xaml.Controls;

namespace BindTestbed
{
    public record BlogPost(string Title, string Teaser, Uri Url, DateTime Published);

    internal sealed partial class InitOnlyRecordTests : UserControl
    {
        public ObservableCollection<BlogPost> BlogPosts { get; } = new ObservableCollection<BlogPost>
        {
            new BlogPost("Test1", "Testing", null, DateTime.Now),
            new BlogPost("Test2", "Testing2", null, DateTime.Now),
        };

        public InitOnlyRecordTests()
        {
            this.InitializeComponent();
        }
    }
}
