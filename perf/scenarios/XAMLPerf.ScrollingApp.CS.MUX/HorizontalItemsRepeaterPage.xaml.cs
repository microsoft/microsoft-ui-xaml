using Interactions;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.ObjectModel;
using System.Threading.Tasks;

namespace XAMLPerf.ScrollingApp.CS.MUX
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class HorizontalItemsRepeaterPage : Page
    {
        private ObservableCollection<Item> Items
        {
            get
            {
                return (Application.Current as App).Items;
            }
        }

        private DispatcherTimer oneShotTimer = new DispatcherTimer();

        public HorizontalItemsRepeaterPage()
        {
            this.InitializeComponent();

            oneShotTimer.Interval = TimeSpan.FromSeconds(3);
            oneShotTimer.Tick += OneShotTimer_Tick;
            oneShotTimer.Start();
        }

        private static Task ExecuteItemsRepeaterScrollingScenario(IDeviceInteractionModel deviceInteractionModel)
        {
            return new ScrollingScenario(new HItemsRepeaterInteractionModel(deviceInteractionModel)).Execute();
        }

        private async void OneShotTimer_Tick(object sender, object e)
        {
            oneShotTimer.Stop();

            Control control = scrollViewer;

            await ExecuteItemsRepeaterScrollingScenario(new KeyboardInteractionModel(control));
            await ExecuteItemsRepeaterScrollingScenario(new MouseInteractionModel(control));
            await ExecuteItemsRepeaterScrollingScenario(new TouchInteractionModel(control));
            await ExecuteItemsRepeaterScrollingScenario(new GamepadInteractionModel(control));

            (Application.Current as App).PageDone();
        }
    }
}