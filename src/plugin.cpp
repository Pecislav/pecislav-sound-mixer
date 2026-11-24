#include "mixer.hpp"
#include "i18n.hpp"
#include <QPointer>
#include <obs-frontend-api.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("pecislav-group-mixer", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
    return "Dockable audio mixer with draggable channels and collapsible groups.";
}
MODULE_EXPORT const char *obs_module_name(void) { return "Pecislav Sound Mixer"; }
MODULE_EXPORT const char *obs_module_author(void) { return "Pecislav"; }

static QPointer<gm::Mixer> mixer;
static constexpr const char *dockId = "pecislav-group-mixer-dock";
static void persistence(obs_data_t *data, bool saving, void *)
{
    if (!mixer) return;
    if (saving) mixer->save(data); else mixer->load(data);
}
static void event(enum obs_frontend_event e, void *)
{
    if (!mixer) return;
    if (e == OBS_FRONTEND_EVENT_EXIT || e == OBS_FRONTEND_EVENT_SCRIPTING_SHUTDOWN)
        mixer->shutdown();
    else if (e == OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING || e == OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP)
        mixer->beginCollectionChange();
    else if (e == OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED)
        mixer->finishCollectionChange();
    else if (e == OBS_FRONTEND_EVENT_FINISHED_LOADING || e == OBS_FRONTEND_EVENT_SCENE_CHANGED ||
             e == OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED)
        mixer->refresh();
}
bool obs_module_load(void)
{
    gm::setTranslationCallback([](const char *key){return obs_module_text(key);});
    mixer = new gm::Mixer(static_cast<QWidget *>(obs_frontend_get_main_window()));
    const auto title=gm::ui("Mixer.Name","Sound Mixer").toUtf8();
    if (!obs_frontend_add_dock_by_id(dockId, title.constData(), mixer)) {
        delete mixer.data();
        gm::setTranslationCallback(nullptr);
        return false;
    }
    obs_frontend_add_save_callback(persistence, nullptr);
    obs_frontend_add_event_callback(event, nullptr);
    blog(LOG_INFO, "[Pecislav Group Mixer] Loaded (OBS 33+, Windows x64)");
    return true;
}
void obs_module_unload(void)
{
    obs_frontend_remove_event_callback(event, nullptr);
    obs_frontend_remove_save_callback(persistence, nullptr);
    if (mixer) mixer->shutdown();
    obs_frontend_remove_dock(dockId);
    gm::setTranslationCallback(nullptr);
    // The frontend owns the dock widget after registration.
}
