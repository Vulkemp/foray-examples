#include <bench/foray_bench.hpp>
#include <chrono>
#include <foray_api.hpp>
#include <fstream>
#include <vector>

namespace loadtimetest {

    namespace chr = std::chrono;

    inline const std::string SCENE_FILE = DATA_DIR "/intelsponza/NewSponza_Main_glTF_002.gltf";

    class LoadtimeTestApp : public foray::base::DefaultAppBase
    {
      protected:
        virtual void ApiInit() override;

        foray::osi::Utf8Path savePath;
        std::fstream         out;
    };

    void LoadtimeTestApp::ApiInit()
    {
        mRenderLoop.GetFrameTiming().DisableFpsLimit();

        std::vector<foray::bench::BenchmarkLog> timestamps(50);

        for(foray::bench::BenchmarkLog& bench : timestamps)
        {  // Load Scene
            foray::scene::Scene scene(&mContext);

            foray::gltf::ModelConverter converter(&scene);

            converter.LoadGltfModel(SCENE_FILE);

            bench = converter.GetBenchmark().GetLogs().back();
        }

        {
            savePath = foray::osi::Utf8Path(fmt::format("BenchLoad.csv")).MakeAbsolute();
            out.open((std::filesystem::path)savePath, std::ios_base::out);

            out << timestamps.front().PrintCsvHeader(';');

            for(foray::bench::BenchmarkLog& bench : timestamps)
            {
                out << bench.PrintCsvLine(';');
            }

            out.flush();
            out.close();
            mRenderLoop.RequestStop(0);
        }
    }

    int example(std::vector<std::string>& args)
    {
        foray::osi::OverrideCurrentWorkingDirectory(CWD_OVERRIDE);
        LoadtimeTestApp app;
        return app.Run();
    }
}  // namespace loadtimetest

int main(int argv, char** args)
{
    std::vector<std::string> argvec(argv);
    for(int i = 0; i < argv; i++)
    {
        argvec[i] = args[i];
    }
    return loadtimetest::example(argvec);
}
