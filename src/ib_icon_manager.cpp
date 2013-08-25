#include "ib_icon_manager.h"
#include "ib_utils.h"
#include "ib_platform.h"
#include "ib_ui.h"
#include "ib_regex.h"
#include "ib_config.h"

ib::IconManager *ib::IconManager::instance_ = 0;

// icon_loader_thread {{{
void ib::_icon_loader(void *p) {
  ib::platform::on_thread_start();
  const int MAX_FLUSH = 200;

  auto manager = ib::IconManager::inst();
  auto listbox = ib::ListWindow::inst()->getListbox();

   std::vector<Fl_RGB_Image*> buf;
  int current_operation_count = -1;
  int listsize;
  int pos;
  int loaded_to_flush;


#define CHECK_THREAD_STATE \
    { ib::platform::ScopedLock lock(manager->getLoaderMutex()); \
      if(!manager->isRunning()) goto exit_thread; \
    }

  while(1){
    { ib::platform::ScopedCondition cond(manager->getLoaderCondition(), 0);

      CHECK_THREAD_STATE;
      { ib::platform::ScopedLock lock(listbox->getMutex());
        current_operation_count = listbox->getOperationCount();
        listsize = listbox->size();
        pos = 1;
        loaded_to_flush= 2;
        ib::utils::delete_pointer_vectors(buf);
      }

      for(int i=1;i <= listsize; ++i){
        CHECK_THREAD_STATE;
        { ib::platform::ScopedLock lock(listbox->getMutex());
          if(current_operation_count != listbox->getOperationCount()){ break; }
          int icon_size = listbox->getValues().at(i-1)->hasDescription() ? 32 : 16;
          buf.push_back(listbox->getValues().at(i-1)->loadIcon(icon_size));
        }

        if((i != 1 && (i-pos) % loaded_to_flush == 0) || i == listsize){
          { ib::FlScopedLock fflock;
            { ib::platform::ScopedLock lock(listbox->getMutex());
              if(current_operation_count != listbox->getOperationCount()){
                ib::utils::delete_pointer_vectors(buf);
                break;
              }
              for(auto it = buf.begin(), last = buf.end(); it != last; ++it, ++pos){
                if(*it != 0) {
                  listbox->destroyIcon(pos);
                  listbox->icon(pos, *it);
                }
              }
              buf.clear();
              loaded_to_flush = std::min<int>(loaded_to_flush*3, MAX_FLUSH);
              Fl::awake((void*)0);
            }
          }
        }
      }
    }
  }
exit_thread:
  ib::platform::exit_thread(0);
#undef CHECK_THREAD_STATE
} // }}}

void ib::IconManager::startLoaderThread() { // {{{
  running_ = true;
  ib::platform::create_thread(&loader_thread_, _icon_loader, 0);
} // }}}

void ib::IconManager::stopLoaderThread() { // {{{
  bool running = true;
  {
    ib::platform::ScopedLock lock(loader_mutex_);
    running = running_;
    if(running_) { running_ = false; }
  }
  if(!running) return;
  ib::platform::notify_condition(&loader_condition_);
  ib::platform::join_thread(&loader_thread_);
  ib::platform::destroy_mutex(&loader_mutex_);
  ib::platform::destroy_condition(&loader_condition_);
  ib::platform::destroy_mutex(&cache_mutex_);
} // }}}

void ib::IconManager::loadCompletionListIcons() { // {{{
  ib::platform::notify_condition(&loader_condition_);
} // }}}

void ib::IconManager::load() { // {{{
  ib::platform::ScopedLock lock(cache_mutex_);
  auto &cfg = ib::Config::inst();
  ib::unique_oschar_ptr osicon_path(ib::platform::utf82oschar(cfg.getIconCachePath().c_str()));
  if(!ib::platform::file_exists(osicon_path.get())) return;

  ib::unique_char_ptr loicon_path(ib::platform::utf82local(cfg.getIconCachePath().c_str()));
  std::ifstream ifs(loicon_path.get(), std::ios::in|std::ios::binary);

  deleteCachedIcons();
  int count;
  ifs.read((char*)&count, sizeof(int));
  for(int i = 0; i < count; ++i){
    int w;
    ifs.read((char*)&w, sizeof(int));
    int h;
    ifs.read((char*)&h, sizeof(int));
    int d;
    ifs.read((char*)&d, sizeof(int));

    std::string::size_type cache_key_size;
    ifs.read((char*)&cache_key_size, sizeof(std::string::size_type));
    char *cache_key_buf = new char[cache_key_size];
    ifs.read(cache_key_buf, cache_key_size);

    long data_size;
    ifs.read((char*)&data_size, sizeof(long));
    unsigned char* data_buf = new unsigned char[data_size];
    ifs.read((char*)data_buf, data_size);

    std::string cache_key(cache_key_buf);
    delete[] cache_key_buf;
    Fl_RGB_Image *icon = new Fl_RGB_Image(data_buf, w, h ,d);
    icon->alloc_array = true;
    createIconCache(cache_key, icon);
  }
  ifs.close();

} // }}}

void ib::IconManager::dump() { // {{{
  ib::platform::ScopedLock lock(cache_mutex_);
  auto &cfg = ib::Config::inst();
  ib::unique_char_ptr loicon_path(ib::platform::utf82local(cfg.getIconCachePath().c_str()));
  std::ofstream ofs(loicon_path.get(), std::ios::out|std::ios::binary|std::ios::trunc);
  int intvalue;
  char *intptr = (char*)&intvalue;

  // cache size
  intvalue = (int)std::min<std::size_t>(cached_icons_.size(), INT_MAX);
  ofs.write(intptr, sizeof(int));

  for(auto it = cached_icons_queue_.begin(), last = cached_icons_queue_.end(); it != last; ++it){
    const std::string cache_key = (*it);
    Fl_RGB_Image *icon = getIconCache(cache_key);
    intvalue = icon->w();
    ofs.write(intptr, sizeof(int));
    intvalue = icon->h();
    ofs.write(intptr, sizeof(int));
    intvalue = icon->d();
    ofs.write(intptr, sizeof(int));

    std::string::size_type size = cache_key.size()+1;
    ofs.write((char*)&size, sizeof(std::string::size_type));
    ofs.write(cache_key.c_str(), sizeof(char) * (cache_key.size()+1));

    long lsize = sizeof(char) * icon->w() * icon->h() * icon->d();
    ofs.write((char*)&lsize, sizeof(long));
    ofs.write((char*)icon->array, lsize);
  }
  ofs.close();
} // }}}

Fl_RGB_Image* ib::IconManager::getAssociatedIcon(const char *path, const int size, const bool cache){ // {{{
  ib::platform::ScopedLock lock(cache_mutex_);
  Fl_RGB_Image *icon;
  std::string cache_key;
  ib::oschar os_path[IB_MAX_PATH];
  ib::platform::utf82oschar_b(os_path, IB_MAX_PATH, path);

  if(cache){
    ib::oschar os_key[IB_MAX_PATH];
    ib::platform::icon_cache_key(os_key, os_path);
    char key[IB_MAX_PATH_BYTE];
    ib::platform::oschar2utf8_b(key, IB_MAX_PATH_BYTE, os_key);
    cache_key = key;
    char buf[24] = {};
    snprintf(buf, 24, "_%d", size);
    cache_key += buf;
    std::cout << path << std::endl;
    std::cout << cache_key << std::endl;
    icon = getIconCache(cache_key);
    if(icon != 0){
      return copyCache(icon);
    }
  }

  if(!ib::platform::is_path(os_path)){
    ib::oschar tmp[IB_MAX_PATH];
    memcpy(tmp, os_path, sizeof(ib::oschar)*IB_MAX_PATH);
    memset(os_path, 0, sizeof(ib::oschar)*IB_MAX_PATH);
    ib::platform::which(os_path, tmp);
  }
  if(!ib::platform::path_exists(os_path)) {
    return getEmptyIcon(size, size);
  }
  icon = ib::platform::get_associated_icon_image(os_path, size);
  if(cache){ 
    createIconCache(cache_key, icon); 
    return copyCache(icon);
  }
  return icon;
} // }}}

Fl_RGB_Image* ib::IconManager::readPngFileIcon(const char *png_file, const int size){ // {{{
  ib::platform::ScopedLock lock(cache_mutex_);
  ib::unique_char_ptr lopath(ib::platform::utf82local(png_file));
  Fl_RGB_Image *tmp_image = new Fl_PNG_Image(lopath.get());
  Fl_RGB_Image *result_image;
  if(tmp_image->w() != size){
    result_image = (Fl_RGB_Image*)tmp_image->copy(size, size);
    delete tmp_image;
  }else{
    result_image = tmp_image;
  }
  return result_image;
} // }}}

Fl_RGB_Image* ib::IconManager::readJpegFileIcon(const char *jpeg_file, const int size){ // {{{
  ib::platform::ScopedLock lock(cache_mutex_);
  ib::unique_char_ptr lopath(ib::platform::utf82local(jpeg_file));
  Fl_RGB_Image *tmp_image = new Fl_JPEG_Image(lopath.get());
  Fl_RGB_Image *result_image;
  if(tmp_image->w() != size){
    result_image = (Fl_RGB_Image*)tmp_image->copy(size, size);
    delete tmp_image;
  }else{
    result_image = tmp_image;
  }
  return result_image;
} // }}}

Fl_RGB_Image* ib::IconManager::readGifFileIcon(const char *gif_file, const int size){ // {{{
  ib::platform::ScopedLock lock(cache_mutex_);
  ib::unique_char_ptr lopath(ib::platform::utf82local(gif_file));
  Fl_GIF_Image *tmp_image = new Fl_GIF_Image(lopath.get());
  Fl_RGB_Image *result_image;
  result_image = (Fl_RGB_Image*)tmp_image->copy(size, size);
  delete tmp_image;
  return result_image;
} // }}}

Fl_RGB_Image* ib::IconManager::readFileIcon(const char *file, const int size){ // {{{
  ib::Regex re("(.*)\\.(\\w+)", ib::Regex::NONE);
  re.init();
  if(re.match(file) == 0){
    std::string ret;
    re._2(ret);
    if(ret == "png" || ret == "PNG") {
      return readPngFileIcon(file, size);
    }else if(ret == "gif" || ret == "GIF") {
      return readGifFileIcon(file, size);
    }else if(ret == "jpg" || ret == "JPG" || ret == "jpeg" || ret == "JPEG") {
      return readJpegFileIcon(file, size);
    }else{
      return getAssociatedIcon(file, size, true);
    }
  }
  return 0;
} // }}}

Fl_RGB_Image* ib::IconManager::getEmptyIcon(const int width, const int height) { // {{{
  ib::platform::ScopedLock lock(cache_mutex_);
  char buf[24] = {};
  snprintf(buf, 24, "%s%dx%d", ":emp_", width, height);
  const std::string cache_key = buf;
  Fl_RGB_Image *icon = getIconCache(cache_key);
  if(icon != 0) return copyCache(icon);

  Fl_RGB_Image *result_image;
  icon = new Fl_RGB_Image(blank_png, 32, 32, 4);
  icon->alloc_array = false;
  result_image = (Fl_RGB_Image*)icon->copy(width, height);
  delete icon;
  createIconCache(cache_key, result_image);
  return copyCache(result_image);
} // }}}

Fl_RGB_Image* ib::IconManager::getLuaIcon(const int size) { // {{{
  return getEmbededIcon(lua_png, "lua", 32, size);
} // }}}

void ib::IconManager::deleteCachedIcons() { // {{{
  ib::platform::ScopedLock lock(cache_mutex_);
  for(auto it = cached_icons_.begin(), last = cached_icons_.end(); it != last; ++it){
    delete (*it).second;
  }
  cached_icons_.clear();
  cached_icons_reverse_.clear();
  std::deque<std::string> empty;
  std::swap(cached_icons_queue_, empty);
} // }}}

void ib::IconManager::shrinkCache() { // {{{
  ib::platform::ScopedLock lock(cache_mutex_);
  const unsigned int max_cache_size = ib::Config::inst().getMaxCachedIcons();
  if(cached_icons_.size() < max_cache_size) return;
  if(cached_icons_.size() != cached_icons_queue_.size()){
    fl_alert("Warning: icon cache seems to be broken. iceberg clears icon cache.");
    deleteCachedIcons();
    return;
  }
  for(std::size_t count = cached_icons_.size() - max_cache_size; count > 0; count--){
    const std::string cache_key = cached_icons_queue_.front();
    Fl_RGB_Image *icon = getIconCache(cache_key);
    deleteIconCache(cache_key);
    delete icon;
    cached_icons_queue_.pop_front();
  }
} // }}} 

void ib::IconManager::createIconCache(const std::string &cache_key, Fl_RGB_Image *icon){ // {{{
  if(isCached(cache_key)) return;
  cached_icons_[cache_key] = icon;
  cached_icons_reverse_[icon] = cache_key;
  cached_icons_queue_.push_back(cache_key);
} // }}}

void ib::IconManager::deleteIconCache(const std::string &cache_key) { // {{{
  if(!isCached(cache_key)) return;
  Fl_RGB_Image *icon = cached_icons_[cache_key];
  cached_icons_reverse_.erase(icon);
  cached_icons_.erase(cache_key);
} // }}}

void ib::IconManager::deleteIconCache(Fl_RGB_Image *icon) { // {{{
  if(!isCached(icon)) return;
  std::string cache_key = cached_icons_reverse_[icon];
  cached_icons_reverse_.erase(icon);
  cached_icons_.erase(cache_key);
} // }}}

Fl_RGB_Image* ib::IconManager::getIconCache(const std::string &cache_key) { // {{{
  auto it = cached_icons_.find(cache_key);
  if(it != cached_icons_.end()){
    return (*it).second;
  }
  return 0;
} // }}}

bool ib::IconManager::isCached(const std::string &cache_key) { // {{{
  return cached_icons_.find(cache_key) != cached_icons_.end();
} // }}}

bool ib::IconManager::isCached(Fl_RGB_Image *icon){ // {{{
  return cached_icons_reverse_.find(icon) != cached_icons_reverse_.end();
} // }}}

Fl_RGB_Image* ib::IconManager::copyCache(Fl_RGB_Image *image) { // {{{
  Fl_RGB_Image* ret = new Fl_RGB_Image(image->array, image->w(), image->h(), image->d());
  ret->alloc_array = false;
  return ret;
} // }}} 

Fl_RGB_Image* ib::IconManager::getEmbededIcon(const unsigned char *data, const char* cache_prefix, const int embsize, const int reqsize) { // {{{
  ib::platform::ScopedLock lock(cache_mutex_);
  char buf[16] = {};
  snprintf(buf, 16, ":%s_%d", cache_prefix, reqsize);
  const std::string cache_key = buf;
  Fl_RGB_Image *icon = getIconCache(cache_key);
  if(icon != 0) return copyCache(icon);

  Fl_RGB_Image *result_image;
  icon = new Fl_RGB_Image(data, embsize, embsize, 4);
  icon->alloc_array = false;
  result_image = (Fl_RGB_Image*)icon->copy(reqsize, reqsize);
  delete icon;
  createIconCache(cache_key, result_image);
  return copyCache(result_image);
} // }}}

