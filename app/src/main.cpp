#include <borealis.hpp>

#include "utils/config.hpp"
#include "utils/download.hpp"
#include "utils/thread.hpp"
#include "api/analytics.hpp"

#include "view/svg_image.hpp"
#include "view/custom_button.hpp"
#include "view/auto_tab_frame.hpp"
#include "view/recycling_grid.hpp"
#include "view/h_recycling.hpp"
#include "view/recyling_video.hpp"
#include "view/video_progress_slider.hpp"
#include "view/gallery_view.hpp"
#include "view/search_list.hpp"
#include "view/video_view.hpp"
#include "view/selector_cell.hpp"
#include "view/button_close.hpp"
#include "view/text_box.hpp"
#include "view/mpv_core.hpp"

#include "activity/main_activity.hpp"
#include "activity/server_list.hpp"
#include "activity/hint_activity.hpp"
#include "tab/server_add.hpp"
#include "tab/home_tab.hpp"
#include "tab/media_folder.hpp"
#include "tab/search_tab.hpp"
#include "tab/remote_tab.hpp"
#include "tab/remote_view.hpp"
#include "tab/setting_tab.hpp"

#if defined(__SDL2__)
#include <SDL2/SDL_main.h>
#endif

#ifdef __WINRT_NEW__
#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include <winrt/windows.applicationmodel.activation.h>
#include <borealis/platforms/winrt/winrt_app.hpp>
#endif

using namespace brls::literals;  // for _i18n

int main(int argc, char* argv[]) {
    std::vector<std::string> items;

#ifdef __WINRT_NEW__
    setlocale (LC_ALL, ".utf8");
    setlocale (LC_NUMERIC, "C");

#if _DEBUG
    if (IsDebuggerPresent())
    {
        brls::Logger::getLogEvent()->subscribe([](brls::Logger::TimePoint now, brls::LogLevel level, const std::string& log) {
            auto message=std::format(L"[{}] {}\n", (int)level, winrt::to_hstring(log));
            OutputDebugString(message.c_str());
            });
    }
#endif

    //for xbox 
    winrt::Windows::UI::Core::SystemNavigationManager::GetForCurrentView ().BackRequested ([] (
        winrt::Windows::Foundation::IInspectable const,
        winrt::Windows::UI::Core::BackRequestedEventArgs const& args
        ) {
            args.Handled (true);
        });

    bool enableDebug=false;
#if _DEBUG
    enableDebug=true;
#endif 

    //TODO
    for (int i = 1; i < argc; i++) {
        if (std::strcmp (argv[i], "-d") == 0) {
            enableDebug=true;
            brls::Application::enableDebuggingView(true);
        } else {
            items.push_back (argv[i]);
        }
    }

    if (enableDebug)
    {
        brls::Logger::setLogLevel(brls::LogLevel::LOG_DEBUG);
        auto appLocal=winrt::Windows::Storage::AppDataPaths::GetDefault().LocalAppData();
        auto const time=std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
        auto logFile=std::format("{}\\switchfin.{:%Y-%m-%d-%H-%M-%S}.log", winrt::to_string(appLocal), time);
        brls::Logger::setLogOutput(std::fopen(logFile.c_str(), "w+"));
    }

#else

    for (int i = 1; i < argc; i++) {
        if (std::strcmp (argv[i], "-d") == 0) {
            brls::Logger::setLogLevel (brls::LogLevel::LOG_DEBUG);
        } else if (std::strcmp (argv[i], "-v") == 0) {
            brls::Application::enableDebuggingView (true);
        } else if (std::strcmp (argv[i], "-t") == 0) {
            MPVCore::DEBUG = true;
        } else if (std::strcmp (argv[i], "-o") == 0) {
            const char* path = (i + 1 < argc) ? argv[++i] : "switchfin.log";
            brls::Logger::setLogOutput (std::fopen (path, "w+"));
        } else if (std::strcmp (argv[i], "-version") == 0) {
            brls::Logger::info ("{} {}", AppVersion::getDeviceName (), AppVersion::getCommit ());
            return 0;
        } else {
            items.push_back (argv[i]);
        }
    }
#endif

    std::setlocale(LC_ALL, "C.UTF-8");
    // Load cookies and settings
    auto& conf = AppConfig::instance();
    if (!conf.init()) {
        return 0;
    }

    // Init the app and i18n
    if (!brls::Application::init()) {
        brls::Logger::error("Unable to init application");
        return EXIT_FAILURE;
    }

    conf.initThemes();
    DownloadManager::instance().init();

    // Return directly to the desktop when closing the application (only for NX)
    brls::Application::getPlatform()->exitToHomeMode(true);

    brls::Application::createWindow(fmt::format("{} for {}", AppVersion::getPackageName(), AppVersion::getPlatform()));

    // Have the application register an action on every activity that will quit when you press BUTTON_START
    brls::Application::setGlobalQuit(false);

    // Register custom views (including tabs, which are views)
    brls::Application::registerXMLView("SVGImage", SVGImage::create);
    brls::Application::registerXMLView("CustomButton", CustomButton::create);
    brls::Application::registerXMLView("SelectorCell", SelectorCell::create);
    brls::Application::registerXMLView("TextBox", TextBox::create);
    brls::Application::registerXMLView("ButtonClose", ButtonClose::create);
    brls::Application::registerXMLView("AutoTabFrame", AutoTabFrame::create);
    brls::Application::registerXMLView("RecyclingGrid", RecyclingGrid::create);
    brls::Application::registerXMLView("HRecyclerFrame", HRecyclerFrame::create);
    brls::Application::registerXMLView("RecylingVideo", RecylingVideo::create);
    brls::Application::registerXMLView("GalleryView", GalleryView::create);
    brls::Application::registerXMLView("SearchList", SearchList::create);
    brls::Application::registerXMLView("VideoProgressSlider", VideoProgressSlider::create);

    brls::Application::registerXMLView("HomeTab", HomeTab::create);
    brls::Application::registerXMLView("MediaFolders", MediaFolders::create);
    brls::Application::registerXMLView("SearchTab", SearchTab::create);
    brls::Application::registerXMLView("RemoteTab", RemoteTab::create);
    brls::Application::registerXMLView("SettingTab", SettingTab::create);

    if (!brls::Application::getPlatform()->isApplicationMode()) {
        brls::Application::pushActivity(new HintActivity());
    } else if (items.size() > 0) {
        RemoteView::play(items.front());
    } else if (!conf.checkLogin()) {
        brls::Application::pushActivity(new ServerList());
    } else {
        brls::Application::pushActivity(new MainActivity());
    }

    GA("open_app",
        {
            {"version", AppVersion::getVersion()},
            {"language", brls::Application::getLocale()},
            {"resolution", fmt::format("{}x{}", brls::Application::windowWidth, brls::Application::windowHeight)},
        })

    std::string v = conf.getItem(AppConfig::APP_UPDATE, std::string("NaN"));
    if (AppVersion::getVersion().compare(v)) AppVersion::checkUpdate();

    // Run the app
    while (brls::Application::mainLoop());

    ThreadPool::instance().stop();

    conf.checkRestart(argv);
    // Exit
    return EXIT_SUCCESS;
}

#ifdef __WINRT_NEW__
int __stdcall wWinMain (HINSTANCE, HINSTANCE, PWSTR szCmdLine, int)
{
    (void)szCmdLine;
    return WinrtApp::RunApp (main);
}
#endif
