// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "TeachingTip", Icon = "TeachingTip.png")]
    public sealed partial class TeachingTipCaseBundle : TestPage
    {
        public TeachingTipCaseBundle()
        {
            InitializeComponent();

            TeachingTipPageButton.Click += delegate { Frame.NavigateWithoutAnimation(typeof(TeachingTipPage), 0); };
            TeachingTipFocusPageButton.Click += delegate { Frame.NavigateWithoutAnimation(typeof(TeachingTipFocusPage), 0); };
        }
    }
}
