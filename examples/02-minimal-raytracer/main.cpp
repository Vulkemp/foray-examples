#include "foray_minimalrtapp.hpp"
#include <foray_basics.hpp>
#include <foray_logger.hpp>
#include <osi/foray_env.hpp>
#include <vector>

namespace minimal_raytracer {


    int example(std::vector<std::string>& args)
    {
        foray::osi::OverrideCurrentWorkingDirectory(CWD_OVERRIDE);
        foray::base::ApplicationLoop<MinimalRaytracerApp> app;
        return app.Run();
    }
}  // namespace minimal_raytracer

int main(int argv, char** args)
{
    std::vector<std::string> argvec(argv);
    for(int i = 0; i < argv; i++)
    {
        argvec[i] = args[i];
    }
    return minimal_raytracer::example(argvec);
}
