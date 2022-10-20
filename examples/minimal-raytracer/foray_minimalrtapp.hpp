#pragma once
#include <base/foray_defaultappbase.hpp>
#include <scene/foray_scene.hpp>
#include <stages/foray_raytracingstage.hpp>
#include <stages/foray_imagetoswapchain.hpp>

namespace foray::minimal_raytracer {

    inline const std::string RAYGEN_FILE = "shaders/raygen.rgen";
    inline const std::string CLOSESTHIT_FILE = "shaders/closesthit.rchit";
    inline const std::string MISS_FILE = "shaders/miss.rmiss";
    inline const std::string SCENE_FILE = DATA_DIR "/gltf/minimal/minimal.gltf";

    class MinimalRaytracingStage : public stages::RaytracingStage
    {
      public:
        virtual void Init(const foray::core::VkContext* context, foray::scene::Scene* scene);
        virtual void CreateRaytraycingPipeline() override;
        virtual void OnShadersRecompiled() override;
        virtual void DestroyShaders() override;

      protected:
        core::ShaderModule mRaygen;
        core::ShaderModule mClosestHit;
        core::ShaderModule mMiss;
    };

    class MinimalRaytracerApp : public base::DefaultAppBase
    {
      protected:
        virtual void Init() override;
        virtual void OnEvent(const foray::Event* event) override;

        virtual void RecordCommandBuffer(foray::base::FrameRenderInfo& renderInfo) override;
        virtual void Destroy() override;

        MinimalRaytracingStage        mRtStage;
        stages::ImageToSwapchainStage mSwapCopyStage;
        std::unique_ptr<scene::Scene> mScene;
    };

}  // namespace foray::minimal_raytracer
