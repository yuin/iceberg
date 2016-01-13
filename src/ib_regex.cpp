#include "ib_regex.h"

std::string ib::Regex::escape(const char*str) { // {{{
  ib::Regex esc("([\\^\\.\\$\\|\\(\\)\\[\\]\\*\\+\\?\\/\\\\])", ib::Regex::NONE);
  esc.init();
  auto ret = esc.gsub(str, "\\\\\\1");
  return ret;
} // }}}

int ib::Regex::match(const char *str, const std::size_t startpos, const std::size_t endpos) { // {{{
  onig_region_clear(region_);
  setString(str);

  const auto len = strlen(str);
  const auto endposs = std::min<std::size_t>(endpos == 0 ? len : endpos, len);
  const auto startposs = std::min<std::size_t>(len, startpos);

  const auto end   = reinterpret_cast<const unsigned char*>(str + endposs);
  const auto start = reinterpret_cast<const unsigned char*>(str + startposs);
  last_result_ = onig_match(reg_, (unsigned char*)str, end, start, region_, ONIG_OPTION_NONE);
  if (last_result_ >= 0) {
    return 0;
  }else{
    return 1;
  }
} // }}}

int ib::Regex::search(const char *str, const std::size_t startpos, const std::size_t endpos) { // {{{
  onig_region_clear(region_);
  setString(str);

  const auto len = strlen(str);
  const auto endposs = std::min<std::size_t>(endpos == 0 ? len : endpos, len);
  const auto startposs = std::min<std::size_t>(len, startpos);

  const auto end   = reinterpret_cast<const unsigned char*>(str + endposs);
  const auto start = reinterpret_cast<const unsigned char*>(str + startposs);
  const auto range = end;
  last_result_ = onig_search(reg_, (unsigned char*)str, end, start, range, region_, ONIG_OPTION_NONE);
  if (last_result_ >= 0) {
    return 0;
  }else{
    return 1;
  }
} // }}}

std::vector<std::string> ib::Regex::split(const char *string) { // {{{
  std::vector<std::string> result;
  const auto len   = strlen(string);

  if(strlen(getPattern()) == 0) {
    for(std::size_t ptr = 0; ptr != len;){
      const auto l = ib::utils::utf8len(string+ptr);
      result.push_back(std::string(string+ptr, l));
      ptr += l;
    }
    return result;
  }

  std::size_t start = 0;
  while(start < len){
    if(search(string, start, 0) != 0) {
      result.emplace_back(string+start, len - start);
      break;
    }
    result.push_back(std::string(string+start, getFirstpos() - start));
    start = getLastpos();
    if(start >= len) {
      result.emplace_back("");
      break;
    }
  }
  return result;
} // }}}

// static void parse_repl(std::vector<IbRegexToken*> &result, const char *string, char sub_meta_char) {{{
class IbRegexToken {
public:
  int t;
  union {
    std::string *str;
    int         n;
  } val;
  ~IbRegexToken(){ if(t == 0) delete val.str; }
};

static void parse_repl(std::vector<IbRegexToken*> &result, const char *string, char sub_meta_char){
  IbRegexToken *token;
  int state = 0;
  auto len   = strlen(string);
  std::size_t buf_ptr = 0;
  std::string res;
  char buf[32];
  char ret[2];
  for(std::size_t ptr = 0; ptr != len; ++ptr){
    const auto c = string[ptr];
    switch(state){
      case 0:
        if(c == sub_meta_char) {
          if(res.size() > 0){
            token = new IbRegexToken;
            token->t = 0;
            token->val.str = new std::string(res);
            res = "";
            result.push_back(token);
          }
          buf_ptr = 0;
          state = 1;
          continue;
        }else{
          ret[0] = c; ret[1] = '\0';
          res += ret;
        }
        break;
      case 1:
        if(c == sub_meta_char) {
          ret[0] = c; ret[1] = '\0';
          res += ret;
          state = 0;
          continue;
        }else{
          ptr--;
          state = 2;
          continue;
        }
        break;
      case 2:
        if(isdigit(c)){
          buf[buf_ptr] = c;
          buf_ptr++;
          if(buf_ptr > 30) buf_ptr = 30;
          continue;
        }else{
          if(buf_ptr != 0) {
            token = new IbRegexToken;
            token->t = 1;
            buf[buf_ptr] = '\0';
            token->val.n = atoi(buf);
            memset(buf, 0, sizeof(buf));
            result.push_back(token);
          }
          ptr--;
          state = 0;
          continue;
        }
        break;
      default:
        break;
    }
  }
  if(state == 0){
    token = new IbRegexToken;
    token->t = 0;
    token->val.str = new std::string(res);
    result.push_back(token);
  }
  if(state == 2){
    token = new IbRegexToken;
    token->t = 1;
    token->val.n = atoi(buf);
    result.push_back(token);
  }
} // }}}

std::string ib::Regex::gsub(const char *string, const char *repl) { // {{{
  if(strlen(getPattern()) == 0) { return std::string(string); }

  std::vector<IbRegexToken*> repltokens;
  parse_repl(repltokens, repl, sub_meta_char_);
  std::string ret;

  std::size_t start = 0;
  const auto len   = strlen(string);
  while(start < len){
    if(search(string, start, 0) != 0) {
      ret.append(string+start, len - start);
      break;
    }
    ret.append(string+start, getFirstpos() - start);
    for(const auto &t : repltokens){
      if(t->t == 0) {
        ret += *(t->val.str);
      }else{
        ret += group(t->val.n);
      }
    }
    start = getLastpos();
  }
  ib::utils::delete_pointer_vectors(repltokens);
  return ret;
} // }}}

std::string ib::Regex::gsub(const char *string, ib::regsubfunc replacer, void *userdata) { // {{{
  if(strlen(getPattern()) == 0) { return std::string(string); }

  std::string ret;
  std::size_t start = 0;
  const auto len   = strlen(string);
  while(start < len){
    if(search(string, start, 0) != 0) {
      ret.append(string+start, len - start);
      break;
    }
    ret.append(string+start, getFirstpos() - start);
    replacer(*this, &ret, userdata);
    start = getLastpos();
  }
  return ret;
} // }}}

