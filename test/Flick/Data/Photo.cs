using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Linq;

namespace Flick
{
    // <photo id="47482260852" owner="65635049@N08" secret="27c7ff6465" server="7839" farm="8" title="Chantilly Arts &amp; Elegance 2016 - Bugatti Veyron 16.4 Super Sport WRC" ispublic="1" isfriend="0" isfamily="0" />
    public class Photo
    {
        public string Id { get; set; }

        public string Owner { get; set; }

        public string Secret { get; set; }

        public string Server { get; set; }

        public string Farm { get; set; }

        public string Title { get; set; }

        public string UrlMedium
        {
            get
            {
                //  https://www.flickr.com/services/api/misc.urls.html
                return string.Format("https://farm{0}.staticflickr.com/{1}/{2}_{3}_m.jpg", Farm, Server, Id, Secret);
            }
        }

        public string LargeUrl
        {
            get
            {
                //  https://www.flickr.com/services/api/misc.urls.html
                return string.Format("https://farm{0}.staticflickr.com/{1}/{2}_{3}_b.jpg", Farm, Server, Id, Secret);
            }
        }

        public string Description { get; set; }

        public int FlexGrow { get; set; }

        public int FlexBasis { get; set; }

        private static string lorem = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam laoreet erat vel massa rutrum, eget mollis massa vulputate. Vivamus semper augue leo, eget faucibus nulla mattis nec. Donec scelerisque lacus at dui ultricies, eget auctor ipsum placerat. Integer aliquet libero sed nisi eleifend, nec rutrum arcu lacinia. Sed a sem et ante gravida congue sit amet ut augue. Donec quis pellentesque urna, non finibus metus. Proin sed ornare tellus. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam laoreet erat vel massa rutrum, eget mollis massa vulputate. Vivamus semper augue leo, eget faucibus nulla mattis nec. Donec scelerisque lacus at dui ultricies, eget auctor ipsum placerat. Integer aliquet libero sed nisi eleifend, nec rutrum arcu lacinia. Sed a sem et ante gravida congue sit amet ut augue. Donec quis pellentesque urna, non finibus metus. Proin sed ornare tellus.";
        private static Random random = new Random();
        
        public static Photo Parse(XElement element)
        {
            Photo photo = new Photo();
            foreach (var attr in element.Attributes())
            {
                switch (attr.Name.ToString())
                {
                    case "id":
                        photo.Id = attr.Value;
                        break;
                    case "owner":
                        photo.Owner = attr.Value;
                        break;
                    case "secret":
                        photo.Secret = attr.Value;
                        break;
                    case "farm":
                        photo.Farm = attr.Value;
                        break;
                    case "title":
                        photo.Title = attr.Value;
                        break;
                    case "server":
                        photo.Server = attr.Value;
                        break;
                    default:
                        break;
                }
            }

            photo.Description = lorem.Substring(0, random.Next(lorem.Length - 1));

            return photo;
        }
    }
}
