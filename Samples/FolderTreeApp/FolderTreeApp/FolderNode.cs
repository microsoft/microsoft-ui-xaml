using System;
using System.Collections.ObjectModel;
using System.IO;

namespace FolderTreeApp
{
    /// <summary>
    /// Represents a folder in the file system for TreeView data binding.
    /// Children are loaded lazily on first expansion.
    /// </summary>
    public class FolderNode
    {
        public string Name { get; }
        public string FullPath { get; }
        public ObservableCollection<FolderNode> Children { get; } = new ObservableCollection<FolderNode>();
        public bool HasUnrealizedChildren { get; private set; }

        // Segoe Fluent / MDL2 glyphs: hard-drive for a drive root, folder otherwise.
        public string Glyph => IsDriveRoot ? "\uEDA2" : "\uE8B7";

        private bool IsDriveRoot =>
            FullPath.Length <= 3 && FullPath.EndsWith(":\\", StringComparison.Ordinal);

        public FolderNode(string path)
        {
            FullPath = path;
            Name = Path.GetFileName(path);
            if (string.IsNullOrEmpty(Name))
            {
                Name = path; // root like "C:\"
            }

            HasUnrealizedChildren = HasSubDirectories(path);
        }

        /// <summary>
        /// Loads immediate subdirectories into Children. Called when the node is first expanded.
        /// </summary>
        public void LoadChildren()
        {
            if (!HasUnrealizedChildren)
            {
                return;
            }

            HasUnrealizedChildren = false;
            Children.Clear();

            try
            {
                foreach (var dir in Directory.GetDirectories(FullPath))
                {
                    Children.Add(new FolderNode(dir));
                }
            }
            catch (UnauthorizedAccessException) { }
            catch (IOException) { }
        }

        private static bool HasSubDirectories(string path)
        {
            try
            {
                var enumerator = Directory.EnumerateDirectories(path).GetEnumerator();
                bool hasAny = enumerator.MoveNext();
                enumerator.Dispose();
                return hasAny;
            }
            catch
            {
                return false;
            }
        }
    }
}
