#include "ib_icon_manager.h"
#include "ib_utils.h"
#include "ib_platform.h"
#include "ib_ui.h"
#include "ib_regex.h"
#include "ib_config.h"
#include "ib_svg.h"
#include "ib_singleton.h"

// icon_loader_thread {{{
struct iconlist {
  std::vector<Fl_Image*> icons;
  int pos;
};

static void _main_thread_awaker(void *p){
  const auto listbox = ib::Singleton<ib::ListWindow>::getInstance()->getListbox();
  auto ilist = reinterpret_cast<struct iconlist*>(p);
  auto pos = ilist->pos;
  for(auto it = ilist->icons.begin(), last = ilist->icons.end(); it != last; ++it, ++pos){
    listbox->destroyIcon(pos);
    if(*it != nullptr) {
      listbox->icon(pos, *it);
    }
  }
  delete ilist;
}

void ib::_icon_loader(void *p) {
  const int MAX_FLUSH = 200;

  const auto listbox = ib::Singleton<ib::ListWindow>::getInstance()->getListbox();

  std::vector<Fl_Image*> buf;
  int current_operation_count = -1;
  int listsize;
  int pos;
  int loaded_to_flush;

  { ib::platform::ScopedLock lock(listbox->getMutex());
    current_operation_count = listbox->getOperationCount();
    listsize = listbox->size();
    pos = 1;
    loaded_to_flush= 2;
  }

  for(int i=1;i <= listsize; ++i){
    { ib::platform::ScopedLock lock(listbox->getMutex());
      if(current_operation_count != listbox->getOperationCount()){ break; }
      int icon_size = listbox->getValues().at(i-1)->hasDescription() ? IB_ICON_SIZE_LARGE : IB_ICON_SIZE_SMALL;
      buf.push_back(listbox->getValues().at(i-1)->loadIcon(icon_size));
    }

    if((i != 1 && (i-pos) % loaded_to_flush == 0) || i == listsize){
      { ib::platform::ScopedLock lock(listbox->getMutex());
        if(current_operation_count != listbox->getOperationCount()){
          ib::utils::delete_pointer_vectors(buf);
          break;
        }
        auto ilist = new iconlist;
        std::copy(buf.begin(), buf.end(), std::back_inserter(ilist->icons));
        ilist->pos = pos;
        Fl::awake(_main_thread_awaker, ilist);
        pos += buf.size();
        buf.clear();
        loaded_to_flush = std::min<int>(loaded_to_flush*3, MAX_FLUSH);
      }
    }
  }
} // }}}

void ib::IconManager::loadCompletionListIcons() { // {{{
  loader_event_.queueEvent((void*)1);
} // }}}

void ib::IconManager::load() { // {{{
  ib::platform::ScopedLock lock(&cache_mutex_);
  const auto* const cfg = ib::Singleton<ib::Config>::getInstance();
  auto osicon_path = ib::platform::utf82oschar(cfg->getIconCachePath().c_str());
  if(!ib::platform::file_exists(osicon_path.get())) return;

  auto loicon_path = ib::platform::utf82local(cfg->getIconCachePath().c_str());
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
    auto cache_key_buf = new char[cache_key_size];
    ifs.read(cache_key_buf, cache_key_size);

    long data_size;
    ifs.read((char*)&data_size, sizeof(long));
    auto data_buf = new unsigned char[data_size];
    ifs.read((char*)data_buf, data_size);

    std::string cache_key(cache_key_buf);
    delete[] cache_key_buf;
    auto icon = new Fl_RGB_Image(data_buf, w, h ,d);
    icon->alloc_array = true;
    createIconCache(cache_key, icon);
  }
  ifs.close();

} // }}}

void ib::IconManager::dump() { // {{{
  ib::platform::ScopedLock lock(&cache_mutex_);
  const auto* const cfg = ib::Singleton<ib::Config>::getInstance();
  auto loicon_path = ib::platform::utf82local(cfg->getIconCachePath().c_str());
  std::ofstream ofs(loicon_path.get(), std::ios::out|std::ios::binary|std::ios::trunc);
  int intvalue;
  char *intptr = (char*)&intvalue;

  // cache size
  intvalue = (int)std::min<std::size_t>(cached_icons_.size(), INT_MAX);
  ofs.write(intptr, sizeof(int));

  for(auto const &cache_key : cached_icons_queue_) {
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

Fl_Image* ib::IconManager::getAssociatedIcon(const char *path, const int size){ // {{{
  ib::platform::ScopedLock lock(&cache_mutex_);
  std::string cache_key("");
  ib::oschar os_path[IB_MAX_PATH];
  ib::platform::utf82oschar_b(os_path, IB_MAX_PATH, path);

  ib::oschar os_key[IB_MAX_PATH];
  ib::platform::icon_cache_key(os_key, os_path);
  char key[IB_MAX_PATH_BYTE];
  ib::platform::oschar2utf8_b(key, IB_MAX_PATH_BYTE, os_key);
  cache_key = key;
  char buf[24] = {0};
  snprintf(buf, 24, "_%d", size);
  cache_key += buf;

  {
    Fl_RGB_Image *icon;
    icon = getIconCache(cache_key);
    if(icon != nullptr){
      return copyCache(icon);
    }
  }
  

  if(!ib::platform::is_path(os_path)){
    ib::oschar tmp[IB_MAX_PATH];
    memcpy(tmp, os_path, sizeof(ib::oschar)*IB_MAX_PATH);
    memset(os_path, 0, sizeof(ib::oschar)*IB_MAX_PATH);
    if(!ib::platform::which(os_path, tmp)) {
      memcpy(os_path, tmp, sizeof(ib::oschar)*IB_MAX_PATH);
    }
  }
  if(ib::platform::is_path(os_path) && !ib::platform::path_exists(os_path)) {
    return getEmptyIcon(size, size);
  }
  auto aicon = ib::platform::get_associated_icon_image(os_path, size);
  if(aicon == nullptr) {
    return getEmptyIcon(size, size);
  }
  auto ricon = dynamic_cast<Fl_RGB_Image*>(aicon);
  if(ricon != nullptr) {
    createIconCache(cache_key, ricon); 
    return copyCache(ricon);
  }
  return aicon;
} // }}}

Fl_Image* ib::IconManager::readPngFileIcon(const char *png_file, const int size){ // {{{
  ib::platform::ScopedLock lock(&cache_mutex_);
  auto lopath = ib::platform::utf82local(png_file);
  Fl_RGB_Image *tmp_image = new Fl_PNG_Image(lopath.get());
  Fl_Image *result_image;
  if(tmp_image->w() != size){
    result_image = tmp_image->copy(size, size);
    delete tmp_image;
  }else{
    result_image = (Fl_Image*)tmp_image;
  }
  return result_image;
} // }}}

Fl_Image* ib::IconManager::readJpegFileIcon(const char *jpeg_file, const int size){ // {{{
  ib::platform::ScopedLock lock(&cache_mutex_);
  auto lopath = ib::platform::utf82local(jpeg_file);
  Fl_RGB_Image *tmp_image = new Fl_JPEG_Image(lopath.get());
  Fl_Image *result_image;
  if(tmp_image->w() != size){
    result_image = tmp_image->copy(size, size);
    delete tmp_image;
  }else{
    result_image = (Fl_Image*)tmp_image;
  }
  return result_image;
} // }}}

Fl_Image* ib::IconManager::readGifFileIcon(const char *gif_file, const int size){ // {{{
  ib::platform::ScopedLock lock(&cache_mutex_);
  auto lopath = ib::platform::utf82local(gif_file);
  Fl_Image *tmp_image = new Fl_GIF_Image(lopath.get());
  Fl_Image *result_image;
  if(tmp_image->w() != size){
    result_image = tmp_image->copy(size, size);
    delete tmp_image;
  }else{
    result_image = tmp_image;
  }
  return result_image;
} // }}}

Fl_Image* ib::IconManager::readSvgFileIcon(const char *svg_file, const int size){ // {{{
  ib::platform::ScopedLock lock(&cache_mutex_);
  auto lopath = ib::platform::utf82local(svg_file);
  auto buf = ib::rasterize_svg_file(svg_file, size);
  if(buf == nullptr) return nullptr;
  auto result_image = new Fl_RGB_Image(buf, size, size, 4);
  return static_cast<Fl_Image*>(result_image);
} // }}}

Fl_Image* ib::IconManager::readXpmFileIcon(const char *xpm_file, const int size){ // {{{
  ib::platform::ScopedLock lock(&cache_mutex_);
  auto lopath = ib::platform::utf82local(xpm_file);
  Fl_Image *tmp_image = new Fl_XPM_Image(lopath.get());
  Fl_Image *result_image;
  if(tmp_image->w() != size){
    result_image = tmp_image->copy(size, size);
    delete tmp_image;
  }else{
    result_image = tmp_image;
  }
  return result_image;
} // }}}

Fl_Image* ib::IconManager::readFileIcon(const char *file, const int size){ // {{{
  ib::platform::ScopedLock lock(&cache_mutex_);
  std::string cache_key;

  ib::oschar osresolved_path[IB_MAX_PATH];
  char resolved_path[IB_MAX_PATH_BYTE];
  ib::oschar osfile[IB_MAX_PATH];
  ib::platform::utf82oschar_b(osfile, IB_MAX_PATH, file);
  ib::platform::resolve_icon(osresolved_path, osfile, size);
  ib::platform::oschar2utf8_b(resolved_path, IB_MAX_PATH_BYTE, osresolved_path);
  cache_key += resolved_path;
  cache_key += "_";
  cache_key += size;
  {
    Fl_RGB_Image *icon = getIconCache(cache_key);
    if(icon != nullptr){
      return copyCache(icon);
    }
  }

  Fl_Image *aicon = nullptr;
  ib::Regex re("(.*)\\.(\\w+)", ib::Regex::NONE);
  re.init();
  if(re.match(resolved_path) == 0){
    auto ret = re._2();
    if(ret == "png" || ret == "PNG") {
      aicon = readPngFileIcon(resolved_path, size);
    }else if(ret == "gif" || ret == "GIF") {
      aicon = readGifFileIcon(resolved_path, size);
    }else if(ret == "jpg" || ret == "JPG" || ret == "jpeg" || ret == "JPEG") {
      aicon =  readJpegFileIcon(resolved_path, size);
    } else if(ret == "svg" || ret == "SVG"){
      aicon = readSvgFileIcon(resolved_path, size);
    } else if(ret == "xpm" || ret == "XPM"){
      aicon = readXpmFileIcon(resolved_path, size);
    }
  }
  if(aicon != nullptr){
    if(dynamic_cast<Fl_RGB_Image*>(aicon) != nullptr) {
      createIconCache(cache_key, (Fl_RGB_Image*)aicon); 
      return copyCache((Fl_RGB_Image*)aicon);
    } else {
      return aicon;
    }
  } else {
    return getAssociatedIcon(file, size);
  }
} // }}}

Fl_Image* ib::IconManager::getEmptyIcon(const int width, const int height) { // {{{
  ib::platform::ScopedLock lock(&cache_mutex_);
  char buf[24] = {};
  snprintf(buf, 24, "%s%dx%d", ":emp_", width, height);
  const std::string cache_key = buf;
  Fl_RGB_Image *icon = getIconCache(cache_key);
  if(icon != nullptr) return copyCache(icon);

  Fl_RGB_Image *result_image;
  icon = new Fl_RGB_Image(blank_png, IB_ICON_SIZE_LARGE, IB_ICON_SIZE_LARGE, 4);
  icon->alloc_array = false;
  result_image = (Fl_RGB_Image*)icon->copy(width, height);
  delete icon;
  createIconCache(cache_key, result_image);
  return copyCache(result_image);
} // }}}

Fl_Image* ib::IconManager::getLuaIcon(const int size) { // {{{
  return getEmbededIcon(lua_png, "lua", IB_ICON_SIZE_LARGE, size);
} // }}}

Fl_Image* ib::IconManager::getIcebergIcon(const int size) { // {{{
  return getEmbededIcon(iceberg_png, "iceberg", IB_ICON_SIZE_LARGE, size);
} // }}}

void ib::IconManager::deleteCachedIcons() { // {{{
  ib::platform::ScopedLock lock(&cache_mutex_);
  for(auto &pair : cached_icons_) {
    delete pair.second;
  }
  cached_icons_.clear();
  cached_icons_reverse_.clear();
  std::deque<std::string> empty;
  std::swap(cached_icons_queue_, empty);
} // }}}

void ib::IconManager::shrinkCache() { // {{{
  ib::platform::ScopedLock lock(&cache_mutex_);
  const auto max_cache_size = ib::Singleton<ib::Config>::getInstance()->getMaxCachedIcons();
  if(cached_icons_.size() < max_cache_size) return;
  if(cached_icons_.size() != cached_icons_queue_.size()){
    ib::utils::message_box("Warning: icon cache seems to be broken. iceberg clears icon cache.");
    deleteCachedIcons();
    return;
  }
  for(std::size_t count = cached_icons_.size() - max_cache_size; count > 0; count--){
    const auto cache_key = cached_icons_queue_.front();
    auto icon = getIconCache(cache_key);
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
  const auto &cache_key = cached_icons_reverse_[icon];
  cached_icons_reverse_.erase(icon);
  cached_icons_.erase(cache_key);
} // }}}

Fl_RGB_Image* ib::IconManager::getIconCache(const std::string &cache_key) { // {{{
  auto it = cached_icons_.find(cache_key);
  if(it != cached_icons_.end()){
    return (*it).second;
  }
  return nullptr;
} // }}}

bool ib::IconManager::isCached(const std::string &cache_key) { // {{{
  return cached_icons_.find(cache_key) != cached_icons_.end();
} // }}}

bool ib::IconManager::isCached(Fl_RGB_Image *icon){ // {{{
  return cached_icons_reverse_.find(icon) != cached_icons_reverse_.end();
} // }}}

Fl_RGB_Image* ib::IconManager::copyCache(Fl_RGB_Image *image) { // {{{
  auto ret = new Fl_RGB_Image(image->array, image->w(), image->h(), image->d());
  ret->alloc_array = false;
  return ret;
} // }}} 

Fl_RGB_Image* ib::IconManager::getEmbededIcon(const unsigned char *data, const char* cache_prefix, const int embsize, const int reqsize) { // {{{
  ib::platform::ScopedLock lock(&cache_mutex_);
  char buf[16] = {};
  snprintf(buf, 16, ":%s_%d", cache_prefix, reqsize);
  const auto cache_key = buf;
  auto icon = getIconCache(cache_key);
  if(icon != nullptr) return copyCache(icon);

  Fl_RGB_Image *result_image;
  icon = new Fl_RGB_Image(data, embsize, embsize, 4);
  icon->alloc_array = false;
  result_image = (Fl_RGB_Image*)icon->copy(reqsize, reqsize);
  delete icon;
  createIconCache(cache_key, result_image);
  return copyCache(result_image);
} // }}}

ib::IconManager::~IconManager() { // {{{
  deleteCachedIcons();
  loader_event_.stopThread();
  ib::platform::destroy_mutex(&cache_mutex_);
  for(auto &pair : cached_icons_) { delete pair.second; }
} // }}}

