#include "eckit/option/CmdArgs.h"
#include "eckit/option/SimpleOption.h"
#include "eckit/log/Log.h"

#include "dasi/tools/DASITool.h"
#include "dasi/api/Dasi.h"

using namespace eckit::option;

namespace dasi::tools {

//----------------------------------------------------------------------------------------------------------------------

class DASIPolicy : public DASITool {

public: // methods

    DASIPolicy(int argc, char **argv) :
        DASITool(argc, argv),
        name_(""),
        value_("") {
            options_.push_back(new SimpleOption<std::string>("name", "The name of the policy or set of policies to print"));
            options_.push_back(new SimpleOption<std::string>("value", "The value of a policy to set. --name is required and must be fully specified"));
    }

private: // methods

    void usage(const std::string &tool) const override;
    void init(const eckit::option::CmdArgs& args) override;
    void execute(const eckit::option::CmdArgs& args) override;

    [[ nodiscard ]]
    int numberOfPositionalArguments() const override { return 1; }

private: // members

    std::string name_;
    std::string value_;
};


void DASIPolicy::usage(const std::string &tool) const {
    eckit::Log::info()
            << std::endl
            << "Usage: " << tool << " [options] <query>" << std::endl
            << std::endl
            << std::endl
            << "Examples:" << std::endl
            << "=========" << std::endl << std::endl
            << tool << " key1=value1/value2,key2=value2 --name access" << std::endl
            << std::endl;
    DASITool::usage(tool);
}

void DASIPolicy::init(const eckit::option::CmdArgs &args) {
    name_ = args.getString("name", name_);
    value_ = args.getString("value", value_);

    if (!value_.empty() && name_.empty()) {
        throw eckit::UserError("--name required if --value specified", Here());
    }
}

void DASIPolicy::execute(const eckit::option::CmdArgs& args) {
    dasi::Query query(args(0));

    if (value_.empty()) {
        for (const auto& policies: dasi().queryPolicy(query, name_)) {
            policies.print(eckit::Log::info(), /* pretty */ true);
        }
    } else {
        for (const auto& policies : dasi().setPolicy(query, {name_, value_})) {
            policies.print(eckit::Log::info(), /* pretty */ true);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------

} // namespace dasi::tools


int main(int argc, char **argv) {
    dasi::tools::DASIPolicy app(argc, argv);
    return app.start();
}

