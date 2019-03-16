using System.Linq;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Hosting;

namespace MUXControlsTestApp.Samples
{
    public sealed partial class ScaleAnimatedVerticalListDemo : Page
    {
        public ScaleAnimatedVerticalListDemo()
        {
            this.InitializeComponent();
            repeater.ItemsSource = Enumerable.Range(0, 100);
            repeater.ElementPrepared += OnElementPrepared;
        }

        private void OnElementPrepared(Microsoft.UI.Xaml.Controls.ItemsRepeater sender, Microsoft.UI.Xaml.Controls.ItemsRepeaterElementPreparedEventArgs args)
        {
            var item = ElementCompositionPreview.GetElementVisual(args.Element);
            var scrollProperties = ElementCompositionPreview.GetScrollViewerManipulationPropertySet(sv);
            var scaleExpresion = scrollProperties.Compositor.CreateExpressionAnimation();

            scaleExpresion.SetReferenceParameter("sv", scrollProperties);
            scaleExpresion.SetReferenceParameter("item", item);
            scaleExpresion.SetScalarParameter("svHeight", 150);
            scaleExpresion.SetScalarParameter("itemHeight", 30);

            scaleExpresion.Expression = "1 - abs((svHeight/2 - sv.Translation.Y) - (item.Offset.Y + itemHeight/2))*(.25/(svHeight/2))";


            item.StartAnimation("Scale.X", scaleExpresion);
            item.StartAnimation("Scale.Y", scaleExpresion);

            var itemContent = ElementCompositionPreview.GetElementVisual((args.Element as Panel).Children[0]);

            var offsetAnimation = scrollProperties.Compositor.CreateExpressionAnimation();
            offsetAnimation.SetReferenceParameter("sv", scrollProperties);
            offsetAnimation.SetReferenceParameter("item", item);
            offsetAnimation.SetScalarParameter("svHeight", 150);
            offsetAnimation.SetScalarParameter("itemHeight", 30);
            offsetAnimation.SetScalarParameter("itemWidth", 200);

            offsetAnimation.Expression = "abs((svHeight/2 - sv.Translation.Y) - (item.Offset.Y + itemHeight/2)) * itemWidth * (.18/(svHeight/2)) ";
            itemContent.StartAnimation("Offset.X", offsetAnimation);
        }

        private void OnItemClicked(object sender, RoutedEventArgs e)
        {
            var button = sender as FrameworkElement;
            var item = button.Parent as UIElement;

            item.StartBringIntoView(new BringIntoViewOptions() {
                VerticalAlignmentRatio = 0.5,
                AnimationDesired = true,
            });

        }
    }
}
