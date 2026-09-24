#define USING_CSWINRT
using System;
using System.Collections.Generic;
#if USING_CSWINRT
using System.Collections.ObjectModel;
using System.ComponentModel;
#else
using Microsoft.UI.Xaml.Data;
#endif
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Text.Json.Serialization;
using System.Threading.Tasks;

namespace WinUISnoopApp.SnoopData
{
    public class TreeItem : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;
        public string Name { get; }
        public string Element { get; }
        public string ElementType { get; }
        private ObservableCollection<TreeItem> m_children;
        public ObservableCollection<TreeItem> Children
        {
            get
            {
                if (m_children == null)
                {
                    m_children = new ObservableCollection<TreeItem>();
                }
                return m_children;
            }
        }

        public ObservableCollection<ElementProperty> Properties { get; }

        private bool m_isExpanded = true;
        [JsonIgnore]
        public bool IsExpanded
        {
            get { return m_isExpanded; }
            set
            {
                if (m_isExpanded != value)
                {
                    m_isExpanded = value;
                    NotifyPropertyChanged();
                }
            }
        }

        private bool m_isSelected;
        public bool IsSelected
        {
            get { return m_isSelected; }

            set
            {
                if (m_isSelected != value)
                {
                    m_isSelected = value;
                    NotifyPropertyChanged();
                }
            }

        }

        private void NotifyPropertyChanged([CallerMemberName] String propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        public TreeItem(string element, string name, string elementType)
        {
            Element = element;
            Name = name;
            ElementType = elementType;
            Properties = new ObservableCollection<ElementProperty>();
        }
    }

}
