// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace MUXControlsTestApp
{
    // The InkCanvas inks by default — a bare <InkCanvas/> produces wet ink for
    // mouse, pen, and touch with no app code. The optional InkToolBar in the XAML
    // drives the canvas's InkPresenter (pen / eraser / stroke color and width).
    [TopLevelTestPage(Name = "InkCanvas")]
    public sealed partial class InkCanvasPage : TestPage
    {
        public InkCanvasPage()
        {
            this.InitializeComponent();
        }
    }
}
