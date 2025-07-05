#ifndef DL_HPP
#define DL_HPP

#include <string>
#include <unordered_map>

//
// This dynamic loader replaces the standard dlopen, dlsym, and dlclose functions.
// On initialize, the original functions are obtained via dlvsym (glibc exclusive)
// and made available under functions with the "o" prefix.
//
// Any call to regular dlopen, dlsym or dlclose is intercepted and may be
// overriden by registering a File override via `Loader::DL::registerFile`.
//

namespace Loader::DL {

    /// Dynamic loader override structure.
    class File {
    public:
        ///
        /// Create a dynamic loader override for a specific file.
        ///
        /// @param filename The name of the file to override.
        ///
        File(std::string filename)
            : filename(std::move(filename)) {}

        ///
        /// Append a symbol to the dynamic loader override.
        ///
        /// @param symbol The name of the symbol to add.
        /// @param address The address of the symbol.
        ///
        void defineSymbol(const std::string& symbol, void* address) {
            symbols[symbol] = address;
        }

        /// Get the filename
        [[nodiscard]] const std::string& getFilename() const { return filename; }
        /// Get all overriden symbols
        [[nodiscard]] const std::unordered_map<std::string, void*>& getSymbols() const { return symbols; }

        // Find a specific symbol
        [[nodiscard]] void* findSymbol(const std::string& symbol) const {
            auto it = symbols.find(symbol);
            return (it != symbols.end()) ? it->second : nullptr;
        }

        /// Get the fake handle
        [[nodiscard]] void* getHandle() const { return handle; }
        /// Get the real handle
        [[nodiscard]] void* getOriginalHandle() const { return handle_orig; }

        /// Set the fake handle
        void setHandle(void* new_handle) { handle = new_handle; }
        /// Set the real handle
        void setOriginalHandle(void* new_handle) { handle_orig = new_handle; }

        /// Copyable, moveable, default destructor
        File(const File&) = default;
        File(File&&) = default;
        File& operator=(const File&) = default;
        File& operator=(File&&) = default;
        ~File() = default;
    private:
        std::string filename;
        std::unordered_map<std::string, void*> symbols;

        void* handle = nullptr;
        void* handle_orig = nullptr;
    };

    ///
    /// Initialize the dynamic loader
    ///
    void initialize();

    ///
    /// Register a file with the dynamic loader.
    ///
    /// @param file The file to register.
    ///
    void registerFile(const File& file);

    ///
    /// Disable hooks temporarily. This may be useful
    /// when loading third-party libraries you wish not
    /// to hook.
    ///
    void disableHooks();

    ///
    /// Re-enable hooks after they were disabled.
    ///
    void enableHooks();

    ///
    /// Call the original dlopen function.
    ///
    /// @param filename The name of the file to open.
    /// @param flag The flags to use when opening the file.
    /// @return A handle to the opened file, or NULL on failure.
    ///
    void* odlopen(const char* filename, int flag);

    ///
    /// Call the original dlsym function.
    ///
    /// @param handle The handle to the opened file.
    /// @param symbol The name of the symbol to look up.
    /// @return A pointer to the symbol, or NULL on failure.
    ///
    void* odlsym(void* handle, const char* symbol);

    ///
    /// Call the original dlclose function.
    ///
    /// @param handle The handle to the opened file.
    /// @return 0 on success, or -1 on failure.
    ///
    int odlclose(void* handle);

}

/// Modified version of the dlopen function.
extern "C" void* dlopen(const char* filename, int flag) noexcept;
/// Modified version of the dlsym function.
extern "C" void* dlsym(void* handle, const char* symbol) noexcept;
/// Modified version of the dlclose function.
extern "C" int dlclose(void* handle) noexcept;

#endif // DL_HPP
