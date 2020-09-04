$picturesFolder = [Environment]::GetFolderPath('MyPictures')
Move-Item $picturesFolder\*.xml $Env:HELIX_WORKITEM_UPLOAD_ROOT
