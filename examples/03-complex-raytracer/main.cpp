#include "foray_complexrtapp.hpp"
#include <foray/basics.hpp>
#include <foray/logger.hpp>
#include <foray/osi/path.hpp>
#include <vector>

namespace complex_raytracer {


    int example(std::vector<std::string>& args)
    {
        foray::osi::OverrideCurrentWorkingDirectory(CWD_OVERRIDE);
        foray::base::ApplicationLoop<ComplexRaytracerApp> app;
        return app.Run();
    }
}  // namespace complex_raytracer

int main(int argv, char** args)
{
    std::vector<std::string> argvec(argv);
    for(int i = 0; i < argv; i++)
    {
        argvec[i] = args[i];
    }
    return complex_raytracer::example(argvec);
}
