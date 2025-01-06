/*
 * (C) Copyright 2022- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */


/// @author Simon Smart
/// @date   Oct 2022

#pragma once

#include <initializer_list>
#include <iosfwd>
#include <map>
#include <string>
#include <string_view>
#include <vector>


namespace dasi {

//-------------------------------------------------------------------------------------------------

class Query {

public: // types

    using value_type = std::vector<std::string>;
    using map_type = std::map<std::string, value_type, std::less<>>;

public: // methods

    Query() = default;
    Query(std::initializer_list<std::pair<const std::string, value_type>>);
    Query(const std::string& strKey);

    [[nodiscard]]
    bool has(const std::string_view& name) const;

    void set(const std::string& k, std::initializer_list<std::string> v);
    void set(const std::string& k, const value_type& v);

    void append(const std::string& k, const std::string& v);

    map_type::size_type size() const;
    const value_type& get(const std::string_view& name) const;

    map_type::const_iterator begin() const;
    map_type::const_iterator end() const;
    map_type::const_iterator cbegin() const;
    map_type::const_iterator cend() const;

    void erase(const std::string& k);
    void erase(const std::string_view& k);
    void erase(const char* k);
    void clear();

private: // methods

    /// TODO: It would be nice to have a query-specific nice formatting
    void print(std::ostream& s) const;

private: // friends

    friend std::ostream& operator<<(std::ostream& s, const Query& k) {
        k.print(s);
        return s;
    }

private: // members

    // n.b. use of transparent comparator --> allow lookup with std::string_view as key
    map_type values_;
};


//-------------------------------------------------------------------------------------------------

} // namespace dasi
