using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WinUISnoopApp
{
    public class TAPType
    {
        public string TapType { get; set; }
        public string ModuleHandle { get; set; }
    }

    public class TAPVisualTreeChange
    {
        public string TapType { get; set; }
        public string Parent { get; set; }
        public string Element { get; set; }
        public int ChildIndex { get; set; }
        public string ElementType { get; set; }
        public string ElementName { get; set; }
        public string MutationType { get; set; }

        public bool IsParentNull() { return Parent != null && Convert.ToUInt64(Parent, 16) == 0; }
    }

    public class TAPProperty
    {
        public string Name { get; set; }
        public string Value { get; set; }
        public string Source { get; set; }

    }

    public class TAPElementProperties
    {
        public string TapType { get; set; }
        public string Element { get; set; }
        public List<TAPProperty> Properties { get; set; }
    }

    public class TAPSubTreeDone
    {
        public string TapType { get; set; }
        public string Element { get; set; }
    }

}
