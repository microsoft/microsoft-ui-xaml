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
    public sealed partial class VerticalListViewPage : Page
    {
        private ObservableCollection<Item> Items
        {
            get
            {
                return (Application.Current as App).Items;
            }
        }

        private DispatcherTimer oneShotTimer = new DispatcherTimer();

        public VerticalListViewPage()
        {
            this.InitializeComponent();

            oneShotTimer.Interval = TimeSpan.FromSeconds(3);
            oneShotTimer.Tick += OneShotTimer_Tick;
            oneShotTimer.Start();
        }

        private static Task ExecuteListViewScrollingScenario(IDeviceInteractionModel deviceInteractionModel)
        {
            return new ScrollingScenario(new VListViewInteractionModel(deviceInteractionModel)).Execute();
        }

        private async void OneShotTimer_Tick(object sender, object e)
        {
            oneShotTimer.Stop();

            Control control = listView;

            await ExecuteListViewScrollingScenario(new KeyboardInteractionModel(control));
            await ExecuteListViewScrollingScenario(new MouseInteractionModel(control));
            await ExecuteListViewScrollingScenario(new TouchInteractionModel(control));
            await ExecuteListViewScrollingScenario(new GamepadInteractionModel(control));

            (Application.Current as App).PageDone();
        }

        private void listView_ItemClick(object sender, ItemClickEventArgs e)
        {
            ++(e.ClickedItem as Item).Count;
        }
    }
}