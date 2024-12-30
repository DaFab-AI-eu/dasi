
#include "dasi/api/Query.h"

#include "eckit/exception/Exceptions.h"
#include "eckit/types/Types.h"
#include "eckit/utils/StringTools.h"

using namespace eckit;

namespace dasi {

//-------------------------------------------------------------------------------------------------

Query::Query(std::initializer_list<std::pair<const std::string, std::vector<std::string>>> l) : values_(l) { }

Query::Query(const std::string& strKey) {
    // TODO: Introduce a more robust parser
    for (const std::string& bit : StringTools::split(",", strKey)) {
        auto kvs = StringTools::split("=", bit);
        if (kvs.size() != 2) { throw UserError("Invalid key supplied", Here()); }
        auto vals = StringTools::split("/", kvs[1]);
        values_.emplace(std::move(kvs[0]), std::move(vals));
    }
}

void Query::print(std::ostream& s) const {
    s << values_;
}

bool Query::has(const std::string_view& name) const {
    return (values_.find(name) != values_.end());
}

void Query::set(const std::string& k, std::initializer_list<std::string> v) {
    values_.insert_or_assign(k, v);
}

void Query::set(const std::string& k, const value_type& v) {
    values_.insert_or_assign(k, v);
}

void Query::append(const std::string& k, const std::string& v) {
    auto it = values_.find(k);
    if (it == values_.end()) {
        set(k, {v});
        return;
    }
    values_[k].push_back(v);
}

auto Query::get(const std::string_view& name) const -> const value_type& {
    auto it = values_.find(name);
    if (it == values_.end()) {
        std::ostringstream ss;
        ss << name << " not found in Query";
        throw eckit::UserError(ss.str(), Here());
    }
    return it->second;
}

Query::map_type::size_type Query::size() const {
    return values_.size();
}

Query::map_type::const_iterator Query::begin() const {
    return cbegin();
}

Query::map_type::const_iterator Query::end() const {
    return cend();
}

Query::map_type::const_iterator Query::cbegin() const {
    return values_.cbegin();
}

Query::map_type::const_iterator Query::cend() const {
    return values_.cend();
}

void Query::erase(const std::string& k) {
    auto it = values_.find(k);
    if (it != values_.end()) { values_.erase(it); }
}

void Query::erase(const std::string_view& k) {
    auto it = values_.find(k);
    if (it != values_.end()) { values_.erase(it); }
}

void Query::erase(const char* k) {
    erase(std::string_view(k));
}

void Query::clear() {
    values_.clear();
}

//-------------------------------------------------------------------------------------------------

}  // namespace dasi
