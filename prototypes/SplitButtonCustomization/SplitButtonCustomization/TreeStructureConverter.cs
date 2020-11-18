using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Runtime.ExceptionServices;
using System.Text;
using System.Threading.Tasks;

namespace SplitButtonCustomization
{
    public abstract class TreeStructureConverter
    {
        private Object m_rootNode;
        public Object RootNode 
        { 
            get { return m_rootNode; } 
            set { m_rootNode = value; BreadCrumbPath = new List<object>() { value }; } 
        }

        // Abstract method to be overriden by the app creator to define how children are retrieved.
        public abstract IReadOnlyList<Object> GetChildren(Object node);

        public List<Object> BreadCrumbPath { get; private set; } = new List<Object>();

        private IReadOnlyList<Object> CreatePathFromRoot(Object node, FlatListBreadCrumb flatListBreadCrumb)
        {
            if (BreadCrumbPath.Contains(node))
            {
                CopyBreadCrumbPathUpToNode(node);
            }
            else if (flatListBreadCrumb.CurrentItem == BreadCrumbPath.Last<Object>())
            {
                AppendChildToList(node);
            }
            else
            {
                CopyBreadCrumbPathUpToNode(flatListBreadCrumb.CurrentItem);
                AppendChildToList(node);
            }

            return new ReadOnlyCollection<Object>(BreadCrumbPath);
        }

        private void CopyBreadCrumbPathUpToNode(Object node)
        {
            BreadCrumbPath = BreadCrumbPath.GetRange(0, BreadCrumbPath.IndexOf(node));
        }

        private void AppendChildToList(Object node)
        {
            BreadCrumbPath.Add(node);
        }

    }
}
