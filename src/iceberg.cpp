#define IB_PUBLIC 1
#include "iceberg.h"
#include "ib_constants.h"
#include "ib_utils.h"
#include "ib_config.h"
#include "ib_ui.h"
#include "ib_controller.h"
#include "ib_lua.h"
#include "ib_platform.h"
#include "ib_icon_manager.h"
#include "ib_history.h"
#include "ib_migemo.h"
#include "ib_server.h"
#include "ib_singleton.h"
#include "ib_regex.h"

#ifdef __GNUC__
__attribute__ ((destructor)) void after_main() { // {{{
#ifdef DEBUG
#ifdef IB_OS_WIN
  // simple memory leaak check tool...
  std::cout << "\n\n=====================================" << std::endl;
  long ignore = 12;
  if(ib::utils::malloc_count() > ignore) {
    std::cout << "memory leak!(" << ib::utils::malloc_count()-ignore << ")" << std::endl;
  }else if(ib::utils::malloc_count() < 0){
    std::cout << "invalid count(" << ib::utils::malloc_count()-ignore << ")" << std::endl;
  }
  std::cout << "=====================================" << std::endl;
#endif
#endif
} // }}}

__attribute__ ((constructor)) void pre_main() { // {{{
#ifdef DEBUG
#ifdef IB_OS_WIN
#endif
#endif
} // }}}
#endif

static void init_common_singletons() {
  ib::Singleton<ib::NullToken>::initInstance();
  ib::Singleton<ib::MainLuaState>::initInstance();
  ib::Singleton<ib::Config>::initInstance();
  ib::Singleton<ib::Controller>::initInstance();
  ib::Singleton<ib::Completer>::initInstance();
}

static void init_gui_singletons() {
  ib::Singleton<ib::MainWindow>::initInstance()->init();
  ib::Singleton<ib::ListWindow>::initInstance()->init();
  ib::Singleton<ib::MessageBox>::initInstance()->init();
  ib::Singleton<ib::Server>::initInstance();
  ib::Singleton<ib::IconManager>::initInstance();
  ib::Singleton<ib::History>::initInstance();
  ib::Singleton<ib::Migemo>::initInstance()->init();
}

int main(int argc, char **argv) { // {{{
  Fl::lock();
  fl_message_hotspot(0);
  ib::OnigrumaService::init();

  init_common_singletons();
  const auto cfg = ib::Singleton<ib::Config>::getInstance();
  const auto controller = ib::Singleton<ib::Controller>::getInstance();
  setlocale(LC_ALL, "");
  controller->loadConfig(argc, argv);

  if(ib::platform::startup_system() != 0) {
    std::cerr << "Failed to start an application." << std::endl;
    ib::SingletonFinalizer::finalize();
    exit(1);
  }

  if(cfg->getOldPid() > -1){
    if(ib::platform::wait_pid(cfg->getOldPid()) != 0){
      fl_alert("Failed to reboot iceberg.");
      ib::utils::exit_application(1);
    }
  }

  if(!cfg->getIpcMessage().empty()){
    const auto code = ib::utils::ipc_message(cfg->getIpcMessage());
    ib::platform::finalize_system();
    ib::SingletonFinalizer::finalize();
    exit(code);
  }

  controller->initFonts();
  controller->initBoxtypes();

  Fl::visual(FL_DOUBLE|FL_RGB);
  init_gui_singletons();

  const auto mainwin = ib::Singleton<ib::MainWindow>::getInstance();
  const auto listwin = ib::Singleton<ib::ListWindow>::getInstance();
  const auto messagebox = ib::Singleton<ib::MessageBox>::getInstance();
  const auto server = ib::Singleton<ib::Server>::getInstance();
  const auto icon_manager = ib::Singleton<ib::IconManager>::getInstance();
  const auto history = ib::Singleton<ib::History>::getInstance();

  mainwin->show();
  listwin->show();
  mainwin->wait_for_expose();
  listwin->wait_for_expose();
  messagebox->wait_for_expose();

  if(ib::platform::init_system() < 0) {
    fl_alert("Failed to initialize the application.(init_system)");
    ib::utils::exit_application(1);
  }


  ib::Error error;
  if(server->start(error) != 0){
    fl_alert("%s", error.getMessage().c_str());
    ib::utils::exit_application(1);
  }

  auto oscache_path = ib::platform::utf82oschar(cfg->getCommandCachePath().c_str());
  if(ib::platform::file_exists(oscache_path.get())){
    controller->loadCachedCommands();
  }

  history->load();
  if(cfg->getEnableIcons()){
    icon_manager->load();
    auto &event = icon_manager->getLoaderEvent();
    event.setMs(1);
    event.startThread();
  }

  auto &event = mainwin->getInput()->getKeyEvent();
  event.setMs(cfg->getKeyEventThreshold());
  event.startThread();
  lua_getglobal(IB_LUA, "on_initialize");
  if (lua_pcall(IB_LUA, 0, 1, 0)) {
      fl_alert("%s", lua_tostring(IB_LUA, lua_gettop(IB_LUA)));
      ib::utils::exit_application(1);
  }
  auto result = static_cast<int>(lua_tonumber(IB_LUA, 1));
  if(result != 0) {
    ib::utils::exit_application(result);
  }

  mainwin->getInput()->take_focus();
  int flag = false;
  while (Fl::wait() > 0) {
#ifdef WIN32 
    Sleep(5);
#endif
    if(!flag){
      listwin->hide();
      messagebox->hide();
      controller->showApplication();
      flag = true;
    }
  }
  ib::utils::exit_application(0);
  return 0;
} // }}}

