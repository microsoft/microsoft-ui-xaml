// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <XamlTailored.h>
#include <ppltasks.h>
#include <vector>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {

        static Platform::Object^ LoadXamlFileOnUIThread(Platform::String^ filename)
        {
            // In WinRT, our file IO is async, which can be a big pain if we're just doing a quick and dirty
            // load of a single file. Luckily, PPL merges nicely with the IAsyncOperation exposed by
            // WinRT, so we can greatly streamline grabbing the operation, subscribing to its completion, and
            // extracting the result! Up until the call to .wait(), we're still dealing with an async task
            // that may have already completed in the background, or may be waiting to run inline.

            // Note that reading the file doesn't need to happen on any particular thread,
            // but we're forbidden from blocking the UI thread via .wait().
            // Also, note the sweet type safety! :)

            Platform::Object^ loadedObject = nullptr;

            Event operationComplete(L"ReadTextCompleted");

            Concurrency::create_task(::Windows::Storage::StorageFile::GetFileFromPathAsync(filename))
                .then([&](::Windows::Storage::StorageFile^ file)
            {
                Concurrency::create_task(::Windows::Storage::FileIO::ReadTextAsync(file))
                    .then([&](Platform::String^ xamlContents)
                {
                    RunOnUIThread([&]()
                    {
                        loadedObject = xaml_markup::XamlReader::Load(xamlContents);
                    });

                    operationComplete.Set();
                });
            });

            // Loading sometimes takes a very large hit in IO.
            // Use 10min for these cases.
            operationComplete.WaitFor(10min);
            return loadedObject;
        }

        static ::Windows::Foundation::Collections::IVector<Platform::String^>^ ReadLinesFromFile(Platform::String^ filename)
        {
            ::Windows::Foundation::Collections::IVector<Platform::String^>^ loadedStrings = nullptr;

            Concurrency::create_task(::Windows::Storage::StorageFile::GetFileFromPathAsync(filename))
                .then([&loadedStrings](::Windows::Storage::StorageFile^ file)
            {
                Concurrency::create_task(::Windows::Storage::FileIO::ReadLinesAsync(file))
                    .then([&loadedStrings](::Windows::Foundation::Collections::IVector<Platform::String^>^ fileContents)
                {
                    loadedStrings = fileContents;
                }).wait();
            }).wait();

            return loadedStrings;
        }

        template<typename Iter>
        static std::vector<Platform::Object^> LoadXamlFilesOnUIThread(Iter begin, Iter end)
        {
            // This is similar to LoadSingleXamlFile, but shows off the ability to start
            // file loads async in one thread, and then wait for their result in another
            // by grabbing the task objects

            const size_t size = std::distance(begin, end);

            // Set up a vector to hold each loaded object
            std::vector<Platform::Object^> loadedObjects(size, nullptr);

            Event operationComplete;

            // Create a vector of tasks that handle loading and parsing the contents
            std::vector<Concurrency::task<void>> loadTasks;
            loadTasks.reserve(size);

            for (size_t index = 0; begin != end; ++index, ++begin)
            {
                Event loadComplete;
                // Load each file in parallel with the others, and in parallel with any parsing
                // that may be occurring on the UI thread
                loadTasks.push_back(Concurrency::create_task(::Windows::Storage::StorageFile::GetFileFromPathAsync(*begin))
                    .then([&](::Windows::Storage::StorageFile^ file)
                {
                    Concurrency::create_task(::Windows::Storage::FileIO::ReadTextAsync(file))
                        .then([&](Platform::String^ xamlContents)
                    {
                        RunOnUIThread([&]()
                        {
                            loadedObjects[index] = xaml_markup::XamlReader::Load(xamlContents);
                            loadComplete.Set();
                        });
                    });
                }));

                loadComplete.WaitForDefault();
            }

            // Wait for all the tasks to complete
            Concurrency::when_all(loadTasks.begin(), loadTasks.end())
                .then([&]
            {
                operationComplete.Set();
            });

            operationComplete.WaitForDefault();
            return loadedObjects;
        }

        static Platform::String^ GetPackageFolder()
        {
            auto deploymentDir = GetTestDeploymentDir();
            return ref new Platform::String(deploymentDir);
        }

        static wsts::IRandomAccessStream^ LoadBinaryFile(Platform::String ^fullPathName)
        {
            wsts::IRandomAccessStream^ result;
            Concurrency::create_task(wst::StorageFile::GetFileFromPathAsync(fullPathName))
                .then([](wst::StorageFile^ file)
            {
                return file->OpenAsync(wst::FileAccessMode::Read);
            })
                .then([&result](wsts::IRandomAccessStream^ stream)
            {
                result = stream;
            }).wait();
            return result;
        }
    }
} } } }
