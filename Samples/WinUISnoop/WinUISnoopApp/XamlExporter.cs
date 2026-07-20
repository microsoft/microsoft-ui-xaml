using System;
using System.Diagnostics;
using System.IO;
using System.Text;
using WinUISnoopApp.SnoopData;

namespace WinUISnoopApp
{
    internal static class XamlExporter
    {
        public static void Export(TreeItem treeItem, bool includeDefaultProperties = true)
        {
            string xaml = BuildElementXaml(treeItem, includeDefaultProperties, indent: 0);
            WriteAndOpen(treeItem, xaml);
        }

        public static void ExportSubTree(TreeItem treeItem, bool includeDefaultProperties = true)
        {
            var sb = new StringBuilder();
            BuildSubTreeXaml(sb, treeItem, includeDefaultProperties, indent: 0);
            WriteAndOpen(treeItem, sb.ToString());
        }

        private static void WriteAndOpen(TreeItem treeItem, string xamlContent)
        {
            string exportDir = Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments),
                "WinUISnoop");
            Directory.CreateDirectory(exportDir);

            string timestamp = DateTime.Now.ToString("yyyyMMdd_HHmmss");
            string safeName = treeItem.Name.Replace(" ", "_").Replace("(", "").Replace(")", "");
            string fileName = $"{safeName}_{timestamp}.xaml";
            string filePath = Path.Combine(exportDir, fileName);

            File.WriteAllText(filePath, xamlContent);

            Process.Start("explorer.exe", $"/select,\"{filePath}\"");
        }

        private static void BuildSubTreeXaml(StringBuilder sb, TreeItem item, bool includeDefaultProperties, int indent)
        {
            string shortType = GetShortType(item.ElementType);
            string xmlns = GetXmlns(item.ElementType);
            string pad = new string(' ', indent * 2);

            sb.Append($"{pad}<{shortType}");

            if (indent == 0 && !string.IsNullOrEmpty(xmlns))
            {
                sb.Append($" xmlns=\"using:{xmlns}\"");
            }

            AppendProperties(sb, item, includeDefaultProperties, pad);

            bool hasChildren = item.Children != null && item.Children.Count > 0;
            if (hasChildren)
            {
                sb.AppendLine(">");
                foreach (var child in item.Children)
                {
                    BuildSubTreeXaml(sb, child, includeDefaultProperties, indent + 1);
                }
                sb.AppendLine($"{pad}</{shortType}>");
            }
            else
            {
                sb.AppendLine(" />");
            }
        }

        private static string BuildElementXaml(TreeItem item, bool includeDefaultProperties, int indent)
        {
            var sb = new StringBuilder();
            string shortType = GetShortType(item.ElementType);
            string xmlns = GetXmlns(item.ElementType);
            string pad = new string(' ', indent * 2);

            sb.Append($"{pad}<{shortType}");

            if (!string.IsNullOrEmpty(xmlns))
            {
                sb.Append($" xmlns=\"using:{xmlns}\"");
            }

            AppendProperties(sb, item, includeDefaultProperties, pad);
            sb.AppendLine(" />");
            return sb.ToString();
        }

        private static void AppendProperties(StringBuilder sb, TreeItem item, bool includeDefaultProperties, string pad)
        {
            if (item.Properties == null || item.Properties.Count == 0)
                return;

            foreach (var prop in item.Properties)
            {
                if (!includeDefaultProperties &&
                    string.Equals(prop.Source, "Default", StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }

                if (prop.Value != null && !prop.Value.Contains('\n'))
                {
                    string escaped = prop.Value
                        .Replace("&", "&amp;")
                        .Replace("\"", "&quot;")
                        .Replace("<", "&lt;")
                        .Replace(">", "&gt;");
                    sb.Append($"\n{pad}  {prop.Name}=\"{escaped}\"");
                }
            }
        }

        private static string GetShortType(string elementType)
        {
            int dotIndex = elementType.LastIndexOf('.');
            return dotIndex >= 0 ? elementType.Substring(dotIndex + 1) : elementType;
        }

        private static string GetXmlns(string elementType)
        {
            int dotIndex = elementType.LastIndexOf('.');
            return dotIndex >= 0 ? elementType.Substring(0, dotIndex) : "";
        }
    }
}
