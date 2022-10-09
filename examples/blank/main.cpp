#include <base/foray_defaultappbase.hpp>
#include <foray_basics.hpp>
#include <foray_logger.hpp>
#include <osi/foray_env.hpp>
#include <vector>

namespace foray::blank {

    class BlankApp : public base::DefaultAppBase
    {
    };

    int example(std::vector<std::string>& args)
    {
        foray::osi::OverrideCurrentWorkingDirectory(CWD_OVERRIDE);
        BlankApp app;
        return app.Run();
    }
}  // namespace foray::blank

int main(int argv, char** args)
{
    std::vector<std::string> argvec(argv);
    for(int i = 0; i < argv; i++)
    {
        argvec[i] = args[i];
    }
    return foray::blank::example(argvec);
}
