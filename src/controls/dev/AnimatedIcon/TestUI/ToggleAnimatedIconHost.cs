﻿using Microsoft.UI.Private.Controls;
using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using IconSource = Microsoft.UI.Xaml.Controls.IconSource;

// The Templated Control item template is documented at https://go.microsoft.com/fwlink/?LinkId=234235

namespace MUXControlsTestApp
{
    public sealed partial class ToggleAnimatedIconHost : CheckBox
    {
        Border m_iconPresenter;
        TextBlock m_transitionTextBlock;

        public static readonly DependencyProperty IconSourceProperty = DependencyProperty.Register(
          "IconSource",
          typeof(IconSource),
          typeof(AnimatedIconHost),
          new PropertyMetadata(null, new PropertyChangedCallback(OnIconSourceChanged))
        );

        public IconSource IconSource
        {
            get { return (IconSource)GetValue(IconSourceProperty); }
            set { SetValue(IconSourceProperty, value); }
        }

        private static void OnIconSourceChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            ((ToggleAnimatedIconHost)d).IconSourceChanged();
        }

        public String Title
        {
            get { return (String)GetValue(TitleProperty); }
            set { SetValue(TitleProperty, value); }
        }

        public static readonly DependencyProperty TitleProperty = DependencyProperty.Register(
          "Title",
          typeof(String),
          typeof(ToggleAnimatedIconHost),
          null
        );

        public ToggleAnimatedIconHost()
        {
            this.DefaultStyleKey = typeof(AnimatedIconHost);
            AnimatedIconTestHooks.LastAnimationSegmentChanged += AnimatedIconTestHooks_LastAnimationSegmentChanged;
        }

        private void AnimatedIconTestHooks_LastAnimationSegmentChanged(AnimatedIcon sender, object args)
        {
            if (sender == GetAnimatedIcon())
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
                AnimatedIcon animatedIcon = new AnimatedIcon();
                AnimatedIconSource source = (AnimatedIconSource)IconSource;
                if (source.Source != null)
                {
                    animatedIcon.Source = source.Source;
                }
                if (source.FallbackIconSource != null)
                {
                    animatedIcon.FallbackIconSource = source.FallbackIconSource;
                }
                if (source.Foreground != null)
                {
                    animatedIcon.Foreground = source.Foreground;
                }
                m_iconPresenter.Child = animatedIcon;
            }
        }
    }
}
