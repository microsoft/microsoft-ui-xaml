using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ItemsRepeaterExperiments.Common
{
    public static class DataSourceCreator<T> where T :  IRandomlyInitializable, new()
    {

        public static List<T> CreateRandomizedList(int itemCount)
        {
            List<T> items = new List<T>(itemCount);
            for(int i = 0;i<itemCount; i++)
            {
                var obj = new T();
                obj.CreateRandomInstance(i);
                items.Add(obj);
            }
            return items;
        }

        public static ObservableCollection<T> CreateRandomizedObservableCollection(int itemCount)
        {
            ObservableCollection<T> items = new ObservableCollection<T>();
            for (int i = 0; i < itemCount; i++)
            {
                var obj = new T();
                obj.CreateRandomInstance(i);
                items.Add(obj);
            }
            return items;
        }
    }
}
