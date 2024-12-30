#include "eckit/exception/Exceptions.h"
#include "eckit/log/Log.h"
#include "eckit/io/Buffer.h"
#include "eckit/io/MemoryHandle.h"
#include "eckit/option/CmdArgs.h"
#include "eckit/option/SimpleOption.h"

#include "dasi/tools/DASITool.h"
#include "dasi/api/Dasi.h"

using namespace eckit;
using namespace eckit::option;

namespace dasi::tools {

//----------------------------------------------------------------------------------------------------------------------

class DASIGet : public DASITool {

public: // methods

    DASIGet(int argc, char **argv) :
            DASITool(argc, argv) {}

private: // methods

    void usage(const std::string &tool) const override;
    void execute(const eckit::option::CmdArgs& args) override;

    [[ nodiscard ]]
    int numberOfPositionalArguments() const override { return 2; }
};

void DASIGet::usage(const std::string &tool) const {
    eckit::Log::info()
            << std::endl
            << "Usage: " << tool << " [options] <query> <path>" << std::endl
            << std::endl
            << std::endl
            << "Examples:" << std::endl
            << "=========" << std::endl << std::endl
            << tool << " key1=value1,key2=value2 path/to/data" << std::endl
            << std::endl;
    DASITool::usage(tool);
}

void DASIGet::execute(const eckit::option::CmdArgs& args) {

    dasi::Query q(args(0));
    eckit::PathName path(args(1));

    std::unique_ptr<DataHandle> out_dh(path.fileHandle());
    std::unique_ptr<DataHandle> dh = dasi().retrieve(q).dataHandle();

    dh->saveInto(*out_dh);
}

//----------------------------------------------------------------------------------------------------------------------

} // namespace dasi::tools


int main(int argc, char **argv) {
    dasi::tools::DASIGet app(argc, argv);
    return app.start();
}

