#ifndef __IB_CONFIG_H__
#define __IB_CONFIG_H__

#include "ib_constants.h"
#include "ib_utils.h"
#include "ib_search_path.h"
#include "ib_completer.h"

namespace ib {

  class Config : public Singleton<Config>{ // {{{
    friend class Singleton<Config>;
    public:
      virtual ~Config() {
        ib::utils::delete_pointer_vectors(search_path_);
        delete completer_;
      }

      unsigned int getDefaultSearchPathDepth() const { return default_search_path_depth_; }
      void setDefaultSearchPathDepth(const unsigned int value){ default_search_path_depth_ = value; }

      bool getEnableIcons() const { return enable_icons_; }
      void setEnableIcons(const bool value){ enable_icons_ = value; }

      unsigned int getMaxCachedIcons() const { return max_cached_icons_; }
      void setMaxCachedIcons(const unsigned int value){ max_cached_icons_ = value; }

      unsigned int getKeyEventThreshold() const { return key_event_threshold_; }
      void setKeyEventThreshold(const unsigned int value){ key_event_threshold_ = value; }

      unsigned int getMaxHistories() const { return max_histories_; }
      void setMaxHistories(const unsigned int value){ max_histories_ = value; }

      unsigned int getMaxCandidates() const { return max_candidates_; }
      void setMaxCandidates(const unsigned int value){ max_candidates_ = value; }

      double getHistoryFactor() const { return history_factor_; }
      void setHistoryFactor(const double value){ history_factor_ = value; }

      const std::string& getFileBrowser() const { return file_browser_; }
      void setFileBrowser(const std::string &value){ file_browser_ = value; }
      void setFileBrowser(const char *value){ file_browser_ = value; }

      unsigned int getServerPort() const { return server_port_; }
      void setServerPort(const unsigned int value){ server_port_ = value; }

      const std::vector<ib::SearchPath*>& getSearchPath() const { return search_path_; }
      void addSearchPath(ib::SearchPath *search_path) {
        search_path_.push_back(search_path);
      }

      ib::Completer* getCompleter() const { return completer_; }
      void setCompleter(ib::Completer *value){ completer_ = value; }

      const int* getHotKey() const { return hot_key_; }
      void setHotKey(const int *value){ memcpy(hot_key_, value, sizeof(int)*3); }

      const int* getEscapeKey() const { return escape_key_; }
      void setEscapeKey(const int *value){ memcpy(escape_key_, value, sizeof(int)*3); }

      const int* getListNextKey() const { return list_next_key_; }
      void setListNextKey(const int *value){ memcpy(list_next_key_, value, sizeof(int)*3); }

      const int* getListPrevKey() const { return list_prev_key_; }
      void setListPrevKey(const int *value){ memcpy(list_prev_key_, value, sizeof(int)*3); }

      const int* getToggleModeKey() const { return toggle_mode_key_; }
      void setToggleModeKey(const int *value){ memcpy(toggle_mode_key_, value, sizeof(int)*3); }

      const int* getKillWordKey() const { return kill_word_key_; }
      void setKillWordKey(const int *value){ memcpy(kill_word_key_, value, sizeof(int)*3); }

      Fl_Boxtype getStyleWindowBoxtype() const { return style_window_boxtype_; }
      void setStyleWindowBoxtype(const Fl_Boxtype value){ style_window_boxtype_ = value; }

      unsigned int getStyleWindowPosx() const { return style_window_posx_; }
      void setStyleWindowPosx(const unsigned int value){ style_window_posx_ = value; }

      unsigned int getStyleWindowPosy() const { return style_window_posy_; }
      void setStyleWindowPosy(const unsigned int value){ style_window_posy_ = value; }

      bool getStyleWindowPosxAuto() const { return style_window_posx_auto_; }
      void setStyleWindowPosxAuto(const bool value){ style_window_posx_auto_ = value; }

      bool getStyleWindowPosyAuto() const { return style_window_posy_auto_; }
      void setStyleWindowPosyAuto(const bool value){ style_window_posy_auto_ = value; }

      unsigned int getStyleWindowWidth() const { return style_window_width_; }
      void setStyleWindowWidth(const unsigned int value){ style_window_width_ = value; }

      unsigned int getStyleWindowHeight() const { return style_window_height_; }
      void setStyleWindowHeight(const unsigned int value){ style_window_height_ = value; }

      unsigned int getStyleWindowPadx() const { return style_window_padx_; }
      void setStyleWindowPadx(const unsigned int value){ style_window_padx_ = value; }

      unsigned int getStyleWindowPady() const { return style_window_pady_; }
      void setStyleWindowPady(const unsigned int value){ style_window_pady_ = value; }

      Fl_Color getStyleWindowBgColor() const { return style_window_bg_color_; }
      void setStyleWindowBgColor(const Fl_Color value){ style_window_bg_color_ = value; }

      unsigned int getStyleTaskbarHeight() const { return style_taskbar_height_; }
      void setStyleTaskbarHeight(const unsigned int value){ style_taskbar_height_ = value; }

      unsigned int getStyleWindowAlpha() const { return style_window_alpha_; }
      void setStyleWindowAlpha(const unsigned int value){ style_window_alpha_ = value; }

      Fl_Boxtype getStyleInputBoxtype() const { return style_input_boxtype_; }
      void setStyleInputBoxtype(const Fl_Boxtype value){ style_input_boxtype_ = value; }

      const std::string& getStyleInputFont() const { return style_input_font_; }
      void setStyleInputFont(const std::string &value){ style_input_font_ = value; }
      void setStyleInputFont(const char *value){ style_input_font_ = value; }

      unsigned int getStyleInputFontSize() const { return style_input_font_size_; }
      void setStyleInputFontSize(const unsigned int value){ style_input_font_size_ = value; }

      Fl_Color getStyleInputFontColor() const { return style_input_font_color_; }
      void setStyleInputFontColor(const Fl_Color value){ style_input_font_color_ = value; }

      Fl_Color getStyleInputBgColor() const { return style_input_bg_color_; }
      void setStyleInputBgColor(const Fl_Color value){ style_input_bg_color_ = value; }

      Fl_Color getStyleInputSelectionBgColor() const { return style_input_selection_bg_color_; }
      void setStyleInputSelectionBgColor(const Fl_Color value){ style_input_selection_bg_color_ = value; }

      Fl_Boxtype getStyleListBoxtype() const { return style_list_boxtype_; }
      void setStyleListBoxtype(const Fl_Boxtype value){ style_list_boxtype_ = value; }

      unsigned int getStyleListPadx() const { return style_list_padx_; }
      void setStyleListPadx(const unsigned int value){ style_list_padx_ = value; }

      unsigned int getStyleListPady() const { return style_list_pady_; }
      void setStyleListPady(const unsigned int value){ style_list_pady_ = value; }

      const std::string& getStyleListFont() const { return style_list_font_; }
      void setStyleListFont(const std::string &value){ style_list_font_ = value; }
      void setStyleListFont(const char *value){ style_list_font_ = value; }

      unsigned int getStyleListFontSize() const { return style_list_font_size_; }
      void setStyleListFontSize(const unsigned int value){ style_list_font_size_ = value; }

      unsigned int getStyleListDescFontSize() const { return style_list_desc_font_size_; }
      void setStyleListDescFontSize(const unsigned int value){ style_list_desc_font_size_ = value; }

      Fl_Color getStyleListFontColor() const { return style_list_font_color_; }
      void setStyleListFontColor(const Fl_Color value){ style_list_font_color_ = value; }

      Fl_Color getStyleListDescFontColor() const { return style_list_desc_font_color_; }
      void setStyleListDescFontColor(const Fl_Color value){ style_list_desc_font_color_ = value; }

      Fl_Color getStyleListSelectionFontColor() const { return style_list_selection_font_color_; }
      void setStyleListSelectionFontColor(const Fl_Color value){ style_list_selection_font_color_ = value; }

      Fl_Color getStyleListSelectionDescFontColor() const { return style_list_selection_desc_font_color_; }
      void setStyleListSelectionDescFontColor(const Fl_Color value){ style_list_selection_desc_font_color_ = value; }

      Fl_Color getStyleListBgColor1() const { return style_list_bg_color1_; }
      void setStyleListBgColor1(const Fl_Color value){ style_list_bg_color1_ = value; }

      Fl_Color getStyleListBgColor2() const { return style_list_bg_color2_; }
      void setStyleListBgColor2(const Fl_Color value){ style_list_bg_color2_ = value; }

      Fl_Color getStyleListSelectionBgColor1() const { return style_list_selection_bg_color1_; }
      void setStyleListSelectionBgColor1(const Fl_Color value){ style_list_selection_bg_color1_ = value; }

      Fl_Color getStyleListSelectionBgColor2() const { return style_list_selection_bg_color2_; }
      void setStyleListSelectionBgColor2(const Fl_Color value){ style_list_selection_bg_color2_ = value; }

      Fl_Color getStyleListBorderColor() const { return style_list_border_color_; }
      void setStyleListBorderColor(const Fl_Color value){ style_list_border_color_ = value; }

      unsigned int getStyleListAlpha() const { return style_list_alpha_; }
      void setStyleListAlpha(const unsigned int value){ style_list_alpha_ = value; }

      /* command line args & internal values */

      bool isLoaded() const { return loaded_; }
      void setLoaded(const bool value){ loaded_ = value; }

      const std::string& getSelfPath() const { return self_path_; }
      void setSelfPath(const std::string & value){ self_path_ = value; }
      void setSelfPath(const char *value){ self_path_ = value; }

      const std::string& getInitialWorkdir() const { return initial_workdir_; }
      void setInitialWorkdir(const std::string & value){ initial_workdir_ = value; }
      void setInitialWorkdir(const char *value){ initial_workdir_ = value; }

      const std::string& getConfigPath() const { return config_path_; }
      void setConfigPath(const std::string & value){ config_path_ = value; }
      void setConfigPath(const char *value){ config_path_ = value; }

      const std::string& getCommandCachePath() const { return command_cache_path_; }
      void setCommandCachePath(const std::string & value){ command_cache_path_ = value; }
      void setCommandCachePath(const char *value){ command_cache_path_ = value; }

      const std::string& getHistoryPath() const { return history_path_; }
      void setHistoryPath(const std::string & value){ history_path_ = value; }
      void setHistoryPath(const char *value){ history_path_ = value; }

      const std::string& getIconCachePath() const { return icon_cache_path_; }
      void setIconCachePath(const std::string & value){ icon_cache_path_ = value; }
      void setIconCachePath(const char *value){ icon_cache_path_ = value; }

      const std::string& getMigemoDictPath() const { return migemo_dict_path_; }
      void setMigemoDictPath(const std::string & value){ migemo_dict_path_ = value; }
      void setMigemoDictPath(const char *value){ migemo_dict_path_ = value; }

      int getOldPid() const { return old_pid_; }
      void setOldPid(const  int value){ old_pid_ = value; }

      const std::string& getPlatform() const { return platform_; }
      void setPlatform(const std::string &value){ platform_ = value; }
      void setPlatform(const char *value){ platform_ = value; }

      const std::string& getIpcMessage() const { return ipc_message_; }
      void setIpcMessage(const std::string &value){ ipc_message_ = value; }
      void setIpcMessage(const char *value){ ipc_message_ = value; }

    protected:
      Config() :
        default_search_path_depth_(5),
        enable_icons_(true),
        max_cached_icons_(1000),
        key_event_threshold_(200),
        max_histories_(200),
        max_candidates_(0),
        history_factor_(0.3),
        file_browser_(),
        server_port_(0),
        search_path_(),
        completer_(0),

        hot_key_(),
        escape_key_(),
        list_next_key_(),
        list_prev_key_(),
        toggle_mode_key_(),
        kill_word_key_(),

        style_window_boxtype_(FL_FLAT_BOX),
        style_window_posx_(0),
        style_window_posy_(0),
        style_window_posx_auto_(true),
        style_window_posy_auto_(true),
        style_window_width_(300),
        style_window_height_(50),
        style_window_padx_(5),
        style_window_pady_(5),
        style_window_bg_color_(fl_rgb_color(0,0,0)),
        style_taskbar_height_(0),
        style_window_alpha_(255),

        style_input_boxtype_(FL_FLAT_BOX),
        style_input_font_("Arial"),
        style_input_font_size_(16),
        style_input_font_color_(fl_rgb_color(255,255,255)),
        style_input_bg_color_(fl_rgb_color(50,50,50)),
        style_input_selection_bg_color_(fl_rgb_color(64,64,64)),

        style_list_boxtype_(FL_FLAT_BOX),
        style_list_padx_(5),
        style_list_pady_(5),
        style_list_font_("Arial"),
        style_list_font_size_(16),
        style_list_desc_font_size_(12),
        style_list_font_color_(fl_rgb_color(255,255,255)),
        style_list_desc_font_color_(fl_rgb_color(200,200,200)),
        style_list_selection_font_color_(fl_rgb_color(255,255,255)),
        style_list_selection_desc_font_color_(fl_rgb_color(200,200,200)),
        style_list_bg_color1_(fl_rgb_color(50,50,50)),
        style_list_selection_bg_color1_(fl_rgb_color(120,120,120)),
        style_list_bg_color2_(fl_rgb_color(50,50,50)),
        style_list_selection_bg_color2_(fl_rgb_color(120,120,120)),
        style_list_border_color_(fl_rgb_color(120,120,120)),
        style_list_alpha_(255),

        loaded_(false),
        self_path_(),
        initial_workdir_(),
        config_path_(),
        command_cache_path_(),
        history_path_(),
        icon_cache_path_(),
        migemo_dict_path_(),
        old_pid_(-1),
        platform_(),
        ipc_message_()
        {}


      unsigned int default_search_path_depth_;
      bool         enable_icons_;
      unsigned int max_cached_icons_;
      unsigned int key_event_threshold_;
      unsigned int max_histories_;
      unsigned int max_candidates_;
      double       history_factor_;
      std::string  file_browser_;
      unsigned int server_port_;
      std::vector<ib::SearchPath*> search_path_;
      ib::Completer *completer_;

      int hot_key_[3];
      int escape_key_[3];
      int list_next_key_[3];
      int list_prev_key_[3];
      int toggle_mode_key_[3];
      int kill_word_key_[3];

      Fl_Boxtype style_window_boxtype_;
      unsigned int style_window_posx_;
      unsigned int style_window_posy_;
      bool style_window_posx_auto_;
      bool style_window_posy_auto_;
      unsigned int style_window_width_;
      unsigned int style_window_height_;
      unsigned int style_window_padx_;
      unsigned int style_window_pady_;
      Fl_Color style_window_bg_color_;
      unsigned int style_taskbar_height_;
      unsigned int style_window_alpha_;

      Fl_Boxtype style_input_boxtype_;
      std::string style_input_font_;
      unsigned int style_input_font_size_;
      Fl_Color style_input_font_color_;
      Fl_Color style_input_bg_color_;
      Fl_Color style_input_selection_bg_color_;

      Fl_Boxtype style_list_boxtype_;
      unsigned int style_list_padx_;
      unsigned int style_list_pady_;
      std::string style_list_font_;
      unsigned int style_list_font_size_;
      unsigned int style_list_desc_font_size_;
      Fl_Color style_list_font_color_;
      Fl_Color style_list_desc_font_color_;
      Fl_Color style_list_selection_font_color_;
      Fl_Color style_list_selection_desc_font_color_;
      Fl_Color style_list_bg_color1_;
      Fl_Color style_list_selection_bg_color1_;
      Fl_Color style_list_bg_color2_;
      Fl_Color style_list_selection_bg_color2_;
      Fl_Color style_list_border_color_;
      unsigned int style_list_alpha_;

      /* command line args & internal values */
      bool loaded_;
      std::string self_path_;
      std::string initial_workdir_;
      std::string config_path_;
      std::string command_cache_path_;
      std::string history_path_;
      std::string icon_cache_path_;
      std::string migemo_dict_path_;
      int old_pid_;
      std::string platform_;
      std::string ipc_message_;
  }; // }}}

}

#endif
