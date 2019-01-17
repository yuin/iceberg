#include "ib_ui.h"
#include "ib_platform.h"
#include "ib_config.h"
#include "ib_lua.h"
#include "ib_controller.h"
#include "ib_comp_value.h"
#include "ib_icon_manager.h"
#include "ib_singleton.h"

// class Input {{{
static void ib_input_copy_callback(Fl_Widget*, void *userdata) { // {{{
  auto in = reinterpret_cast<ib::Input*>(userdata);
  in->copy(1);
} // }}}

static void ib_input_paste_calback(Fl_Widget*, void *userdata) { // {{{
  auto in = reinterpret_cast<ib::Input*>(userdata);
  Fl::paste(*in);
  in->scan();
  ib::Singleton<ib::MainWindow>::getInstance()->updateIconbox();
  ib::Singleton<ib::Controller>::getInstance()->showCompletionCandidates();
} // }}}

static void ib_input_cut_callback(Fl_Widget*, void *userdata) { // {{{
  auto in = reinterpret_cast<ib::Input*>(userdata);
  in->copy(1);
  in->cut();
  in->scan();
  ib::Singleton<ib::MainWindow>::getInstance()->updateIconbox();
  ib::Singleton<ib::Controller>::getInstance()->showCompletionCandidates();
} // }}}

// void ib::Input::initLayout(){ /* {{{ */
static void _blink_cursor(void *p) {
  static int state = 0;
  const auto* const cfg = ib::Singleton<ib::Config>::getInstance();
  auto input = reinterpret_cast<ib::Input*>(p);
  state =!state;
  if(state){ input->cursor_color(cfg->getStyleInputFontColor());
  }else{ input->cursor_color(cfg->getStyleInputBgColor()); }
  input->redraw();
  Fl::repeat_timeout(0.5,_blink_cursor, (void*)input);
}
void ib::Input::initLayout(){
  const auto* const cfg = ib::Singleton<ib::Config>::getInstance();

  textfont(ib::Fonts::input);
  textcolor(cfg->getStyleInputFontColor());
  cursor_color(cfg->getStyleInputFontColor());
  textsize(cfg->getStyleInputFontSize());
  color(cfg->getStyleInputBgColor());
  selection_color(cfg->getStyleInputSelectionBgColor());
  box((Fl_Boxtype)cfg->getStyleInputBoxtype());
  Fl::add_timeout(0.5,_blink_cursor, this);
} /* }}} */

bool ib::Input::isEmpty() const { // {{{
  const auto length = strlen(value());
  return length == 0 || (isUsingCwd() && length == 1);
} // }}}

// key_event_handler {{{
static void _main_thread_awaker(void *p){
  ib::Singleton<ib::Controller>::getInstance()->showCompletionCandidates();
}

void ib::_key_event_handler(void *p) {
  Fl::awake(_main_thread_awaker, 0);
}
// }}}

int ib::Input::handle(int e){ /* {{{ */
  int accept = 0;
  const auto* const cfg = ib::Singleton<ib::Config>::getInstance();
  const auto controller = ib::Singleton<ib::Controller>::getInstance();
  auto key = Fl::event_key();
  const auto state = Fl::event_state();
  const auto mods = state & (FL_META|FL_CTRL|FL_ALT);
  const auto shift = state & FL_SHIFT;
  const auto selected = (position() != mark()) ? 1 : 0;

  switch(e){
    case FL_KEYUP:
      key = ib::platform::convert_keysym(key);
      if(!getImeComposition() && key != 0xe5 && key != 0xfee9){
keyup:
        // ignore shift key up
        if(key == 65505 && state == 0) { return 1; }
        // ignore hot key
        if(ib::utils::matches_key(cfg->getHotKey(), key, state)) { return 1;}
        // calls an event handler
        lua_getglobal(IB_LUA, "on_key_up");
        if (lua_pcall(IB_LUA, 0, 1, 0)) {
            ib::utils::message_box("%s", lua_tostring(IB_LUA, lua_gettop(IB_LUA)));
            return 1;
        }
        accept = (int)lua_tonumber(IB_LUA, 1);
        ib::Singleton<ib::MainLuaState>::getInstance()->clearStack();
        if(accept) { return 1; }
        if(
           // paste
           (key == FL_Insert && mods==0 && shift) ||
           (key == 'v' && mods==FL_COMMAND) ||
           // cut
           //(mods == 0 && shift && selected) ||
           (key == 'x' && mods==FL_COMMAND) ||
           // kill word
           ib::utils::matches_key(cfg->getKillWordKey(), key, state)
           ){
          key_event_.cancelEvent();
          controller->showCompletionCandidates();
          return 1;
        }

        if(ib::utils::matches_key(cfg->getListNextKey(), key, state) ||
           ib::utils::matches_key(cfg->getListPrevKey(), key, state) || 
           ib::utils::matches_key(cfg->getToggleModeKey(), key, state) ||
           key == FL_Enter
            ){
          key_event_.cancelEvent();
          return 1;
        }

        if(!ib::utils::event_key_is_control_key()){
          key_event_.queueEvent((void*)1);
          return 1;
        }else{
          key_event_.cancelEvent();
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
        if(ib::utils::matches_key(cfg->getHotKey(), key, state)) { return 1;}
        // calls an event handler
        lua_getglobal(IB_LUA, "on_key_down");
        if (lua_pcall(IB_LUA, 0, 1, 0)) {
            ib::utils::message_box("%s", lua_tostring(IB_LUA, lua_gettop(IB_LUA)));
            return 1;
        }
        accept = (int)lua_tonumber(IB_LUA, 1);
        ib::Singleton<ib::MainLuaState>::getInstance()->clearStack();
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
          ib::Singleton<ib::MainWindow>::getInstance()->updateIconbox();
          return 1;
        }
        
        if(key == FL_Enter){

          lua_getglobal(IB_LUA, "on_enter");
          if (lua_pcall(IB_LUA, 0, 1, 0)) {
              ib::utils::message_box("%s", lua_tostring(IB_LUA, lua_gettop(IB_LUA)));
              return 1;
          }
          accept = (int)lua_tonumber(IB_LUA, 1);
          ib::Singleton<ib::MainLuaState>::getInstance()->clearStack();
          if(!accept){
            controller->executeCommand();
          }
          return 1;
        }
        if(ib::utils::matches_key(cfg->getEscapeKey(), key, state)){
          controller->hideApplication();
          return 1;
        }
        if(ib::utils::matches_key(cfg->getListNextKey(), key, state)){
          controller->selectNextCompletion();
          return 1;
        }
        if(ib::utils::matches_key(cfg->getListPrevKey(), key, state)){
          controller->selectPrevCompletion();
          return 1;
        }
        if(ib::utils::matches_key(cfg->getToggleModeKey(), key, state)){
          controller->toggleHistorySearchMode();
          return 1;
        }
        if(ib::utils::matches_key(cfg->getKillWordKey(), key, state)){
          controller->killWord();
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
        const auto m = rclick_menu->popup(Fl::event_x(), Fl::event_y(), 0, 0, 0);
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
  auto cursor_pos = static_cast<unsigned int>(position());
  prev_cursor_token_index_ = cursor_token_index_;
  cursor_token_index_ = 0;
  ib::Token *token = nullptr;
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
  ib::Singleton<ib::MainWindow>::getInstance()->clearIconbox();
} // }}}

void ib::Input::adjustSize() { // {{{
  const auto* const cfg = ib::Singleton<ib::Config>::getInstance();
  const auto mainwin = ib::Singleton<ib::MainWindow>::getInstance();
  int screen_x, screen_y, screen_w, screen_h;
  Fl::screen_xywh(screen_x, screen_y, screen_w, screen_h, 0, 0);

  const auto min_window_width = cfg->getStyleWindowWidth();
  const auto pad_x = cfg->getStyleWindowPadx();
  const auto max_window_width = screen_w - (screen_w/2 - min_window_width/2);
  Fl_Font font = textfont();
  auto tsize = cfg->getStyleInputFontSize();
  fl_font(font, tsize);
  auto isize = fl_width(value()) + tsize;
  const auto curret_w = w();
  auto new_window_width = mainwin->w() + (isize - curret_w);
  auto new_input_width  = w() + (isize - curret_w);
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
    return ib::Singleton<ib::NullToken>::getInstance();
  }
  return lexer_.getTokens().at(cursor_token_index_);
} // }}}

const std::string& ib::Input::getCursorValue() const { // {{{
  return getCursorToken()->getValue();
} // }}}

void ib::Input::draw() { // {{{
  if (input_type() == FL_HIDDEN_INPUT) return;
  const auto b = box();
  if (damage() & FL_DAMAGE_ALL) draw_box(b, color());
  adjustSize();
  Fl_Input_::drawtext(x()+Fl::box_dx(b), y()+Fl::box_dy(b), w()-Fl::box_dw(b), h()-Fl::box_dh(b));
} // }}}
/* }}} */

// class MainWindow {{{
void ib::MainWindow::initLayout(){ /* {{{ */
  begin();
  int screen_x, screen_y, screen_w, screen_h;
  const auto* const cfg = ib::Singleton<ib::Config>::getInstance();

  Fl::screen_xywh(screen_x, screen_y, screen_w, screen_h, 0, 0);
  const auto w = cfg->getStyleWindowWidth();
  const auto h = cfg->getStyleWindowHeight();
  const auto pad_x = cfg->getStyleWindowPadx();
  const auto pad_y = cfg->getStyleWindowPady();

  clear_border();
  color(cfg->getStyleWindowBgColor());
  const auto x = cfg->getStyleWindowPosxAuto() ? screen_w/2 - w/2 : cfg->getStyleWindowPosx();
  const auto y = cfg->getStyleWindowPosyAuto() ? screen_h/2 - h/2 : cfg->getStyleWindowPosy();
  resize(x, y, w, h);
  box((Fl_Boxtype)cfg->getStyleWindowBoxtype());

  iconbox_ = new Fl_Box(pad_x, h/2 - 16, IB_ICON_SIZE_LARGE, IB_ICON_SIZE_LARGE);
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
  if(!ib::Singleton<ib::Config>::getInstance()->getEnableIcons()) return;
  auto image = iconbox_->image();
  if(image) delete image;
  iconbox_->image(ib::Singleton<ib::IconManager>::getInstance()->getEmptyIcon(IB_ICON_SIZE_LARGE,IB_ICON_SIZE_LARGE));
  redraw();
} // }}}

void ib::MainWindow::setIconbox(Fl_Image *image) { // {{{
  if(!ib::Singleton<ib::Config>::getInstance()->getEnableIcons()) return;
  auto bimage = iconbox_->image();
  if(bimage) delete bimage;
  iconbox_->image(image);
  redraw();
}
/* }}} */

void ib::MainWindow::updateIconbox() { // {{{
  if(!ib::Singleton<ib::Config>::getInstance()->getEnableIcons()) return;
  const auto input = ib::Singleton<ib::MainWindow>::getInstance()->getInput();
  if(input->isEmpty()) {
    clearIconbox();
    return;
  }
  const auto &cmd = input->getFirstValue();
  const auto &commands = ib::Singleton<ib::Controller>::getInstance()->getCommands();
  auto it = commands.find(cmd);
  if(it != commands.end()){
    setIconbox((*it).second->loadIcon(IB_ICON_SIZE_LARGE));
  }else{
    CompletionString comp_value(cmd.c_str());
    setIconbox(comp_value.loadIcon(IB_ICON_SIZE_LARGE));
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
  auto l = reinterpret_cast<FL_BLINE*>(item);
  auto str = l->txt;
  const auto* const cfg = ib::Singleton<ib::Config>::getInstance();

  const auto line = reinterpret_cast<std::size_t>(l->data);
  if(line%2 == 0){
    if (l->flags & 1) {
      fl_color(cfg->getStyleListSelectionBgColor1());
    }else{
      fl_color(cfg->getStyleListBgColor1());
    }
  }else{
    if (l->flags & 1) {
      fl_color(cfg->getStyleListSelectionBgColor2());
    }else{
      fl_color(cfg->getStyleListBgColor2());
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
  char *description = nullptr;
  if(ptr) {
    *ptr = 0;
    description = ptr+1;
  }

  left += 2;
  width -= 2;
  int tsize = cfg->getStyleListFontSize();
  Fl_Font font = textfont();
  Fl_Color lcol = textcolor();
  if (l->flags & 1) lcol = cfg->getStyleListSelectionFontColor();
  if (!active_r())  lcol = fl_inactive(lcol);
  fl_font(font, tsize);
  fl_color(lcol);
  fl_draw(str, left, top);

  if(description){
    top += cfg->getStyleListFontSize();
    *ptr = '\t';
    lcol = cfg->getStyleListDescFontColor();
    if (l->flags & 1) lcol = cfg->getStyleListSelectionDescFontColor();
    if (!active_r())  lcol = fl_inactive(lcol);
    fl_font(font, cfg->getStyleListDescFontSize());
    fl_color(lcol);
    fl_draw(description, left, top);
  }
} // }}}

int ib::Listbox::item_height(void *item) const { // {{{
  auto l = reinterpret_cast<FL_BLINE*>(item);
  const auto str = l->txt;
  if(strchr(str, '\t') != 0){
    return IB_ICON_SIZE_LARGE + 4;
  }else{
    return IB_ICON_SIZE_SMALL + 4;
  }
} // }}}

int ib::Listbox::item_width(void *item) const{ // {{{
  auto l = reinterpret_cast<FL_BLINE*>(item);
  char* str = l->txt;
  const auto* const cfg = ib::Singleton<ib::Config>::getInstance();

  char *ptr = strchr(str, '\t');
  char *description = nullptr;
  if(ptr) {
    *ptr = 0;
    description = ptr+1;
  }
  int left_pad = 2;
  if(cfg->getEnableIcons()){
    left_pad += 2 + (description ? IB_ICON_SIZE_LARGE : IB_ICON_SIZE_SMALL);
  }

  int tsize = cfg->getStyleListFontSize();
  Fl_Font font = textfont();
  fl_font(font, tsize);
  size_t w1 = fl_width(str) + tsize;
  size_t w2 = 0;

  if(description){
    *ptr = '\t';
    tsize = cfg->getStyleListDescFontSize();
    fl_font(font, tsize);
    w2 = fl_width(description) + tsize;
  }

  return (int)(std::max<int>((int)w1, (int)w2)) + left_pad;
} // }}}

void ib::Listbox::initLayout(){ // {{{
  const auto* const cfg = ib::Singleton<ib::Config>::getInstance();

  textfont(ib::Fonts::list);
  textcolor(cfg->getStyleListFontColor());
  textsize(cfg->getStyleListFontSize());
  color(cfg->getStyleListBgColor1());
  selection_color(cfg->getStyleListSelectionBgColor1());
  has_scrollbar(Fl_Browser_::VERTICAL);
  box(FL_FLAT_BOX);
  static int widths[] = { (int)cfg->getStyleListFontSize()*2+3, 1, 0};
  column_widths(widths);
  scrollbar.selection_color(cfg->getStyleListBgColor1());
  scrollbar.color(cfg->getStyleListBgColor1());
  scrollbar.labelcolor(cfg->getStyleListFontColor());
} // }}}

int ib::Listbox::handle(int e){ /* {{{ */
  if(e == FL_RELEASE) return 1;
  const auto controller = ib::Singleton<ib::Controller>::getInstance();
  auto selected_b = value();
  auto ret = Fl_Select_Browser::handle(e);
  auto selected_a = value();
  const auto is_leftclick = e == FL_PUSH && Fl::event_button() == FL_LEFT_MOUSE;
  const auto is_rightclick = e == FL_PUSH && Fl::event_button() == FL_RIGHT_MOUSE;
  if(selected_b != selected_a || 
    (is_leftclick && selected_a == 1 && selected_b == 1)) {
    selectLine(selected_a, false);
    controller->completionInput();
    ret = 1;
  }
  if(is_rightclick){
    const auto context_path = getValues().at(value()-1)->getContextMenuPath();
    if(context_path != nullptr){
      ib::oschar osbuf[IB_MAX_PATH];
      ib::platform::utf82oschar_b(osbuf, IB_MAX_PATH, context_path->c_str());
      ib::platform::show_context_menu(osbuf);
    }
    ret = 1;
  }
  if(is_leftclick && Fl::event_clicks() > 0) {
    Fl::event_clicks(0);
    controller->executeCommand();
  }
  return ret;
} /* }}} */

void ib::Listbox::addValue(ib::CompletionValue* value){ // {{{
  values_.push_back(value);
} // }}}

void ib::Listbox::removeValue(int line){ // {{{
  ib::platform::ScopedLock lock(&mutex_);
  incOperationCount();
  destroyIcon(line);
  remove(line);
  values_.erase(values_.begin() + line -1);
} // }}}

ib::CompletionValue* ib::Listbox::selectedValue() const { // {{{
  return values_.at(value()-1);
} // }}}

void ib::Listbox::clearAll(){ // {{{
  ib::platform::ScopedLock lock(&mutex_);
  incOperationCount();
  const auto main_window = ib::Singleton<ib::MainWindow>::getInstance();

  if(main_window->getInput()->getCursorTokenIndex() == 0) {
    main_window->clearIconbox();
  }
  for(int i = 1, last = size(); i <= last; ++i){ destroyIcon(i); }
  for(auto &v : values_) {
    if(v->isTemporary()){ delete v; }
  }
  values_.clear();
  std::vector<ib::CompletionValue*>().swap (values_);
  is_autocompleted_ = false;
  max_width_ = 0;
  clear();
  ib::Singleton<ib::IconManager>::getInstance()->shrinkCache();
} // }}}

void ib::Listbox::destroyIcon(const int line){ // {{{
  auto image_icon = icon(line);
  if(image_icon == nullptr) return;

  remove_icon(line);
  icon(line, nullptr);
  delete image_icon;
} // }}}

void ib::Listbox::startUpdate(){ /* {{{ */
  ib::platform::ScopedLock lock(&mutex_);
  incOperationCount();
} /* }}} */

void ib::Listbox::endUpdate(const bool use_max_candidates){ /* {{{ */
  const auto* const cfg = ib::Singleton<ib::Config>::getInstance();
  {
    ib::platform::ScopedLock lock(&mutex_);
    incOperationCount();
    if(size() == 0) { max_width_ = 0;}
    if(isEmpty()) return;

    auto max_candidates = cfg->getMaxCandidates();
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
  }

  if(cfg->getEnableIcons()){
    ib::Singleton<ib::IconManager>::getInstance()->loadCompletionListIcons();
  }
} /* }}} */

void ib::Listbox::setEmptyIcons(){ /* {{{ */
  if(isEmpty()) return;
  unsigned int icon_height;
  for(int i=1, last = size(); i <= last; ++i){
    icon_height = values_.at(i-1)->hasDescription() ? IB_ICON_SIZE_LARGE : IB_ICON_SIZE_SMALL;
    auto icon_width = icon_height;
    if(!ib::Singleton<ib::Config>::getInstance()->getEnableIcons()){
      icon_width = 1;
    }

    icon(i, ib::Singleton<ib::IconManager>::getInstance()->getEmptyIcon(icon_width, icon_height));
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
  if(ib::Singleton<ib::MainWindow>::getInstance()->getInput()->getCursorTokenIndex() == 0) {
    ib::Singleton<ib::MainWindow>::getInstance()->setIconbox(selectedValue()->loadIcon(IB_ICON_SIZE_LARGE));
  }
} // }}
// }}}

void ib::Listbox::adjustSize() { // {{{
  const auto main_window = ib::Singleton<ib::MainWindow>::getInstance();
  const auto list_window = ib::Singleton<ib::ListWindow>::getInstance();
  const auto* const cfg   = ib::Singleton<ib::Config>::getInstance();
  const auto font_size    = cfg->getStyleListFontSize();

  int screen_x, screen_y, screen_w, screen_h;
  Fl::screen_xywh(screen_x, screen_y, screen_w, screen_h, 0, 0);
  const auto width_a = screen_w - main_window->x() - font_size;
  auto width_b = getMaxWidth() + font_size*3;
  if(has_scrollbar() & Fl_Browser_::VERTICAL){
    width_b -= Fl::scrollbar_size();
  }
  const auto new_width = std::max(static_cast<int>(cfg->getStyleWindowWidth()), std::min<int>(width_b , width_a));

  const auto height_a = screen_h - (main_window->y()+main_window->h()+cfg->getStyleTaskbarHeight());
  const auto height_b = item_height(item_first()) * size() + font_size;

  const auto new_height = ((height_a > height_b) ? height_b : height_a);
  resize(x(), y(), 
      new_width - cfg->getStyleListPadx()*2, 
      new_height - cfg->getStyleListPady()*2);
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
  resize(x(), y(), 0, 0);
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
  const auto* const cfg = ib::Singleton<ib::Config>::getInstance();
  const auto main_window = ib::Singleton<ib::MainWindow>::getInstance();

  const auto pad_x = cfg->getStyleListPadx();
  const auto pad_y = cfg->getStyleListPady();

  const auto main_x = main_window->x();
  const auto main_y = main_window->y();
  const auto main_w = main_window->w();
  const auto main_h = main_window->h();
  resize(main_x, main_y + main_h, main_w, 1);
  clear_border();
  color(cfg->getStyleListBorderColor());
  box((Fl_Boxtype)cfg->getStyleListBoxtype());

  listbox_ = new ib::Listbox(pad_x , pad_y , w()-pad_x*2, h()-pad_y*2);
  listbox_->initLayout();

  end();
} /* }}} */

int ib::ListWindow::handle(int e){ /* {{{ */
  return Fl_Window::handle(e);
} /* }}} */
// }}}

static void ib_mb_ok_callback(Fl_Widget*, void *userdata) { // {{{
  ib::Singleton<ib::MessageBox>::getInstance()->hide();
  ib::Singleton<ib::Controller>::getInstance()->showApplication();
} // }}}

// class MessageBox {{{

void ib::MessageBox::initLayout(){ // {{{
  const auto* const cfg = ib::Singleton<ib::Config>::getInstance();
  begin();
  clear_border();
  set_override();
  position(100, 100);
  size(300, 300);
  color(cfg->getStyleListFontColor());

  output_ = new Fl_Multiline_Output(3, 3, 100, 100);
  output_->box(FL_FLAT_BOX);
  output_->color(cfg->getStyleListBgColor1());
  output_->textfont(ib::Fonts::list);
  output_->textcolor(cfg->getStyleListFontColor());
  output_->textsize(15);

  button_ = new Fl_Button(0,0,100,20, "OK");
  button_->callback(ib_mb_ok_callback);
  button_->box(FL_FLAT_BOX);
  button_->labelfont(ib::Fonts::list);
  button_->labelcolor(cfg->getStyleListBgColor1());
  button_->labelsize(15);
  button_->color(cfg->getStyleListFontColor());

  end();
} /* }}} */

void ib::MessageBox::hide(){ /* {{{ */
  clear_visible();
  Fl_Window::hide();
  ib::platform::hide_window(this);
} /* }}} */

void ib::MessageBox::show(){ /* {{{ */
#ifndef IB_OS_WIN
  ib::platform::move_to_current_desktop(this);
#endif
  set_modal();
  Fl_Window::show();
  while (shown()) Fl::wait();
  output_->value("");
  message_.clear();
} /* }}} */

void ib::MessageBox::adjustSize(){ /* {{{ */
  std::stringstream ss(message_);
  std::string to;
  int screen_x, screen_y, screen_w, screen_h;
  const auto mainwin = ib::Singleton<ib::MainWindow>::getInstance();
  double width = 0;
  int height = 0;

  Fl::screen_xywh(screen_x, screen_y, screen_w, screen_h, 0, 0);
  int h = fl_height(button_->labelfont(), button_->labelsize());
  fl_font(button_->labelfont(), button_->labelsize());
  while(std::getline(ss,to,'\n')){
    width = std::max(width, fl_width(to.c_str()));
    height += h;
  }
  width += 10 * 2 + button_->labelsize()*3;
  height += h * 4;

  const auto x = screen_w/2 - width/2;
  const auto y = screen_h/2 - height/2 + (mainwin->x() > 200 ? -50 : 50);

  resize(x, y, width, height);
  output_->resize(3, 3, width - 6, height - 6);
  button_->position(width/2 - fl_width("OK"), height - h * 2);
  button_->size(fl_width("OK")+20, h);
} /* }}} */

void ib::MessageBox::show(const char *message){ /* {{{ */
  message_ = message;
  adjustSize();
  output_->value(message_.c_str());
  redraw();
  show();
} /* }}} */

int ib::MessageBox::handle(int e){ /* {{{ */
  const auto state = Fl::event_state();
  const auto mods = state & (FL_META|FL_CTRL|FL_ALT);
  int ret=0;

  if(shown()) {
    if(e == FL_KEYDOWN && Fl::event_key() == FL_Enter) {
      ib_mb_ok_callback(this, this);
      ret = 1;
    } else if(Fl::event_key() == 'c' && mods == FL_COMMAND) {
      ib::utils::set_clipboard(message_.c_str());
    }
  }
  return ret ? 1 : button_->handle(e);
} /* }}} */

// }}}
