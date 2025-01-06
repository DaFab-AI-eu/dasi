
#include "dasi/tools/DASITool.h"

#include "eckit/exception/Exceptions.h"
#include "eckit/option/CmdArgs.h"
#include "eckit/option/SimpleOption.h"

using namespace eckit::option;

namespace dasi::tools {

//----------------------------------------------------------------------------------------------------------------------

static DASITool* instance_ = nullptr;

DASITool::DASITool(int argc, char** argv) : eckit::Tool(argc, argv) {
    ASSERT(instance_ == nullptr);
    instance_ = this;
    options_.push_back(new SimpleOption<std::string>("config", "Configuration file"));
}

static void usage(const std::string& tool) {
    ASSERT(instance_);
    instance_->usage(tool);
}

void DASITool::run() {
    CmdArgs args(&dasi::tools::usage, options_, numberOfPositionalArguments(), minimumPositionalArguments());

    initInternal(args);
    init(args);
    execute(args);
    finish(args);
}

void DASITool::initInternal(const CmdArgs& args) {
    configPath_ = args.getString("config", "./dasi.yaml");
}

Dasi& DASITool::dasi() {
    if (!dasi_) { dasi_.emplace(configPath_.localPath()); }
    return dasi_.value();
}

//----------------------------------------------------------------------------------------------------------------------

DASIToolException::DASIToolException(const std::string& message) : eckit::Exception(message) { }

DASIToolException::DASIToolException(const std::ostringstream& message) : DASIToolException(message.str()) { }

DASIToolException::DASIToolException(const std::string& message, const eckit::CodeLocation& location)
    : eckit::Exception(message, location) { }

//----------------------------------------------------------------------------------------------------------------------

}  // namespace dasi::tools
