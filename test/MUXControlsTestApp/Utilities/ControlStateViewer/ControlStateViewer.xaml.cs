﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using System.Reflection;
using System.Runtime.CompilerServices;

namespace MUXControlsTestApp.Utilities
{
    // Test control intended to help you see all the different visual states your control can be in
    public sealed partial class ControlStateViewer : UserControl
    {
        Type _controlType;
        List<string> _states;
        Style _style;

        public ControlStateViewer()
        {
            this.InitializeComponent();
        }

        public ControlStateViewer(Type controlType, List<string> states, Style style = null)
        {
            this.InitializeComponent();

            _controlType = controlType;
            _states = states;
            _style = style;

            UpdateGrid();
        }

        public Type ControlType
        {
            get 
            {
                return _controlType;
            }

            set
            {
                _controlType = value;
                UpdateGrid();
            }
        }

        public List<string> States
        {
            get
            {
                return _states;
            }

            set
            {
                _states = value;
                UpdateGrid();
            }
        }

        private void UpdateGrid()
        {
            StateGridView.Items.Clear();

            if (_controlType == null || _states == null)
            {
                return;
            }

            foreach (string state in _states)
            {
                StackPanel sp = new StackPanel();
                sp.Margin = new Thickness(4);

                TextBlock textBlock = new TextBlock();
                textBlock.Text = state;
                textBlock.FontSize = 9;
                sp.Children.Add(textBlock);

                Control c = Activator.CreateInstance(_controlType) as Control;
                if (_style != null)
                {
                    c.Style = _style;
                }
                c.Loaded += Control_Loaded;
                c.DataContext = state;

                // Special setup for some controls that need a bit of help
                if (_controlType == typeof(Slider))
                {
                    c.Width = 100;
                    (c as Slider).Value = 30;
                }

                ContentControl cc = c as ContentControl;
                if (cc != null)
                {
                    string[] s = _controlType.ToString().Split('.');
                    cc.Content = s[s.Length - 1];
                }

                sp.Children.Add(c);

                StateGridView.Items.Add(sp);
            }
        }

        private void Control_Loaded(object sender, RoutedEventArgs e)
        {
            Control c = sender as Control;
            if (c != null)
            {
                foreach (string state in (c.DataContext as string).Split('|'))
                {
                    VisualStateManager.GoToState(c, state, false);
                }
            }
        }
    }
}
