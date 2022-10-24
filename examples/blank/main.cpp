#include <foray_api.hpp>
#include <vector>

namespace blankapp {

    class BlankApp : public foray::base::DefaultAppBase
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
    return blankapp::example(argvec);
}
