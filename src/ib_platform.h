#ifndef __IB_PLATFORM_H__
#define __IB_PLATFORM_H__

#include "ib_constants.h"
#include "ib_utils.h"

#ifdef IB_OS_WIN
#  include "ib_platform_win.h"
#endif

#include "ib_comp_value.h"

namespace ib {
  namespace platform {
    int  startup_system();
    int  init_system();
    void finalize_system();
    void get_runtime_platform(char *result);
    /* string conversion functions */
    ib::oschar* utf82oschar(const char *src);
    void utf82oschar_b(ib::oschar *buf, const unsigned int bufsize, const char *src);
    char* oschar2utf8(const ib::oschar *src);
    void oschar2utf8_b(char *buf, const unsigned int bufsize, const ib::oschar *src);
    char* oschar2local(const ib::oschar *src);
    void oschar2local_b(char *buf, const unsigned int bufsize, const ib::oschar *src);
    char* utf82local(const char *src);
    char* local2utf8(const char *src);
    ib::oschar* quote_string(ib::oschar *result, const ib::oschar *str);

    /* application functions */
    void hide_window(Fl_Window *window);
    void show_window(Fl_Window *window);
    void set_window_alpha(Fl_Window *window, int alpha);
    int  shell_execute(const std::string &path, const std::vector<ib::unique_string_ptr> &params, const std::string &cwd, ib::Error& error);
    int  shell_execute(const std::string &path, const std::vector<std::string*> &params, const std::string &cwd, ib::Error& error);
    int command_output(std::string &sstdout, std::string &sstderr, const char *command, ib::Error &error);
    int ib_key2os_key(const int key);
    int ib_modkey2os_modkey(const int key);
    void list_all_windows(std::vector<ib::whandle> &result);
    int show_context_menu(ib::oschar *path);
    void on_command_init(ib::Command *command);

    /* path functions */
    Fl_RGB_Image* get_associated_icon_image(const ib::oschar *path, const int size);
    ib::oschar* join_path(ib::oschar *result, const ib::oschar *parent, const ib::oschar *child);
    ib::oschar* normalize_path(ib::oschar *result, const ib::oschar *path);
    ib::oschar* normalize_join_path(ib::oschar *result, const ib::oschar *parent, const ib::oschar *child);
    ib::oschar* dirname(ib::oschar *result, const ib::oschar *path);
    ib::oschar* basename(ib::oschar *result, const ib::oschar *path);
    ib::oschar* to_absolute_path(ib::oschar *result, const ib::oschar *dir, const ib::oschar *path);
    bool is_directory(const ib::oschar *path);
    bool is_path(const ib::oschar *path);
    bool is_relative_path(const ib::oschar *path);
    bool directory_exists(const ib::oschar *path);
    bool file_exists(const ib::oschar *path);
    bool path_exists(const ib::oschar *path);
    int walk_dir(std::vector<ib::unique_oschar_ptr> &result, const ib::oschar *dir, ib::Error &error, bool recursive = false);
    ib::oschar* get_self_path(ib::oschar *result);
    ib::oschar* get_current_workdir(ib::oschar *result);
    int set_current_workdir(ib::oschar *dir, ib::Error &error);
    bool which(ib::oschar *result, const ib::oschar *name);
    int list_drives(std::vector<ib::unique_oschar_ptr> &result, ib::Error &error);

    /* filesystem functions */
    int remove_file(const ib::oschar *path, ib::Error &error);
    int copy_file(const ib::oschar *source, const ib::oschar *dest, ib::Error &error);
    int file_size(size_t &size, const ib::oschar *path, ib::Error &error);

    /* thread functions */
    void create_thread(ib::thread *t, ib::threadfunc f, void* p);
    void on_thread_start();
    void join_thread(ib::thread *t);
    void exit_thread(int exit_code);
    void create_mutex(ib::mutex *m);
    void destroy_mutex(ib::mutex *m);
    void acquire_lock(ib::mutex *m);
    void release_lock(ib::mutex *m);
    void create_condition(ib::condition *c);
    void destroy_condition(ib::condition *c);
    bool wait_condition(ib::condition *c, int ms);
    void notify_condition(ib::condition *c);
    void reset_condition(ib::condition *c);

    /* process functions */
    int wait_pid(const int pid);
    int get_pid();

    /* dynamic loading functions */
    int load_library(ib::module &dl, const ib::oschar *name, ib::Error &error);
    void* get_dynamic_symbol(ib::module dl, const char *name);
    void close_library(ib::module dl);

    /* socket functions */
    void close_socket(FL_SOCKET s);

    /* system functions */
    int get_num_of_cpu();

    class ScopedLock : private NonCopyable<ScopedLock> { // {{{
      public:
        ScopedLock(ib::mutex &mutex) : mutex_(mutex) { ib::platform::acquire_lock(&mutex_); }
        ~ScopedLock(){ ib::platform::release_lock(&mutex_); }
      protected:
        ib::mutex mutex_;
    }; // }}}

    class ScopedCondition : private NonCopyable<ScopedCondition> { // {{{
      public:
        ScopedCondition(ib::condition &condition, int ms) : condition_(condition), timeout_(false) { timeout_ = !ib::platform::wait_condition(&condition_, ms); }
        ~ScopedCondition(){ ib::platform::reset_condition(&condition_); }
        bool timeout() const { return timeout_; }

      protected:
        ib::condition condition_;
        bool timeout_;

    }; // }}}

  }
}

#endif
