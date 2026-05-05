#include "client/local.hpp"
#include "utils/misc.hpp"
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