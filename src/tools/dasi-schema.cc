/*
 * Copyright 2023- European Centre for Medium-Range Weather Forecasts (ECMWF).
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "dasi/api/Dasi.h"
#include "dasi/tools/DASITool.h"
#include "eckit/config/YAMLConfiguration.h"
#include "eckit/filesystem/LocalPathName.h"
#include "eckit/log/Log.h"
#include "eckit/option/CmdArgs.h"
#include "eckit/option/SimpleOption.h"
#include "eckit/parser/YAMLParser.h"

#include <filesystem>
#include <regex>

using namespace eckit::option;

namespace dasi::tools {

//----------------------------------------------------------------------------------------------------------------------

class DASISchema : public DASITool {
public:  // methods
    DASISchema(int argc, char** argv): DASITool(argc, argv) {
        options_.push_back(
            new SimpleOption<bool>("query", "Prints out the query string(s) (defined in the schema file)."));
        options_.push_back(new SimpleOption<bool>(
            "scan", "Prints out the keys found in the root paths (defined in the configuration yaml file)."));
        options_.push_back(
            new SimpleOption<bool>("root", "Prints out the root paths defined in the configuration yaml file."));
        options_.push_back(new SimpleOption<bool>("list", "Prints out the data and index paths for a given query."));
    }

private:  // methods
    void usage(const std::string& tool) const override;

    void init(const CmdArgs& args) override;

    void execute(const CmdArgs& args) override;

    using RuleList = std::vector<std::vector<std::string>>;

    [[nodiscard]]
    RuleList tokenizeRules();

    using PathList = std::vector<eckit::LocalPathName>;

    [[nodiscard]]
    PathList getRootPaths() const;

    void query();

    void scan();

    void list(const CmdArgs& args);

private:  // members
    bool query_ {false};
    bool scan_ {false};

    RuleList rules_;
};

//----------------------------------------------------------------------------------------------------------------------

void DASISchema::usage(const std::string& tool) const {
    eckit::Log::info() << eckit::newl                                       //
                       << "Usage: " << tool << " [options]" << eckit::newl  //
                       << eckit::newl                                       //
                       << eckit::newl                                       //
                       << "Examples:" << eckit::newl                        //
                       << "=========" << eckit::newl                        //
                       << eckit::newl                                       //
                       << tool << " --config=dasi.yaml" << eckit::newl       //
                       << tool << " --query" << eckit::newl                 //
                       << tool << " --scan" << eckit::newl                  //
                       << tool << " --root" << eckit::newl                  //
                       << tool << " --list key1=value1,key3=value3" << eckit::newl;
    DASITool::usage(tool);
}

void DASISchema::init(const CmdArgs& args) {
    query_ = args.getBool("query", query_);
    scan_  = args.getBool("scan", scan_);

    if (query_ || scan_) { rules_ = tokenizeRules(); }
}

void DASISchema::execute(const CmdArgs& args) {
    if (query_) { return query(); }

    if (scan_) { return scan(); }

    if (args.getBool("list", false)) { return list(args); }

    if (args.getBool("root", false)) {
        for (auto&& root : getRootPaths()) { eckit::Log::info() << root << eckit::newl; }
    }
}

//----------------------------------------------------------------------------------------------------------------------

DASISchema::RuleList DASISchema::tokenizeRules() {
    std::vector<std::string> rules;

    {
        std::stringstream sbuf;
        dasi().dumpSchema(sbuf);

        std::string line;
        while (std::getline(sbuf, line)) { rules.push_back(line); }
    }

    RuleList result;

    for (auto&& rule : rules) {
        // format
        rule.erase(0, 1);
        std::replace(rule.begin(), rule.end(), '[', ',');
        rule = std::regex_replace(rule, std::regex("[ \\]\\?]"), "");

        // tokenize (comma delimited)
        std::vector<std::string> tokens;
        {
            std::istringstream sbuf(rule);
            std::string        token;
            while (std::getline(sbuf, token, ',')) { tokens.push_back(token); }
        }

        result.push_back(tokens);
    }

    return result;
}

//----------------------------------------------------------------------------------------------------------------------

DASISchema::PathList DASISchema::getRootPaths() const {
    // find the root paths defined in the config yaml

    eckit::YAMLConfiguration configPath(configPath_);

    if (configPath.has("spaces.handler.roots")) {
        eckit::Log::info() << "Using config file: " << configPath_ << eckit::newl;
    }

    // const auto config = eckit::YAMLParser::decodeFile(configPath_);
    // const auto roots  = config["spaces"]["handler"]["roots"];
    // ASSERT(roots.isList());

    PathList result;

    // for (size_t i = 0; i < roots.size(); ++i) { result.emplace_back(roots[i]["path"]); }

    return result;
}

//----------------------------------------------------------------------------------------------------------------------

void DASISchema::query() {
    for (auto&& rule : rules_) {
        eckit::Log::info() << "{";
        for (auto&& key : rule) {
            const char* separator = ",";
            if (&key == &rule.back()) { separator = ""; }
            eckit::Log::info() << key << "=%s" << separator;
        }
        eckit::Log::info() << "}" << eckit::newl;
    }
}

//----------------------------------------------------------------------------------------------------------------------

void DASISchema::scan() {
    for (auto&& path : getRootPaths()) {
        // directories found in this root path
        PathList dirs;

        {
            PathList files;
            path.children(files, dirs);
        }

        for (auto&& dir : dirs) {
            std::string value;
            {
                // get the first value from the directory name
                std::stringstream sbuf;
                sbuf << (dir.baseName(false));
                std::getline(sbuf, value, ':');
            }
            for (auto&& rule : rules_) {
                const dasi::Query query(rule[0] + "=" + value);
                for (const auto& elem : dasi().list(query)) {
                    elem.print(eckit::Log::info(), false);
                    eckit::Log::info() << eckit::newl;
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------

void DASISchema::list(const CmdArgs& args) {
    for (size_t i = 0; i < args.count(); ++i) {
        const dasi::Query q(args(i));
        for (const auto& elem : dasi().list(q)) {
            std::filesystem::path fpath = elem.location.uri.name();

            if (fpath.extension() == ".data") {
                eckit::Log::info() << fpath << eckit::newl;                              // file.data
                eckit::Log::info() << fpath.replace_extension(".index") << eckit::newl;  // file.index
            }
        }
    }
}

}  // namespace dasi::tools

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char** argv) {
    ::setenv("AUTO_LOAD_PLUGINS", "false", true);

    dasi::tools::DASISchema app(argc, argv);

    return app.start();
}
