#ifndef __IB_UI_H__
#define __IB_UI_H__

#include "ib_constants.h"
#include "ib_utils.h"
#include "ib_lexer.h"
#include "ib_comp_value.h"
#include "ib_event.h"
#include "ib_platform.h"

namespace ib {
  void _key_event_handler(void *);
  class MainWindow;
  class Input : public Fl_Input, private NonCopyable<Input> { // {{{
    friend class MainWindow;
    public:
      ~Input() {}
      void setImeComposition(const int v) { this->ime_composition_ = v;}
      int getImeComposition() const { return this->ime_composition_; }
      bool isEmpty() const;
      void sendEndCompositionEvent() { handle(IB_EVENT_END_COMPOSITION); }
      void unfocus() { handle(FL_UNFOCUS); }
      void initLayout();
      void scan();
      void clear();
      void adjustSize();
      const char* getValue() const { return value();}
      void setValue(const char* val) { value(val); scan();}
      const ib::Token* getCursorToken() const;
      const std::string& getCursorValue() const;
      int getCursorTokenIndex() const { return cursor_token_index_; }
      int getPrevCursorTokenIndex() const { return prev_cursor_token_index_; }
      const std::vector<ib::Token*>& getTokens() const { return lexer_.getTokens();}
      const std::vector<std::string*>& getParamValues() const { return lexer_.getParamValues();}
      const std::string& getFirstValue() const { return lexer_.getTokens().at(0)->getValue(); }
      const std::string& getPrevCursorValue() const { return prev_cursor_value_; }
      bool isUsingCwd() const { return lexer_.isUsingCwd(); }
      ib::CancelableEvent& getKeyEvent() { return key_event_; }

    protected:
      Input(const int x, const int y, const int w, const int h) : Fl_Input(x,y,w,h), ime_composition_(0), lexer_(), cursor_token_index_(0), prev_cursor_token_index_(0), prev_cursor_value_(""), key_event_(0, _key_event_handler) {};
      void draw();

      int ime_composition_;
      ib::CommandLexer lexer_;
      int cursor_token_index_;
      int prev_cursor_token_index_;
      std::string prev_cursor_value_;

      ib::CancelableEvent key_event_;
    private:
      int handle(int e);

  }; // }}}

  class MainWindow : public Fl_Double_Window, private NonCopyable<MainWindow> { // {{{
    public:
      static MainWindow* inst() { return instance_; }
      static void init() { instance_ = new MainWindow(); instance_->end(); }

      ~MainWindow() { delete input_;}
      Input* getInput() const { return input_;}
      Fl_Input *_getClipboard() const { return clipboard_;}
      Fl_Box   * getIconbox() const { return iconbox_;}
      void initLayout();
      void hide();
      void close();
      void clearIconbox();
      void setIconbox(Fl_Image *image);
      void updateIconbox();

    protected:
      MainWindow(): Fl_Double_Window(0,0,"icebergMainWindow"){};

      static MainWindow *instance_;
      Input *input_;
      Fl_Input *clipboard_;
      Fl_Box   *iconbox_;

    private:
      int handle(int e);
  }; // }}}

  class ListWindow;
  class Listbox : public Fl_Select_Browser, private NonCopyable<Listbox> { // {{{
    friend class ListWindow;
    public:
      ~Listbox(){
        ib::platform::destroy_mutex(&mutex_);
      }

      int getMaxWidth() const { return max_width_;}
      void initLayout();
      void clearAll();
      void destroyIcon(const int line);
      void addValue(ib::CompletionValue *value);
      void removeValue(int line);
      const std::vector<ib::CompletionValue*>& getValues() const { return values_; }
      ib::CompletionValue* selectedValue() const;
      bool hasSelectedIndex() const { return value() != 0; }
      bool isEmpty() const { return values_.size() == 0; }
      bool isAutocompleted() const { return is_autocompleted_; }
      void startUpdate();
      void endUpdate(const bool use_max_candidates);
      void setEmptyIcons();
      void seek(int line) { 
        select(std::max<int>(std::min<int>(size(),line), 1)); 
        middleline(std::max<int>(std::min<int>(size(),line), 1)); 
      }
      void selectNext();
      void selectPrev();
      void selectLine(const int line, const bool move2middle = true);
      void adjustSize();

      ib::mutex& getMutex() { return mutex_; }
      int getOperationCount() const { return operation_count_; }
      void incOperationCount(){ operation_count_++;}
      int item_height(void *item) const;
      int item_width(void *item) const;

    protected:
      Listbox(const int x, const int y, const int w, const int h) : Fl_Select_Browser(x,y,w,h), max_width_(0), values_(), is_autocompleted_(false),mutex_(), operation_count_(0) {
        ib::platform::create_mutex(&mutex_);
      };
      void item_draw (void *item, int X, int Y, int W, int H) const;

      int max_width_;
      std::vector<ib::CompletionValue*> values_;
      bool is_autocompleted_;
      ib::mutex     mutex_;
      int operation_count_;


    private:
      int handle(int e);
  }; // }}}
 
  class ListWindow : public Fl_Double_Window, private NonCopyable<ListWindow> { // {{{

    public:
      static ListWindow* inst() { return instance_; }
      static void init() { instance_ = new ListWindow(); instance_->end();}

      ~ListWindow() { delete listbox_;}
      ib::Listbox* getListbox() const { return listbox_;}
      void initLayout();

      void hide();
      void show();
      void close();

    protected:
      ListWindow() : Fl_Double_Window(0, 0, "icebergListWindow"), listbox_(0), show_init_(false) {};

      static ListWindow *instance_;
      Listbox *listbox_;
      bool show_init_;

    private:
      int handle(int e);

  }; // }}}

}

#endif
