#include "config.hpp"
#include "diagnostics.hpp"
#include "globals.hpp"
#include "patches.hpp"

#include <hellforge_mod.h>

using globals::hfApi;
using globals::hfGame;
using globals::hfMod;

namespace
{

void gameLoaded(HfMod* mod, HfEvent event, void* user)
{
    hfMod = mod;
    hfApi = HF_API(mod);
    hfGame = hfApi->module_base("game.dll", nullptr);

    if (hfGame == 0)
    {
        hfApi->log(hfMod, "game.dll not found");
        return;
    }

    const config::Config config = config::read();
    const patches::Result patchResult = patches::apply(config);
    if (!patchResult.success)
    {
        if (patchResult.count == 0)
            hfApi->log(hfMod, "not patched: no patches applied");
        else
            hfApi->log(hfMod,
                       "not fully patched: %d of %d patches applied",
                       patchResult.count,
                       patchResult.expected);

        return;
    }

    if (config.verboseLogging)
    {
        diagnostics::installHooks();
    }
}
} // namespace

extern "C" HF_MOD_EXPORT int hellforge_mod_init(const HfApi* api, HfMod* mod)
{
    if (api->version < 16)
        return 1;
    api->on(mod, HF_EVENT_GAME_LOADED, gameLoaded, nullptr);
    return 0;
}
