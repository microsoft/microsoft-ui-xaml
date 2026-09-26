// -------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All Rights Reserved.
// -------------------------------------------------------------------
namespace Win8Xaml.CompilerProxies
{
    using System;
    using System.IO;
    using System.Reflection;

    public class KnownVersions
    {
        public const string Latest = BuildKnownVersions.WindowsSdkTargetPlatformVersion;

        public const string Y19H1 = "10.0.18362.0";
        public const string RS5 = "10.0.17763.0";
        public const string RS4 = "10.0.17134.0";
        public const string RS3 = "10.0.16299.0";
        public const string RS2 = "10.0.15063.0";
        public const string RS1 = "10.0.14393.0";

        private static string _foundationContractVersion = null;
        public static string FoundationContractVersion
        {
            get
            {
                if (_foundationContractVersion == null)
                {
                    _foundationContractVersion = DetectContractVersion("Windows.Foundation.FoundationContract");
                }
                return _foundationContractVersion;
            }
        }

        private static string _universalApiContractVersion = null;
        public static string UniversalApiContractVersion
        {
            get
            {
                if (_universalApiContractVersion == null)
                {
                    _universalApiContractVersion = DetectContractVersion("Windows.Foundation.UniversalApiContract");
                }
                return _universalApiContractVersion;
            }
        }

        private static string FindSdkReferencesRoot()
        {
            string programFilesX86 = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86);
            if (!String.IsNullOrEmpty(programFilesX86))
            {
                string sdkRefsPath = Path.Combine(programFilesX86, @"Windows Kits\10\References");
                if (Directory.Exists(sdkRefsPath))
                {
                    return sdkRefsPath;
                }
            }

            string fallbackPath = Path.Combine(@"C:\Program Files (x86)", @"Windows Kits\10\References");
            if (Directory.Exists(fallbackPath))
            {
                return fallbackPath;
            }

            throw new DirectoryNotFoundException(
                @"Windows SDK references were not found under Program Files (x86)\Windows Kits\10\References.");
        }

        private static string DetectContractVersion(string contractName)
        {
            string sdkReferencesPath = Path.Combine(FindSdkReferencesRoot(), Latest);
            if (!Directory.Exists(sdkReferencesPath))
            {
                throw new DirectoryNotFoundException(
                    $"The Windows SDK targeted by this build ({Latest}) was not found at '{sdkReferencesPath}'.");
            }

            string contractPath = Path.Combine(sdkReferencesPath, contractName);
            if (!Directory.Exists(contractPath))
            {
                throw new DirectoryNotFoundException(
                    $"The contract '{contractName}' was not found in Windows SDK {Latest}.");
            }

            Version latestVersion = null;
            string latestVersionName = null;
            foreach (string directory in Directory.GetDirectories(contractPath))
            {
                string versionName = Path.GetFileName(directory);
                Version version;
                if (!Version.TryParse(versionName, out version))
                {
                    throw new InvalidDataException(
                        $"The contract directory '{directory}' does not have a version-shaped name.");
                }

                if (latestVersion == null || version.CompareTo(latestVersion) > 0)
                {
                    latestVersion = version;
                    latestVersionName = versionName;
                }
            }

            if (latestVersionName == null)
            {
                throw new DirectoryNotFoundException(
                    $"The contract '{contractName}' has no version directories in Windows SDK {Latest}.");
            }

            return latestVersionName;
        }
    }

    public class ProxyHelper
    {
        const string ProgramFiles = @"C:\Program Files\";
        const string ProgramFilesx86 = @"C:\Program Files (x86)\";
        const string XamlBuildTaskFile = @"Microsoft.UI.Xaml.Markup.Compiler.dll";
        public const string WinUIWinmdFile = @"Microsoft.UI.Xaml.winmd";

        static Assembly _xamlCompilerAssembly;
        static string _xamlCompilerAssemblyPath = null;
        static string _winuiWinmdPath = null;
        static string buildTaskPath = null;
        Type _type;

        private static string _sdkPath = null;
        public static string SdkPath
        {
            get
            {
                if (_sdkPath == null)
                {
                    _sdkPath = FindWindowsSdkDir("Windows Kits\\10");
                }

                return _sdkPath;
            }
        }

        public static Version TargetPlatformMinVersion = new Version(KnownVersions.Latest);

        public static string FindProgramFilesFile(string path, params string[] versions)
        {
            foreach (string ver in versions)
            {
                string versionPath = string.Format(path, ver);

                if (File.Exists(ProgramFiles + versionPath))
                {
                    return ProgramFiles + versionPath;
                }
                else if (File.Exists(ProgramFilesx86 + versionPath))
                {
                    return ProgramFilesx86 + versionPath;
                }
            }
            throw new InvalidOperationException(string.Format("Cannot find path '{0}' using versions '{1}'", path, string.Join(", ", versions)));
        }

        public static string FindProgramFilesDir(string path)
        {
            if (Directory.Exists(ProgramFiles + path))
            {
                return ProgramFiles + path;
            }
            else if (Directory.Exists(ProgramFilesx86 + path))
            {
                return ProgramFilesx86 + path;
            }
            throw new InvalidOperationException(string.Format("Cannot find path '{0}'", path));
        }

        public static string XamlCompilerPath
        {
            get
            {
                if (_xamlCompilerAssemblyPath == null)
                {
                    FileInfo fileInfo = new FileInfo(Assembly.GetExecutingAssembly().Location);
                    _xamlCompilerAssemblyPath = Path.Combine(fileInfo.DirectoryName, XamlBuildTaskFile);
                }
                return _xamlCompilerAssemblyPath;
            }
            set
            {
                _xamlCompilerAssemblyPath = value;
            }
        }

        public static string WinUIWinmdPath
        {
            get
            {
                if (_winuiWinmdPath == null)
                {
                    FileInfo fileInfo = new FileInfo(Assembly.GetExecutingAssembly().Location);
                    _winuiWinmdPath = Path.Combine(fileInfo.DirectoryName, WinUIWinmdFile);
                }
                return _winuiWinmdPath;
            }
            set
            {
                _winuiWinmdPath = value;
            }
        }

        private static Assembly XamlCompilerCoreAssembly
        {
            get
            {
                if (_xamlCompilerAssembly == null)
                {
                    _xamlCompilerAssembly = Assembly.LoadFrom(ProxyHelper.XamlCompilerPath);
                }
                return _xamlCompilerAssembly;
            }
        }

        public ProxyHelper(string fullName)
        {
            _type = XamlCompilerCoreAssembly.GetType(fullName, true);
        }

        public object CreateInstance()
        {
            return Activator.CreateInstance(_type);
        }

        public object CreateInstance(object[] args)
        {
            try
            {
                return Activator.CreateInstance(_type, args);
            }
            catch (Exception e)
            {
                throw e.InnerException ?? e;
            }
        }

        public PropertyInfo GetProperty(string name, bool isInternal = false)
        {
            PropertyInfo pi = null;
            if (!isInternal)
            {
                pi = _type.GetProperty(name);
            }
            else
            {
                BindingFlags bif = BindingFlags.NonPublic | BindingFlags.Instance;
                pi = _type.GetProperty(name, bif);
            }
            if (pi == null)
            {
                throw new InvalidOperationException(string.Format("Could not get the Property '{0}' from Type {1}.", name, _type.FullName));
            }
            return pi;
        }

        public MethodInfo GetMethod(string name)
        {
            return this.GetMethod(name, BindingFlags.Public | BindingFlags.Instance);
        }

        public MethodInfo GetMethod(string name, BindingFlags bflags)
        {
            MethodInfo mi = null;
            try
            {
                mi = _type.GetMethod(name, bflags);
            }
            catch (AmbiguousMatchException)
            {
                throw new ArgumentException("Method '" + name + "' on Type '" + _type.Name + "' is Ambigious.  Use the GetMethod() override that takes Parameter Types");
            }
            if (mi == null)
            {
                throw new InvalidOperationException(string.Format("Could not get Method {0} from Type {1}.", name, _type.FullName));
            }
            return mi;
        }

        public MethodInfo GetMethod(string name,
                                    int argCount,
                                    Type[] argTypes = null,
                                    BindingFlags bflags = BindingFlags.Public | BindingFlags.Instance)
        {
            MethodInfo[] methodInfos = _type.GetMethods(bflags);
            foreach (MethodInfo mi in methodInfos)
            {
                if (mi.Name == name)
                {
                    ParameterInfo[] paramInfos = mi.GetParameters();
                    if (paramInfos.Length == argCount)
                    {
                        if (argTypes == null || argTypes.Length == 0)
                        {
                            return mi;
                        }
                        bool match = true;
                        for (int i = 0; i < paramInfos.Length; i++)
                        {
                            if (paramInfos[i].ParameterType != argTypes[i])
                            {
                                match = false;
                                break;
                            }
                        }
                        if (match)
                        {
                            return mi;
                        }
                    }
                }
            }
            return null;
        }

        public MethodInfo GetMethod(string name,
                            int argCount,
                            string[] argTypeFullNames,
                            BindingFlags bflags = BindingFlags.Public | BindingFlags.Instance)
        {
            MethodInfo[] methodInfos = _type.GetMethods(bflags);
            foreach (MethodInfo mi in methodInfos)
            {
                if (mi.Name == name)
                {
                    ParameterInfo[] paramInfos = mi.GetParameters();
                    if (paramInfos.Length == argCount)
                    {
                        if (argTypeFullNames == null || argTypeFullNames.Length == 0)
                        {
                            return mi;
                        }
                        bool match = true;
                        for (int i = 0; i < paramInfos.Length; i++)
                        {
                            if (paramInfos[i].ParameterType.FullName != argTypeFullNames[i])
                            {
                                match = false;
                                break;
                            }
                        }
                        if (match)
                        {
                            return mi;
                        }
                    }
                }
            }
            return null;
        }

        public MethodInfo GetStaticMethod(string name)
        {
            MethodInfo mi;
            try
            {
                mi = _type.GetMethod(name, BindingFlags.Public | BindingFlags.Static);
            }
            catch (AmbiguousMatchException)
            {
                throw new ArgumentException("Method '" + name + "' on Type '" + _type.Name + "' is Ambigious.  Use the GetMethod() override that takes Parameter Types");
            }
            if (mi == null)
            {
                throw new InvalidOperationException(string.Format("Could not get Static Method {0} from Type {1}.", name, _type.FullName));
            }
            return mi;
        }

        public MethodInfo GetStaticMethod(string name, int argCount, Type[] argTypes = null)
        {
            MethodInfo mi = GetMethod(name, argCount, argTypes, BindingFlags.Public | BindingFlags.Static);
            return mi;
        }

        public EventInfo GetEvent(string name)
        {
            EventInfo ei = _type.GetEvent(name, BindingFlags.Public | BindingFlags.Instance);
            if (ei == null)
            {
                throw new InvalidOperationException(string.Format("Couln not get Event {0} from type {1}.", name, _type.FullName));
            }
            return ei;
        }

        private static string FindWindowsSdkDir(string path)
        {
            string windowsSdkDir = System.Environment.GetEnvironmentVariable("WindowsSdkDir");
            
            if (windowsSdkDir.IndexOf(path) != -1)
            {
                return windowsSdkDir;
            }

            return FindProgramFilesDir(path);
        }
    }
}