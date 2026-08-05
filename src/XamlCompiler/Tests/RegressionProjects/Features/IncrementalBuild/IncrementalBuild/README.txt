How to test this scenario:

Incremental build with changing local assembly:
1. Open IncrementalBuild.sln
2. Open MainPage.xml.cs under the IncrementalBuildLocalAsm project
3. Do a clean rebuild of the entire solution
4. Add or remove INotifyPropertyChanged from MainPage under MainPage.xaml.cs
5. Do an incremental build of the solution
6. Verify MainPage.g.cs has changed
7. Add/remove INotifyPropertyChanged from Person under MainPage.xaml.cs
8. Do an incremental build of the solution
9. Verify MainPage.g.cs has not changed, and that its timestamp also hasn't changed (you may need to wait a minute before this step to easily check)

To test an incremental build with a changing remote assembly, repeat the steps above except in Step 4, instead add/remove INotifyPropertyChanged from RemoteShirt under RemoteShirt.cs
in the IncrementalBuildRemoteAsm project.
