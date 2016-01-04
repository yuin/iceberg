#include "ib_lexer.h"

// class Lexer {{{
void ib::Lexer::clear() { // {{{
  state_ = 0;
  c_ = '\0';
  pc_ = '\0';
  utf8_len_ = 0;
  ptr_ = 0;
  input_ = 0;
} // }}}

void ib::Lexer::onBeforeLoop() { // {{{
} // }}}

void ib::Lexer::parse(const char *input) { // {{{
  clear();
  if(input == 0 || input[0] == '\0'){ return; }
  input_ = input;

  onBeforeLoop();
  do {
    // read an utf8 character
    c_ = input[ptr_];
    if(c_ == '\0') break;
    utf8_len_ = ib::utils::utf8len(c_);
    onReadChar();
    ppc_ = pc_;
    pc_ = c_;
  }while(1);
  onAfterLoop();
} // }}}
// }}}

// class CommandLexer {{{
void ib::CommandLexer::clear() { // {{{
  Lexer::clear();
  token_ = 0;
  strbuf_.clear();
  for (auto it = tokens_.begin(), last = tokens_.end(); it != last; ++it ) {
    if(!(*it)->isNullToken()) delete *it;
  }
  tokens_.clear();
  ib::utils::delete_pointer_vectors(param_values_);
  param_values_.clear();
  is_using_cwd_ = false;
} // }}}

void ib::CommandLexer::onReadChar() { // {{{
  if(ptr_ == 0 && c_ == '!'){
    is_using_cwd_ = true;
    ptr_++;
    return;
  }
  
  switch(state_){
    //initial state
    case 0:
      strbuf_.clear();
      strbuf_.append(input_+ptr_, utf8_len_);
      if(c_ == '"' && pc_ != '\\'){
        token_ = new UntermStringToken(ptr_);
        state_ = 1; /* read string */
      }else if(isspace(c_)) {
        token_ = new DelimiterToken(ptr_);
        state_ = 2; /* read delim */
      }else{
        token_ = new ValueToken(ptr_);
        state_ = 3; /* read value */
      }
      ptr_ += utf8_len_;
      break;
    // read string
    case 1:
      if(c_ == '"'){
        // double quote
        if(pc_ != '\\') {
          auto saved_start_pos = token_->getStartPos();
          delete token_;
          token_ = new StringToken(saved_start_pos);
          strbuf_.append(input_+ptr_, utf8_len_);
          token_->setToken(strbuf_);
          tokens_.push_back(token_);

          ptr_++;
          state_ = 0;
        // escaped double quote
        }else{
          strbuf_ = strbuf_.substr(0, strbuf_.length()-1);
          strbuf_.append(input_+ptr_, utf8_len_);
          ptr_ += utf8_len_;
        }
      // others
      }else{
        strbuf_.append(input_+ptr_, utf8_len_);
        ptr_ += utf8_len_;
      }
      break;
    // read delim
    case 2:
      if(!isspace(c_)) {
        token_->setToken(strbuf_);
        tokens_.push_back(token_);
        state_ = 0;
      }else{
        strbuf_.append(input_+ptr_, utf8_len_);
        ptr_ += utf8_len_;
      }
      break;
    // read value
    case 3:
      if(isspace(c_) || (c_ == '"' && pc_ != '\\')){
        token_->setToken(strbuf_);
        tokens_.push_back(token_);
        state_ = 0;
      }else{
        strbuf_.append(input_+ptr_, utf8_len_);
        ptr_ += utf8_len_;
      }
      break;
    default:
      fl_alert("unknown state");
  }

} // }}}

void ib::CommandLexer::onAfterLoop() { // {{{
  if(state_ != 0 && token_ != 0){
    token_->setToken(strbuf_);
    tokens_.push_back(token_);
  }

  if(tokens_.size() > 1){
    for(auto params_it = std::next(tokens_.begin()), last = tokens_.end(); params_it != last; ++params_it){
      if((*params_it)->isValueToken()) {
        param_values_.push_back(new std::string((*params_it)->getValue().c_str()));
      }
    }
  }

} // }}}
// }}}

// class SimpleTemplateLexer {{{
void ib::SimpleTemplateLexer::clear() { // {{{
  Lexer::clear();
  strbuf_.clear();
  current_varname_.clear();
} // }}}

void ib::SimpleTemplateLexer::parse(std::string &ret, const std::string &input, const ib::string_map &values) { // {{{
  values_ = &values;
  ret_ = &ret;
  Lexer::parse(input);
} // }}}

void ib::SimpleTemplateLexer::onReadChar() { // {{{
  switch(state_) {
    // initial state
    case 0:
      strbuf_.clear();
      if(((ppc_ == '\\' && pc_ == '\\') || (pc_ != '\\')) && c_ == '$'){
        state_ = 1; // read var1
      }else if(pc_ == '\\' && c_ == '$') {
        strbuf_ = strbuf_.substr(0, strbuf_.size()-1);
        strbuf_.append(input_+ptr_, utf8_len_);
      }else if(pc_ == '\\' && c_ == '\\') {
        state_ = 10; // read string
      }else{
        strbuf_.append(input_+ptr_, utf8_len_);
        state_ = 10; // read string
      }
      ptr_ += utf8_len_;
      break;
    // read var1
    case 1:
      strbuf_.append(input_+ptr_, utf8_len_);
      if(c_ == '{') {
        current_varname_.clear();
        state_ = 2; // read var2
      }else{
        ret_->append(strbuf_);
        state_ = 0; // initial state
      }
      ptr_ += utf8_len_;
      break;
    // read var2
    case 2:
      strbuf_.append(input_+ptr_, utf8_len_);
      if(c_ == '}') {
        auto it = values_->find(current_varname_);
        if(it != values_->end()){
          ret_->append(it->second);
        }else{
          ret_->append("");
        }
        state_ = 0;
      }else{
        current_varname_.append(input_+ptr_, utf8_len_);
      }
      ptr_ += utf8_len_;
      break;
    // read string
    case 10:
      if(((ppc_ == '\\' && pc_ == '\\') || (pc_ != '\\')) && c_ == '$'){
        ret_->append(strbuf_);
        state_ = 0;
      }else if(pc_ == '\\' && c_ == '$') {
        strbuf_ = strbuf_.substr(0, strbuf_.size()-1);
        strbuf_.append(input_+ptr_, utf8_len_);
        ptr_ += utf8_len_;
      }else if(pc_ == '\\' && c_ == '\\'){
        ptr_ += utf8_len_;
      }else{
        strbuf_.append(input_+ptr_, utf8_len_);
        ptr_ += utf8_len_;
      }
      break;
    default:
      fl_alert("unknown state");
  }

} // }}}

void ib::SimpleTemplateLexer::onAfterLoop() { // {{{
  if(state_ != 0){
    ret_->append(strbuf_);
  }
} // }}}
// }}}

