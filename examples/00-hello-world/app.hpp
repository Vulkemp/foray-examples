#include <foray_api.hpp>

namespace example {
    class App : public foray::base::DefaultAppBase
    {
        virtual void ApiInit() override;

        virtual void ApiRender(foray::base::FrameRenderInfo& renderInfo) override;

        virtual void ApiDestroy() override;
    };

}  // namespace example