
#include "dasi/api/Key.h"

#include "eckit/exception/Exceptions.h"
#include "eckit/log/CodeLocation.h"
#include "eckit/types/Types.h"
#include "eckit/utils/StringTools.h"

#include <initializer_list>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace dasi {

//-------------------------------------------------------------------------------------------------

Key::Key(std::initializer_list<std::pair<const std::string, std::string>> l) : values_(l) { }

Key::Key(const std::string& strKey) {
    /// TODO: Introduce a more robust parser
    for (const std::string& bit : eckit::StringTools::split(",", strKey)) {
        auto kvs = eckit::StringTools::split("=", bit);
        if (kvs.size() != 2) { throw eckit::UserError("Invalid key supplied", Here()); }
        if (kvs[1].find('/') != std::string::npos) {
            throw eckit::UserError("Query supplied when key required", Here());
        }
        values_.emplace(std::move(kvs[0]), std::move(kvs[1]));
    }
}

void Key::print(std::ostream& s) const {
    s << values_;
}

bool Key::has(const char* name) const {
    return has(std::string_view {name});
}

bool Key::has(const std::string_view& name) const {
    return values_.find(name) != values_.end();
}

bool Key::has(const std::string& name) const {
    return values_.find(name) != values_.end();
}

Key::map_type::iterator Key::set(const std::string& k, const value_type& v) {
    return values_.insert_or_assign(k, v).first;
}

Key::map_type::const_iterator Key::begin() const {
    return values_.begin();
}

Key::map_type::const_iterator Key::end() const {
    return values_.end();
}

Key::map_type::size_type Key::size() const {
    return values_.size();
}

const Key::value_type& Key::get(const std::string& keyword) const {
    auto iter = values_.find(keyword);
    if (iter == values_.end()) {
        std::ostringstream ss;
        ss << keyword << " not found in Key";
        throw eckit::UserError(ss.str(), Here());
    }
    return iter->second;
}

const Key::value_type& Key::get(const std::string_view& keyword) const {
    auto iter = values_.find(keyword);
    if (iter == values_.end()) {
        std::ostringstream ss;
        ss << keyword << " not found in Key";
        throw eckit::UserError(ss.str(), Here());
    }
    return iter->second;
}

const Key::value_type& Key::get(const char* keyword) const {
    return get(std::string_view(keyword));
}

void Key::erase(const std::string& k) {
    auto iter = values_.find(k);
    if (iter != values_.end()) { values_.erase(iter); }
}

void Key::erase(const std::string_view& k) {
    auto iter = values_.find(k);
    if (iter != values_.end()) { values_.erase(iter); }
}

void Key::erase(const char* k) {
    erase(std::string_view(k));
}

void Key::clear() {
    values_.clear();
}

bool Key::operator<(const Key& rhs) const {
    return values_ < rhs.values_;
}

bool Key::operator==(const Key& rhs) const {
    return values_ == rhs.values_;
}

bool Key::operator>(const Key& rhs) const {
    return values_ > rhs.values_;
}

bool Key::operator!=(const Key& rhs) const {
    return values_ != rhs.values_;
}

//-------------------------------------------------------------------------------------------------

}  // namespace dasi
