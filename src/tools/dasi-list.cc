#include "eckit/option/CmdArgs.h"
#include "eckit/option/SimpleOption.h"
#include "eckit/log/Log.h"

#include "dasi/tools/DASITool.h"
#include "dasi/api/Dasi.h"

using namespace eckit::option;

namespace dasi::tools {

//----------------------------------------------------------------------------------------------------------------------

class DASIList : public DASITool {

public: // methods

    DASIList(int argc, char **argv) :
        DASITool(argc, argv),
        location_(false) {
        options_.push_back(new SimpleOption<bool>("location", "Also print the location of each field"));
    }

private: // methods

    void usage(const std::string &tool) const override;
    void init(const eckit::option::CmdArgs& args) override;
    void execute(const eckit::option::CmdArgs& args) override;

    bool location_;
};

void DASIList::usage(const std::string &tool) const {
    eckit::Log::info()
            << std::endl
            << "Usage: " << tool << " [options] <query> [<query> ...]" << std::endl
            << std::endl
            << std::endl
            << "Examples:" << std::endl
            << "=========" << std::endl << std::endl
            << tool << " key1=value1/value2,key2=value2" << std::endl
            << std::endl;
    DASITool::usage(tool);
}

void DASIList::init(const eckit::option::CmdArgs &args) {
    location_ = args.getBool("location", location_);
}

void DASIList::execute(const eckit::option::CmdArgs& args) {

    for (size_t i = 0; i < args.count(); ++i) {

        dasi::Query q(args(i));
        for (const auto& elem : dasi().list(q)) {
            elem.print(eckit::Log::info(), location_);
            eckit::Log::info() << eckit::newl;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------

} // namespace dasi::tools


int main(int argc, char **argv) {
    dasi::tools::DASIList app(argc, argv);
    return app.start();
}

