// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp;
using System;
using Windows.Foundation.Metadata;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media.Animation;

namespace MUXControlsTestApp
{
    public class NavigateToTestCommand : System.Windows.Input.ICommand
    {
        public event EventHandler CanExecuteChanged;

        public NavigateToTestCommand()
        {
            CanExecuteChanged?.Invoke(this, null);
        }

        public TestFrame TestFrame { get; set; }

        public bool CanExecute(object parameter)
        {
            return (parameter != null && parameter is TestDeclaration);
        }

        public void Execute(object parameter)
        {
            var testDeclaration = parameter as TestDeclaration;
            TestFrame.NavigateWithoutAnimation(testDeclaration.PageType, testDeclaration.Name);
        }
    }

    public static class FrameExtensions
    {
        public static void NavigateWithoutAnimation(this Frame frame, Type sourcePageType)
        {
            frame.Navigate(sourcePageType, new SuppressNavigationTransitionInfo());
        }

        public static void NavigateWithoutAnimation(this Frame frame, Type sourcePageType, object parameter)
        {
            frame.Navigate(sourcePageType, parameter, new SuppressNavigationTransitionInfo());
        }
    }
}
