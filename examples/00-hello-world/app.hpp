#include <foray/api.hpp>

namespace example {
    class App : public foray::base::DefaultAppBase
    {
      public:
        inline explicit App(foray::base::AppLoopBase* apploop) : foray::base::DefaultAppBase(apploop) {}
        virtual ~App();

      protected:
        virtual void ApiInit() override;

        virtual void ApiRender(foray::base::FrameRenderInfo& renderInfo) override;

    };

}  // namespace example