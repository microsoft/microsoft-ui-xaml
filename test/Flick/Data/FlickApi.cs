using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Linq;
using Windows.Storage;

namespace Flick
{
    public class FlickApi
    {
        private static bool loadImagesLocally = false;

        public static bool LoadImagesLocally
        {
            get
            {
                return loadImagesLocally;
            }
            set
            {
                loadImagesLocally = value;
            }
        }

        public static async Task<PhotoReel> GetPhotos(string tag, int count = 100)
        {
            PhotoReel photos =  await LoadFeed(tag, count);
            return photos;
        }

        private static async Task<PhotoReel> LoadFeedLocally(string tag)
        {
            string folderPath = "Assets/Tulips";
            PhotoReel reel = new PhotoReel() { Name = tag };

            var filePaths = await GetAssetFiles(folderPath);
            foreach (var filePath in filePaths)
            {
                reel.Add(new Photo { LocalSourcePath = filePath, UseLocalSourcePath = true });
            }

            return reel;
        }

        static async Task<IEnumerable<string>> GetAssetFiles(string rootPath)
        {
            string folderPath = System.IO.Path.Combine(
                Windows.ApplicationModel.Package.Current.InstalledLocation.Path,
                rootPath.Replace('/', '\\').TrimStart('\\')
            );
            var folder = await StorageFolder.GetFolderFromPathAsync(folderPath);
            var files = await folder.GetFilesAsync();
            var relativePaths = from file in files select (rootPath + "/" + file.Name);
            return relativePaths;
        }

        private static async Task<PhotoReel> LoadFeed(string tag, int count)
        {
            if (LoadImagesLocally)
            {
                return await LoadFeedLocally(tag);
            }

            // https://www.flickr.com/services/api/flickr.photos.search.html
            var url = string.Format("https://api.flickr.com/services/rest/?method=flickr.photos.search&api_key=6c67e040d4f2639b17b2a00fc181f79d&tags={0}&styles=depthoffield&safe_search=1&per_page={1}&page=1", tag, count);


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
