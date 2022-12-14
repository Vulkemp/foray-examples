#include <foray_api.hpp>
#include "gbufferdemo.hpp"
#include <vector>

namespace gbuffer {

    int example(std::vector<std::string>& args)
    {
        foray::osi::OverrideCurrentWorkingDirectory(CWD_OVERRIDE);
        GBufferDemoApp app;
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
