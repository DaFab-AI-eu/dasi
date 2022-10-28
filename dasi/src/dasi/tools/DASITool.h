
/// @author Simon Smart
/// @date   Oct 2022

#pragma once

#include "eckit/runtime/Tool.h"

#include <vector>

namespace eckit::option {
    class Option;
    class CmdArgs;
}

namespace dasi::tools {

//-------------------------------------------------------------------------------------------------

class DASITool : public eckit::Tool {

protected: // methods

    DASITool(int argc, char** argv);
    ~DASITool() override = default;

public: // methods

    virtual void usage(const std::string& tool) const {}

private: // methods

    void run() override;

    virtual void init(const eckit::option::CmdArgs& args) {}
    virtual void finish(const eckit::option::CmdArgs& args) {}
    virtual void execute(const eckit::option::CmdArgs& args) = 0;

    [[ nodiscard ]]
    virtual int numberOfPositionalArguments() const { return -1; }
    [[ nodiscard ]]
    virtual int minimumPositionalArguments() const { return -1; }

protected: // members

    std::vector<eckit::option::Option*> options_;
};

//-------------------------------------------------------------------------------------------------

} // namespace dasi::tools
