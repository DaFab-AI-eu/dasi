
/// @author Simon Smart
/// @date   Oct 2022

#pragma once

#include "dasi/api/Dasi.h"
#include "eckit/filesystem/PathName.h"
#include "eckit/runtime/Tool.h"
#include "eckit/utils/Optional.h"

#include <vector>

namespace eckit::option {
class Option;
class CmdArgs;
}  // namespace eckit::option

namespace dasi::tools {

//-------------------------------------------------------------------------------------------------

class DASITool : public eckit::Tool {
protected:  // methods
    DASITool(int argc, char** argv);
    ~DASITool() override = default;

    dasi::Dasi& dasi();

public:  // methods
    virtual void usage(const std::string&) const { }

private:  // methods
    void run() override;

    virtual void init(const eckit::option::CmdArgs&) { }

    virtual void finish(const eckit::option::CmdArgs&) { }

    virtual void execute(const eckit::option::CmdArgs& args) = 0;

    [[nodiscard]]
    virtual int numberOfPositionalArguments() const {
        return -1;
    }

    [[nodiscard]]
    virtual int minimumPositionalArguments() const {
        return -1;
    }

    void initInternal(const eckit::option::CmdArgs& args);

protected:  // members
    std::vector<eckit::option::Option*> options_;

    eckit::PathName configPath_;

    eckit::Optional<dasi::Dasi> dasi_;
};

//-------------------------------------------------------------------------------------------------

class DASIToolException : public eckit::Exception {
public:
    explicit DASIToolException(const std::ostringstream& message);

    explicit DASIToolException(const std::string& message);

    DASIToolException(const std::string& message, const eckit::CodeLocation& location);
};

//-------------------------------------------------------------------------------------------------

}  // namespace dasi::tools
