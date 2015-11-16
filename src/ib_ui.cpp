#include "ib_ui.h"
#include "ib_platform.h"
#include "ib_config.h"
#include "ib_lua.h"
#include "ib_controller.h"
#include "ib_comp_value.h"
#include "ib_icon_manager.h"

ib::MainWindow *ib::MainWindow::instance_ = 0;
ib::ListWindow *ib::ListWindow::instance_ = 0;

// class Input {{{
static void ib_input_copy_callback(Fl_Widget*, void *userdata) { // {{{
  ib::Input *in = (ib::Input*)userdata;
  in->copy(1);
} // }}}

static void ib_input_paste_calback(Fl_Widget*, void *userdata) { // {{{
  ib::Input *in = (ib::Input*)userdata;
  Fl::paste(*in);
  in->scan();
  ib::MainWindow::inst()->updateIconbox();
  ib::Controller::inst().showCompletionCandidates();
} // }}}

static void ib_input_cut_callback(Fl_Widget*, void *userdata) { // {{{
  ib::Input *in = (ib::Input*)userdata;
  in->copy(1);
  in->cut();
  in->scan();
  ib::MainWindow::inst()->updateIconbox();
  ib::Controller::inst().showCompletionCandidates();
} // }}}

// void ib::Input::initLayout(){ /* {{{ */
static void _blink_cursor(void *p) {
  static int state = 0;
  auto &cfg = ib::Config::inst();
  auto input = (ib::Input*)p;
  state =!state;
  if(state){ input->cursor_color(cfg.getStyleInputFontColor());
  }else{ input->cursor_color(cfg.getStyleInputBgColor()); }
  input->redraw();
  Fl::repeat_timeout(0.5,_blink_cursor, (void*)input);
}
void ib::Input::initLayout(){
  auto &cfg = ib::Config::inst();

  textfont(ib::Fonts::input);
  textcolor(cfg.getStyleInputFontColor());
  cursor_color(cfg.getStyleInputFontColor());
  textsize(cfg.getStyleInputFontSize());
  color(cfg.getStyleInputBgColor());
  selection_color(cfg.getStyleInputSelectionBgColor());
  box((Fl_Boxtype)cfg.getStyleInputBoxtype());
  Fl::add_timeout(0.5,_blink_cursor, this);
} /* }}} */

bool ib::Input::isEmpty() const { // {{{
  const std::size_t length = strlen(value());
  return length == 0 || (isUsingCwd() && length == 1);
} // }}}

// key_event_handler {{{
static void _main_thread_awaker(void *p){
  ib::Controller::inst().showCompletionCandidates();
}

void ib::_key_event_handler(void *p) {
  Fl::awake(_main_thread_awaker, 0);
}
// }}}

int ib::Input::handle(int e){ /* {{{ */
  int accept = 0;
  const ib::Config &cfg = ib::Config::inst();
  int key = Fl::event_key();
  const int state = Fl::event_state();
  const int mods = state & (FL_META|FL_CTRL|FL_ALT);
  const int shift = state & FL_SHIFT;
  const int selected = (position() != mark()) ? 1 : 0;
  const int threshold = cfg.getKeyEventThreshold();

  switch(e){
    case FL_KEYUP:
      key = ib::platform::convert_keysym(key);
      if(!getImeComposition() && key != 0xe5 && key != 0xfee9){
keyup:
        // ignore shift key up
        if(key == 65505 && state == 0) { return 1; }
        // ignore hot key
        if(ib::utils::matches_key(cfg.getHotKey(), key, state)) { return 1;}
        // calls an event handler
        lua_getglobal(IB_LUA, "on_key_up");
        if (lua_pcall(IB_LUA, 0, 1, 0)) {
            fl_alert("%s", lua_tostring(IB_LUA, lua_gettop(IB_LUA)));
            return 1;
        }
        accept = (int)lua_tonumber(IB_LUA, 1);
        ib::MainLuaState::inst().clearStack();
        if(accept) { return 1; }
        if(
           // paste
           (key == FL_Insert && mods==0 && shift) ||
           (key == 'v' && mods==FL_COMMAND) ||
           // cut
           //(mods == 0 && shift && selected) ||
           (key == 'x' && mods==FL_COMMAND) ||
           // kill word
           ib::utils::matches_key(cfg.getKillWordKey(), key, state)
           ){
          if(threshold > 0) key_event_.cancelEvent();
          ib::Controller::inst().showCompletionCandidates();
          return 1;
        }

        if(ib::utils::matches_key(cfg.getListNextKey(), key, state) ||
           ib::utils::matches_key(cfg.getListPrevKey(), key, state) || 
           ib::utils::matches_key(cfg.getToggleModeKey(), key, state) ||
           key == FL_Enter
            ){
          if(threshold > 0) key_event_.cancelEvent();
          return 1;
        }

        if(!ib::utils::event_key_is_control_key()){
          if(threshold > 0) {
            key_event_.queueEvent((void*)1);
          }else{
            ib::Controller::inst().showCompletionCandidates();
          }
          return 1;
        }else{
          if(threshold > 0) key_event_.cancelEvent();
          Fl_Input::handle(e);
          return 1;
        }
      }
      break;

    case FL_KEYDOWN:
      if(getImeComposition()) return 1;
      if(!getImeComposition() && key != 0xe5 && key != 0xfee9){
        // ignore shift key down
        if(key == 65505 && state == 0) { return 1; }
        // ignore hot key
        if(ib::utils::matches_key(cfg.getHotKey(), key, state)) { return 1;}
        // close modal window by an enter key
        for(Fl_Window *w = Fl::first_window(); w != 0;w = Fl::next_window(w)){
          if(w->modal()) { 
            for(int i = 0; i < w->as_group()->children(); ++i){
              Fl_Widget *widget = w->as_group()->child(i);
              Fl_Button *button = dynamic_cast<Fl_Button*>(widget);
              if(button && button->visible()){
                button->do_callback();
                return 1;
              }
            }
          }
        }

        // calls an event handler
        lua_getglobal(IB_LUA, "on_key_down");
        if (lua_pcall(IB_LUA, 0, 1, 0)) {
            fl_alert("%s", lua_tostring(IB_LUA, lua_gettop(IB_LUA)));
            return 1;
        }
        accept = (int)lua_tonumber(IB_LUA, 1);
        ib::MainLuaState::inst().clearStack();
        if(accept) { return 1; }
        // handle copy&paste events
        if(
           // paste
           (key == FL_Insert && mods==0 && shift) ||
           (key == 'v' && mods==FL_COMMAND) ||
           // cut
           (mods == 0 && shift && selected) ||
           (key == 'x' && mods==FL_COMMAND)){
          Fl_Input::handle(e);
          scan();
          ib::MainWindow::inst()->updateIconbox();
          return 1;
        }
        
        if(key == FL_Enter){

          lua_getglobal(IB_LUA, "on_enter");
          if (lua_pcall(IB_LUA, 0, 1, 0)) {
              fl_alert("%s", lua_tostring(IB_LUA, lua_gettop(IB_LUA)));
              return 1;
          }
          accept = (int)lua_tonumber(IB_LUA, 1);
          ib::MainLuaState::inst().clearStack();
          if(!accept){
            ib::Controller::inst().executeCommand();
          }
          return 1;
        }
        if(ib::utils::matches_key(cfg.getEscapeKey(), key, state)){
          ib::Controller::inst().hideApplication();
          return 1;
        }
        if(ib::utils::matches_key(cfg.getListNextKey(), key, state)){
          ib::Controller::inst().selectNextCompletion();
          return 1;
        }
        if(ib::utils::matches_key(cfg.getListPrevKey(), key, state)){
          ib::Controller::inst().selectPrevCompletion();
          return 1;
        }
        if(ib::utils::matches_key(cfg.getToggleModeKey(), key, state)){
          ib::Controller::inst().toggleHistorySearchMode();
          return 1;
        }
        if(ib::utils::matches_key(cfg.getKillWordKey(), key, state)){
          ib::Controller::inst().killWord();
          return 1;
        }
      }
      Fl_Input::handle(e);
      return 1;
      break;

    case IB_EVENT_END_COMPOSITION:
      goto keyup;
      break;

    case FL_PUSH:
      if (Fl::event_button() == FL_RIGHT_MOUSE){
        Fl_Menu_Item rclick_menu[] = {
          { "    copy    ",  0, ib_input_copy_callback,  (void*)this },
          { "    cut     ",  0, ib_input_cut_callback,   (void*)this },
          { "    paste   ",  0, ib_input_paste_calback,  (void*)this },
          { 0 }
        };
        const Fl_Menu_Item *m = rclick_menu->popup(Fl::event_x(), Fl::event_y(), 0, 0, 0);
        if (m) m->do_callback(0, m->user_data());
        return 1;
      }
      break;
    case FL_RELEASE:
      if ( Fl::event_button() == FL_RIGHT_MOUSE ) {
        return 1;
      }
      break;
  }
  return Fl_Input::handle(e);
} /* }}} */

void ib::Input::scan() { // {{{
  // copy the value(not reference)
  prev_cursor_value_ = getCursorToken()->getValue();
  lexer_.clear();
  lexer_.parse(value());

  // set the cursor_token
  unsigned int cursor_pos = position();
  prev_cursor_token_index_ = cursor_token_index_;
  cursor_token_index_ = 0;
  ib::Token *token = 0;
  unsigned int i = 0;
  for(;i < lexer_.getTokens().size(); ++i){
    token = lexer_.getTokens().at(i);
    if(token->getStartPos() <= cursor_pos && token->getEndPos() >= cursor_pos){
      if(token->isValueToken()) {
        cursor_token_index_ = i;
      }else if(i > 0) {
        cursor_token_index_ = i-1;
      }
      break;
    }
  }
  if(i == lexer_.getTokens().size()-1){
    cursor_token_index_ = i;
  }
} // }}}

void ib::Input::clear() { // {{{
  lexer_.clear();
  cursor_token_index_ = 0;
  prev_cursor_token_index_ = 0;
  value("");
  ib::MainWindow::inst()->clearIconbox();
} // }}}

void ib::Input::adjustSize() { // {{{
  auto &cfg = ib::Config::inst();
  auto mainwin = ib::MainWindow::inst();
  int screen_x, screen_y, screen_w, screen_h;
  Fl::screen_xywh(screen_x, screen_y, screen_w, screen_h, 0, 0);

  const int min_window_width = cfg.getStyleWindowWidth();
  const int pad_x = cfg.getStyleWindowPadx();
  const int max_window_width = screen_w - (screen_w/2 - min_window_width/2);
  Fl_Font font = textfont();
  unsigned int tsize = cfg.getStyleInputFontSize();
  fl_font(font, tsize);
#ifdef IB_OS_WIN
  ib::unique_oschar_ptr osstr(ib::platform::utf82oschar(value()));
  int isize = (int)ib::platform::win_calc_text_width(osstr.get()) + tsize;
#else
  int isize = (int)fl_width(value()) + tsize;
#endif
  const int curret_w = w();
  int new_window_width = mainwin->w() + (isize - curret_w);
  int new_input_width  = w() + (isize - curret_w);
  if(new_window_width < min_window_width) {
    new_window_width = min_window_width;
    new_input_width  = min_window_width - (pad_x*3 + 32);
  }else if(new_window_width > max_window_width){
    new_window_width = max_window_width;
    new_input_width  = max_window_width - (pad_x*3 + 32);
  }

  mainwin->size(new_window_width, mainwin->h());
  size(new_input_width, h());
} // }}}

const ib::Token* ib::Input::getCursorToken() const { // {{{
  if(lexer_.getTokens().size() == 0) {
    return &ib::NullToken::inst();
  }
  return lexer_.getTokens().at(cursor_token_index_);
} // }}}

const std::string& ib::Input::getCursorValue() const { // {{{
  return getCursorToken()->getValue();
} // }}}

void ib::Input::draw() { // {{{
  if (input_type() == FL_HIDDEN_INPUT) return;
  Fl_Boxtype b = box();
  if (damage() & FL_DAMAGE_ALL) draw_box(b, color());
  adjustSize();
  Fl_Input_::drawtext(x()+Fl::box_dx(b), y()+Fl::box_dy(b),
		      w()-Fl::box_dw(b), h()-Fl::box_dh(b));
} // }}}
/* }}} */

// class MainWindow {{{
void ib::MainWindow::initLayout(){ /* {{{ */
  begin();
  int screen_x, screen_y, screen_w, screen_h;
  auto &cfg = ib::Config::inst();

  Fl::screen_xywh(screen_x, screen_y, screen_w, screen_h, 0, 0);
  const unsigned int w = cfg.getStyleWindowWidth();
  const unsigned int h = cfg.getStyleWindowHeight();
  const unsigned int pad_x = cfg.getStyleWindowPadx();
  const unsigned int pad_y = cfg.getStyleWindowPady();

  clear_border();
  color(cfg.getStyleWindowBgColor());
  const unsigned int x = cfg.getStyleWindowPosxAuto() ? screen_w/2 - w/2 : cfg.getStyleWindowPosx();
  const unsigned int y = cfg.getStyleWindowPosyAuto() ? screen_h/2 - h/2 : cfg.getStyleWindowPosy();
  resize(x, y, w, h);
  box((Fl_Boxtype)cfg.getStyleWindowBoxtype());

  iconbox_ = new Fl_Box(pad_x, h/2 - 16, 32, 32);
  input_ = new ib::Input(pad_x*2+32,pad_y, w - (pad_x*3 + 32), h - pad_y*2);
  input_->initLayout();

  clipboard_ = new Fl_Input(0,0,0,0,"");
  clipboard_->hide();

  end();
} /* }}} */

int ib::MainWindow::handle(int e){ /* {{{ */
  return Fl_Window::handle(e);
} /* }}} */

void ib::MainWindow::show(){ /* {{{ */
#ifndef IB_OS_WIN
  ib::platform::move_to_current_desktop(this);
#endif
  return Fl_Window::show();
} /* }}} */

void ib::MainWindow::hide(){ /* {{{ */
  clear_visible();
  ib::platform::hide_window(this);
} /* }}} */

void ib::MainWindow::close() { // {{{
  Fl_Window::hide();
} // }}}

void ib::MainWindow::clearIconbox() { // {{{
  if(!ib::Config::inst().getEnableIcons()) return;
  Fl_Image *image = iconbox_->image();
  if(image) delete image;
  iconbox_->image(ib::IconManager::inst()->getEmptyIcon(32,32));
  redraw();
} // }}}

void ib::MainWindow::setIconbox(Fl_Image *image) { // {{{
  if(!ib::Config::inst().getEnableIcons()) return;
  Fl_Image *bimage = iconbox_->image();
  if(bimage) delete bimage;
  iconbox_->image(image);
  redraw();
}
/* }}} */

void ib::MainWindow::updateIconbox() { // {{{
  if(!ib::Config::inst().getEnableIcons()) return;
  auto input = ib::MainWindow::inst()->getInput();
  if(input->isEmpty()) {
    clearIconbox();
    return;
  }
  const std::string &cmd = input->getFirstValue();
  auto it = ib::Controller::inst().getCommands().find(cmd);
  if(it != ib::Controller::inst().getCommands().end()){
    setIconbox((*it).second->loadIcon(32));
  }else{
    CompletionString comp_value(cmd.c_str());
    setIconbox(comp_value.loadIcon(32));
  }
}
/* }}} */
// }}}

// class Listbox {{{

struct FL_BLINE {  // {{{
  FL_BLINE* prev;
  FL_BLINE* next;
  void* data;
  Fl_Image* icon;
  short length;  
  char flags;    
  char txt[1];
}; // }}}

void ib::Listbox::item_draw (void *item, int X, int Y, int W, int H) const { // {{{
  FL_BLINE* l = (FL_BLINE*)item;
  char* str = l->txt;
  auto &cfg = ib::Config::inst();

  const std::size_t line = (std::size_t)l->data;
  if(line%2 == 0){
    if (l->flags & 1) {
      fl_color(cfg.getStyleListSelectionBgColor1());
    }else{
      fl_color(cfg.getStyleListBgColor1());
    }
  }else{
    if (l->flags & 1) {
      fl_color(cfg.getStyleListSelectionBgColor2());
    }else{
      fl_color(cfg.getStyleListBgColor2());
    }
  }
  fl_rectf(X, Y, W, H);

  int left = X;
  int top  = Y+16; // why 16?
  int width = W;
  // int height = H;

  if (l->icon) {
    l->icon->draw(X+2,Y+2);
    int iconw = l->icon->w()+2;
    left  += iconw; 
    width -= iconw;
  }

  char *ptr = strchr(str, '\t');
  char *description = 0;
  if(ptr) {
    *ptr = 0;
    description = ptr+1;
  }

  left += 2;
  width -= 2;
  int tsize = cfg.getStyleListFontSize();
  Fl_Font font = textfont();
  Fl_Color lcol = textcolor();
  if (l->flags & 1) lcol = cfg.getStyleListSelectionFontColor();
  if (!active_r())  lcol = fl_inactive(lcol);
  fl_font(font, tsize);
  fl_color(lcol);

#ifdef IB_OS_WIN
  // fl_draw on windwos is slow.
  ib::unique_oschar_ptr osstr(ib::platform::utf82oschar(str));
  ib::platform::win_draw_text(osstr.get(), left, top, width);
#else
  fl_draw(str, left, top);
#endif

  if(description){
    top += cfg.getStyleListFontSize()-2;
    *ptr = '\t';
    lcol = cfg.getStyleListDescFontColor();
    if (l->flags & 1) lcol = cfg.getStyleListSelectionDescFontColor();
    if (!active_r())  lcol = fl_inactive(lcol);
    fl_font(font, cfg.getStyleListDescFontSize());
    fl_color(lcol);
#ifdef IB_OS_WIN
    // fl_draw on windwos is slow.
    ib::unique_oschar_ptr osdesc(ib::platform::utf82oschar(description));
    ib::platform::win_draw_text(osdesc.get(), left, top, width);
#else
    fl_draw(description, left, top);
#endif
  }
} // }}}

int ib::Listbox::item_height(void *item) const { // {{{
  FL_BLINE* l = (FL_BLINE*)item;
  char* str = l->txt;
  if(strchr(str, '\t') != 0){
    return 32 + 4;
  }else{
    return 16 + 4;
  }
} // }}}

int ib::Listbox::item_width(void *item) const{ // {{{
  FL_BLINE* l = (FL_BLINE*)item;
  char* str = l->txt;
  const auto &cfg = ib::Config::inst();

  char *ptr = strchr(str, '\t');
  char *description = 0;
  if(ptr) {
    *ptr = 0;
    description = ptr+1;
  }
  int left_pad = 2;
  if(cfg.getEnableIcons()){
    left_pad += 2 + (description ? 32 : 16);
  }

  int tsize = cfg.getStyleListFontSize();
  Fl_Font font = textfont();
  fl_font(font, tsize);
#ifdef IB_OS_WIN
  ib::unique_oschar_ptr osstr(ib::platform::utf82oschar(str));
  size_t w1 = ib::platform::win_calc_text_width(osstr.get()) + tsize*2;
#else
  size_t w1 = fl_width(str) + tsize;
#endif
  size_t w2 = 0;

  if(description){
    *ptr = '\t';
    tsize = cfg.getStyleListDescFontSize();
    fl_font(font, tsize);
#ifdef IB_OS_WIN
    ib::unique_oschar_ptr osdesc(ib::platform::utf82oschar(description));
    w2 = ib::platform::win_calc_text_width(osdesc.get()) + tsize*2;
#else
    w2 = fl_width(description) + tsize;
#endif
  }

  return (int)(std::max<int>((int)w1, (int)w2)) + left_pad;
} // }}}

void ib::Listbox::initLayout(){ // {{{
  auto &cfg = ib::Config::inst();

  textfont(ib::Fonts::list);
  textcolor(cfg.getStyleListFontColor());
  textsize(cfg.getStyleListFontSize());
  color(cfg.getStyleListBgColor1());
  selection_color(cfg.getStyleListSelectionBgColor1());
  has_scrollbar(Fl_Browser_::VERTICAL);
  box(FL_FLAT_BOX);
  static int widths[] = { (int)cfg.getStyleListFontSize()*2+3, 1, 0};
  column_widths(widths);
  scrollbar.selection_color(cfg.getStyleListBgColor1());
  scrollbar.color(cfg.getStyleListBgColor1());
  scrollbar.labelcolor(cfg.getStyleListFontColor());
} // }}}

int ib::Listbox::handle(int e){ /* {{{ */
  if(e == FL_RELEASE) return 1;
  int selected_b = value();
  int ret = Fl_Select_Browser::handle(e);
  int selected_a = value();
  if(selected_b != selected_a){
    selectLine(selected_a, false);
    ib::Controller::inst().completionInput();
    ret = 1;
  }
  if(e == FL_PUSH && Fl::event_button() == FL_RIGHT_MOUSE){
    const std::string *context_path = getValues().at(value()-1)->getContextMenuPath();
    if(context_path != 0){
      ib::oschar osbuf[IB_MAX_PATH];
      ib::platform::utf82oschar_b(osbuf, IB_MAX_PATH, context_path->c_str());
      ib::platform::show_context_menu(osbuf);
    }
    ret = 1;
  }
  return ret;
} /* }}} */

void ib::Listbox::addValue(ib::CompletionValue* value){ // {{{
  values_.push_back(value);
} // }}}

void ib::Listbox::removeValue(int line){ // {{{
  ib::platform::ScopedLock lock(mutex_);
  incOperationCount();
  destroyIcon(line);
  remove(line);
  values_.erase(values_.begin() + line -1);
} // }}}

ib::CompletionValue* ib::Listbox::selectedValue() const { // {{{
  return values_.at(value()-1);
} // }}}

void ib::Listbox::clearAll(){ // {{{
  ib::platform::ScopedLock lock(mutex_);
  incOperationCount();

  if(ib::MainWindow::inst()->getInput()->getCursorTokenIndex() == 0) {
    ib::MainWindow::inst()->clearIconbox();
  }
  for(int i = 1, last = size(); i <= last; ++i){ destroyIcon(i); }
  for(auto it = values_.begin(), last = values_.end(); it != last; ++it){
    if((*it)->isTemporary()){ delete *it; }
  }
  values_.clear();
  std::vector<ib::CompletionValue*>().swap (values_);
  is_autocompleted_ = false;
  max_width_ = 0;
  clear();
  ib::IconManager::inst()->shrinkCache();
} // }}}

void ib::Listbox::destroyIcon(const int line){ // {{{
  Fl_RGB_Image *image_icon = (Fl_RGB_Image*)(icon(line));
  if(image_icon == 0) return;

  remove_icon(line);
  icon(line, 0);
  delete image_icon;
} // }}}

void ib::Listbox::startUpdate(){ /* {{{ */
  ib::platform::ScopedLock lock(mutex_);
  incOperationCount();
} /* }}} */

void ib::Listbox::endUpdate(const bool use_max_candidates){ /* {{{ */
  ib::platform::ScopedLock lock(mutex_);
  incOperationCount();
  if(size() == 0) { max_width_ = 0;}
  if(isEmpty()) return;

  unsigned int max_candidates = ib::Config::inst().getMaxCandidates();
  if(max_candidates == 0) max_candidates = UINT_MAX;
  int width = 0;
  unsigned int i = 1;
  for(auto it = values_.begin(), last = values_.end(); it != last; ++it, ++i) {
    if(use_max_candidates && i > max_candidates) break;

    if((*it)->hasDescription()) {
      add(((*it)->getDispvalue() + "\t    " + (*it)->getDescription()).c_str(), (void*)(intptr_t)i);
      // char scorebuf[124];
      // sprintf(scorebuf, "%1.2f", ((ib::BaseCommand*)(*it))->getScore());
      // add(((*it)->getDispvalue() + "(score:" + scorebuf + ")\n \t\n    " + (*it)->getDescription()).c_str(), (void*)i);
    }else{
      add((*it)->getDispvalue().c_str(), (void*)(intptr_t)i);
    }
    width = item_width(item_last());
    max_width_ = (max_width_ > width) ? max_width_ : width;
  }

  if(values_.at(0)->isAutocompleteEnable()){
    selectLine(1);
    is_autocompleted_ = true;
  }

  setEmptyIcons();
  adjustSize();

  if(ib::Config::inst().getEnableIcons()){
    ib::IconManager::inst()->loadCompletionListIcons();
  }
} /* }}} */

void ib::Listbox::setEmptyIcons(){ /* {{{ */
  if(isEmpty()) return;
  unsigned int icon_height;
  for(int i=1, last = size(); i <= last; ++i){
    icon_height = values_.at(i-1)->hasDescription() ? 32 : 16;
    int icon_width = icon_height;
    if(!ib::Config::inst().getEnableIcons()){
      icon_width = 1;
    }

    icon(i, ib::IconManager::inst()->getEmptyIcon(icon_width, icon_height));
  }
} /* }}} */

void ib::Listbox::selectNext() { // {{{
  if(is_autocompleted_) {
    is_autocompleted_ = false;
    return;
  }
  selectLine(value()+1);
} // }}}

void ib::Listbox::selectPrev() { // {{{
  if(is_autocompleted_) {
    is_autocompleted_ = false;
    return;
  }
  selectLine(value()-1);
} // }}
// }}}

void ib::Listbox::selectLine(const int line, const bool move2middle) { // {{{
  if(is_autocompleted_) {
    is_autocompleted_ = false;
  }
  if(!move2middle){
    select(std::max<int>(std::min<int>(size(),line), 1)); 
  }else{
    seek(std::max<int>(std::min<int>(size(),line), 1)); 
  }
  if(ib::MainWindow::inst()->getInput()->getCursorTokenIndex() == 0) {
    ib::MainWindow::inst()->setIconbox(selectedValue()->loadIcon(32));
  }
} // }}
// }}}

void ib::Listbox::adjustSize() { // {{{
  auto main_window = ib::MainWindow::inst();
  auto list_window = ib::ListWindow::inst();
  auto &cfg     = ib::Config::inst();
  const int font_size    = cfg.getStyleListFontSize();

  int screen_x, screen_y, screen_w, screen_h;
  Fl::screen_xywh(screen_x, screen_y, screen_w, screen_h, 0, 0);
  const int width_a = screen_w - main_window->x() - font_size;
  int width_b = getMaxWidth() + font_size*3;
  if(has_scrollbar() & Fl_Browser_::VERTICAL){
    width_b -= Fl::scrollbar_size();
  }
  const int new_width = std::max<unsigned int>(cfg.getStyleWindowWidth(), (unsigned int)std::min<int>(width_b , width_a));

  const int height_a = screen_h - (main_window->y()+main_window->h()+cfg.getStyleTaskbarHeight());
  const int height_b = item_height(item_first()) * size() + font_size;

  const int new_height = ((height_a > height_b) ? height_b : height_a);
  resize(x(), y(), 
      new_width - cfg.getStyleListPadx()*2, 
      new_height - cfg.getStyleListPady()*2);
  list_window->resize(main_window->x(), main_window->y()+main_window->h(), new_width, new_height);
} // }}
// }}}

// }}}

// class ListWindow {{{
void ib::ListWindow::hide(){ /* {{{ */
  clear_visible();
#ifdef IB_OS_WIN
  resize(-1, -1, 1, 1);
#else
  resize(0, 0, 1, 1);
#endif
} /* }}} */

void ib::ListWindow::show(){ /* {{{ */
#ifndef IB_OS_WIN
  ib::platform::move_to_current_desktop(this);
#endif
  if(!show_init_){
    show_init_ = true;
    return Fl_Window::show();
  }else{
    if(w() == 1) {
      listbox_->adjustSize();
    }
    ib::platform::raise_window(this);
  }
} /* }}} */

void ib::ListWindow::close() { // {{{
  Fl_Window::hide();
} // }}}

void ib::ListWindow::initLayout(){ // {{{
  begin();
  auto &cfg = ib::Config::inst();

  const unsigned int pad_x = cfg.getStyleListPadx();
  const unsigned int pad_y = cfg.getStyleListPady();

  const int main_x = ib::MainWindow::inst()->x();
  const int main_y = ib::MainWindow::inst()->y();
  const int main_w = ib::MainWindow::inst()->w();
  const int main_h = ib::MainWindow::inst()->h();
  resize(main_x, main_y + main_h, main_w, 1);
  clear_border();
  color(cfg.getStyleListBorderColor());
  box((Fl_Boxtype)cfg.getStyleListBoxtype());

  listbox_ = new ib::Listbox(pad_x , pad_y , w()-pad_x*2, h()-pad_y*2);
  listbox_->initLayout();

  end();
} /* }}} */

int ib::ListWindow::handle(int e){ /* {{{ */
  return Fl_Window::handle(e);
} /* }}} */
// }}}

