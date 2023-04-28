#pragma once
#include <foray_api.hpp>
#include <stages/foray_comparerstage.hpp>

namespace gbuffer {

    inline const std::string SCENE_FILE = DATA_DIR "/gltf/outdoorbox/scene.gltf";

    inline const std::string GBUFFER_ABOUT =
        "The GBuffer stage is a rasterized render pass which collects in depth meta information about the scene."
        "Similar techniques are traditionally used for rasterized deferred rendering, but have found great use in raytraced rendering too.";

    inline const std::string OUTPUT_ABOUT_TEXTS[] = {
        "Contains worldspace hitpoint positions in a rgba16f image. Can be used to launch rays from a first intersect calculated by rasterization, and as an input for denoising.",
        "Contains worldpace hitpoint normals in a rgba16f image. Can be used to launch rays from a first intersect calculated by rasterization, and as an input for denoising.",
        "Contains material base color in a rgba16f image. Can be used to calculate lighting information and as an input for denoising.",
        "Contains screenspace pixel flow information in a rg16f image. For each pixel, defines a projection from current UV coordinates to previous frame UV coordinates. Used for "
        "temporal reprojection.",
        "Contains material indices in a r32i image. Can be used as an input for denoisers and for deferred lighting calculation.",
        "Contains mesh instance indices in a r32u image. Can be used as an input for denoisers and object identification.",
        "Contains processed depth information in a rg16f image. First channel is linear depth, second channel is a depth gradient. Useful for denoising.",
        "Contains default vulkan depth format (can be read from shaders as r32f)."};

    enum class EOutput
    {
        Position,
        Normal,
        Albedo,
        Motion,
        MaterialIdx,
        MeshInstanceIdx,
        LinearZ,
        Depth,
    };

    class GBufferDemoApp : public foray::base::DefaultAppBase
    {
      public:
        GBufferDemoApp() = default;

      protected:
        virtual void ApiBeforeInstanceCreate(vkb::InstanceBuilder& instanceBuilder) override;
        virtual void ApiInit() override;
        virtual void ApiRender(foray::base::FrameRenderInfo& renderInfo) override;
        virtual void ApiDestroy() override;

        void SetView(int32_t index, EOutput view);

        void HandleImGui();

        /// @brief The GBuffer stage renders scene information using rasterization
        foray::Heap<foray::stages::ConfigurableRasterStage> mGBufferStage;
        /// @brief The comparer stage allows side by side view of multi-type input images
        foray::Heap<foray::stages::ComparerStage> mComparerStage;
        /// @brief The ImGui stage renders the GUI
        foray::Heap<foray::stages::ImguiStage> mImguiStage;
        /// @brief The swap copy stage blits the output image onto the swapchain
        foray::Heap<foray::stages::ImageToSwapchainStage> mSwapCopyStage;

        /// @brief The currently viewed GBuffer outputs (left and right side)
        EOutput mView[2];
        /// @brief Dirty markerfor the GBuffer output views (left and right side)
        bool mViewChanged[2] = {false, false};

        /// @brief Scene
        foray::Heap<foray::scene::Scene> mScene;
    };
}  // namespace gbuffer
