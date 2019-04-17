using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Linq;

namespace Flick
{
    public class FlickApi
    {
        public static async Task<PhotoReel> GetPhotos(string tag, int count = 100)
        {
            PhotoReel photos =  await LoadFeed(tag, count);
            return photos;
        }

        private static async Task<PhotoReel> LoadFeed(string tag, int count)
        {
            // https://www.flickr.com/services/api/flickr.photos.search.html
            var url = string.Format("https://api.flickr.com/services/rest/?method=flickr.photos.search&api_key=dc318dbb02bf7cd2ab3daca1ae7d93c8&tags={0}&styles=depthoffield&safe_search=1&per_page={1}&page=1", tag, count);


            Windows.Web.Http.HttpClient httpClient = new Windows.Web.Http.HttpClient();
            Uri requestUri = new Uri(url);
            Windows.Web.Http.HttpResponseMessage httpResponse = new Windows.Web.Http.HttpResponseMessage();
            string httpResponseBody = "";

            try
            {
                //Send the GET request
                httpResponse = await httpClient.GetAsync(requestUri);
                httpResponse.EnsureSuccessStatusCode();
                httpResponseBody = await httpResponse.Content.ReadAsStringAsync();
            }
            catch (Exception ex)
            {
                httpResponseBody = "Error: " + ex.HResult.ToString("X") + " Message: " + ex.Message;
            }

            XElement root = XElement.Parse(httpResponseBody);
            var entries = root.Descendants()
                          .Where(x => x.Name.LocalName == "photo")
                          .ToList();
            
            PhotoReel reel = new PhotoReel() { Name = tag };
            foreach (XElement v in entries)
            {
                reel.Add(Photo.Parse(v));
            }

            return reel;
        }
    }
}
