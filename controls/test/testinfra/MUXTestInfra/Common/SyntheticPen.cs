// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.ComponentModel;
using System.Drawing;
using System.Runtime.InteropServices;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common
{
    // The legacy input wrapper supports complete pen gestures, but not pen hover
    // or interleaving an independent pointer while the pen is down.
    public sealed class SyntheticPen : IDisposable
    {
        private const uint PenType = 3;
        private const uint InRange = 0x2;
        private const uint InContact = 0x4;
        private const uint FirstButton = 0x10;
        private const uint Canceled = 0x8000;
        private const uint Down = 0x10000;
        private const uint Update = 0x20000;
        private const uint Up = 0x40000;
        private IntPtr device;
        private Point position;
        private bool pressed;

        public SyntheticPen()
        {
            device = CreateSyntheticPointerDevice(PenType, 1, 3 /* POINTER_FEEDBACK_NONE */);
            if (device == IntPtr.Zero)
            {
                throw new Win32Exception(Marshal.GetLastWin32Error(), "CreateSyntheticPointerDevice(PT_PEN)");
            }
        }

        public void Hover(Point point)
        {
            if (pressed) { throw new InvalidOperationException("Release the pen before hovering."); }
            Inject(point, InRange | Update);
        }

        public void Press(Point point)
        {
            if (pressed) { throw new InvalidOperationException("The pen is already pressed."); }
            Inject(point, InRange | InContact | FirstButton | Down);
            pressed = true;
        }

        public void Move(Point point)
        {
            if (!pressed) { throw new InvalidOperationException("Press the pen before moving in contact."); }
            Inject(point, InRange | InContact | FirstButton | Update);
        }

        public void Release()
        {
            if (pressed)
            {
                Inject(position, InRange | Up);
                pressed = false;
            }
        }

        public void Cancel()
        {
            if (!pressed) { throw new InvalidOperationException("There is no pen contact to cancel."); }
            Inject(position, Up | Canceled);
            pressed = false;
        }

        private void Inject(Point point, uint flags)
        {
            if (device == IntPtr.Zero) { throw new ObjectDisposedException(nameof(SyntheticPen)); }
            var input = new PointerTypeInfo
            {
                Type = PenType,
                Info = new PointerUnion
                {
                    Pen = new PenInfo
                    {
                        Pointer = new PointerInfo
                        {
                            Type = PenType,
                            Id = 1,
                            Flags = flags,
                            PixelLocation = point
                        },
                        Mask = 1 /* PEN_MASK_PRESSURE */,
                        Pressure = (flags & InContact) != 0 ? 512u : 0u
                    }
                }
            };
            if (!InjectSyntheticPointerInput(device, new[] { input }, 1))
            {
                throw new Win32Exception(Marshal.GetLastWin32Error(), $"InjectSyntheticPointerInput flags=0x{flags:x}");
            }
            position = point;
            Wait.ForIdle();
        }

        public void Dispose()
        {
            if (device != IntPtr.Zero)
            {
                // Destroying our device also cancels an outstanding contact after a failed assertion.
                DestroySyntheticPointerDevice(device);
                device = IntPtr.Zero;
            }
        }

        [DllImport("user32.dll", SetLastError = true)]
        private static extern IntPtr CreateSyntheticPointerDevice(uint type, uint maxCount, uint feedback);

        [DllImport("user32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool InjectSyntheticPointerInput(IntPtr device, [In] PointerTypeInfo[] input, uint count);

        [DllImport("user32.dll")]
        private static extern void DestroySyntheticPointerDevice(IntPtr device);

        // Native winuser.h layout. The union includes touch solely to preserve its
        // native size/alignment for both 32-bit and 64-bit test hosts.
        [StructLayout(LayoutKind.Sequential)]
        private struct PointerTypeInfo { public uint Type; public PointerUnion Info; }

        [StructLayout(LayoutKind.Explicit)]
        private struct PointerUnion
        {
            [FieldOffset(0)] public PenInfo Pen;
            [FieldOffset(0)] public TouchInfo Touch;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct PointerInfo
        {
            public uint Type, Id, FrameId, Flags;
            public IntPtr SourceDevice, Target;
            public Point PixelLocation, HimetricLocation, PixelLocationRaw, HimetricLocationRaw;
            public uint Time, HistoryCount;
            public int InputData;
            public uint KeyStates;
            public ulong PerformanceCount;
            public uint ButtonChangeType;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct PenInfo
        {
            public PointerInfo Pointer;
            public uint Flags, Mask, Pressure, Rotation;
            public int TiltX, TiltY;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct TouchInfo
        {
            public PointerInfo Pointer;
            public uint Flags, Mask;
            public int Left, Top, Right, Bottom, RawLeft, RawTop, RawRight, RawBottom;
            public uint Orientation, Pressure;
        }
    }
}
