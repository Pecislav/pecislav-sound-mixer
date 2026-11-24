#include "mixer.hpp"
#include "i18n.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QLabel>
#include <QSlider>
#include <QThread>
#include <QToolButton>
#include <QTimer>
#include <QFrame>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QAction>
#include <QScrollArea>
#include <QScrollBar>
#include <QWheelEvent>
#include <QMenu>
#include <QPushButton>
#include <util/text-lookup.h>
#include <util/platform.h>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void check(bool ok,const char *message) { if(!ok)throw std::runtime_error(message); }
static const char *testName(void *) {return "Group Mixer isolated test source";}
static void *testCreate(obs_data_t *,obs_source_t *s) {return s;}
static void testDestroy(void *) {}
static lookup_t *catalogue=nullptr;
static const char *translate(const char *key){const char *value=nullptr;return catalogue&&text_lookup_getstr(catalogue,key,&value)?value:nullptr;}
static void pump(int ms=120)
{
    QElapsedTimer elapsed;elapsed.start();
    while(elapsed.elapsed()<ms){QApplication::processEvents();QThread::msleep(8);}
}
static QWidget *channel(gm::Mixer &m,const QString &id)
{
    for(auto *w:m.findChildren<QWidget *>())if(w->property("sourceUuid").toString()==id)return w;
    throw std::runtime_error("Channel widget not found");
}
static QMenu *openMenu(const QString &name)
{
    for(auto *widget:QApplication::topLevelWidgets())
        if(widget->objectName()==name)if(auto *menu=qobject_cast<QMenu *>(widget))return menu;
    return nullptr;
}

int main(int argc,char **argv)
{
    QApplication app(argc,argv);
    app.setFont(QFont("Segoe UI",9));
    QApplication::setStyle("Fusion");
    QPalette palette;palette.setColor(QPalette::Window,QColor("#262932"));
    palette.setColor(QPalette::WindowText,QColor("#f0f0f2"));palette.setColor(QPalette::Base,QColor("#242730"));
    palette.setColor(QPalette::Text,QColor("#f0f0f2"));palette.setColor(QPalette::Button,QColor("#30343f"));
    palette.setColor(QPalette::ButtonText,QColor("#f0f0f2"));palette.setColor(QPalette::Highlight,QColor("#5074d7"));
    app.setPalette(palette);
    if(!obs_startup("en-US",nullptr,nullptr))return 2;
    obs_audio_info ai{};ai.samples_per_sec=48000;ai.speakers=SPEAKERS_STEREO;
    if(!obs_reset_audio(&ai)){obs_shutdown();return 3;}
    obs_source_info info{};info.id="group_mixer_isolated_audio";info.type=OBS_SOURCE_TYPE_INPUT;
    info.output_flags=OBS_SOURCE_AUDIO;info.get_name=testName;info.create=testCreate;info.destroy=testDestroy;
    obs_register_source(&info);
    std::array<obs_source_t *,3> sources{};
    const char *names[]={"Mikrofon / AUX","Discord","Alerty / Twitch"};
    for(int i=0;i<3;++i){sources[i]=obs_source_create(info.id,names[i],nullptr,nullptr);obs_set_output_source(i+1,sources[i]);}
    int exitCode=0;
    try {
        const QString mic=QString::fromUtf8(obs_source_get_uuid(sources[0]));
        const QString discord=QString::fromUtf8(obs_source_get_uuid(sources[1]));
        const QString alerts=QString::fromUtf8(obs_source_get_uuid(sources[2]));
        gm::Model model;auto voice=model.addGroup("Hlas a komunikace"),effects=model.addGroup("Alerty a efekty");
        model.move(mic,voice);model.move(discord,voice);model.move(alerts,effects);
        for(const QString &locale:QStringList{"cs-CZ","en-US","sk-SK","de-DE","pl-PL"}){
            catalogue=text_lookup_create("data/locale/en-US.ini");check(catalogue,"English catalogue failed to load");
            const auto path=("data/locale/"+locale+".ini").toUtf8();check(text_lookup_add(catalogue,path.constData()),"Locale catalogue failed to load");
            gm::setTranslationCallback(translate);
            const QHash<QString,QString> expectedTitles={{"cs-CZ",QString::fromUtf8("Zvukový mixér")},{"en-US","Sound Mixer"},{"sk-SK",QString::fromUtf8("Zvukový mixér")},{"de-DE","Audiomixer"},{"pl-PL",QString::fromUtf8("Mikser dźwięku")}};
            check(gm::ui("Mixer.Name","Sound Mixer")==expectedTitles.value(locale),"Locale did not produce the expected translated title");
            check(gm::ui("Missing.Message","Fallback")=="Fallback","Missing translation has no English fallback");
            {
                gm::Mixer localized;localized.setAttribute(Qt::WA_DontShowOnScreen);localized.resize(640,470);
                auto *data=obs_data_create();const auto json=QJsonDocument(model.toJson()).toJson(QJsonDocument::Compact);
                obs_data_set_string(data,"pecislav_group_mixer",json.constData());localized.load(data);localized.refresh();localized.show();pump();
                check(localized.findChild<QAction *>("gmLayoutRows")->text()==gm::ui("Layout.Rows","Compact rows"),"Layout action not translated");
                check(localized.findChild<QToolButton *>("gmGlobalSettings")->accessibleName()==gm::ui("Mixer.Settings","Mixer settings"),"Settings label not translated");
                check(channel(localized,mic)->findChild<QLabel *>("gmSourceName")->accessibleName()=="Mikrofon / AUX","User source name was translated");
                obs_data_release(data);
            }
            gm::setTranslationCallback(nullptr);text_lookup_destroy(catalogue);catalogue=nullptr;
        }
        obs_source_set_volume(sources[0],0.668f);obs_source_set_volume(sources[1],0.214f);
        QTimer feed;
        double phase=0;
        QObject::connect(&feed,&QTimer::timeout,&app,[&] {
            float samples[2][960];
            for(int i=0;i<960;++i){const float sample=float(std::sin(phase)*0.16);phase+=2.0*3.141592653589793*440.0/48000.0;samples[0][i]=sample;samples[1][i]=sample*.8f;}
            obs_source_audio audio{};audio.data[0]=reinterpret_cast<const uint8_t *>(samples[0]);audio.data[1]=reinterpret_cast<const uint8_t *>(samples[1]);
            audio.frames=960;audio.speakers=SPEAKERS_STEREO;audio.format=AUDIO_FORMAT_FLOAT_PLANAR;audio.samples_per_sec=48000;audio.timestamp=os_gettime_ns();
            for(auto *s:sources)obs_source_output_audio(s,&audio);
        });feed.start(20);
        {
            gm::Mixer mixer;mixer.setAttribute(Qt::WA_DontShowOnScreen,true);mixer.resize(840,470);
            obs_data_t *data=obs_data_create();
            const auto json=QJsonDocument(model.toJson()).toJson(QJsonDocument::Compact);
            obs_data_set_string(data,"pecislav_group_mixer",json.constData());mixer.load(data);mixer.refresh();mixer.show();pump(650);
            auto *widget=channel(mixer,mic);auto *slider=widget->findChild<QSlider *>();check(slider,"volume slider missing");
            auto *sourceName=widget->findChild<QLabel *>("gmSourceName");
            check(sourceName&&sourceName->toolTip().contains("Mikrofon / AUX"),"full name tooltip missing");
            check(!sourceName->text().contains("⋮⋮"),"drag dots remain in source name");
            auto *settings=widget->findChild<QToolButton *>("gmSettings");check(settings&&!settings->icon().isNull()&&settings->text().isEmpty(),"ellipsis settings button missing");
            slider->setValue(500);pump();check(std::abs(obs_source_get_volume(sources[0])-.668f)>.01f,"slider did not update libobs");
            const float volumeBeforeWheel=obs_source_get_volume(sources[0]);const int sliderBeforeWheel=slider->value();
            slider->setFocus();
            QWheelEvent wheel(QPointF(8,8),QPointF(slider->mapToGlobal(QPoint(8,8))),QPoint(),QPoint(0,120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
            QApplication::sendEvent(slider,&wheel);pump();
            check(slider->value()==sliderBeforeWheel&&obs_source_get_volume(sources[0])==volumeBeforeWheel,"mouse wheel changed volume");
            obs_source_set_volume(sources[0],.5f);pump(180);
            check(widget->findChild<QLabel *>("gmDb")->text().contains("-6.0"),"external volume change not reflected");
            for(auto *button:widget->findChildren<QToolButton *>())if(button->accessibleName()==gm::ui("Channel.Mute","Mute source"))button->click();
            check(obs_source_muted(sources[0]),"mute did not update libobs");obs_source_set_muted(sources[0],false);
            obs_source_set_muted(sources[0],true);pump(320);
            check(widget->property("muted").toBool(),"channel mute styling missing");
            auto *overlay=widget->findChild<QWidget *>("gmMuteOverlay");
            check(overlay&&overlay->property("progress").toDouble()>.99,"mute animation did not complete");
            QDir().mkpath("test-output");check(mixer.grab().save("test-output/muted.png"),"mute capture failed");
            obs_source_set_muted(sources[0],false);pump(320);
            check(!widget->property("muted").toBool()&&overlay->property("progress").toDouble()<.01,"unmute animation failed");
            auto *move=widget->findChild<QComboBox *>("gmMove");
            QMetaObject::invokeMethod(move,"activated",Qt::DirectConnection,Q_ARG(int,2));pump();
            mixer.save(data);auto state=gm::Model::fromJson(QJsonDocument::fromJson(obs_data_get_string(data,"pecislav_group_mixer")).object());
            check(state.membership.value(mic)==effects,"UI move failed");
            const float before=obs_source_get_volume(sources[0]);mixer.moveSource(mic,{});pump();mixer.save(data);
            state=gm::Model::fromJson(QJsonDocument::fromJson(obs_data_get_string(data,"pecislav_group_mixer")).object());
            check(state.membership.value(mic).isEmpty(),"extract failed");check(obs_source_get_volume(sources[0])==before,"extraction changed sound");
            mixer.moveSource(mic,voice);pump();
            const auto audioMask=obs_source_get_audio_mixers(sources[0]);
            mixer.hideSource(mic);pump();mixer.save(data);
            state=gm::Model::fromJson(QJsonDocument::fromJson(obs_data_get_string(data,"pecislav_group_mixer")).object());
            check(state.hidden.contains(mic)&&state.membership.value(mic)==voice,"hide did not preserve group/persistence");
            bool foundHidden=false;for(auto *w:mixer.findChildren<QWidget *>())foundHidden|=w->property("sourceUuid").toString()==mic;
            check(!foundHidden,"hidden channel still visible");
            check(!obs_source_muted(sources[0])&&obs_source_get_volume(sources[0])==before&&obs_source_get_audio_mixers(sources[0])==audioMask,"hide changed audio");
            mixer.showHiddenSources();pump();check(channel(mixer,mic),"restore hidden channel failed");
            // A group drop uses the same native Qt event route as a physical drag.
            QFrame *groupTarget=nullptr;
            for(auto *frame:mixer.findChildren<QFrame *>())if(frame->objectName()=="gmGroup"&&frame->property("groupId").toString()==voice)groupTarget=frame;
            check(groupTarget,"group drop target missing");
            QMimeData groupData;groupData.setData("application/x-pecislav-mixer-group",effects.toUtf8());
            QDragEnterEvent enter(QPoint(8,8),Qt::MoveAction,&groupData,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(groupTarget,&enter);
            check(enter.isAccepted(),"group drag rejected");
            QDropEvent drop(QPointF(8,8),Qt::MoveAction,&groupData,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(groupTarget,&drop);pump();mixer.save(data);
            state=gm::Model::fromJson(QJsonDocument::fromJson(obs_data_get_string(data,"pecislav_group_mixer")).object());
            check(state.groups[0].id==effects&&state.membership.value(mic)==voice,"group drag failed or changed channel membership");
            mixer.moveGroup(voice,effects);pump();
            auto *verticalScroll=mixer.findChild<QScrollArea *>("gmScroll");
            for(int width:{560,840,1000}){
                mixer.resize(width,470);pump();
                QFrame *freeArea=nullptr;
                for(auto *frame:mixer.findChildren<QFrame *>())
                    if(frame->objectName()=="gmGroup"&&frame->property("groupId").toString().isEmpty())freeArea=frame;
                check(freeArea,"ungrouped area missing");
                check(freeArea->mapTo(verticalScroll->widget(),QPoint(freeArea->width(),0)).x()==verticalScroll->widget()->width(),"ungrouped area ends before right dock edge");
                auto *empty=freeArea->findChild<QLabel *>("gmEmptyGroup");
                check(empty&&empty->text()==gm::ui("Group.NoChannels","No channels"),"unclear empty group message remains");
            }
            mixer.resize(840,470);pump();
            QDir().mkpath("test-output");check(mixer.grab().save("test-output/vertical.png"),"vertical capture failed");
            auto *orientation=mixer.findChild<QAction *>("gmLayoutRows");
            check(orientation,"orientation menu action missing");orientation->trigger();pump();
            check(channel(mixer,mic)->objectName()=="gmRow","horizontal mode failed");
            mixer.resize(840,470);pump();check(mixer.grab().save("test-output/horizontal.png"),"horizontal capture failed");
            auto *scroll=mixer.findChild<QScrollArea *>("gmScroll");check(scroll,"scroll area missing");
            for(int width:{840,560,360,240,180}){
                mixer.resize(width,470);pump(180);
                check(mixer.width()==width,"dock refused narrow width");
                check(scroll->widget()->width()<=scroll->viewport()->width(),"compact content wider than viewport");
                check(scroll->horizontalScrollBar()->maximum()==0,"horizontal overflow in compact rows");
                auto *row=channel(mixer,mic);auto *levelMeter=row->findChild<QWidget *>("gmMeter");auto *rowSlider=row->findChild<QSlider *>();
                check(levelMeter->height()==12&&levelMeter->width()>30,"row meter has invalid size");
                check(levelMeter->mapTo(row,QPoint()).x()==rowSlider->mapTo(row,QPoint()).x()&&levelMeter->width()==rowSlider->width(),"row meter/slider misaligned");
                check(levelMeter->property("displayChannels").toInt()==2,"meter must show two bars");
                auto *footer=mixer.findChild<QWidget *>("gmFooter");
                for(auto *button:footer->findChildren<QToolButton *>())
                    check(button->mapTo(&mixer,QPoint(button->width(),0)).x()<=mixer.width(),"footer button clipped");
                if(width==240)check(mixer.grab().save("test-output/horizontal-narrow.png"),"narrow capture failed");
            }
            mixer.resize(560,470);pump();
            // Feed an actual mono source: rendering is mirrored, audio stays mono.
            auto *monoSource=obs_source_create(info.id,"Mono display test",nullptr,nullptr);obs_set_output_source(4,monoSource);
            mixer.refresh();pump();
            float monoSamples[960];for(auto &sample:monoSamples)sample=.1f;
            obs_source_audio monoAudio{};monoAudio.data[0]=reinterpret_cast<const uint8_t *>(monoSamples);monoAudio.frames=960;
            monoAudio.speakers=SPEAKERS_MONO;monoAudio.format=AUDIO_FORMAT_FLOAT_PLANAR;monoAudio.samples_per_sec=48000;monoAudio.timestamp=os_gettime_ns();
            obs_source_output_audio(monoSource,&monoAudio);pump(100);
            const auto monoId=QString::fromUtf8(obs_source_get_uuid(monoSource));
            auto *monoMeter=channel(mixer,monoId)->findChild<QWidget *>("gmMeter");
            check(monoMeter->property("sourceChannels").toUInt()==1&&monoMeter->property("displayChannels").toUInt()==2,"mono meter was not mirrored");
            obs_set_output_source(4,nullptr);obs_source_remove(monoSource);mixer.refresh();pump();obs_source_release(monoSource);
            mixer.save(data);const auto roundtrip=gm::Model::fromJson(QJsonDocument::fromJson(obs_data_get_string(data,"pecislav_group_mixer")).object());
            check(roundtrip.horizontal,"orientation not persisted");
            // Remove a live source while the UI and meter still hold references.
            obs_set_output_source(2,nullptr);obs_source_remove(sources[1]);mixer.refresh();pump();
            bool stillVisible=false;for(auto *w:mixer.findChildren<QWidget *>())stillVisible|=w->property("sourceUuid").toString()==discord;
            check(!stillVisible,"removed source remains visible");
            mixer.beginCollectionChange();pump();
            bool anyChannel=false;for(auto *w:mixer.findChildren<QWidget *>())anyChannel|=!w->property("sourceUuid").toString().isEmpty();
            check(!anyChannel,"collection cleanup retained channel widgets");
            mixer.load(data);mixer.finishCollectionChange();pump();
            check(channel(mixer,mic)->objectName()=="gmRow","collection reload did not restore layout");
            bool sourceMenuExisted=false;
            std::cerr<<"Checking source menu across collection changes\n";
            QTimer::singleShot(30,&mixer,[&]{
                auto *menu=openMenu("gmSourceMenu");
                sourceMenuExisted=menu!=nullptr;
                mixer.beginCollectionChange();mixer.load(data);mixer.finishCollectionChange();
                if(menu)menu->close();
            });
            mixer.showSourceMenu(mic,channel(mixer,mic),QPoint(10,10));pump();
            check(sourceMenuExisted,"source menu lifecycle test did not open a menu");
            bool groupMenuExisted=false;
            std::cerr<<"Checking group menu across collection changes\n";
            QPushButton *groupHandle=nullptr;
            for(auto *button:mixer.findChildren<QPushButton *>())if(button->objectName()=="gmGroupHandle"&&button->property("groupId").toString()==voice)groupHandle=button;
            check(groupHandle,"group menu lifecycle target missing");
            QTimer::singleShot(30,&mixer,[&]{
                auto *menu=openMenu("gmGroupMenu");groupMenuExisted=menu!=nullptr;
                mixer.beginCollectionChange();mixer.load(data);mixer.finishCollectionChange();
                if(menu)menu->close();
            });
            QMetaObject::invokeMethod(groupHandle,"customContextMenuRequested",Qt::DirectConnection,Q_ARG(QPoint,QPoint(2,2)));pump();
            check(groupMenuExisted,"group menu lifecycle test did not open a menu");
            obs_data_release(data);mixer.shutdown();
        }
        feed.stop();
        std::cout<<"PASS: five locales, English fallback, live audio, disabled wheel, mono bars, responsive layout, gray mute state, group/source menus across collection changes, persistence\n";
    } catch(const std::exception &e){std::cerr<<"FAIL: "<<e.what()<<'\n';exitCode=1;}
    for(int i=0;i<3;++i){obs_set_output_source(i+1,nullptr);obs_source_release(sources[i]);}
    obs_shutdown();return exitCode;
}
