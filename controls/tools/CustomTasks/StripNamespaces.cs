using System.Collections.Generic;
using System.Linq;
using System.Text.RegularExpressions;
using System.Xml.Linq;

namespace CustomTasks
{
    class StripNamespaces
    {

        public static Dictionary<string, int> universalApiContractVersionMapping = new Dictionary<string, int>()
        {
            { "RS1", 3 },
            { "RS2", 4 },
            { "RS3", 5 },
            { "RS4", 6 },
            { "RS5", 7 },
            { "19H1", 8},
            { "21H1", 14}
        };

        static List<string> BuildNamespaceToBeRemoved(int apiVersion)
        {            
            List<string> namespaces = new List<string>();
            for (int i = apiVersion; i >= universalApiContractVersionMapping.Values.Min(); i--)
            {
                namespaces.Add("http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractNotPresent(Windows.Foundation.UniversalApiContract," + i +")");
            }

            for (int i = apiVersion + 1; i <= universalApiContractVersionMapping.Values.Max(); i++)
            {
                namespaces.Add("http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Windows.Foundation.UniversalApiContract," + i + ")");
            }

            return namespaces;
        }

        static List<string> BuildRegexPatternToBeReplaced()
        {
            List<string> patterns = new List<string>();

            string conditionalPatternAll = "contract[0-9]+(Not|)Present";

            // xmlns:contract5Present="http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Windows.Foundation.UniversalApiContract,5)"
            patterns.Add("(?m)xmlns:" + conditionalPatternAll + "=\"[^\"]+\"");
            
            // Remove tag only contract5NotPresent:
            patterns.Add(conditionalPatternAll + ":");
            return patterns;
        }

        public static string StripNamespaceForAPIVersion(string content, int apiVersion)
        {
            content = Utils.EscapeAmpersand(content);
            // strip all elements and attributes in specific NotPresent namespace which is <= api version or Present > api version
            var namespaceToBeRemoved = BuildNamespaceToBeRemoved(apiVersion);
            string newContent = StripNamespace(content, namespaceToBeRemoved);

            // replace all conditional xaml
            foreach (var pattern in BuildRegexPatternToBeReplaced())
            {
                newContent = Regex.Replace(newContent, pattern, "", RegexOptions.Multiline);
            }
            return Utils.UnEscapeAmpersand(newContent);
        }

        static string StripNamespace(string xml, List<string> namespaceToBeRemoved)
        {
            var doc = XDocument.Parse(xml);

            //get all elements in specific namespace and remove
            doc.Descendants()
           .Where(o => namespaceToBeRemoved.Contains(o.Name.Namespace.NamespaceName))
           .Remove();

            //get all attributes in specific namespace and remove
            doc.Descendants()
           .Attributes()
           .Where(o => namespaceToBeRemoved.Contains(o.Name.Namespace.NamespaceName))
           .Remove();

            //remove attributes in root
            doc.Root.Attributes()
                .Where(o => namespaceToBeRemoved.Contains(o.Name.Namespace.NamespaceName))
                .Remove();

            return Utils.DocumentToString(writer => doc.WriteTo(writer));
        }
    }
}
