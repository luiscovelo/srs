#include <srs_app_record_mode_cache.hpp>
#include <sstream>
using namespace std;

#include <srs_kernel_error.hpp>
#include <srs_kernel_log.hpp>
#include <srs_core_autofree.hpp>

SrsRecordModeCache* SrsRecordModeCache::_instance = NULL;

SrsRecordModeCache* SrsRecordModeCache::instance()
{
    if (!_instance) {
        _instance = new SrsRecordModeCache();
    }
    return _instance;
}

void SrsRecordModeCache::set(const std::string& key, bool is_record_audio)
{
    items[key] = is_record_audio;
}

bool SrsRecordModeCache::get(const std::string& key, bool default_value)
{    
    if (auto it = items.find(key); it != items.end()) {
        return it->second;
    }
    
    srs_trace2("SrsRecordModeCache","Using default is_record_audio=%s for key=%s",
              default_value ? "true" : "false", key.c_str());
    return default_value;
}

void SrsRecordModeCache::del(const std::string& key)
{
    items.erase(key);
}

void SrsRecordModeCache::clear()
{
    items.clear();
}
