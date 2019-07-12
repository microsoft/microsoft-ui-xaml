// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation;

using Microsoft.UI.Private.Controls;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "SliderInteraction", Icon = "Slider.png")]
    public sealed partial class SliderInteractionPage : TestPage
    {
        public SliderInteractionPage()
        {
            this.InitializeComponent();

            HorizontalSliderInteraction = new SliderInteraction()
            {
                Minimum = 0,
                Maximum = 200 - 2 - 20 // Subtract 2x host border thickness and the width of the scrubber thumb
            };

            horizontalElement.Interactions.Add(HorizontalSliderInteraction);

            VerticalSliderInteraction = new SliderInteraction()
            {
                Orientation = Orientation.Vertical,
                Minimum = 0,
                Maximum = 200 - 2 - 20 // Subtract 2x host border thickness and the height of the scrubber thumb
            };

            verticalElement.Interactions.Add(VerticalSliderInteraction);
        }

        public SliderInteraction HorizontalSliderInteraction
        {
            get; private set;
        }

        public SliderInteraction VerticalSliderInteraction
        {
            get; private set;
        }
    }
}
