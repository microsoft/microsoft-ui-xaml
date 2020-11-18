using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SplitButtonCustomization
{
    public class TreeStructureConverterImpl : TreeStructureConverter
    {
        public override IReadOnlyList<object> GetChildren(object node)
        {
            TreeNode treeNode = node as TreeNode;
            return new ReadOnlyCollection<object>(treeNode.Children);
        }
    }
}
