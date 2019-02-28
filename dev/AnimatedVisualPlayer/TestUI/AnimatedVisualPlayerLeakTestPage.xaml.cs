using System;
using System.Threading.Tasks;
using Windows.UI.Composition;
using AnimatedVisualPlayerTests;
using Windows.Foundation.Metadata;
using Windows.UI.Xaml;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace MUXControlsTestApp
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class AnimatedVisualPlayerLeakTestPage : TestPage
    {
        public AnimatedVisualPlayerLeakTestPage()
        {
            this.InitializeComponent();

            ToAnimatedVisualPlayerPageButton.Click += delegate { Frame.NavigateWithoutAnimation(typeof(AnimatedVisualPlayerPage), 0); };
        }
    }
}
