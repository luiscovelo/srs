#ifndef SRS_APP_RECORD_MODE_CACHE_HPP
#define SRS_APP_RECORD_MODE_CACHE_HPP

#include <map>
#include <string>
#include <mutex>

class SrsRecordModeCache 
{
private:
    static SrsRecordModeCache* _instance;
    std::map<std::string, bool> items;
    
    SrsRecordModeCache() {}
    
public:
    static SrsRecordModeCache* instance();
    
    void set(const std::string& key, bool is_record_audio);
    bool get(const std::string& key, bool default_value = false);
    void del(const std::string& key);
    void clear();
};

#endif