using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Windows.Graphics;

namespace NonRectangularWindow
{
    public partial class App : Application
    {
        private Window _inputTarget;
        private MainWindow _window;

        public App()
        {
            InitializeComponent();
        }

        protected override void OnLaunched(LaunchActivatedEventArgs args)
        {
            int clicks = 0;
            var target = new Button
            {
                Content = "Click through the hole or outside the oval.\nClicks received: 0",
                HorizontalAlignment = HorizontalAlignment.Stretch,
                VerticalAlignment = VerticalAlignment.Stretch,
                HorizontalContentAlignment = HorizontalAlignment.Right,
                VerticalContentAlignment = VerticalAlignment.Bottom,
                Padding = new Thickness(24),
                Background = (Brush)Resources["AccentFillColorDefaultBrush"]
            };
            target.Click += (_, _) =>
                target.Content = $"Click through the hole or outside the oval.\nClicks received: {++clicks}";
            _inputTarget = new Window { Title = "Input target - ordinary WinUI window", Content = target };
            _inputTarget.Closed += (_, _) => _inputTarget = null;
            _inputTarget.AppWindow.MoveAndResize(new RectInt32(120, 120, 900, 650));
            _inputTarget.Activate();

            _window = new MainWindow();
            _window.Closed += (_, _) => _inputTarget?.Close();
            _window.Activate();
        }
    }
}
