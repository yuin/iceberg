#include "ib_migemo.h"
#include "ib_platform.h"
#include "ib_config.h"
#include "ib_singleton.h"

const unsigned int ib::Migemo::MIN_LENGTH = 3;

void ib::Migemo::init() {
  ib::Error error;
  ib::unique_oschar_ptr name(ib::platform::utf82oschar("migemo"));
  if(ib::platform::load_library(dl_, name.get(), error) == 0){
    has_migemo_ = true;
    _open = (migemo* (*)(const char*))ib::platform::get_dynamic_symbol(dl_, "migemo_open");
    _close = (void (*)(migemo*))ib::platform::get_dynamic_symbol(dl_, "migemo_close");
    _query = (unsigned char* (*)(migemo*, const unsigned char*))ib::platform::get_dynamic_symbol(dl_, "migemo_query");
    _release = (void (*)(migemo*, unsigned char*))ib::platform::get_dynamic_symbol(dl_, "migemo_release");
    _load = (int (*)(migemo*, int, const char*))ib::platform::get_dynamic_symbol(dl_, "migemo_load");
    _isEnable = (int (*)(migemo*))ib::platform::get_dynamic_symbol(dl_, "migemo_is_enable");
    _setOperator = (int (*)(migemo*, int, const unsigned char*))ib::platform::get_dynamic_symbol(dl_, "migemo_set_operator");
    _getOperator = (const unsigned char* (*)(migemo*, int))ib::platform::get_dynamic_symbol(dl_, "migemo_get_operator");
    _setprocChar2int = (void (*)(migemo*, MIGEMO_PROC_CHAR2INT))ib::platform::get_dynamic_symbol(dl_, "migemo_setproc_char2int");
    _setprocInt2char = (void (*)(migemo*, MIGEMO_PROC_INT2CHAR))ib::platform::get_dynamic_symbol(dl_, "migemo_setproc_int2char");

    migemo_ = _open(0);
    if(migemo_ != 0){
      const std::string &dict_dir = ib::Singleton<ib::Config>::getInstance()->getMigemoDictPath();
      ib::oschar  osdict_dir[IB_MAX_PATH];
      ib::platform::utf82oschar_b(osdict_dir, IB_MAX_PATH, dict_dir.c_str());
      ib::oschar  osdict_fullpath[IB_MAX_PATH];
      ib::oschar  osname[IB_MAX_PATH];
      char fullpath[IB_MAX_PATH_BYTE];

      ib::platform::utf82oschar_b(osname, IB_MAX_PATH, "migemo-dict");
      ib::platform::join_path(osdict_fullpath, osdict_dir, osname);
      ib::platform::oschar2local_b(fullpath, IB_MAX_PATH_BYTE, osdict_fullpath);
      _load(migemo_, MIGEMO_DICTID_MIGEMO, fullpath);

      ib::platform::utf82oschar_b(osname, IB_MAX_PATH, "han2zen.dat");
      ib::platform::join_path(osdict_fullpath, osdict_dir, osname);
      ib::platform::oschar2local_b(fullpath, IB_MAX_PATH_BYTE, osdict_fullpath);
      _load(migemo_, MIGEMO_DICTID_HAN2ZEN, fullpath);

      ib::platform::utf82oschar_b(osname, IB_MAX_PATH, "hira2kata.dat");
      ib::platform::join_path(osdict_fullpath, osdict_dir, osname);
      ib::platform::oschar2local_b(fullpath, IB_MAX_PATH_BYTE, osdict_fullpath);
      _load(migemo_, MIGEMO_DICTID_HIRA2KATA, fullpath);

      ib::platform::utf82oschar_b(osname, IB_MAX_PATH, "roma2hira.dat");
      ib::platform::join_path(osdict_fullpath, osdict_dir, osname);
      ib::platform::oschar2local_b(fullpath, IB_MAX_PATH_BYTE, osdict_fullpath);
      _load(migemo_, MIGEMO_DICTID_ROMA2HIRA, fullpath);

      ib::platform::utf82oschar_b(osname, IB_MAX_PATH, "zen2han.dat");
      ib::platform::join_path(osdict_fullpath, osdict_dir, osname);
      ib::platform::oschar2local_b(fullpath, IB_MAX_PATH_BYTE, osdict_fullpath);
      _load(migemo_, MIGEMO_DICTID_ZEN2HAN, fullpath);
    }

  }else{
    has_migemo_ = false;
  };
}

ib::Migemo::~Migemo() {
  if(migemo_ != 0) { _close(migemo_); }
  if(dl_ != 0) { ib::platform::close_library(dl_); }
}
