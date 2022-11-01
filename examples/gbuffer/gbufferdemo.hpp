#pragma once
#include <foray_api.hpp>
#include <stages/foray_comparerstage.hpp>

namespace gbuffer {

    inline const std::string SCENE_FILE = "/mnt/bigssd/Projects/Master/hsk_rt_rpf_sponza_sample/glTF-Sample-Models/2.0/Sponza/glTF/Sponza.gltf";

    inline const std::string GBUFFER_ABOUT = "The GBuffer stage is a rasterized render pass which collects in depth meta information about the scene."
    "Similar techniques are traditionally used for rasterized deferred rendering, but have found great use in raytraced rendering too.";

    inline const std::string OUTPUT_ABOUT_TEXTS[] = {
      "Contains worldspace hitpoint positions in a rgba32f image. Can be used to launch rays from a first intersect calculated by rasterization, and as an input for denoising.",
      "Contains worldpace hitpoint normals in a rgba32f image. Can be used to launch rays from a first intersect calculated by rasterization, and as an input for denoising.",
      "Contains material base color in a rgba32f image. Can be used to calculate lighting information and as an input for denoising.",
      "Contains pixel flow information in screenspace. For each pixel, defines a projection from current UV coordinates to previous frame UV coordinates. Used for temporal reprojection.",
      "Contains material indices. Can be used as an input for denoisers and for deferred lighting calculation",
      "Contains mesh instance indices. Can be used as an input for denoisers and object identification",
      "Contains default vulkan depth format"
    };

    class GBufferDemoApp : public foray::base::DefaultAppBase
    {
      public:
        GBufferDemoApp() = default;

      protected:
        virtual void ApiInit() override;
        virtual void ApiRender(foray::base::FrameRenderInfo& renderInfo) override;
        virtual void ApiOnEvent(const foray::osi::Event* event) override;
        virtual void ApiDestroy() override;

        void SetView(int32_t index, foray::stages::GBufferStage::EOutput view);

        void HandleImGui();

        foray::stages::GBufferStage          mGBufferStage;
        foray::stages::ComparerStage         mComparerStage;
        foray::stages::ImguiStage            mImguiStage;
        foray::stages::ImageToSwapchainStage mSwapCopyStage;

        foray::stages::GBufferStage::EOutput mView[2];
        bool                                 mViewChanged[2] = {false, false};

        std::unique_ptr<foray::scene::Scene> mScene;
    };
}  // namespace gbuffer
