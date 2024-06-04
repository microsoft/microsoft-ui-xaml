// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace MUXControlsTestApp
{
    public sealed partial class TextPage : TestPage
    {
        public TextPage()
        {
            this.InitializeComponent();

            for (int i=0; i<32; i++)
                this.textBox.Text += "Text " + i + "\r\n";
        }
    }
}
