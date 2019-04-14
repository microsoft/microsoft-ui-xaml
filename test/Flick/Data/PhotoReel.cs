using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Flick
{
    public class PhotoReel : ObservableCollection<Photo>
    {
        public PhotoReel(IEnumerable<Photo> photos)
        : base(photos)
        {

        }

        public PhotoReel() { }

        public string Name { get; set; }
    }
}
