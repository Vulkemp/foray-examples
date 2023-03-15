#include <foray_api.hpp>

namespace example {
    class App : public foray::base::DefaultAppBase
    {
        virtual void ApiInit() override;

        virtual void ApiOnResized(VkExtent2D size) override;

        virtual void ApiOnEvent(const foray::osi::Event* event) override;

        virtual void ApiRender(foray::base::FrameRenderInfo& renderInfo) override;

        virtual void ApiOnShadersRecompiled(std::unordered_set<uint64_t>& recompiledShaderKeys) override;

        virtual void ApiDestroy() override;
    };

}  // namespace example