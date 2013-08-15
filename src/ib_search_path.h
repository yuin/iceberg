#ifndef __IB_SEARCH_PATH_H__
#define __IB_SEARCH_PATH_H__

#include "ib_constants.h"
#include "ib_utils.h"

namespace ib {

  class SearchPath : private NonCopyable<SearchPath> { // {{{
    public:
      SearchPath(const char* path) : path_(path), category_("default"), depth_(0), pattern_("*"), exclude_pattern_("") {}
      const std::string& getPath() const { return path_; }
      void setPath(const std::string &value){ path_ = value; }
      const std::string& getCategory() const { return category_; }
      void setCategory(const std::string &value){ category_ = value; }
      void setCategory(const char *value){ category_ = value; }
      const unsigned int& getDepth() const { return depth_; }
      void setDepth(const unsigned int value){ depth_ = value; }
      const std::string& getPattern() const { return pattern_; }
      void setPattern(const std::string &value){ pattern_ = value; }
      void setPattern(const char *value){ pattern_ = value; }
      const std::string& getExcludePattern() const { return exclude_pattern_; }
      void setExcludePattern(const std::string &value){ exclude_pattern_ = value; }
      void setExcludePattern(const char *value){ exclude_pattern_ = value; }

    protected:
      std::string path_;
      std::string category_;
      unsigned int depth_;
      std::string pattern_;
      std::string exclude_pattern_;
  }; // }}}

}

#endif
