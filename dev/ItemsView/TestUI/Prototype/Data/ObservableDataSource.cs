using Microsoft.UI.Xaml.Controls;
using System.Collections.Generic;
using System.Collections.ObjectModel;

namespace MUXControlsTestApp.ItemsViewPrototype
{
    //public class ObservableDataSource<T> : DataSource
    //{
    //    ObservableCollection<T> _data = null;
    //    public ObservableCollection<T> Data
    //    {
    //        get
    //        {
    //            if(_data == null)
    //            {
    //                _data = new ObservableCollection<T>();
    //            }
    //            return _data;
    //        }

    //        set
    //        {
    //            _data = value;
    //            OnDataSourceChanged(new DataSourceChangedEventArgs(DataSourceChangedAction.Reset, -1, -1, -1, -1));
    //        }
    //    }

    //    public bool SupportUniqueIds { get; set; }

    //    public ObservableDataSource()
    //    {
    //        SupportUniqueIds = true;
    //    }

    //    public ObservableDataSource(IEnumerable<T> data)
    //    {
    //        Data = new ObservableCollection<T>(data);
    //    }

    //    protected override int GetSizeCore()
    //    {
    //        return _data != null ? _data.Count : 0;
    //    }

    //    protected override object GetAtCore(int index)
    //    {
    //        return _data[index];
    //    }

    //    protected override bool HasUniqueIdsCore()
    //    {
    //        return SupportUniqueIds;
    //    }

    //    protected override string GetUniqueIdCore(int index)
    //    {
    //        return _data[index].GetHashCode().ToString();
    //    }
    //}
}
