// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Forward declarations for external headers.  Please use in header files instead of declaring manually.

#include <abi/xaml_abi.h>

XAML_ABI_NAMESPACE_BEGIN
namespace Windows {
namespace Storage {
    class AppDataPaths;
    class ApplicationData;
    class ApplicationDataCompositeValue;
    class ApplicationDataContainer;
    class ApplicationDataContainerSettings;
    class SetVersionDeferral;
    class SetVersionRequest;
    class StorageFile;
    class StorageFolder;
    class StorageLibrary;
    class StorageLibraryChange;
    class StorageLibraryChangeReader;
    class StorageLibraryChangeTracker;
    class StorageProvider;
    class StorageStreamTransaction;
    class StreamedFileDataRequest;
    class SystemAudioProperties;
    class SystemDataPaths;
    class SystemGPSProperties;
    class SystemImageProperties;
    class SystemMediaProperties;
    class SystemMusicProperties;
    class SystemPhotoProperties;
    class SystemVideoProperties;
    class UserDataPaths;
    enum CreationCollisionOption : int;
    enum FileAccessMode : int;
    enum FileAttributes : unsigned int;
    enum NameCollisionOption : int;
    interface IAppDataPaths;
    interface IAppDataPathsStatics;
    interface IApplicationData;
    interface IApplicationData2;
    interface IApplicationData3;
    interface IApplicationDataContainer;
    interface IApplicationDataSetVersionHandler;
    interface IApplicationDataStatics;
    interface IApplicationDataStatics2;
    interface ICachedFileManagerStatics;
    interface IDownloadsFolderStatics;
    interface IDownloadsFolderStatics2;
    interface IFileIOStatics;
    interface IKnownFoldersCameraRollStatics;
    interface IKnownFoldersPlaylistsStatics;
    interface IKnownFoldersSavedPicturesStatics;
    interface IKnownFoldersStatics;
    interface IKnownFoldersStatics2;
    interface IKnownFoldersStatics3;
    interface IPathIOStatics;
    interface ISetVersionDeferral;
    interface ISetVersionRequest;
    interface IStorageFile;
    interface IStorageFile2;
    interface IStorageFilePropertiesWithAvailability;
    interface IStorageFileStatics;
    interface IStorageFolder;
    interface IStorageFolder2;
    interface IStorageFolder3;
    interface IStorageFolderStatics;
    interface IStorageItem;
    interface IStorageItem2;
    interface IStorageItemProperties;
    interface IStorageItemProperties2;
    interface IStorageItemPropertiesWithProvider;
    interface IStorageLibrary;
    interface IStorageLibrary2;
    interface IStorageLibrary3;
    interface IStorageLibraryChange;
    interface IStorageLibraryChangeReader;
    interface IStorageLibraryChangeTracker;
    interface IStorageLibraryStatics;
    interface IStorageLibraryStatics2;
    interface IStorageProvider;
    interface IStorageProvider2;
    interface IStorageStreamTransaction;
    interface IStreamedFileDataRequest;
    interface IStreamedFileDataRequestedHandler;
    interface ISystemAudioProperties;
    interface ISystemDataPaths;
    interface ISystemDataPathsStatics;
    interface ISystemGPSProperties;
    interface ISystemImageProperties;
    interface ISystemMediaProperties;
    interface ISystemMusicProperties;
    interface ISystemPhotoProperties;
    interface ISystemProperties;
    interface ISystemVideoProperties;
    interface IUserDataPaths;
    interface IUserDataPathsStatics;
namespace Streams {
    class Buffer;
    class DataReader;
    class DataReaderLoadOperation;
    class DataWriter;
    class DataWriterStoreOperation;
    class FileInputStream;
    class FileOutputStream;
    class FileRandomAccessStream;
    class InMemoryRandomAccessStream;
    class InputStreamOverStream;
    class OutputStreamOverStream;
    class RandomAccessStreamOverStream;
    class RandomAccessStreamReference;
    enum InputStreamOptions : unsigned int;
    interface IBuffer;
    interface IBufferFactory;
    interface IBufferStatics;
    interface IContentTypeProvider;
    interface IDataReader;
    interface IDataReaderFactory;
    interface IDataReaderStatics;
    interface IDataWriter;
    interface IDataWriterFactory;
    interface IFileRandomAccessStreamStatics;
    interface IInputStream;
    interface IInputStreamReference;
    interface IOutputStream;
    interface IRandomAccessStream;
    interface IRandomAccessStreamReference;
    interface IRandomAccessStreamReferenceStatics;
    interface IRandomAccessStreamStatics;
    interface IRandomAccessStreamWithContentType;

    using IStreamReadOperation = wf::IAsyncOperationWithProgress<IBuffer *, UINT32>;
    using IStreamReadCompletedEventHandler = wf::IAsyncOperationWithProgressCompletedHandler<IBuffer*, UINT32>;
} // Streams
} // Storage
} // Windows
XAML_ABI_NAMESPACE_END
