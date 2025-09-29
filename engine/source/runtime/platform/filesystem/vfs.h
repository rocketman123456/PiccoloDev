#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Piccolo
{
    namespace vfs
    {

        enum class Status
        {
            OK             = 0,
            Failed         = -1,
            PathNotFound   = -2,
            NotImplemented = -3
        };

        // A Blob is a package for untyped data, typically read from a file
        class IBlob
        {
        public:
            virtual ~IBlob() = default;

            [[nodiscard]] virtual const void* data() const = 0;
            [[nodiscard]] virtual size_t      size() const = 0;

            static bool isEmpty(const IBlob& blob) { return blob.data() != nullptr && blob.size() != 0; }
        };

        // Specific Blob implementation that owns the data and frees it when deleted
        class Blob : public IBlob
        {
        public:
            Blob(void* data, size_t size);
            ~Blob() override;

            [[nodiscard]] const void* data() const override;
            [[nodiscard]] size_t      size() const override;

        private:
            void*  data_ = nullptr;
            size_t size_ = 0;
        };

        using enumerate_callback_t = const std::function<void(std::string_view)>&;

        inline std::function<void(std::string_view)> enumerate_to_vector(std::vector<std::string>& v)
        {
            return [&v](std::string_view s) { v.push_back(std::string(s)); };
        }

        // Basic interface for the virtual file system
        class IFileSystem
        {
        public:
            virtual ~IFileSystem() = default;

            virtual std::filesystem::path getFullPath(const std::filesystem::path& name) const = 0;

            // Test if a folder exists
            virtual bool isFolderExists(const std::filesystem::path& name) = 0;

            // Test if a file exists
            virtual bool isFileExists(const std::filesystem::path& name) = 0;

            // Read the entire file.
            // Returns nullptr if the file cannot be read
            virtual std::shared_ptr<IBlob> readFile(const std::filesystem::path& name) = 0;

            // Write the entire file
            // Returns false if the file cannot be written
            virtual bool writeFile(const std::filesystem::path& name, const void* data, size_t size) = 0;

            // Search for files with any of the provided 'extensions' in 'path'.
            // Extensions should not include any wildcard characters.
            // Returns the number of files found, or a negative number on errors - see muggle::vfs::Status.
            // The file names, relative to the 'path', are passed to 'callback' in no particular order.
            virtual int enumerateFiles(
                const std::filesystem::path&    path,
                const std::vector<std::string>& extensions,
                enumerate_callback_t            callback,
                bool                            allowDuplicates = false
            ) = 0;

            // Search for directories in 'path'.
            // Returns the number of directories found, or a negative number on errors - see muggle::vfs::Status.
            // The directory names, relative to the 'path', are passed to 'callback' in no particular order.
            virtual int enumerateDirectories(const std::filesystem::path& path, enumerate_callback_t callback, bool allowDuplicates = false) = 0;
        };

        // An implementation of virtual file system that directly maps to the OS files
        class NativeFileSystem : public IFileSystem
        {
        public:
            std::filesystem::path getFullPath(const std::filesystem::path& name) const override { return name; }

            bool                   isFolderExists(const std::filesystem::path& name) override;
            bool                   isFileExists(const std::filesystem::path& name) override;
            std::shared_ptr<IBlob> readFile(const std::filesystem::path& name) override;
            bool                   writeFile(const std::filesystem::path& name, const void* data, size_t size) override;
            int                    enumerateFiles(
                                   const std::filesystem::path&    path,
                                   const std::vector<std::string>& extensions,
                                   enumerate_callback_t            callback,
                                   bool                            allowDuplicates /* = false */
                               ) override;
            int enumerateDirectories(const std::filesystem::path& path, enumerate_callback_t callback, bool allowDuplicates /* = false */) override;
        };

        // A layer that represents some path in the underlying file system as an entire FS.
        // Effectively, just prepends the provided base path to every file name and passes the requests
        // to the underlying FS
        class RelativeFileSystem : public IFileSystem
        {
        public:
            RelativeFileSystem(std::shared_ptr<IFileSystem> fs, const std::filesystem::path& basePath);

            [[nodiscard]] const std::filesystem::path& getBasePath() const { return basePath_; }

            std::filesystem::path getFullPath(const std::filesystem::path& name) const override { return basePath_ / name.relative_path(); }

            bool                   isFolderExists(const std::filesystem::path& name) override;
            bool                   isFileExists(const std::filesystem::path& name) override;
            std::shared_ptr<IBlob> readFile(const std::filesystem::path& name) override;
            bool                   writeFile(const std::filesystem::path& name, const void* data, size_t size) override;
            int                    enumerateFiles(
                                   const std::filesystem::path&    path,
                                   const std::vector<std::string>& extensions,
                                   enumerate_callback_t            callback,
                                   bool                            allowDuplicates /* = false */
                               ) override;
            int enumerateDirectories(const std::filesystem::path& path, enumerate_callback_t callback, bool allowDuplicates /* = false */) override;

        private:
            std::shared_ptr<IFileSystem> underlyingFS_;
            std::filesystem::path        basePath_;
        };

        // A virtual file system that allows mounting, or attaching, other VFS objects to paths.
        // Does not have any file systems by default, all of them must be mounted first.
        class VFileSystem : public IFileSystem
        {
        public:
            std::filesystem::path getFullPath(const std::filesystem::path& name) const;

            void mount(const std::filesystem::path& path, std::shared_ptr<IFileSystem> fs);
            void mount(const std::filesystem::path& path, const std::filesystem::path& nativePath);
            bool unmount(const std::filesystem::path& path);

            bool                   isFolderExists(const std::filesystem::path& name) override;
            bool                   isFileExists(const std::filesystem::path& name) override;
            std::shared_ptr<IBlob> readFile(const std::filesystem::path& name) override;
            bool                   writeFile(const std::filesystem::path& name, const void* data, size_t size) override;
            int                    enumerateFiles(
                                   const std::filesystem::path&    path,
                                   const std::vector<std::string>& extensions,
                                   enumerate_callback_t            callback,
                                   bool                            allowDuplicates /* = false */
                               ) override;
            int enumerateDirectories(const std::filesystem::path& path, enumerate_callback_t callback, bool allowDuplicates /* = false */) override;

        private:
            bool findMountPoint(const std::filesystem::path& path, std::filesystem::path* pRelativePath, IFileSystem** ppFS) const;

            std::vector<std::pair<std::string, std::shared_ptr<IFileSystem>>> mountPoints_;
        };

        std::string getFileSearchRegex(const std::filesystem::path& path, const std::vector<std::string>& extensions);

        // utility function to get the path to the executable
        std::filesystem::path getCurrentProcessDirectory();

    } // namespace vfs

} // namespace Piccolo