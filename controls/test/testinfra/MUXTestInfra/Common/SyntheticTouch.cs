// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.ComponentModel;
using System.Drawing;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common
{
    // The legacy input wrapper supports whole touch gestures only. This injects separate
    // press/hold/move/release steps and can keep two contacts down at once.
    public sealed class SyntheticTouch : IDisposable
    {
        private const uint TouchType = 2;
        private const uint InRange = 0x2;
        private const uint InContact = 0x4;
        private const uint Primary = 0x2000;
        private const uint Down = 0x10000;
        private const uint Update = 0x20000;
        private const uint Up = 0x40000;
        private readonly Point?[] contacts;
        private IntPtr device;

        public SyntheticTouch(int maxContacts = 2)
        {
            contacts = new Point?[maxContacts];
            device = CreateSyntheticPointerDevice(TouchType, (uint)maxContacts, 3 /* POINTER_FEEDBACK_NONE */);
            if (device == IntPtr.Zero)
            {
                throw new Win32Exception(Marshal.GetLastWin32Error(), "CreateSyntheticPointerDevice(PT_TOUCH)");
            }
        }

        public void Press(int contact, Point point)
        {
            if (contacts[contact].HasValue) { throw new InvalidOperationException("The contact is already down."); }
            contacts[contact] = point;
            InjectFrame(contact, InRange | InContact | Down);
            Wait.ForIdle();
        }

        public void Move(int contact, Point point)
        {
            if (!contacts[contact].HasValue) { throw new InvalidOperationException("Press the contact before moving it."); }
            contacts[contact] = point;
            InjectFrame(contact, InRange | InContact | Update);
        }

        // Keeps every contact still, re-reporting it so the system does not time it out.
        public void Hold(int milliseconds)
        {
            for (int elapsed = 0; elapsed < milliseconds; elapsed += 50)
            {
                InjectFrame(-1, 0);
                Thread.Sleep(50);
            }
        }

        public void Release(int contact)
        {
            if (contacts[contact].HasValue)
            {
                InjectFrame(contact, Up);
                contacts[contact] = null;
                Wait.ForIdle();
            }
        }

        // Every frame reports all contacts that are down; only 'contact' changes state.
        private void InjectFrame(int contact, uint flags)
        {
            if (device == IntPtr.Zero) { throw new ObjectDisposedException(nameof(SyntheticTouch)); }
            var frame = Enumerable.Range(0, contacts.Length)
                .Where(i => contacts[i].HasValue)
                .Select(i => new PointerTypeInfo
                {
                    Type = TouchType,
                    Info = new PointerUnion
                    {
                        Touch = new TouchInfo
                        {
                            Pointer = new PointerInfo
                            {
                                Type = TouchType,
                                Id = (uint)i,
                                Flags = (i == contact ? flags : InRange | InContact | Update) | (i == 0 ? Primary : 0),
                                PixelLocation = contacts[i].Value
                            }
                        }
                    }
                })
                .ToArray();
            if (frame.Length > 0 && !InjectSyntheticPointerInput(device, frame, (uint)frame.Length))
            {
                throw new Win32Exception(Marshal.GetLastWin32Error(), $"InjectSyntheticPointerInput contact={contact} flags=0x{flags:x}");
            }
        }

        public void Dispose()
        {
            if (device != IntPtr.Zero)
            {
                try
                {
                    for (int i = 0; i < contacts.Length; ++i)
                    {
                        Release(i);
                    }
                }
                finally
                {
                    DestroySyntheticPointerDevice(device);
                    device = IntPtr.Zero;
                }
            }
        }

        [DllImport("user32.dll", SetLastError = true)]
        private static extern IntPtr CreateSyntheticPointerDevice(uint type, uint maxCount, uint feedback);

        [DllImport("user32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool InjectSyntheticPointerInput(IntPtr device, [In] PointerTypeInfo[] input, uint count);

        [DllImport("user32.dll")]
        private static extern void DestroySyntheticPointerDevice(IntPtr device);

        // Native winuser.h layout; the pen member keeps the union's native size.
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
