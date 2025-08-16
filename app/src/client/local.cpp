#include "client/local.hpp"

#ifdef USE_BOOST_FILESYSTEM
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#elif __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#elif __has_include("experimental/filesystem")
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#error "Failed to include <filesystem> header!"
#endif
#if defined(_WINRT_)
#include <winrt/windows.foundation.h>
#endif
namespace remote {

std::vector<DirEntry> Local::list(const std::string& path) {
    std::vector<DirEntry> s = {{EntryType::UP}};

#if defined(_WINRT_)
    std::string p;
    //file:///c:/xx no host
    if (path.starts_with ("file:///")) { 
        p = path.substr (8);
    }
    //file://server/other/ 
    else if (path.starts_with ("file://")) {
        p = path.substr (7);
    } else {
        p = path;
    }
#elif
    std::string p = path.rfind ("file://") == 0 ? path.substr (7) : path;
#endif
    auto it = fs::directory_iterator(p);
    for (const auto& fp : it) {
        DirEntry item;
        auto& p = fp.path();
        item.name = p.filename().string();
        item.path = p.string();
        if (fs::is_directory(fp)) {
            item.type = EntryType::DIR;
        } else {
            item.type = EntryType::FILE;
            item.fileSize = fs::file_size(p);
        }
        s.push_back(item);
    }
    return s;
}

}  // namespace remote