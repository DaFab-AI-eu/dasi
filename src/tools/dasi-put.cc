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

class DASIPut : public DASITool {

public: // methods

    DASIPut(int argc, char **argv) :
            DASITool(argc, argv) {}

private: // methods

    void usage(const std::string &tool) const override;
    void execute(const eckit::option::CmdArgs& args) override;
};

void DASIPut::usage(const std::string &tool) const {
    eckit::Log::info()
            << std::endl
            << "Usage: " << tool << " [options] <key> <path> [...]" << std::endl
            << std::endl
            << std::endl
            << "Examples:" << std::endl
            << "=========" << std::endl << std::endl
            << tool << " key1=value1,key2=value2 path/to/data" << std::endl
            << std::endl;
    DASITool::usage(tool);
}

void DASIPut::execute(const eckit::option::CmdArgs& args) {

    if (args.count() % 2 != 0) {
        throw UserError("Invalid arguments supplied. Key and data required for all objects", Here());
    }

    for (size_t i = 0; i < args.count()/2; ++i) {

        dasi::Key k(args(2*i));
        eckit::PathName path(args(2*i+1));

        std::unique_ptr<DataHandle> dh(path.fileHandle());
        MemoryHandle mh;
        dh->copyTo(mh);

        dasi().archive(k, mh.data(), mh.size());
    }
}

//----------------------------------------------------------------------------------------------------------------------

} // namespace dasi::tools


int main(int argc, char **argv) {
    dasi::tools::DASIPut app(argc, argv);
    return app.start();
}

