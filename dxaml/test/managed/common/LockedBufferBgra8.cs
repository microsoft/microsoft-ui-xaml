// Copyright (c) Microsoft Corporation. All rights reserved.
//  swiped from %SDXROOT%\avcore\mf\photography\exception\MediaFrame\drt\CS\LockedBufferBgra8.cs

using System;
using System.Diagnostics.CodeAnalysis;
using System.Threading.Tasks;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.WindowsRuntime;

using Windows.Foundation;
using Windows.Graphics.Imaging;
using Windows.Storage;
using Windows.Storage.Streams;
using Windows.UI;

namespace Microsoft.UI.Xaml.Tests.Common
{
    public static class LockedBufferBgra8Extensions
    {
        public static LockedBufferBgra8
        LockBufferBgra8(
            this SoftwareBitmap bitmap,
            BitmapBufferAccessMode access)
        {
            return new LockedBufferBgra8(bitmap, access);
        }
    }

    public class LockedBufferBgra8 : IDisposable
    {
        public readonly int PixelWidth;
        public readonly int PixelHeight;

        // Low-level unsafe data
        public readonly unsafe byte* Data; // Points to the byte of the first pixel in the first row
        public readonly BitmapPlaneDescription Description;

        private BitmapBuffer m_buffer;
        private IClosableByteAccess m_byteAccess;
        private int m_rowIndex;
        private unsafe byte* m_rowData;
        private uint m_rowReadableCapacity;
        private uint m_rowWriteableCapacity;
        private bool m_locked;

        public unsafe
        LockedBufferBgra8(
            SoftwareBitmap bitmap,
            BitmapBufferAccessMode access)
        {
            if (bitmap.BitmapPixelFormat != BitmapPixelFormat.Bgra8)
            {
                throw new ArgumentException("Invalid BitmapPixelFormat");
            }

            m_buffer = bitmap.LockBuffer(access);
            m_byteAccess = (IClosableByteAccess)(Object)m_buffer;

            byte* data;
            uint capacity;
            m_byteAccess.Lock(out data, out capacity);
            m_locked = true;

            Description = m_buffer.GetPlaneDescription(0);
            PixelWidth = Description.Width;
            PixelHeight = Description.Height;
            Data = data + Description.StartIndex;

            m_rowIndex = 0;
            m_rowData = Data;
            m_rowReadableCapacity = (access == BitmapBufferAccessMode.Write ? 0 : (uint)Description.Stride);
            m_rowWriteableCapacity = (access == BitmapBufferAccessMode.Read ? 0 : (uint)Description.Stride);
        }

        public unsafe int
        CurrentRow
        {
            get
            {
                return m_rowIndex;
            }
            set
            {
                if ((uint)value >= PixelHeight)
                {
                    throw new ArgumentOutOfRangeException("value");
                }
                m_rowIndex = value;
                m_rowData = Data + value * Description.Stride;
            }
        }

        /// <summary>
        /// Access pixels in the current row.
        /// </summary>
        /// <remarks>Property getter returns 0 outside of bounds, property setter ignore values outside of bounds.</remarks>
        public unsafe byte
        this[int j]
        {
            // Returns 0 if position outside bounds (generates faster code than throwing an exception)
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return (uint)j >= m_rowReadableCapacity ? (byte)0 : m_rowData[j];
            }

            // Ignores value if position outside bounds (generates faster code than throwing an exception)
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set
            {
                if ((uint)j < m_rowWriteableCapacity)
                {
                    m_rowData[j] = value;
                }
            }
        }

        /// <summary>
        /// Access pixels in the whole plane.
        /// </summary>
        /// <remarks>Property getter returns 0 outside of bounds, property setter ignore values outside of bounds.</remarks>
        [SuppressMessage("Microsoft.Design", "CA1023: Indexers should not be multidimensional")]
        public unsafe byte
        this[int i, int j]
        {
            // Returns 0 if position outside bounds (generates faster code than throwing an exception)
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return ((uint)i >= PixelHeight) || ((uint)j >= m_rowReadableCapacity) ? (byte)0 : Data[i * Description.Stride + j];
            }

            // Ignores value if position outside bounds (generates faster code than throwing an exception)
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set
            {
                if (((uint)i < PixelHeight) && ((uint)j < m_rowReadableCapacity))
                {
                    Data[i * Description.Stride + j] = value;
                }
            }
        }

        public Color GetPixelColor(Point point)
        {
            int offset = (int)(this.PixelHeight * point.Y);
            this.CurrentRow = offset;

            offset = (int)(this.PixelWidth * point.X);
            var color = ColorHelper.FromArgb(
                    this[offset + 3],
                    this[offset + 2],
                    this[offset + 1],
                    this[offset + 0]);
            return color;
        }

        public unsafe void
        Dispose()
        {
            m_rowIndex = -1;

            // Prevents further reading/writing
            m_rowReadableCapacity = 0;
            m_rowWriteableCapacity = 0;

            if (m_locked)
            {
                m_byteAccess.Unlock();
                m_byteAccess = null;
                m_locked = false;
            }
            if (m_buffer != null)
            {
                m_buffer.Dispose();
                m_buffer = null;
            }

            GC.SuppressFinalize(this);
        }
    }

    [ComImport]
    [Guid("4ACCE8C4-3A72-4B17-8CFB-771DA0D12AFB")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    unsafe interface IClosableByteAccess
    {
        void Lock(out byte* buffer, out uint capacity);
        void Unlock();
    }
}
