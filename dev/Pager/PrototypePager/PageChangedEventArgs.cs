using System;
using System.Collections.Generic;
using System.Text;
using Windows.UI.Xaml;

namespace MUXControlsTestApp
{
    public sealed class PageChangedEventArgs : EventArgs
    {
        public int CurrentPage { get; private set; }
        public int PreviousPage { get; private set; }

        public PageChangedEventArgs(int OldIndex, int NewIndex)
        {
            PreviousPage = OldIndex;
            CurrentPage = NewIndex;
        }
    }
}
