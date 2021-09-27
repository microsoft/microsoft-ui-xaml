using Microsoft.UI.Private.Controls;
using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Documents;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using IconSource = Microsoft.UI.Xaml.Controls.IconSource;

// The Templated Control item template is documented at https://go.microsoft.com/fwlink/?LinkId=234235

namespace MUXControlsTestApp
{
    public sealed class AnimatedIconHost : Button
    {
        Border m_iconPresenter;
        TextBlock m_transitionTextBlock;

        public IconSource IconSource
        {
            get { return (IconSource)GetValue(IconSourceProperty); }
            set { SetValue(IconSourceProperty, value); }
        }

        public static readonly DependencyProperty IconSourceProperty = DependencyProperty.Register(
          "IconSource",
          typeof(IconSource),
          typeof(AnimatedIconHost),
          new PropertyMetadata(null, new PropertyChangedCallback(OnIconSourceChanged))
        );

        private static void OnIconSourceChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            ((AnimatedIconHost)d).IconSourceChanged();
        }

        public String Title
        {
            get { return (String)GetValue(TitleProperty); }
            set { SetValue(TitleProperty, value); }
        }

        public static readonly DependencyProperty TitleProperty = DependencyProperty.Register(
          "Title",
          typeof(String),
          typeof(AnimatedIconHost),
          null
        );

        public String FallbackGlyph
        {
            get { return (String)GetValue(FallbackGlyphProperty); }
            set { SetValue(FallbackGlyphProperty, value); }
        }

        public static readonly DependencyProperty FallbackGlyphProperty = DependencyProperty.Register(
          "FallbackGlyph",
          typeof(String),
          typeof(AnimatedIconHost),
          null
        );

        public AnimatedIconHost()
        {
            this.DefaultStyleKey = typeof(AnimatedIconHost);
            AnimatedIconTestHooks.LastAnimationSegmentChanged += AnimatedIconTestHooks_LastAnimationSegmentChanged;
        }

        private void AnimatedIconTestHooks_LastAnimationSegmentChanged(AnimatedIcon sender, object args)
        {
            if(sender == GetAnimatedIcon())
            {
                m_transitionTextBlock.Text = AnimatedIconTestHooks.GetLastAnimationSegment(sender);
            }
        }

        public AnimatedIcon GetAnimatedIcon()
        {
            return m_iconPresenter.Child as AnimatedIcon;
        }

        override protected void OnApplyTemplate()
        {
            m_iconPresenter = (Border)GetTemplateChild("Icon");
            m_transitionTextBlock = (TextBlock)GetTemplateChild("TransitionTextBlock");

            IconSourceChanged();
        }

        private void IconSourceChanged()
        {
            if (m_iconPresenter != null)
            {
                m_iconPresenter.Child = IconSource.CreateIconElement();
            }
        }
    }
}
