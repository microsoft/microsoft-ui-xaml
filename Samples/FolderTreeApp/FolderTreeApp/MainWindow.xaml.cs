using System;
using System.Collections.ObjectModel;
using System.Numerics;
using Microsoft.UI;
using Microsoft.UI.Composition;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Hosting;

namespace FolderTreeApp
{
    public sealed partial class MainWindow : Window
    {
        public ObservableCollection<FolderNode> RootNodes { get; } = new ObservableCollection<FolderNode>();

        public MainWindow()
        {
            this.InitializeComponent();
            LoadRootFolder();
        }

        private void LoadRootFolder()
        {
            const string rootPath = @"C:\Users\t-rdiggavi";
            var rootNode = new FolderNode(rootPath);
            rootNode.LoadChildren();
            RootNodes.Add(rootNode);
            StatusText.Text = $"Browsing: {rootPath}";
        }

        private void FolderTreeView_Expanding(TreeView sender, TreeViewExpandingEventArgs args)
        {
            if (args.Item is FolderNode folder && folder.HasUnrealizedChildren)
            {
                folder.LoadChildren();
            }
        }

        private void FolderTreeView_SelectionChanged(TreeView sender, TreeViewSelectionChangedEventArgs args)
        {
            if (sender.SelectedItem is FolderNode folder)
            {
                StatusText.Text = $"Selected: {folder.Name}";
            }
        }

        private AnimationWindow m_animationWindow;

        private void OpenAnimations_Click(object sender, RoutedEventArgs e)
        {
            if (m_animationWindow == null)
            {
                m_animationWindow = new AnimationWindow();
                m_animationWindow.Closed += (s, a) => m_animationWindow = null;
            }
            m_animationWindow.Activate();
        }

        // Build a couple of composition visuals by hand and host them in named Borders, so the
        // composition tree shows more than the SpriteVisuals XAML normally emits: a ShapeVisual
        // (with a CompositionSpriteShape) and a ContainerVisual holding several SpriteVisuals.
        private void RootGrid_Loaded(object sender, RoutedEventArgs e)
        {
            var compositor = ElementCompositionPreview.GetElementVisual(RootGrid).Compositor;

            BuildShapeVisual(compositor);
            BuildContainerVisual(compositor);
        }

        // A ShapeVisual containing a stroked, filled ellipse — plus a forever rotation animation
        // so it is easy to spot in a live capture.
        private void BuildShapeVisual(Compositor compositor)
        {
            var shapeVisual = compositor.CreateShapeVisual();
            shapeVisual.Size = new Vector2(140, 140);
            shapeVisual.CenterPoint = new Vector3(70, 70, 0);
            shapeVisual.Comment = "FolderTreeApp.Showcase.ShapeVisual";

            var ellipseGeometry = compositor.CreateEllipseGeometry();
            ellipseGeometry.Radius = new Vector2(55, 45);
            ellipseGeometry.Center = new Vector2(70, 70);

            var ellipseShape = compositor.CreateSpriteShape(ellipseGeometry);
            ellipseShape.FillBrush = compositor.CreateColorBrush(Colors.MediumPurple);
            ellipseShape.StrokeBrush = compositor.CreateColorBrush(Colors.Purple);
            ellipseShape.StrokeThickness = 4;
            shapeVisual.Shapes.Add(ellipseShape);

            ElementCompositionPreview.SetElementChildVisual(ShapeVisualHost, shapeVisual);

            var rotate = compositor.CreateScalarKeyFrameAnimation();
            rotate.InsertKeyFrame(1f, 360f, compositor.CreateLinearEasingFunction());
            rotate.Duration = TimeSpan.FromSeconds(6);
            rotate.IterationBehavior = AnimationIterationBehavior.Forever;
            shapeVisual.StartAnimation("RotationAngleInDegrees", rotate);
        }

        // A ContainerVisual parenting three colored SpriteVisuals.
        private void BuildContainerVisual(Compositor compositor)
        {
            var container = compositor.CreateContainerVisual();
            container.Size = new Vector2(200, 80);
            container.Comment = "FolderTreeApp.Showcase.ContainerVisual";

            var colors = new[] { Colors.Tomato, Colors.Gold, Colors.MediumSeaGreen };
            for (int i = 0; i < colors.Length; i++)
            {
                var sprite = compositor.CreateSpriteVisual();
                sprite.Size = new Vector2(56, 56);
                sprite.Offset = new Vector3(12 + i * 64, 12, 0);
                sprite.Brush = compositor.CreateColorBrush(colors[i]);
                sprite.Comment = $"FolderTreeApp.Showcase.Sprite{i}";
                container.Children.InsertAtTop(sprite);
            }

            ElementCompositionPreview.SetElementChildVisual(ContainerVisualHost, container);
        }
    }
}
