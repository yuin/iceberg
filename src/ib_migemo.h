#ifndef __IB_MIGEMO_H__
#define __IB_MIGEMO_H__

#include "ib_constants.h"
#include "ib_utils.h"
#include "ib_platform.h"
#include "ib_server.h"

namespace ib{
  class Migemo : private NonCopyable<Migemo> { // {{{
    friend class ib::Singleton<Migemo>;
    public:
      const static unsigned int MIN_LENGTH;
      ~Migemo();

      void init();
      bool hasMigemo() const { return has_migemo_; }
      migemo* get() { return migemo_; }
      bool isEnable() const { return has_migemo_ && _isEnable(migemo_) != 0;}
      unsigned char* query(const unsigned char* query) { return _query(migemo_, query);}
      void release(unsigned char *string) { _release(migemo_, string); }

      migemo* (*_open)(const char* dict);
      void (*_close)(migemo* object);
      unsigned char* (*_query)(migemo* object, const unsigned char* query);
      void (*_release)(migemo* object, unsigned char* string);
      int (*_load)(migemo* obj, int dict_id, const char* dict_file);
      int (*_isEnable)(migemo* obj);
      int (*_setOperator)(migemo* object, int index, const unsigned char* op);
      const unsigned char* (*_getOperator)(migemo* object, int index);
      void (*_setprocChar2int)(migemo* object, MIGEMO_PROC_CHAR2INT proc);
      void (*_setprocInt2char)(migemo* object, MIGEMO_PROC_INT2CHAR proc);


    protected:
      Migemo() : has_migemo_(false), dl_(0), migemo_(0) {}

      bool has_migemo_;
      ib::module dl_;
      migemo *migemo_;
   }; // }}}
}
#endif
