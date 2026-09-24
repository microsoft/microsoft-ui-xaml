using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace WinUISnoopApp.SnoopData
{
    public class ElementProperty : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;

        public string Name { get; }

        private string m_value;
        public string Value
        {
            get { return m_value; }
            set
            {
                if (m_value != value)
                {
                    m_value = value;
                    NotifyPropertyChanged();
                }
            }
        }

        private string m_source;
        public string Source
        {
            get { return m_source; }
            set
            {
                if (m_source != value)
                {
                    m_source = value;
                    NotifyPropertyChanged();
                    NotifyPropertyChanged("SourceAbbreviation");
                }
            }
        }

        public string SourceAbbreviation { get { return m_source != null ? m_source.Substring(0, 1) : ""; } }

        public void CopyFrom(ElementProperty other)
        {
            System.Diagnostics.Debug.Assert(this.Name == other.Name, "Morphing ElementProperty.Name is not supported.");
            this.Value = other.Value;
            this.Source = other.Source;
        }

        private void NotifyPropertyChanged([CallerMemberName] String propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        public ElementProperty(string name)
        {
            Name = name;
        }
    }
}
