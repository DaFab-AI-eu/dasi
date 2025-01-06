#include "dasi/lib/LibDasi.h"
#include "dasi/tools/DASITool.h"
#include "eckit/option/CmdArgs.h"
#include "eckit/option/SimpleOption.h"

namespace dasi::tools {

//----------------------------------------------------------------------------------------------------------------------

class DASIInfo : public DASITool {

public:  // methods
    DASIInfo(int argc, char** argv) : DASITool(argc, argv), all_(false), version_(false) {

        options_.push_back(new eckit::option::SimpleOption<bool>("all", "Print all information"));
        options_.push_back(new eckit::option::SimpleOption<bool>("version", "Print the version of the FDB being used"));
    }

private:  // methods
    void usage(const std::string& tool) const override;
    void init(const eckit::option::CmdArgs& args) override;
    void execute(const eckit::option::CmdArgs& args) override;

    bool all_;
    bool version_;
};

void DASIInfo::usage(const std::string& tool) const {
    eckit::Log::info() << std::endl
                       << "Usage: " << tool << " [options]" << std::endl
                       << std::endl
                       << std::endl
                       << "Examples:" << std::endl
                       << "=========" << std::endl
                       << std::endl
                       << tool << " --all" << std::endl
                       << tool << " --version" << std::endl
                       << std::endl;
    DASITool::usage(tool);
}

void DASIInfo::init(const eckit::option::CmdArgs& args) {
    all_     = args.getBool("all", false);
    version_ = args.getBool("version", false);

    if (!(all_ || version_)) { all_ = true; }
}

void DASIInfo::execute(const eckit::option::CmdArgs& args) {

    if (all_ || version_) {
        eckit::Log::info() << (all_ ? "Version: " : "") << LibDasi::instance().version() << std::endl;
        if (!all_) { return; }
    }
}

//----------------------------------------------------------------------------------------------------------------------

}  // namespace dasi::tools

int main(int argc, char** argv) {
    dasi::tools::DASIInfo app(argc, argv);
    return app.start();
}
