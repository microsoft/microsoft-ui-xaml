using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Threading.Tasks;
using Windows.Foundation;
using Windows.Gaming.Input;
using Windows.System;
using Windows.UI.Input.Preview.Injection;

namespace Interactions
{
    public static class Common
    {
        public static async Task Repeat(uint count, uint delayMs, Func<Task> taskFactory)
        {
            if (taskFactory != null)
            {
                for (uint i = 0; i < count; ++i)
                {
                    await taskFactory();
                    await Task.Delay((int)delayMs);
                }
            }
        }
    }

    public struct PointT
    {
        public Point Position { get; set; }
        public uint TimeDeltaMs { get; set; }
    }

    static class Impl
    {
        public static Rect GetScreenBoundsInDips()
        {
            return new Rect(new Point(), Coord.ScreenInDips());
        }

        public static void ValidatePointsOnScreen(params Point[] pointsInScreenDips)
        {
            Rect screenBoundsInDips = GetScreenBoundsInDips();

            foreach (var point in pointsInScreenDips)
            {
                if (!screenBoundsInDips.Contains(point))
                {
                    throw new ArgumentOutOfRangeException();
                }
            }
        }

        public static InputInjector InputInjector
        {
            get
            {
                if (_inputInjector == null)
                {
                    _inputInjector = InputInjector.TryCreate();
                }
                return _inputInjector;
            }
        }

        static InputInjector _inputInjector;
    }

    public static class Touch
    {
        static Touch()
        {
            Impl.InputInjector.InitializeTouchInjection(InjectedInputVisualizationMode.Default);
        }

        public static async Task Flick(Point startPointInScreenDips, Vector2 velocityDipsPerSec, uint activeTimeInMs)
        {
            Impl.ValidatePointsOnScreen(startPointInScreenDips);

            await DownMoveUpHelper(
                Motion.GenerateLinear(startPointInScreenDips, velocityDipsPerSec, activeTimeInMs));
        }

        public static async Task Pan(Point startPointInScreenDips, Point endPointInScreenDips, double speedDipsPerSec)
        {
            Impl.ValidatePointsOnScreen(startPointInScreenDips, endPointInScreenDips);

            await DownMoveUpHelper(
                Motion.GenerateLinear(startPointInScreenDips, endPointInScreenDips, speedDipsPerSec));
        }

        private static async Task DownMoveUpHelper(List<PointT> motionInScreenDips)
        {
            uint pointerId = GetPointerId();

            Rect screenBoundsInDips = Impl.GetScreenBoundsInDips();

            foreach (var pointInScreenDips in motionInScreenDips)
            {
                if (screenBoundsInDips.Contains(pointInScreenDips.Position))
                {
                    Impl.InputInjector.InjectTouchInput(
                        CreateInjectedInputPointerInfoList(
                            pointerId,
                            InjectedInputPointerOptions.PointerDown | InjectedInputPointerOptions.InContact | InjectedInputPointerOptions.InRange,
                            Coord.DipsToRaw(pointInScreenDips.Position)));

                    await Task.Delay((int)pointInScreenDips.TimeDeltaMs);
                }
            }

            Impl.InputInjector.InjectTouchInput(CreateInjectedInputPointerInfoList(pointerId, InjectedInputPointerOptions.PointerUp, null));
        }

        public static async Task Tap(Point pointInScreenDips)
        {
            Impl.ValidatePointsOnScreen(pointInScreenDips);

            uint pointerId = GetPointerId();

            Impl.InputInjector.InjectTouchInput(
                CreateInjectedInputPointerInfoList(
                    pointerId,
                    InjectedInputPointerOptions.PointerDown | InjectedInputPointerOptions.InContact | InjectedInputPointerOptions.New,
                    Coord.DipsToRaw(pointInScreenDips)));

            await Task.Delay(100);

            Impl.InputInjector.InjectTouchInput(CreateInjectedInputPointerInfoList(pointerId, InjectedInputPointerOptions.PointerUp, null));
        }

        private static InjectedInputPointerInfo CreateInjectedInputPointerInfo(uint pointerId, InjectedInputPointerOptions options, Point? pointInRaw)
        {
            return new InjectedInputPointerInfo
            {
                PointerId = pointerId,
                PointerOptions = options,
                PixelLocation = new InjectedInputPoint
                {
                    PositionX = (int)(pointInRaw?.X ?? 0),
                    PositionY = (int)(pointInRaw?.Y ?? 0)
                }
            };
        }

        private static List<InjectedInputTouchInfo> CreateInjectedInputPointerInfoList(uint pointerId, InjectedInputPointerOptions options, Point? pointInRaw)
        {
            InjectedInputRectangle contact = new InjectedInputRectangle();

            if (pointInRaw.HasValue)
            {
                contact.Left = (int)pointInRaw.Value.X - 4;
                contact.Right = (int)pointInRaw.Value.X + 4;
                contact.Top = (int)pointInRaw.Value.Y - 4;
                contact.Bottom = (int)pointInRaw.Value.Y - 4;
            }

            return new List<InjectedInputTouchInfo>
            {
                new InjectedInputTouchInfo
                {
                    Contact = contact,
                    PointerInfo = CreateInjectedInputPointerInfo(pointerId, options, pointInRaw),
                    Pressure = 1.0,
                    TouchParameters = InjectedInputTouchParameters.Pressure | InjectedInputTouchParameters.Contact
                }
            };
        }

        private static uint GetPointerId()
        {
            return lastPointerId++;
        }

        private static uint lastPointerId = 100000;
    }

    public enum MouseButton
    {
        Left,
        Middle,
        Right
    };

    public static class Mouse
    {
        private static InjectedInputMouseOptions[] leftButtonOptions = { InjectedInputMouseOptions.LeftDown, InjectedInputMouseOptions.LeftUp };
        private static InjectedInputMouseOptions[] middleButtonOptions = { InjectedInputMouseOptions.MiddleDown, InjectedInputMouseOptions.MiddleUp };
        private static InjectedInputMouseOptions[] rightButtonOptions = { InjectedInputMouseOptions.RightDown, InjectedInputMouseOptions.RightUp };

        public static async Task Click(MouseButton button)
        {
            InjectedInputMouseOptions[] options = null;

            switch (button)
            {
                case MouseButton.Left:
                    options = leftButtonOptions;
                    break;

                case MouseButton.Middle:
                    options = middleButtonOptions;
                    break;

                case MouseButton.Right:
                    options = rightButtonOptions;
                    break;
            }

            Impl.InputInjector.InjectMouseInput(
                CreateInjectedInputMouseInfoList(options[0], 0, null));

            await Task.Delay(100);

            Impl.InputInjector.InjectMouseInput(
                CreateInjectedInputMouseInfoList(options[1], 0, null));
        }

        public static async Task ScrollWheel(int clicks)
        {
            const int wheelConst = 120;

            Impl.InputInjector.InjectMouseInput(
                CreateInjectedInputMouseInfoList(
                    InjectedInputMouseOptions.Wheel,
                    (uint)(clicks * wheelConst),
                    null));

            await Task.Delay(50);
        }

        public static async Task MovePointer(Point startPointInScreenDips, Point endPointInScreenDips, double speedDipsPerSec)
        {
            Impl.ValidatePointsOnScreen(startPointInScreenDips, endPointInScreenDips);

            Rect screenBoundsInDips = Impl.GetScreenBoundsInDips();

            var motionInScreenDips = Motion.GenerateLinear(startPointInScreenDips, endPointInScreenDips, speedDipsPerSec);

            foreach (var pointInScreenDips in motionInScreenDips)
            {
                if (screenBoundsInDips.Contains(pointInScreenDips.Position))
                {
                    Impl.InputInjector.InjectMouseInput(
                        CreateInjectedInputMouseInfoList(
                            InjectedInputMouseOptions.Absolute | InjectedInputMouseOptions.Move,
                            0,
                            Coord.MouseScreenToNormalized(pointInScreenDips.Position)));

                    await Task.Delay((int)pointInScreenDips.TimeDeltaMs);
                }
            }
        }

        private static List<InjectedInputMouseInfo> CreateInjectedInputMouseInfoList(InjectedInputMouseOptions options, uint mouseData, Point? pointInNormalized)
        {
            return new List<InjectedInputMouseInfo>
            {
                new InjectedInputMouseInfo
                {
                    MouseOptions = options,
                    MouseData = mouseData,
                    DeltaX = (int)(pointInNormalized?.X ?? 0),
                    DeltaY = (int)(pointInNormalized?.Y ?? 0)
                }
            };
        }
    }

    public enum Thumbstick
    {
        Left,
        Right
    }

    public enum TriggerButton
    {
        Left,
        Right
    }

    public static class Gamepad
    {
        static Gamepad()
        {
            Impl.InputInjector.InitializeGamepadInjection();
        }

        public static async Task PressButton(GamepadButtons buttons)
        {
            Impl.InputInjector.InjectGamepadInput(
                new InjectedInputGamepadInfo
                {
                    Buttons = buttons
                });

            await Task.Delay(200);

            Impl.InputInjector.InjectGamepadInput(
                new InjectedInputGamepadInfo
                {
                    Buttons = GamepadButtons.None
                });
        }

        public static async Task PressTrigger(TriggerButton triggerButton, double amount, uint durationMs)
        {
            if (amount < 0.0 || amount > 1.0)
            {
                throw new ArgumentOutOfRangeException();
            }

            double? leftTriggerAmount = null;
            double? rightTriggerAmount = null;

            switch (triggerButton)
            {
                case TriggerButton.Left:
                    leftTriggerAmount = amount;
                    break;

                case TriggerButton.Right:
                    rightTriggerAmount = amount;
                    break;
            }

            Impl.InputInjector.InjectGamepadInput(
                new InjectedInputGamepadInfo
                {
                    LeftTrigger = leftTriggerAmount ?? 0.0,
                    RightTrigger = rightTriggerAmount ?? 0.0
                });

            await Task.Delay((int)durationMs);

            Impl.InputInjector.InjectGamepadInput(
                new InjectedInputGamepadInfo
                {
                    LeftTrigger = 0.0,
                    RightTrigger = 0.0
                });
        }

        public static async Task MoveThumbstick(Thumbstick thumbStick, Vector2 direction, uint durationMs)
        {
            if (Math.Abs(direction.X) > 1.0 ||
                Math.Abs(direction.Y) > 1.0)
            {
                throw new ArgumentOutOfRangeException();
            }

            Vector2? leftThumbstickDirection = null;
            Vector2? rightThumbstickDirection = null;

            switch (thumbStick)
            {
                case Thumbstick.Left:
                    leftThumbstickDirection = direction;
                    break;

                case Thumbstick.Right:
                    rightThumbstickDirection = direction;
                    break;
            }

            Impl.InputInjector.InjectGamepadInput(
                new InjectedInputGamepadInfo
                {
                    LeftThumbstickX = leftThumbstickDirection?.X ?? 0.0,
                    LeftThumbstickY = leftThumbstickDirection?.Y ?? 0.0,
                    RightThumbstickX = rightThumbstickDirection?.X ?? 0.0,
                    RightThumbstickY = rightThumbstickDirection?.Y ?? 0.0
                });

            await Task.Delay((int)durationMs);

            Impl.InputInjector.InjectGamepadInput(
                new InjectedInputGamepadInfo
                {
                    LeftThumbstickX = 0.0,
                    LeftThumbstickY = 0.0,
                    RightThumbstickX = 0.0,
                    RightThumbstickY = 0.0
                });
        }
    }

    public static class Keyboard
    {
        public static async Task PressKeys(params VirtualKey[] virtualKeys)
        {
            Impl.InputInjector.InjectKeyboardInput(
                CreateInjectedInputKeyboardInfo(
                    InjectedInputKeyOptions.None,
                    virtualKeys));

            await Task.Delay(100);

            Impl.InputInjector.InjectKeyboardInput(
                CreateInjectedInputKeyboardInfo(
                    InjectedInputKeyOptions.KeyUp,
                    virtualKeys));
        }

        private static List<InjectedInputKeyboardInfo> CreateInjectedInputKeyboardInfo(InjectedInputKeyOptions options, params VirtualKey[] virtualKeys)
        {
            return new List<InjectedInputKeyboardInfo>(virtualKeys.Select(
                x => new InjectedInputKeyboardInfo
                {
                    KeyOptions = options,
                    VirtualKey = (ushort)x
                }));
        }
    }
}
