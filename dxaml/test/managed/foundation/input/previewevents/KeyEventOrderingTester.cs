// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Text;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Private.Infrastructure;
using Microsoft.UI.Xaml.Tests.Common;

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Input;
using Windows.Foundation;
using Microsoft.UI.Xaml.Markup;

namespace Microsoft.UI.Xaml.Tests.Common
{
    public sealed class KeyEventOrderingTester : IDisposable
    {
        public StringBuilder EventOrder;
        private readonly FrameworkElement[] ElementsToTest;
        bool RegisterCharacterReceived;

        public KeyEventOrderingTester(FrameworkElement[] elements, StringBuilder eventOrder)
            : this(elements, eventOrder, false /*characterReceivedEventToo*/)
        {    
        }

        public KeyEventOrderingTester(FrameworkElement[] elements, StringBuilder eventOrder, bool characterReceivedEventToo)
        {
            this.EventOrder = eventOrder;
            this.ElementsToTest = elements;
            this.RegisterCharacterReceived = characterReceivedEventToo;

            FrameworkElementPreviewKeyDownHandler = new KeyEventHandler((source, e) =>
            {
                FrameworkElement sourceAsFrameworkElement = (FrameworkElement)source;
                EventOrder.Append("[" + sourceAsFrameworkElement.Name + "PreviewKeyDown:" + e.OriginalKey.ToString() + GetHandled(e.Handled) + "]");
            });

            FrameworkElementPreviewKeyUpHandler = new KeyEventHandler((source, e) =>
            {
                FrameworkElement sourceAsFrameworkElement = (FrameworkElement)source;
                EventOrder.Append("[" + sourceAsFrameworkElement.Name + "PreviewKeyUp:" + e.OriginalKey.ToString() + GetHandled(e.Handled) + "]");
            });

            FrameworkElementKeyDownHandler = new KeyEventHandler((source, e) =>
            {
                FrameworkElement sourceAsFrameworkElement = (FrameworkElement)source;
                EventOrder.Append("[" + sourceAsFrameworkElement.Name + "KeyDown:" + e.OriginalKey.ToString() + GetHandled(e.Handled) + "]");
            });

            FrameworkElementKeyUpHandler = new KeyEventHandler((source, e) =>
            {
                FrameworkElement sourceAsFrameworkElement = (FrameworkElement)source;
                EventOrder.Append("[" + sourceAsFrameworkElement.Name + "KeyUp:" + e.OriginalKey.ToString() + GetHandled(e.Handled) + "]");
            });

            FrameworkElementCharacterReceivedHandler = new TypedEventHandler<UIElement, CharacterReceivedRoutedEventArgs>((source, e) =>
            {
                FrameworkElement sourceAsFrameworkElement = (FrameworkElement)source;
                EventOrder.Append("[" + sourceAsFrameworkElement.Name + "CharacterReceived:" + e.Character.ToString() + GetHandled(e.Handled) + "]");
            });

            UIExecutor.Execute(() =>
            {
                for (int i = 0; i < ElementsToTest.Length; i++)
                {
                    this.AddEventsToFrameworkElement(ElementsToTest[i]);
                }
            });
         }


        private void AddEventsToFrameworkElement(FrameworkElement element)
        {
            element.AddHandler(
                UIElement.KeyDownEvent,
                FrameworkElementKeyDownHandler,
                true /*handledEventsToo*/
                );
            element.AddHandler(
                UIElement.KeyUpEvent,
                FrameworkElementKeyUpHandler,
                true /*handledEventsToo*/
                );
            element.AddHandler(
                UIElement.PreviewKeyDownEvent,
                FrameworkElementPreviewKeyDownHandler,
                true /*handledEventsToo*/
                );
            element.AddHandler(
                UIElement.PreviewKeyUpEvent,
                FrameworkElementPreviewKeyUpHandler,
                true /*handledEventsToo*/
                );

            if(RegisterCharacterReceived)
            {
                element.AddHandler(
                  UIElement.CharacterReceivedEvent,
                  FrameworkElementCharacterReceivedHandler,
                  true /*handledEventsToo*/
                  );
            }
        }

        private void RemoveEventsFromFrameworkElement(FrameworkElement element)
        {

            element.RemoveHandler(
                UIElement.PreviewKeyDownEvent,
                FrameworkElementPreviewKeyDownHandler);
            element.RemoveHandler(
                UIElement.PreviewKeyUpEvent,
                FrameworkElementPreviewKeyUpHandler);
            element.RemoveHandler(
                UIElement.KeyDownEvent,
                FrameworkElementKeyDownHandler);
            element.RemoveHandler(
                UIElement.KeyUpEvent,
                FrameworkElementKeyUpHandler);
            if (RegisterCharacterReceived)
            {
                element.RemoveHandler(
                  UIElement.CharacterReceivedEvent,
                  FrameworkElementCharacterReceivedHandler);
            }
        }

        private string GetHandled(bool handled)
        {
            if(handled)
            {
                return ":Handled";
            }
            return "";
        }

        KeyEventHandler FrameworkElementPreviewKeyDownHandler;
        KeyEventHandler FrameworkElementPreviewKeyUpHandler;
        KeyEventHandler FrameworkElementKeyDownHandler;
        KeyEventHandler FrameworkElementKeyUpHandler;
        TypedEventHandler<UIElement, CharacterReceivedRoutedEventArgs> FrameworkElementCharacterReceivedHandler;

        public void Dispose()
        {
            UIExecutor.Execute(() =>
            {
                for (int i = 0; i < ElementsToTest.Length; i++)
                {
                    this.RemoveEventsFromFrameworkElement(ElementsToTest[i]);
                }
            });
        }
    }
}