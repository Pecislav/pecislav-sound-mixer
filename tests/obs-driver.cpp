// Only copied into the isolated portable OBS test, never the user package.
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QApplication>
#include <QWidget>
#include <QTimer>
#include <QDockWidget>
OBS_DECLARE_MODULE()
bool obs_module_load(void)
{
    QTimer::singleShot(2500,qApp,[] {
        auto *main=static_cast<QWidget *>(obs_frontend_get_main_window());
        QWidget *mixer=main?main->findChild<QWidget *>("groupMixer"):nullptr;
        QDockWidget *dock=nullptr;
        for(QWidget *p=mixer;p;p=p->parentWidget())if((dock=qobject_cast<QDockWidget *>(p)))break;
        auto *module=obs_get_module("pecislav-group-mixer");
        const auto title=module?QString::fromUtf8(obs_module_get_locale_text(module,"Mixer.Name")):QString();
        const bool found=mixer && dock && !title.isEmpty() && dock->windowTitle()==title;
        blog(found?LOG_INFO:LOG_ERROR,"[GM integration] Real OBS dock registration: %s",found?"PASS":"FAIL");
        blog(LOG_INFO,"[GM integration] OBS locale: %s; panel title: %s",obs_get_locale(),title.toUtf8().constData());
        obs_frontend_save();
        if(main)main->close();
        qApp->quit();
    });
    return true;
}
