using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace MUXControlsTestApp.Samples
{
    public sealed partial class LCView : UserControl
    {
        public static readonly DependencyProperty ViewModelProperty =
            DependencyProperty.Register(
                "ViewModel",
                typeof(LCItem),
                typeof(LCView),
                new PropertyMetadata(null));

        /// <summary>
        /// Gets or sets the viewmodel the chrome binds to.
        /// </summary>
        public LCItem ViewModel
        {
            get { return (LCItem)this.GetValue(ViewModelProperty); }
            set { this.SetValue(ViewModelProperty, value); }
        }

        public LCView()
        {
            this.InitializeComponent();
        }
    }
}
