#include <foray/api.hpp>
#include "gbufferdemo.hpp"
#include <vector>

namespace gbuffer {

    int example(std::vector<std::string>& args)
    {
        foray::osi::OverrideCurrentWorkingDirectory(CWD_OVERRIDE);
        foray::base::ApplicationLoop<GBufferDemoApp> app;
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
    return gbuffer::example(argvec);
}
