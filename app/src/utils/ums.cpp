#include "utils/ums.hpp"
#ifdef __WINRT__
#include <ppltasks.h>
#include <winrt/windows.storage.h>
#include <winrt/windows.applicationmodel.core.h>
#include <winrt/windows.foundation.collections.h>
#endif
#ifdef USE_LIBUSBHSFS
#include <usbhsfs.h>

int Ums::init() {
    Result rc = usbHsFsInitialize(0);
    if (R_FAILED(rc)) return rc;
    usbHsFsSetPopulateCallback(
        [](const UsbHsFsDevice *devices, u32 device_count, void *user_data) {
            auto *self = static_cast<Ums *>(user_data);
            DeviceList ndev;
            ndev.reserve(device_count + 1);
            ndev.push_back({.id = -1, .name = "SD Card", .mount = "sdmc:"});

            for (u32 i = 0; i < device_count; ++i) {
                auto &d = devices[i];

                std::string name;
                if (auto sv = std::string_view(d.product_name); !sv.empty())
                    name = sv;
                else if (sv = std::string_view(d.manufacturer); !sv.empty())
                    name = sv;
                else if (sv = std::string_view(d.serial_number); !sv.empty())
                    name = sv;
                else
                    name = "Unnamed device";
                ndev.push_back({.id = d.usb_if_id, .name = std::move(name), .mount = d.name});
            }
            self->devices = std::move(ndev);
            self->event.fire(self->devices);
        },
        this);

    if (!usbHsFsGetMountedDeviceCount()) {
        this->devices.push_back({.id = -1, .name = "SD Card", .mount = "sdmc:"});
    }

    brls::Application::getExitEvent()->subscribe([this]() {
        usbHsFsSetPopulateCallback(nullptr, nullptr);
        for (auto &dev : this->devices)
            if (dev.id >= 0) this->unmount(dev);
        this->devices.clear();
        usbHsFsExit();
    });
    return 0;
}

bool Ums::unmount(const Device &dev) {
    UsbHsFsDevice d = {.usb_if_id = dev.id};
    return usbHsFsUnmountDevice(&d, true);
}

#else

#if defined(__PSV__)

int Ums::init() {
    this->devices.push_back(Device{.id = -1, .name = "Memory Stock", .mount = "ux0:/data"});
    return 0;
}
#elif defined(_WINRT_)
int Ums::init () {
    concurrency::create_task ([&] {
        int id = -1;
        std::string appDataPath = winrt::to_string (winrt::Windows::Storage::AppDataPaths::GetDefault ().LocalAppData ());
        this->devices.push_back (Device{ .id = id--, .name = "Local Data", .mount = appDataPath });

        std::string videosPath = winrt::to_string (winrt::Windows::Storage::KnownFolders::VideosLibrary ().Path ());
        if (!videosPath.empty ()) {
            this->devices.push_back (Device{ .id = id--, .name = "Videos", .mount = videosPath });
        }
        std::string musicPath = winrt::to_string (winrt::Windows::Storage::KnownFolders::MusicLibrary ().Path ());
        if (!musicPath.empty ()) {
            this->devices.push_back (Device{ .id = id--, .name = "Music", .mount = musicPath });
        }

        auto removableDevices = winrt::Windows::Storage::KnownFolders::RemovableDevices ();
        if (removableDevices) {
            auto folders = removableDevices.GetFoldersAsync ().get ();
            for (auto const& folder : folders) {
                std::string name = winrt::to_string (folder.Name ());
                std::string path = winrt::to_string (folder.Path ());
                if (path.ends_with ("\\")) {
                    path = path.substr (0, path.size() - 1);
                }
                this->devices.push_back (Device{ .id = id--, .name = name, .mount = path });
            }
        }
        this->event.fire (this->devices);
        });

    return 0;
}
#elif defined(__PS4__)
int Ums::init() {
    this->devices.push_back(Device{.id = -1, .name = "HardDisk", .mount = "/data"});
    return 0;
}

#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>

int Ums::init() {
    WCHAR wpath[MAX_PATH];
    std::vector<char> lpath(MAX_PATH);
    SHGetSpecialFolderPathW(0, wpath, CSIDL_MYVIDEO, false);
    WideCharToMultiByte(CP_UTF8, 0, wpath, std::wcslen(wpath), lpath.data(), lpath.size(), nullptr, nullptr);
    this->devices.push_back({.id = -1, .name = lpath.data(), .mount = lpath.data()});
    return 0;
}

#else
int Ums::init() { return 0; }
#endif

bool Ums::unmount(const Device& dev) { return false; }

#endif