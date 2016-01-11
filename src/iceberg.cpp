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


static void ib_window_callback(Fl_Widget* w){ // {{{
  // disable default escape key behaviour
  if(Fl::event() ==  FL_SHORTCUT && Fl::event_key() == FL_Escape){
  }else{
    ib::utils::exit_application();
  }
} // }}}

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

int main(int argc, char **argv) { // {{{
  Fl::lock();
  fl_message_hotspot(0);
  auto &cfg = ib::Config::inst();
  setlocale(LC_ALL, "");

  if(ib::platform::startup_system() != 0) {
    std::cerr << "Failed to start an application." << std::endl;
    exit(1);
  }

  ib::Controller::inst().loadConfig(argc, argv);
  if(cfg.getOldPid() > -1){
    if(ib::platform::wait_pid(cfg.getOldPid()) != 0){
      fl_alert("Failed to reboot iceberg.");
      ib::utils::exit_application(1);
    }
  }

  if(!cfg.getIpcMessage().empty()){
    const auto code = ib::utils::ipc_message(cfg.getIpcMessage());
    ib::platform::finalize_system();
    exit(code);
  }

  ib::Controller::inst().initFonts();
  ib::Controller::inst().initBoxtypes();

  Fl::visual(FL_DOUBLE|FL_INDEX);
  ib::MainWindow::init();
  ib::MainWindow::inst()->initLayout();
  ib::ListWindow::init();
  ib::ListWindow::inst()->initLayout();
  ib::MainWindow::inst()->callback(ib_window_callback);
  ib::ListWindow::inst()->callback(ib_window_callback);

  ib::MainWindow::inst()->show();
  ib::ListWindow::inst()->show();

  if(ib::platform::init_system() < 0) {
    fl_alert("Failed to initialize the application.(init_system)");
    ib::utils::exit_application(1);
  }
  ib::Error error;
  if(ib::Server::inst().start(error) != 0){
    fl_alert("%s", error.getMessage().c_str());
    ib::utils::exit_application(1);
  }

  ib::IconManager::init();
  ib::unique_oschar_ptr oscache_path(ib::platform::utf82oschar(cfg.getCommandCachePath().c_str()));
  if(ib::platform::file_exists(oscache_path.get())){
    ib::Controller::inst().loadCachedCommands();
  }
  ib::History::inst().load();
  if(ib::Config::inst().getEnableIcons()){
    ib::IconManager::inst()->load();
    auto &event = ib::IconManager::inst()->getLoaderEvent();
    event.setMs(1);
    event.startThread();
  }
  ib::Migemo::inst().init();

  auto &event = ib::MainWindow::inst()->getInput()->getKeyEvent();
  event.setMs(ib::Config::inst().getKeyEventThreshold());
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

  int flag = false;
  while (Fl::wait() > 0) {
#ifdef WIN32 
    Sleep(5);
#endif
    if(!flag){
      ib::ListWindow::inst()->hide();
      ib::Controller::inst().showApplication();
      flag = true;
    }
  }
  ib::utils::exit_application(0);
  return 0;
} // }}}

