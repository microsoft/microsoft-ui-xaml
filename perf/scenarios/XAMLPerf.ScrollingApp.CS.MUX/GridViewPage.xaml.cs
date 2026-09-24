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
    public sealed partial class GridViewPage : Page
    {
        private ObservableCollection<Item> Items
        {
            get
            {
                return (Application.Current as App).Items;
            }
        }

        private DispatcherTimer oneShotTimer = new DispatcherTimer();

        public GridViewPage()
        {
            this.InitializeComponent();

            oneShotTimer.Interval = TimeSpan.FromSeconds(3);
            oneShotTimer.Tick += OneShotTimer_Tick;
            oneShotTimer.Start();
        }

        private static Task ExecuteGridViewScrollingScenario(IDeviceInteractionModel deviceInteractionModel)
        {
            return new ScrollingScenario(new HListViewInteractionModel(deviceInteractionModel)).Execute();
        }

        private async void OneShotTimer_Tick(object sender, object e)
        {
            oneShotTimer.Stop();

            Control control = gridView;

            await ExecuteGridViewScrollingScenario(new KeyboardInteractionModel(control));
            await ExecuteGridViewScrollingScenario(new MouseInteractionModel(control));
            await ExecuteGridViewScrollingScenario(new TouchInteractionModel(control));
            await ExecuteGridViewScrollingScenario(new GamepadInteractionModel(control));

            (Application.Current as App).PageDone();
        }

        private void gridView_ItemClick(object sender, ItemClickEventArgs e)
        {
            ++(e.ClickedItem as Item).Count;
        }
    }
}