#pragma once
#include <map>
#include <string>
#include <optional>
#include <cstddef>
namespace sim {
// In-memory persistent media: retain this object across simulated reboots.
struct Media {
    std::map<std::string,std::string> files;
    std::map<std::pair<std::string,std::string>,std::string> preferences;
};
enum class StorageResult { ok, unavailable, read_only, invalid_path, not_found, full };
class Storage {
public:
    explicit Storage(Media& media): media_(media) {}
    bool inserted=true, usb_owned=false, read_only=false, nvs_available=true;
    std::size_t capacity=16*1024*1024;
    StorageResult write(const std::string& path,const std::string& bytes) {
        if (!inserted || usb_owned) return StorageResult::unavailable;
        if (read_only) return StorageResult::read_only;
        if (!valid(path)) return StorageResult::invalid_path;
        std::size_t used=0;
        for (const auto& entry:media_.files) if(entry.first!=path) {
            if(entry.second.size()>capacity-used) return StorageResult::full;
            used+=entry.second.size();
        }
        if(bytes.size()>capacity-used) return StorageResult::full;
        media_.files[path]=bytes; return StorageResult::ok;
    }
    StorageResult read(const std::string& path,std::string& out) const {
        if (!inserted || usb_owned) return StorageResult::unavailable;
        if (!valid(path)) return StorageResult::invalid_path;
        auto it=media_.files.find(path);
        if(it==media_.files.end()) return StorageResult::not_found;
        out=it->second; return StorageResult::ok;
    }
    StorageResult remove(const std::string& path) {
        if (!inserted || usb_owned) return StorageResult::unavailable;
        if (read_only) return StorageResult::read_only;
        if (!valid(path)) return StorageResult::invalid_path;
        return media_.files.erase(path)?StorageResult::ok:StorageResult::not_found;
    }
    bool put(const std::string& ns,const std::string& key,const std::string& value) {
        if(!nvs_available || !valid_key(ns) || !valid_key(key)) return false;
        media_.preferences[{ns,key}]=value; return true;
    }
    std::optional<std::string> get(const std::string& ns,const std::string& key) const {
        if(!nvs_available) return {};
        auto it=media_.preferences.find({ns,key});
        if(it==media_.preferences.end()) return {};
        return it->second;
    }
private:
    Media& media_;
    static bool valid_key(const std::string& s) { return !s.empty() && s.size()<=15 && s.find('\0')==std::string::npos; }
    static bool valid(const std::string& p) {
        if(p.size()<2 || p.front()!='/' || p.find('\\')!=std::string::npos || p.find('\0')!=std::string::npos) return false;
        std::size_t start=1;
        while(start<=p.size()) {
            auto end=p.find('/',start);
            auto part=p.substr(start,end-start);
            if(part.empty() || part=="." || part=="..") return false;
            if(end==std::string::npos) return true;
            start=end+1;
        }
        return false;
    }
};
}
