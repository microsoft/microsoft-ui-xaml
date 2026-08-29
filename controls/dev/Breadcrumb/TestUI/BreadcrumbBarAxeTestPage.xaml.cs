// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System.Collections.ObjectModel;

namespace MUXControlsTestApp
{
    [AxeScanTestPage(Name = "BreadcrumbBar-Axe")]
    public sealed partial class BreadcrumbBarAxeTestPage : TestPage
    {
        public ObservableCollection<object> breadCrumbList { get; } = new ObservableCollection<object>();

        public BreadcrumbBarAxeTestPage()
        {
            for(int i = 0; i < 10; i++)
            {
                breadCrumbList.Add("Item " + i);
            }
            this.InitializeComponent();
        }
    }
}
