#pragma once
#include <foray_api.hpp>

namespace minimal_raytracer {

    inline const std::string RAYGEN_FILE     = "shaders/raygen.rgen";
    inline const std::string CLOSESTHIT_FILE = "shaders/closesthit.rchit";
    inline const std::string MISS_FILE       = "shaders/miss.rmiss";
    inline const std::string SCENE_FILE      = DATA_DIR "/gltf/testbox/scene.gltf";

    class MinimalRaytracingStage : public foray::stages::DefaultRaytracingStageBase
    {
      public:
        MinimalRaytracingStage(foray::core::Context* context, foray::scene::Scene* scene, foray::stages::RenderDomain* domain);
        virtual ~MinimalRaytracingStage() = default;

        virtual void ApiCreateRtPipeline() override;
        virtual void ApiDestroyRtPipeline() override;

      protected:
        foray::Local<foray::core::ShaderModule> mRaygen;
        foray::Local<foray::core::ShaderModule> mClosestHit;
        foray::Local<foray::core::ShaderModule> mMiss;
    };

    class MinimalRaytracerApp : public foray::base::DefaultAppBase
    {
      protected:
        virtual void ApiInit() override;
        virtual void ApiRender(foray::base::FrameRenderInfo& renderInfo) override;
        virtual void ApiDestroy() override;

        foray::Heap<MinimalRaytracingStage>               mRtStage;
        foray::Heap<foray::stages::ImageToSwapchainStage> mSwapCopyStage;
        foray::Heap<foray::scene::Scene>                  mScene;
    };

}  // namespace minimal_raytracer
