using Interactions;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using System;
using System.Threading.Tasks;
using Windows.System;

namespace Interactions
{
    public interface IDeviceInteractionModel
    {
        Func<Task> BuildPrepareStateAction();
        Func<Task> BuildInvokeAction();
        Func<Task> BuildGoToBeginningAction();
        Func<Task> BuildGoToEndAction();
        Func<Task> BuildScrollLineUpAction();
        Func<Task> BuildScrollLineDownAction();
        Func<Task> BuildScrollPageUpAction();
        Func<Task> BuildScrollPageDownAction();
        Func<Task> BuildScrollLineLeftAction();
        Func<Task> BuildScrollLineRightAction();
        Func<Task> BuildScrollPageLeftAction();
        Func<Task> BuildScrollPageRightAction();
    }

    public interface IControlInteractionModel
    {
        Func<Task> BuildPrepareStateAction();
        Func<Task> BuildInvokeAction();
        Func<Task> BuildGoToBeginningAction();
        Func<Task> BuildGoToEndAction();
        Func<Task> BuildScrollLineBackAction();
        Func<Task> BuildScrollLineForwardAction();
        Func<Task> BuildScrollPageBackAction();
        Func<Task> BuildScrollPageForwardAction();
    }

    public class KeyboardInteractionModel : IDeviceInteractionModel
    {
        Control control;

        public KeyboardInteractionModel(Control control)
        {
            this.control = control;
        }

        public Func<Task> BuildPrepareStateAction()
        {
            return async () =>
            {
                await Task.Delay(1000);
                control.Focus(FocusState.Keyboard);
            };
        }

        public Func<Task> BuildInvokeAction()
        {
            return () => Keyboard.PressKeys(VirtualKey.Space);
        }

        public Func<Task> BuildGoToBeginningAction()
        {
            return () => Keyboard.PressKeys(VirtualKey.Home);
        }

        public Func<Task> BuildGoToEndAction()
        {
            return () => Keyboard.PressKeys(VirtualKey.End);
        }

        public Func<Task> BuildScrollLineDownAction()
        {
            return () => Keyboard.PressKeys(VirtualKey.Down);
        }

        public Func<Task> BuildScrollLineUpAction()
        {
            return () => Keyboard.PressKeys(VirtualKey.Up);
        }

        public Func<Task> BuildScrollPageDownAction()
        {
            return () => Keyboard.PressKeys(VirtualKey.PageDown);
        }

        public Func<Task> BuildScrollPageUpAction()
        {
            return () => Keyboard.PressKeys(VirtualKey.PageUp);
        }

        public Func<Task> BuildScrollLineLeftAction()
        {
            return () => Keyboard.PressKeys(VirtualKey.Left);
        }

        public Func<Task> BuildScrollLineRightAction()
        {
            return () => Keyboard.PressKeys(VirtualKey.Right);
        }

        public Func<Task> BuildScrollPageLeftAction()
        {
            return null;
        }

        public Func<Task> BuildScrollPageRightAction()
        {
            return null;
        }
    }

    public class MouseInteractionModel : IDeviceInteractionModel
    {
        Control control;

        public MouseInteractionModel(Control control)
        {
            this.control = control;
        }

        public Func<Task> BuildPrepareStateAction()
        {
            return async () =>
            {
                await Task.Delay(1000);

                control.Focus(FocusState.Pointer);

                await Mouse.MovePointer(Coord.ElementToScreen(control, control.ActualSize().Top().Left()),
                                        Coord.ElementToScreen(control, control.ActualSize().VCenter().HCenter()),
                                        400.0);
            };
        }

        public Func<Task> BuildInvokeAction()
        {
            return () => Mouse.Click(MouseButton.Left);
        }

        public Func<Task> BuildGoToBeginningAction()
        {
            return async () =>
            {
                control.Focus(FocusState.Keyboard);
                await Keyboard.PressKeys(VirtualKey.Home);
                control.Focus(FocusState.Pointer);
            };
        }

        public Func<Task> BuildGoToEndAction()
        {
            return async () =>
            {
                control.Focus(FocusState.Keyboard);
                await Keyboard.PressKeys(VirtualKey.End);
                control.Focus(FocusState.Pointer);
            };
        }

        public Func<Task> BuildScrollLineDownAction()
        {
            return null;
        }

        public Func<Task> BuildScrollLineUpAction()
        {
            return null;
        }

        public Func<Task> BuildScrollPageDownAction()
        {
            return () => Mouse.ScrollWheel(-10);
        }

        public Func<Task> BuildScrollPageUpAction()
        {
            return () => Mouse.ScrollWheel(10);
        }

        public Func<Task> BuildScrollLineLeftAction()
        {
            return null;
        }

        public Func<Task> BuildScrollLineRightAction()
        {
            return null;
        }

        public Func<Task> BuildScrollPageLeftAction()
        {
            return null;
        }

        public Func<Task> BuildScrollPageRightAction()
        {
            return null;
        }
    }

    public class TouchInteractionModel : IDeviceInteractionModel
    {
        Control control;

        public TouchInteractionModel(Control control)
        {
            this.control = control;
        }

        public Func<Task> BuildPrepareStateAction()
        {
            return async () =>
            {
                await Task.Delay(1000);

                control.Focus(FocusState.Pointer);
            };
        }

        public Func<Task> BuildInvokeAction()
        {
            return () => Touch.Tap(Coord.ElementToScreen(control, control.ActualSize().VCenter().HCenter()));
        }

        public Func<Task> BuildGoToBeginningAction()
        {
            return async () =>
            {
                await Task.Delay(1000);
                control.Focus(FocusState.Keyboard);
                await Keyboard.PressKeys(VirtualKey.Home);
                control.Focus(FocusState.Pointer);
            };
        }

        public Func<Task> BuildGoToEndAction()
        {
            return async () =>
            {
                await Task.Delay(1000);
                control.Focus(FocusState.Keyboard);
                await Keyboard.PressKeys(VirtualKey.End);
                control.Focus(FocusState.Pointer);
            };
        }

        private int edgeOffset = 6;

        public Func<Task> BuildScrollLineDownAction()
        {
            return () => Touch.Pan(Coord.ElementToScreen(control, control.ActualSize().Bottom().VOffset(-edgeOffset).HCenter()),
                                   Coord.ElementToScreen(control, control.ActualSize().VCenter().HCenter()),
                                   500.0);
        }

        public Func<Task> BuildScrollLineUpAction()
        {
            return () => Touch.Pan(Coord.ElementToScreen(control, control.ActualSize().Top().VOffset(edgeOffset).HCenter()),
                                   Coord.ElementToScreen(control, control.ActualSize().VCenter().HCenter()),
                                   500.0);
        }

        public Func<Task> BuildScrollPageDownAction()
        {
            return () => Touch.Pan(Coord.ElementToScreen(control, control.ActualSize().Bottom().VOffset(-edgeOffset).HCenter()),
                                   Coord.ElementToScreen(control, control.ActualSize().Top().VOffset(edgeOffset).HCenter()),
                                   3000.0);
        }

        public Func<Task> BuildScrollPageUpAction()
        {
            return () => Touch.Pan(Coord.ElementToScreen(control, control.ActualSize().Top().VOffset(edgeOffset).HCenter()),
                                   Coord.ElementToScreen(control, control.ActualSize().Bottom().VOffset(-edgeOffset).HCenter()),
                                   3000.0);
        }

        public Func<Task> BuildScrollLineLeftAction()
        {
            return () => Touch.Pan(Coord.ElementToScreen(control, control.ActualSize().Left().HOffset(edgeOffset).VCenter()),
                                   Coord.ElementToScreen(control, control.ActualSize().HCenter().VCenter()),
                                   500.0);
        }

        public Func<Task> BuildScrollLineRightAction()
        {
            return () => Touch.Pan(Coord.ElementToScreen(control, control.ActualSize().Right().HOffset(-edgeOffset).VCenter()),
                                   Coord.ElementToScreen(control, control.ActualSize().HCenter().VCenter()),
                                   500.0);
        }

        public Func<Task> BuildScrollPageLeftAction()
        {
            return () => Touch.Pan(Coord.ElementToScreen(control, control.ActualSize().Left().HOffset(edgeOffset).VCenter()),
                                   Coord.ElementToScreen(control, control.ActualSize().Right().HOffset(-edgeOffset).VCenter()),
                                   3000.0);
        }

        public Func<Task> BuildScrollPageRightAction()
        {
            return () => Touch.Pan(Coord.ElementToScreen(control, control.ActualSize().Right().HOffset(-edgeOffset).VCenter()),
                                   Coord.ElementToScreen(control, control.ActualSize().Left().HOffset(edgeOffset).VCenter()),
                                   3000.0);
        }
    }

    public class GamepadInteractionModel : IDeviceInteractionModel
    {
        Control control;

        public GamepadInteractionModel(Control control)
        {
            this.control = control;
        }

        public Func<Task> BuildPrepareStateAction()
        {
            return async () =>
            {
                await Task.Delay(1000);
                control.Focus(FocusState.Keyboard);
            };
        }

        public Func<Task> BuildInvokeAction()
        {
            return () => Keyboard.PressKeys(VirtualKey.GamepadA);
        }

        public Func<Task> BuildGoToBeginningAction()
        {
            return () => Keyboard.PressKeys(VirtualKey.Home);
        }

        public Func<Task> BuildGoToEndAction()
        {
            return () => Keyboard.PressKeys(VirtualKey.End);
        }

        public Func<Task> BuildScrollLineDownAction()
        {
            return () => Keyboard.PressKeys(VirtualKey.GamepadDPadDown);
        }

        public Func<Task> BuildScrollLineUpAction()
        {
            return () => Keyboard.PressKeys(VirtualKey.GamepadDPadUp);
        }

        public Func<Task> BuildScrollPageDownAction()
        {
            return () => Keyboard.PressKeys(VirtualKey.GamepadRightTrigger);
        }

        public Func<Task> BuildScrollPageUpAction()
        {
            return () => Keyboard.PressKeys(VirtualKey.GamepadLeftTrigger);
        }

        public Func<Task> BuildScrollLineLeftAction()
        {
            return () => Keyboard.PressKeys(VirtualKey.GamepadDPadLeft);
        }

        public Func<Task> BuildScrollLineRightAction()
        {
            return () => Keyboard.PressKeys(VirtualKey.GamepadDPadRight);
        }

        public Func<Task> BuildScrollPageLeftAction()
        {
            return () => Keyboard.PressKeys(VirtualKey.GamepadLeftShoulder);
        }

        public Func<Task> BuildScrollPageRightAction()
        {
            return () => Keyboard.PressKeys(VirtualKey.GamepadRightShoulder);
        }
    }

    public class VListViewInteractionModel : IControlInteractionModel
    {
        readonly IDeviceInteractionModel deviceInteractionModel;

        public VListViewInteractionModel(IDeviceInteractionModel deviceInteractionModel)
        {
            this.deviceInteractionModel = deviceInteractionModel;
        }

        public Func<Task> BuildPrepareStateAction()
        {
            return deviceInteractionModel.BuildPrepareStateAction();
        }

        public Func<Task> BuildGoToBeginningAction()
        {
            return deviceInteractionModel.BuildGoToBeginningAction();
        }

        public Func<Task> BuildGoToEndAction()
        {
            return deviceInteractionModel.BuildGoToEndAction();
        }

        public Func<Task> BuildInvokeAction()
        {
            return deviceInteractionModel.BuildInvokeAction();
        }

        public Func<Task> BuildScrollLineForwardAction()
        {
            return deviceInteractionModel.BuildScrollLineDownAction();
        }

        public Func<Task> BuildScrollLineBackAction()
        {
            return deviceInteractionModel.BuildScrollLineUpAction();
        }

        public Func<Task> BuildScrollPageForwardAction()
        {
            return deviceInteractionModel.BuildScrollPageDownAction();
        }

        public Func<Task> BuildScrollPageBackAction()
        {
            return deviceInteractionModel.BuildScrollPageUpAction();
        }
    }

    public class HListViewInteractionModel : IControlInteractionModel
    {
        readonly IDeviceInteractionModel deviceInteractionModel;

        public HListViewInteractionModel(IDeviceInteractionModel deviceInteractionModel)
        {
            this.deviceInteractionModel = deviceInteractionModel;
        }

        public Func<Task> BuildPrepareStateAction()
        {
            return deviceInteractionModel.BuildPrepareStateAction();
        }

        public Func<Task> BuildGoToBeginningAction()
        {
            return deviceInteractionModel.BuildGoToBeginningAction();
        }

        public Func<Task> BuildGoToEndAction()
        {
            return deviceInteractionModel.BuildGoToEndAction();
        }

        public Func<Task> BuildInvokeAction()
        {
            return deviceInteractionModel.BuildInvokeAction();
        }

        public Func<Task> BuildScrollLineForwardAction()
        {
            if (deviceInteractionModel is MouseInteractionModel)
            {
                return deviceInteractionModel.BuildScrollLineUpAction();
            }
            else
            {
                return deviceInteractionModel.BuildScrollLineRightAction();
            }
        }

        public Func<Task> BuildScrollLineBackAction()
        {
            if (deviceInteractionModel is MouseInteractionModel)
            {
                return deviceInteractionModel.BuildScrollLineDownAction();
            }
            else
            {
                return deviceInteractionModel.BuildScrollLineLeftAction();
            }
        }

        public Func<Task> BuildScrollPageForwardAction()
        {
            if (deviceInteractionModel is TouchInteractionModel ||
                deviceInteractionModel is GamepadInteractionModel)
            {
                return deviceInteractionModel.BuildScrollPageRightAction();
            }
            else
            {
                return deviceInteractionModel.BuildScrollPageDownAction();
            }
        }

        public Func<Task> BuildScrollPageBackAction()
        {
            if (deviceInteractionModel is TouchInteractionModel ||
                deviceInteractionModel is GamepadInteractionModel)
            {
                return deviceInteractionModel.BuildScrollPageLeftAction();
            }
            else
            {
                return deviceInteractionModel.BuildScrollPageUpAction();
            }
        }
    }

    public class VItemsRepeaterInteractionModel : IControlInteractionModel
    {
        readonly IDeviceInteractionModel deviceInteractionModel;

        public VItemsRepeaterInteractionModel(IDeviceInteractionModel deviceInteractionModel)
        {
            this.deviceInteractionModel = deviceInteractionModel;
        }

        public Func<Task> BuildPrepareStateAction()
        {
            return deviceInteractionModel.BuildPrepareStateAction();
        }

        public Func<Task> BuildGoToBeginningAction()
        {
            return deviceInteractionModel.BuildGoToBeginningAction();
        }

        public Func<Task> BuildGoToEndAction()
        {
            return deviceInteractionModel.BuildGoToEndAction();
        }

        public Func<Task> BuildInvokeAction()
        {
            return null;
        }

        public Func<Task> BuildScrollLineForwardAction()
        {
            return deviceInteractionModel.BuildScrollLineDownAction();
        }

        public Func<Task> BuildScrollLineBackAction()
        {
            return deviceInteractionModel.BuildScrollLineUpAction();
        }

        public Func<Task> BuildScrollPageForwardAction()
        {
            return deviceInteractionModel.BuildScrollPageDownAction();
        }

        public Func<Task> BuildScrollPageBackAction()
        {
            return deviceInteractionModel.BuildScrollPageUpAction();
        }
    }

    public class HItemsRepeaterInteractionModel : IControlInteractionModel
    {
        readonly IDeviceInteractionModel deviceInteractionModel;

        public HItemsRepeaterInteractionModel(IDeviceInteractionModel deviceInteractionModel)
        {
            this.deviceInteractionModel = deviceInteractionModel;
        }

        public Func<Task> BuildPrepareStateAction()
        {
            return deviceInteractionModel.BuildPrepareStateAction();
        }

        public Func<Task> BuildGoToBeginningAction()
        {
            return deviceInteractionModel.BuildGoToBeginningAction();
        }

        public Func<Task> BuildGoToEndAction()
        {
            return deviceInteractionModel.BuildGoToEndAction();
        }

        public Func<Task> BuildInvokeAction()
        {
            return null;
        }

        public Func<Task> BuildScrollLineForwardAction()
        {
            if (deviceInteractionModel is MouseInteractionModel)
            {
                return deviceInteractionModel.BuildScrollLineUpAction();
            }
            else
            {
                return deviceInteractionModel.BuildScrollLineRightAction();
            }
        }

        public Func<Task> BuildScrollLineBackAction()
        {
            if (deviceInteractionModel is MouseInteractionModel)
            {
                return deviceInteractionModel.BuildScrollLineDownAction();
            }
            else
            {
                return deviceInteractionModel.BuildScrollLineLeftAction();
            }
        }

        public Func<Task> BuildScrollPageForwardAction()
        {
            if (deviceInteractionModel is TouchInteractionModel ||
                deviceInteractionModel is GamepadInteractionModel)
            {
                return deviceInteractionModel.BuildScrollPageRightAction();
            }
            else
            {
                return deviceInteractionModel.BuildScrollPageDownAction();
            }
        }

        public Func<Task> BuildScrollPageBackAction()
        {
            if (deviceInteractionModel is TouchInteractionModel ||
                deviceInteractionModel is GamepadInteractionModel)
            {
                return deviceInteractionModel.BuildScrollPageLeftAction();
            }
            else
            {
                return deviceInteractionModel.BuildScrollPageUpAction();
            }
        }
    }
}
