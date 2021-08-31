#pragma once

#include "app.h"
#include "config.h"

namespace dosa {

template <class AppT>
class AppBuilder final
{
   public:
    AppBuilder()
    {
#ifdef DOSA_DEBUG
        config.wait_for_serial = true;
        config.log_level = dosa::LogLevel::DEBUG;
#else
        config.wait_for_serial = false;
        config.log_level = dosa::LogLevel::INFO;
#endif
    };

    Config& getConfig()
    {
        return config;
    }

    AppT& getApp()
    {
        static AppT app(config);
        return app;
    }

   private:
    Config config;
};

}  // namespace dosa