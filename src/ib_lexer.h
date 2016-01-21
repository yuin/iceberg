#ifndef __IB_LEXER_H__
#define __IB_LEXER_H__

#include "ib_constants.h"
#include "ib_utils.h"
#include "ib_server.h"

namespace ib {

  // Tokens {{{
  class Token : private NonCopyable<Token> { // {{{
    public:
      Token(const std::size_t start_pos) : start_pos_(start_pos), end_pos_(0), token_(""), value_("") { }
      virtual ~Token() {}
      virtual bool isNullToken() const { return false; }
      virtual bool isValueToken() const { return true; }
      std::size_t getStartPos() const { return start_pos_; }
      std::size_t getLength() const { return length_; }
      std::size_t getEndPos() const { return end_pos_; }
      const std::string& getToken() const { return token_; }
      virtual void setToken(const std::string &token) {
        token_ = token;
        value_ = token;
        end_pos_ = start_pos_ + token.length();
        length_ = end_pos_ - start_pos_;
      }
      const std::string& getValue() const { return value_; }
    
    protected:
      std::size_t start_pos_;
      std::size_t end_pos_;
      std::size_t length_;
      std::string  token_;
      std::string  value_;


  }; // }}}

  class ValueToken : public Token { // {{{
    public:
      ValueToken(const std::size_t start_pos) : Token(start_pos) {}
  }; // }}}

  class DelimiterToken : public Token { // {{{
    public:
      DelimiterToken(const std::size_t start_pos) : Token(start_pos) {}
      bool isValueToken() const { return false; }
  }; // }}}

  class NullToken : public Token, private NonCopyable<NullToken> { // {{{
    friend class ib::Singleton<NullToken>;
    public:
      bool isNullToken() const { return true; }
      bool isValueToken() const { return false; }
    protected:
      NullToken() : Token(0) {}
  }; // }}}

  class StringToken : public Token { // {{{
    public:
      StringToken(const std::size_t start_pos) : Token(start_pos) {}
      void setToken(const std::string &token) {
        Token::setToken(token);
        value_ = token_.substr(1, token_.length()-2);
      }
  }; // }}}

  class UntermStringToken : public Token { // {{{
    public:
      UntermStringToken(const std::size_t start_pos) : Token(start_pos) {}
      void setToken(const std::string &token) {
        Token::setToken(token);
        value_ = token_.substr(1);
      }
  }; // }}}

  // }}}

  class Lexer : private NonCopyable<Lexer> { // {{{
    public:
      Lexer() : state_(0), c_('\0'), pc_('\0'), ppc_('\0'), utf8_len_(0), ptr_(0), input_(0) {}
      virtual ~Lexer() {}
      virtual void clear();
      void parse(const char *input);
      void parse(const std::string &input) { return parse(input.c_str()); }
      virtual void onBeforeLoop();
      virtual void onReadChar() = 0;
      virtual void onAfterLoop() = 0;

    protected:
      unsigned char state_;
      unsigned char c_;
      unsigned char pc_;
      unsigned char ppc_;
      unsigned int utf8_len_;
      std::size_t ptr_;
      const char *input_;
  }; // }}}

  class CommandLexer : public Lexer { // {{{
    public:
      CommandLexer() : Lexer(), tokens_(), param_values_(), strbuf_(""), token_(0), is_using_cwd_(false) {}
      ~CommandLexer() { clear(); }

      void clear();
      void onReadChar();
      void onAfterLoop();

      const std::vector<Token*>& getTokens() const { return tokens_;}
      const std::vector<std::string*>& getParamValues() const { return param_values_;}
      const std::string& getFirstValue() const { return tokens_.at(0)->getValue(); }
      bool isUsingCwd() const { return is_using_cwd_; }

    protected:
      std::vector<Token*> tokens_;
      std::vector<std::string*> param_values_;
      std::string strbuf_;
      Token *token_;
      bool is_using_cwd_;
  }; // }}}

  class SimpleTemplateLexer : public Lexer { // {{{
    public:
      SimpleTemplateLexer() : Lexer(), strbuf_(""), current_varname_(""), ret_(0), values_(0) {}
      ~SimpleTemplateLexer() { clear(); }

      void clear();
      void onReadChar();
      void onAfterLoop();
      void parse(std::string &ret, const std::string &input, const ib::string_map &values);

    protected:
      std::string strbuf_;
      std::string current_varname_;
      std::string *ret_;
      const ib::string_map *values_;

  }; // }}}
}
#endif
