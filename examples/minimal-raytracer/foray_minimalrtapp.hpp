#pragma once
#include <foray_api.hpp>
#include <stages/foray_raytracingstage.hpp>

namespace minimal_raytracer {

    inline const std::string RAYGEN_FILE = "shaders/raygen.rgen";
    inline const std::string CLOSESTHIT_FILE = "shaders/closesthit.rchit";
    inline const std::string MISS_FILE = "shaders/miss.rmiss";
    inline const std::string SCENE_FILE = DATA_DIR "/gltf/minimal/minimal.gltf";

    class MinimalRaytracingStage : public foray::stages::RaytracingStage
    {
      public:
        virtual void Init(foray::core::Context* context, foray::scene::Scene* scene);
        virtual void CreateRaytraycingPipeline() override;
        virtual void OnShadersRecompiled() override;
        virtual void DestroyShaders() override;

      protected:
        foray::core::ShaderModule mRaygen;
        foray::core::ShaderModule mClosestHit;
        foray::core::ShaderModule mMiss;
    };

    class MinimalRaytracerApp : public foray::base::DefaultAppBase
    {
      protected:
        virtual void ApiInit() override;
        virtual void ApiOnEvent(const foray::osi::Event* event) override;

        virtual void ApiRender(foray::base::FrameRenderInfo& renderInfo) override;
        virtual void ApiDestroy() override;

        MinimalRaytracingStage        mRtStage;
        foray::stages::ImageToSwapchainStage mSwapCopyStage;
        std::unique_ptr<foray::scene::Scene> mScene;
    };

}  // namespace foray::minimal_raytracer
