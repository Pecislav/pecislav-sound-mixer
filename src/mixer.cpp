#include "mixer.hpp"
#include "ballistics.hpp"
#include "i18n.hpp"
#include <obs-frontend-api.h>
#include <util/config-file.h>

#include <QActionGroup>
#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFormLayout>
#include <QInputDialog>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QToolTip>
#include <QHelpEvent>
#include <QVariantAnimation>
#include <QGridLayout>
#include <QWheelEvent>
#include <QStyleOptionButton>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <functional>
#include <limits>
#include <memory>

namespace gm {
static constexpr const char *mimeType = "application/x-pecislav-mixer-source";
static constexpr const char *groupMimeType = "application/x-pecislav-mixer-group";
static int activeDrags = 0;
static QString text(const char *s) { return QString::fromUtf8(s ? s : ""); }
static long long nowMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

static QString askText(QWidget *parent,const QString &title,const QString &label,const QString &initial,bool *ok)
{
    QInputDialog dialog(parent);dialog.setWindowTitle(title);dialog.setLabelText(label);
    dialog.setInputMode(QInputDialog::TextInput);dialog.setTextValue(initial);
    dialog.setOkButtonText(ui("Common.Ok","OK"));dialog.setCancelButtonText(ui("Common.Cancel","Cancel"));
    *ok=dialog.exec()==QDialog::Accepted;
    return dialog.textValue();
}

class DropFrame final : public QFrame {
public:
    std::function<void(const QString &)> dropped;
    std::function<void(const QString &)> groupDropped;
    explicit DropFrame(QWidget *parent = nullptr) : QFrame(parent) { setAcceptDrops(true); }
protected:
    void dragEnterEvent(QDragEnterEvent *e) override
    {
        if (e->mimeData()->hasFormat(mimeType) || (groupDropped && e->mimeData()->hasFormat(groupMimeType))) {
            setProperty("dropTarget", true);
            style()->unpolish(this); style()->polish(this);
            e->acceptProposedAction();
        }
    }
    void dragMoveEvent(QDragMoveEvent *e) override
    {
        if (e->mimeData()->hasFormat(mimeType) || (groupDropped && e->mimeData()->hasFormat(groupMimeType))) e->acceptProposedAction();
    }
    void clearDrop()
    {
        setProperty("dropTarget", false);
        style()->unpolish(this); style()->polish(this);
    }
    void dragLeaveEvent(QDragLeaveEvent *e) override { clearDrop(); e->accept(); }
    void dropEvent(QDropEvent *e) override
    {
        clearDrop();
        if(groupDropped && e->mimeData()->hasFormat(groupMimeType)){
            const auto id=QString::fromUtf8(e->mimeData()->data(groupMimeType));
            e->setDropAction(Qt::MoveAction);e->accept();groupDropped(id);return;
        }
        if (!e->mimeData()->hasFormat(mimeType)) return;
        const auto id = QString::fromUtf8(e->mimeData()->data(mimeType));
        e->setDropAction(Qt::MoveAction); e->accept();
        if (dropped) dropped(id);
    }
};

class DragLabel final : public QLabel {
    QString uuid;
    QPoint start;
public:
    DragLabel(const QString &id, QWidget *parent) : QLabel(parent), uuid(id)
    {
        setCursor(Qt::OpenHandCursor);
        setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
        setAttribute(Qt::WA_AlwaysShowToolTips);
        setTextFormat(Qt::PlainText);
        setToolTip(ui("Drag.Channel","Drag into a group, before another channel, or into Ungrouped."));
    }
protected:
    bool event(QEvent *e) override
    {
        if(e->type()==QEvent::ToolTip){
            auto *help=static_cast<QHelpEvent *>(e);
            QToolTip::showText(help->globalPos(),toolTip(),this);
            e->accept();return true;
        }
        return QLabel::event(e);
    }
    void mousePressEvent(QMouseEvent *e) override
    {
        if (e->button() == Qt::LeftButton) start = e->position().toPoint();
        QLabel::mousePressEvent(e);
    }
    void mouseMoveEvent(QMouseEvent *e) override
    {
        if (!(e->buttons() & Qt::LeftButton) ||
            (e->position().toPoint()-start).manhattanLength() < QApplication::startDragDistance()) return;
        auto *mime = new QMimeData;
        mime->setData(mimeType, uuid.toUtf8());
        auto *drag = new QDrag(this);
        drag->setMimeData(mime);
        drag->setPixmap(grab());
        ++activeDrags;
        drag->exec(Qt::MoveAction);
        --activeDrags;
    }
};

class RailButton final : public QPushButton {
    QString groupId;
    QPoint start;
    bool dragged=false;
    bool vertical=true;
public:
    QSize minimumSizeHint()const override{return vertical?QSize(36,160):QSize(30,25);}
    bool collapsed = false;
    bool free = false;
    RailButton(const QString &label, QWidget *parent,const QString &id={},bool row=false)
        : QPushButton(label, parent),groupId(id),vertical(!row)
    {
        if(vertical){setFixedWidth(36);setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Expanding);setMinimumHeight(160);}
        if(!id.isEmpty())setCursor(Qt::OpenHandCursor);
    }
protected:
    void mousePressEvent(QMouseEvent *e) override
    {
        if(e->button()==Qt::LeftButton){start=e->position().toPoint();dragged=false;}
        QPushButton::mousePressEvent(e);
    }
    void mouseMoveEvent(QMouseEvent *e) override
    {
        if(groupId.isEmpty() || !(e->buttons()&Qt::LeftButton) ||
            (e->position().toPoint()-start).manhattanLength()<QApplication::startDragDistance()){
            QPushButton::mouseMoveEvent(e);return;
        }
        dragged=true;setDown(false);
        auto *mime=new QMimeData;mime->setData(groupMimeType,groupId.toUtf8());
        auto *drag=new QDrag(this);drag->setMimeData(mime);drag->setPixmap(grab());
        ++activeDrags;drag->exec(Qt::MoveAction);--activeDrags;
    }
    void mouseReleaseEvent(QMouseEvent *e) override
    {
        if(dragged){dragged=false;setDown(false);e->accept();return;}
        QPushButton::mouseReleaseEvent(e);
    }
    void paintEvent(QPaintEvent *event) override
    {
        if(!vertical){
            Q_UNUSED(event);
            QStyleOptionButton option;initStyleOption(&option);
            option.text=fontMetrics().elidedText(text(),Qt::ElideRight,std::max(0,width()-18));
            QPainter p(this);style()->drawControl(QStyle::CE_PushButton,&option,&p,this);return;
        }
        QPainter p(this);
        p.fillRect(rect(), underMouse() ? QColor("#3a4253") : QColor("#30343e"));
        p.setPen(QColor("#e7e9f0"));
        p.drawText(QRect(0, 6, width(), 20), Qt::AlignCenter, free ? "" : collapsed ? "▸" : "◂");
        p.save();
        p.translate(width()/2.0, 38);
        p.rotate(90);
        p.drawText(QRect(0, -width()/2, height()-48, width()), Qt::AlignVCenter|Qt::AlignLeft, text());
        p.restore();
        if (hasFocus()) {
            p.setPen(QColor("#7695e4"));
            p.drawRect(rect().adjusted(1,1,-2,-2));
        }
    }
};

class Meter final : public QWidget {
    obs_volmeter_t *meter = nullptr;
    std::array<std::atomic<float>, MAX_AUDIO_CHANNELS> peaks;
    std::array<std::atomic<float>, MAX_AUDIO_CHANNELS> magnitudes;
    std::array<std::atomic<float>, MAX_AUDIO_CHANNELS> pendingPeaks;
    std::array<MeterEnvelope, MAX_AUDIO_CHANNELS> envelopes;
    std::atomic<long long> received{0};
    bool horizontal;
    bool muted = false;
    double decayRate=23.53;
    long long painted=0;
    QTimer timer;
    static void receive(void *data, const float magnitude[MAX_AUDIO_CHANNELS],
                        const float peak[MAX_AUDIO_CHANNELS], const float[MAX_AUDIO_CHANNELS])
    {
        auto *self = static_cast<Meter *>(data);
        for (size_t i=0; i<MAX_AUDIO_CHANNELS; ++i){
            self->peaks[i].store(peak[i], std::memory_order_relaxed);
            self->magnitudes[i].store(magnitude[i],std::memory_order_relaxed);
            float previous=self->pendingPeaks[i].load(std::memory_order_relaxed);
            while(peak[i]>previous && !self->pendingPeaks[i].compare_exchange_weak(previous,peak[i],std::memory_order_relaxed)){}
        }
        self->received.store(nowMs(), std::memory_order_relaxed);
    }
public:
    Meter(obs_source_t *source, bool row, QWidget *parent,double decay,bool truePeak)
        : QWidget(parent), horizontal(row),decayRate(decay)
    {
        for (auto &p : peaks) p.store(-100.0f);
        for(auto &p:magnitudes)p.store(-100.0f);
        for(auto &p:pendingPeaks)p.store(-INFINITY);
        setObjectName("gmMeter");setProperty("displayChannels",2);
        if (horizontal) { setFixedHeight(12);setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed); }
        else { setFixedWidth(48); setMinimumHeight(180); }
        meter = obs_volmeter_create(OBS_FADER_LOG);
        if (meter) {
            obs_volmeter_set_peak_meter_type(meter,truePeak?TRUE_PEAK_METER:SAMPLE_PEAK_METER);
            obs_volmeter_add_callback(meter, receive, this);
            obs_volmeter_attach_source(meter, source);
        }
        setAccessibleName(ui("Meter.Label","Audio level meter"));
        connect(&timer, &QTimer::timeout, this, [this] { if (isVisible()) update(); });
        timer.setTimerType(Qt::PreciseTimer);timer.start(16);
    }
    ~Meter() override
    {
        timer.stop();
        if (meter) {
            obs_volmeter_remove_callback(meter, receive, this);
            obs_volmeter_detach_source(meter);
            obs_volmeter_destroy(meter);
        }
    }
    void setMuted(bool value) { muted = value; }
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        const auto now=nowMs();
        const double dt=painted?double(now-painted)/1000.0:0.016;
        painted=now;
        const bool stale = now-received.load(std::memory_order_relaxed)>300;
        const unsigned actualCount = meter ? unsigned(std::clamp(obs_volmeter_get_nr_channels(meter), 1, 2)) : 1u;
        setProperty("sourceChannels",actualCount);
        for(unsigned c=0;c<actualCount;++c){
            const float sample=std::max(peaks[c].load(std::memory_order_relaxed),pendingPeaks[c].exchange(-INFINITY,std::memory_order_relaxed));
            envelopes[c].advance(stale?-60.0:double(sample),stale?-60.0:double(magnitudes[c].load(std::memory_order_relaxed)),dt,decayRate);
        }
        const int length = horizontal ? width() : height()-8;
        for (unsigned c=0; c<2; ++c) {
            const auto &envelope=envelopes[actualCount==1?0:c];
            const int cross = int(c)*7;
            QRect bar = horizontal ? QRect(0,cross,length,5) : QRect(cross,4,6,length);
            p.fillRect(bar, QColor("#122e17"));
            const double ratio = std::clamp((envelope.peak+60.0)/60.0,0.0,1.0);
            for (int segment=0; segment<3; ++segment) {
                const double low = segment==0 ? 0.0 : segment==1 ? 40.0/60.0 : 51.0/60.0;
                const double high = segment==0 ? 40.0/60.0 : segment==1 ? 51.0/60.0 : 1.0;
                const QColor color = muted?QColor("#8c909a"):segment==0 ? QColor("#27a734") : segment==1 ? QColor("#d3ad25") : QColor("#d84154");
                QRect background = horizontal ? QRect(int(length*low),cross,int(length*(high-low))+1,5)
                                               : QRect(cross,4+int(length*(1-high)),6,int(length*(high-low))+1);
                p.fillRect(background, color.darker(310));
                if (ratio<=low) continue;
                const double top=std::min(high,ratio);
                QRect fill = horizontal ? QRect(int(length*low),cross,int(length*(top-low))+1,5)
                                        : QRect(cross,4+int(length*(1-top)),6,int(length*(top-low))+1);
                p.fillRect(fill,color);
            }
            const double vu=std::clamp((envelope.magnitude+60.0)/60.0,0.0,1.0);
            const double held=std::clamp((envelope.held+60.0)/60.0,0.0,1.0);
            if(ratio>0.0){
                p.fillRect(horizontal?QRect(int(length*vu),cross,2,5):QRect(cross,4+int(length*(1-vu)),6,2),QColor("#20242a"));
                p.fillRect(horizontal?QRect(int(length*held),cross,1,5):QRect(cross,4+int(length*(1-held)),6,1),muted?QColor("#b0b0b0"):QColor("#e5e6e8"));
            }
        }
        setProperty("displayPeak",envelopes[0].peak);
        if (!horizontal) {
            QFont f=font(); f.setPointSizeF(7.5); p.setFont(f); p.setPen(QColor("#a3a6ae"));
            for (int db=0; db>=-60; db-=6) {
                const int y=4+int((-double(db)/60.0)*(height()-8));
                p.drawText(QRect(18,y-6,30,12),Qt::AlignVCenter|Qt::AlignLeft,QString::number(db));
            }
        }
    }
};

static QIcon headphoneIcon(bool enabled)
{
    QPixmap image(18,18); image.fill(Qt::transparent);
    QPainter p(&image); p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor("#f0f1f4"),2));
    p.drawArc(QRectF(3,2,12,13),0,180*16);
    p.drawRoundedRect(QRectF(2,8,3,7),1,1);
    p.drawRoundedRect(QRectF(13,8,3,7),1,1);
    if (!enabled) { p.setPen(QPen(QColor("#a7abb5"),1.5)); p.drawLine(5,14,13,6); }
    return QIcon(image);
}

static QIcon volumeIcon(QWidget *widget,bool muted)
{
    QPixmap image=widget->style()->standardIcon(muted?QStyle::SP_MediaVolumeMuted:QStyle::SP_MediaVolume).pixmap(18,18);
    QPainter p(&image);p.setCompositionMode(QPainter::CompositionMode_SourceIn);p.fillRect(image.rect(),QColor("#eff0f4"));
    return QIcon(image);
}

static QIcon settingsIcon()
{
    QPixmap image(16,18);image.fill(Qt::transparent);
    QPainter p(&image);p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);p.setBrush(QColor("#eef0f5"));
    for(int y:{4,9,14})p.drawEllipse(QPointF(8,y),1.8,1.8);
    return QIcon(image);
}

class MuteOverlay final : public QWidget {
    double progress=0.0;
public:
    MuteOverlay(bool,QWidget *parent):QWidget(parent)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setObjectName("gmMuteOverlay");
    }
    void setProgress(double value){progress=value;setProperty("progress",value);setVisible(value>0.001);update();}
protected:
    void paintEvent(QPaintEvent *)override
    {
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing);p.setOpacity(progress);
        p.fillRect(rect(),QColor(54,56,63,105));
    }
};

class VolumeSlider final : public QSlider {
public:
    VolumeSlider(Qt::Orientation orientation,QWidget *parent):QSlider(orientation,parent){}
protected:
    void wheelEvent(QWheelEvent *event)override{event->ignore();}
};

class Channel final : public QWidget {
    obs_source_t *source;
    obs_fader_t *fader = nullptr;
    QSlider *volume;
    QLabel *value;
    QLabel *scope;
    DragLabel *name;
    QToolButton *mute;
    QToolButton *monitor;
    Meter *meter;
    QTimer timer;
    QTimer saveAudio;
    MuteOverlay *muteOverlay=nullptr;
    QVariantAnimation muteAnimation;
    bool muteKnown=false;
    bool lastMuted=false;
public:
    Channel(Mixer *owner, obs_source_t *s, const QString &id, bool row,
            const Model &model, QWidget *parent) : QWidget(parent), source(obs_source_get_ref(s))
    {
        setObjectName(row?"gmRow":"gmStrip");
        setProperty("sourceUuid",id);
        QVBoxLayout *layout=nullptr;
        QGridLayout *rowLayout=nullptr;
        if(row){
            rowLayout=new QGridLayout(this);rowLayout->setContentsMargins(7,5,7,6);
            rowLayout->setHorizontalSpacing(6);rowLayout->setVerticalSpacing(4);rowLayout->setColumnStretch(0,1);
            setMinimumWidth(0);setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
        }else{
            layout=new QVBoxLayout(this);layout->setContentsMargins(5,0,5,5);layout->setSpacing(4);
            setFixedWidth(101);setMinimumHeight(390);
        }
        scope = new QLabel(this); scope->setObjectName("gmScope");
        scope->setAlignment(Qt::AlignCenter);
        if (!row) layout->addWidget(scope); else scope->hide();
        auto *head = new QWidget(this);
        auto *headLayout = new QHBoxLayout(head); headLayout->setContentsMargins(0,0,0,0); headLayout->setSpacing(1);
        name = new DragLabel(id,head); name->setMinimumWidth(0);
        name->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
        name->setObjectName("gmSourceName");
        auto *menu = new QToolButton(head); menu->setIcon(settingsIcon());menu->setObjectName("gmSettings");
        menu->setFixedWidth(19);
        menu->setToolTip(ui("Channel.SettingsTip","Channel settings, filters and movement")); menu->setAccessibleName(ui("Channel.Menu","Channel menu"));
        headLayout->addWidget(name,1); headLayout->addWidget(menu);
        head->setMinimumWidth(0);head->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
        if(row)rowLayout->addWidget(head,0,0);else layout->addWidget(head);
        value = new QLabel(this); value->setObjectName("gmDb");
        if(row){value->setFixedWidth(55);value->setAlignment(Qt::AlignRight|Qt::AlignVCenter);rowLayout->addWidget(value,0,1);}
        else layout->addWidget(value);
        auto *faderPanel = new QWidget(this);
        auto *faderLayout = new QBoxLayout(row?QBoxLayout::TopToBottom:QBoxLayout::LeftToRight,faderPanel);
        faderPanel->setObjectName("gmFaderPanel");faderPanel->setMinimumWidth(0);
        faderLayout->setContentsMargins(0,0,0,0); faderLayout->setSpacing(row?4:5);
        volume = new VolumeSlider(row?Qt::Horizontal:Qt::Vertical,faderPanel);
        volume->setRange(0,1000); volume->setSingleStep(5); volume->setPageStep(25);
        volume->setAccessibleName(ui("Channel.Volume","Volume")); volume->setToolTip(ui("Channel.VolumeTip","OBS source volume"));
        volume->setProperty("mixerVolume",true);
        if (!row) volume->setFixedWidth(24);
        meter = new Meter(source,row,faderPanel,owner->property("meterDecayRate").toDouble(),owner->property("truePeak").toBool());
        if (row) {
            volume->setFixedHeight(18);volume->setMinimumWidth(0);
            faderPanel->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
            faderLayout->addWidget(meter);faderLayout->addWidget(volume);
        }
        else { faderLayout->addWidget(volume); faderLayout->addWidget(meter); }
        if(row)rowLayout->addWidget(faderPanel,1,0,1,3);else layout->addWidget(faderPanel,1);
        auto *buttons=new QWidget(this); auto *buttonLayout=new QHBoxLayout(buttons);
        buttonLayout->setContentsMargins(0,0,0,0); buttonLayout->setSpacing(3);
        mute=new QToolButton(buttons); mute->setCheckable(true); mute->setToolTip(ui("Channel.Mute","Mute source"));
        mute->setAccessibleName(ui("Channel.Mute","Mute source"));
        monitor=new QToolButton(buttons); monitor->setCheckable(true); monitor->setObjectName("gmMonitor");
        monitor->setToolTip(ui("Channel.Monitor","Monitor source")); monitor->setAccessibleName(ui("Channel.Monitor","Monitor source"));
        buttonLayout->addWidget(mute); buttonLayout->addWidget(monitor);
        if(row)rowLayout->addWidget(buttons,0,2);else layout->addWidget(buttons);
        auto *move=new QComboBox(this); move->setObjectName("gmMove");
        move->addItem(ui("Channel.Move","Move…"),QVariant());
        for (const auto &g:model.groups) move->addItem(g.name,g.id);
        move->addItem(ui("Channel.Ungrouped","↗ Ungrouped"),QString());
        move->setToolTip(ui("Channel.MoveTip","Move channel into another group or out of its group"));
        move->setAccessibleName(ui("Channel.MoveLabel","Move channel"));
        if(row)move->hide();else layout->addWidget(move);
        muteOverlay=new MuteOverlay(row,this);muteOverlay->hide();
        muteAnimation.setDuration(180);muteAnimation.setEasingCurve(QEasingCurve::InOutCubic);
        connect(&muteAnimation,&QVariantAnimation::valueChanged,this,[this](const QVariant &v){muteOverlay->setProgress(v.toDouble());});
        fader=obs_fader_create(OBS_FADER_LOG);
        if (fader) obs_fader_attach_source(fader,source);
        saveAudio.setSingleShot(true);
        connect(&saveAudio,&QTimer::timeout,this,[] { obs_frontend_save(); });
        sync();
        connect(volume,&QSlider::valueChanged,this,[this](int v) {
            if (fader) obs_fader_set_deflection(fader,float(v)/1000.0f);
            saveAudio.start(800);
            sync();
        });
        connect(mute,&QToolButton::clicked,this,[this](bool v) { obs_source_set_muted(source,v); sync(); obs_frontend_save(); });
        connect(monitor,&QToolButton::clicked,this,[this](bool v) { obs_source_set_monitoring_enabled(source,v); sync(); obs_frontend_save(); });
        connect(move,&QComboBox::activated,this,[owner,move,id](int i) {
            if (i>0) owner->moveSource(id,move->itemData(i).toString());
        });
        connect(menu,&QToolButton::clicked,this,[owner,menu,id] {
            owner->showSourceMenu(id,menu,menu->mapToGlobal(QPoint(0,menu->height())));
        });
        name->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(name,&QWidget::customContextMenuRequested,this,[owner,name=name,id](const QPoint &p){
            owner->showSourceMenu(id,name,name->mapToGlobal(p));
        });
        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this,&QWidget::customContextMenuRequested,this,[owner,this,id](const QPoint &p) {
            owner->showSourceMenu(id,this,mapToGlobal(p));
        });
        connect(&timer,&QTimer::timeout,this,[this] { sync(); }); timer.start(100);
    }
    ~Channel() override
    {
        timer.stop();
        saveAudio.stop();
        delete meter; // Disconnect the audio callback before releasing our source.
        if (fader) { obs_fader_detach_source(fader); obs_fader_destroy(fader); }
        obs_source_release(source);
    }
private:
    void resizeEvent(QResizeEvent *event)override
    {
        QWidget::resizeEvent(event);
        if(muteOverlay){muteOverlay->setGeometry(rect());muteOverlay->raise();}
    }
    void sync()
    {
        const QString currentName=text(obs_source_get_name(source));
        name->setText(name->fontMetrics().elidedText(currentName,Qt::ElideRight,std::max(10,name->width())));
        name->setAccessibleName(currentName);
        name->setToolTip(currentName+"\n"+ui("Drag.NameTip","Drag into a group or into Ungrouped."));
        const float db=fader?obs_fader_get_db(fader):-INFINITY;
        value->setText(std::isfinite(db)?QString::number(db,'f',1)+" dB":"−∞ dB");
        { QSignalBlocker block(volume);
          if (!volume->isSliderDown() && fader)
              volume->setValue(qRound(obs_fader_get_deflection(fader)*1000.0f)); }
        const bool muted=obs_source_muted(source), listening=obs_source_get_monitoring_enabled(source);
        if(!muteKnown || muted!=lastMuted){
            const bool initial=!muteKnown;muteKnown=true;lastMuted=muted;
            setProperty("muted",muted);
            volume->setProperty("muted",muted);name->setProperty("muted",muted);scope->setProperty("muted",muted);
            for(QWidget *w:{static_cast<QWidget *>(volume),static_cast<QWidget *>(name),static_cast<QWidget *>(scope)}){w->style()->unpolish(w);w->style()->polish(w);}
            muteAnimation.stop();
            if(initial)muteOverlay->setProgress(muted?1.0:0.0);
            else{
                muteAnimation.setStartValue(muteOverlay->property("progress").toDouble());
                muteAnimation.setEndValue(muted?1.0:0.0);muteAnimation.start();
            }
            muteOverlay->raise();
        }
        { QSignalBlocker block(mute); mute->setChecked(muted); }
        { QSignalBlocker block(monitor); monitor->setChecked(listening); }
        mute->setIcon(volumeIcon(this,muted));
        monitor->setIcon(headphoneIcon(listening)); meter->setMuted(muted);
        bool global=false;
        for (uint32_t channel=0; channel<6; ++channel) {
            obs_source_t *s=obs_get_output_source(channel);
            global=global || s==source;
            if (s) obs_source_release(s);
        }
        scope->setText(global?ui("Scope.Global","Global"):obs_source_active(source)?ui("Scope.Active","Active"):ui("Scope.Inactive","Inactive"));
        volume->setAccessibleDescription(currentName+": "+value->text());
    }
};

Mixer::Mixer(QWidget *parent) : QWidget(parent)
{
    setObjectName("groupMixer");
    setProperty("meterDecayRate",23.53);setProperty("truePeak",false);
    setStyleSheet(R"(
      QWidget#groupMixer { background:#262932; color:#f0f0f2; }
      QWidget#groupMixer QLabel { color:#eceef3; }
      QWidget#groupMixer QToolButton { border:0; padding:3px; background:transparent; min-width:17px; }
      QWidget#groupMixer QToolButton:hover { background:#3d4659; }
      QWidget#groupMixer QToolButton::menu-indicator { image:none; width:0px; height:0px; }
      QWidget#groupMixer QToolButton:checked { background:#863743; border-radius:3px; }
      QWidget#groupMixer QToolButton#gmMonitor:checked { background:#198b29; }
      QWidget#groupMixer QLabel#gmScope { background:#25386e; color:#8ba9ff; font-size:11px; }
      QWidget#groupMixer QLabel#gmDb { color:#a8acb5; font-size:11px; }
      QWidget#groupMixer QLabel#gmSourceName[muted=true] { color:#92959e; }
      QWidget#groupMixer QLabel#gmScope[muted=true] { color:#a3a5ae; background:#3a3d47; }
      QWidget#groupMixer QFrame#gmGroup { border:1px solid #414653; background:#262932; }
      QWidget#groupMixer QFrame#gmChannel { border-right:1px solid #3b414e; }
      QWidget#groupMixer QFrame[dropTarget=true] { border:2px solid #8daaff; background:#354563; }
      QWidget#groupMixer QWidget#gmGroupHeader { background:#30343e; }
      QWidget#groupMixer QWidget#gmRow { border:1px solid #434958; border-radius:3px; background:#2c303a; }
      QWidget#groupMixer QComboBox#gmMove { font-size:10px; padding:2px; min-height:18px; }
      QWidget#groupMixer QSlider[mixerVolume=true]::groove:vertical { background:#525762; width:4px; }
      QWidget#groupMixer QSlider[mixerVolume=true]::sub-page:vertical { background:#525762; width:4px; }
      QWidget#groupMixer QSlider[mixerVolume=true]::add-page:vertical { background:#5074d7; width:4px; }
      QWidget#groupMixer QSlider[mixerVolume=true]::handle:vertical { background:#f4f4f5; height:20px; margin:0 -3px; border-radius:4px; }
      QWidget#groupMixer QSlider[mixerVolume=true]::groove:horizontal { background:#525762; height:4px; }
      QWidget#groupMixer QSlider[mixerVolume=true]::sub-page:horizontal { background:#5074d7; height:4px; }
      QWidget#groupMixer QSlider[mixerVolume=true]::add-page:horizontal { background:#525762; height:4px; }
      QWidget#groupMixer QSlider[mixerVolume=true]::handle:horizontal { background:#f4f4f5; width:20px; margin:-3px 0; border-radius:4px; }
      QWidget#groupMixer QSlider[muted=true]::add-page:vertical { background:#727783; }
      QWidget#groupMixer QSlider[muted=true]::sub-page:horizontal { background:#727783; }
    )");
    auto *layout=new QVBoxLayout(this); layout->setContentsMargins(0,0,0,0); layout->setSpacing(3);
    scroll=new QScrollArea(this); scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
    scroll->setObjectName("gmScroll");scroll->setMinimumWidth(0);scroll->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    layout->addWidget(scroll,1);
    auto *bar=new QWidget(this);bar->setObjectName("gmFooter");auto *barLayout=new QHBoxLayout(bar);
    barLayout->setContentsMargins(4,2,4,2); barLayout->setSpacing(5);
    auto *options=new QToolButton(bar);options->setIcon(settingsIcon());options->setFixedSize(28,28);
    options->setObjectName("gmGlobalSettings");options->setAccessibleName(ui("Mixer.Settings","Mixer settings"));options->setToolTip(ui("Mixer.Settings","Mixer settings"));
    auto *optionsMenu=new QMenu(options);
    auto *add=optionsMenu->addAction(ui("Group.NewAction","＋ New group…"));
    auto *layoutMenu=optionsMenu->addMenu(ui("Layout.Menu","Layout"));
    stripsAction=layoutMenu->addAction(ui("Layout.Strips","Vertical channels"));stripsAction->setObjectName("gmLayoutStrips");stripsAction->setCheckable(true);
    rowsAction=layoutMenu->addAction(ui("Layout.Rows","Compact rows"));rowsAction->setObjectName("gmLayoutRows");rowsAction->setCheckable(true);
    auto *layoutActions=new QActionGroup(this);layoutActions->addAction(stripsAction);layoutActions->addAction(rowsAction);layoutActions->setExclusive(true);
    stripsAction->setChecked(true);
    connect(stripsAction,&QAction::triggered,this,[this]{model.horizontal=false;changed();});
    connect(rowsAction,&QAction::triggered,this,[this]{model.horizontal=true;changed();});
    optionsMenu->addSeparator();
    auto *active=optionsMenu->addAction(ui("Mixer.ActiveOnly","Active audio sources only")); active->setCheckable(true);
    auto *expand=optionsMenu->addAction(ui("Group.ExpandAll","Expand all groups"));
    auto *collapse=optionsMenu->addAction(ui("Group.CollapseAll","Collapse all groups"));
    hiddenButton=new QToolButton(bar);hiddenButton->setObjectName("gmHiddenSources");
    hiddenButton->setText(ui("Hidden.Label","Hidden (%1)").arg(0));hiddenButton->setToolTip(ui("Hidden.Tip","Restore hidden channels. Hiding does not change audio."));
    auto *hiddenMenu=new QMenu(hiddenButton);hiddenButton->setMenu(hiddenMenu);hiddenButton->setPopupMode(QToolButton::InstantPopup);
    connect(hiddenMenu,&QMenu::aboutToShow,this,[this,hiddenMenu]{
        hiddenMenu->clear();
        if(model.hidden.isEmpty()){hiddenMenu->addAction(ui("Hidden.None","No hidden channels"))->setEnabled(false);return;}
        auto *all=hiddenMenu->addAction(ui("Hidden.ShowAll","Show all hidden channels"));
        connect(all,&QAction::triggered,this,[this]{showHiddenSources();});
        hiddenMenu->addSeparator();
        QStringList ids=model.hidden.values();
        const auto displayName=[this](const QString &id){auto *s=sources.value(id);return s?text(obs_source_get_name(s)):id;};
        std::sort(ids.begin(),ids.end(),[&](const QString &a,const QString &b){return displayName(a)<displayName(b);});
        for(const auto &id:ids){
            auto *s=sources.value(id);auto *action=hiddenMenu->addAction(s?text(obs_source_get_name(s)):ui("Source.Unavailable","Unavailable source · %1").arg(id));
            connect(action,&QAction::triggered,this,[this,id]{model.hidden.remove(id);changed();});
        }
    });
    options->setMenu(optionsMenu); options->setPopupMode(QToolButton::InstantPopup);
    connect(optionsMenu,&QMenu::aboutToShow,this,[this,active] {
        active->setChecked(model.activeOnly);stripsAction->setChecked(!model.horizontal);rowsAction->setChecked(model.horizontal);
    });
    connect(active,&QAction::triggered,this,[this](bool v) { model.activeOnly=v; changed(); refresh(); });
    connect(expand,&QAction::triggered,this,[this] { for(auto &g:model.groups)g.collapsed=false; changed(); });
    connect(collapse,&QAction::triggered,this,[this] { for(auto &g:model.groups)g.collapsed=true; changed(); });
    barLayout->addWidget(options);barLayout->addWidget(hiddenButton);barLayout->addStretch();
    layout->addWidget(bar);
    connect(add,&QAction::triggered,this,[this] { newGroup(); });
    discovery=new QTimer(this); connect(discovery,&QTimer::timeout,this,[this] { refresh(); });
    discovery->start(750);
    QTimer::singleShot(0,this,[this] { refresh(); });
}

Mixer::~Mixer() { shutdown(); }
void Mixer::beginCollectionChange()
{
    ++collectionRevision;
    switching=true;
    delete scroll->takeWidget();
    for(auto *source:sources)obs_source_release(source);
    sources.clear();available.clear();
}
void Mixer::finishCollectionChange()
{
    switching=false;
    refresh();
    requestRebuild();
}
void Mixer::shutdown()
{
    if (stopped) return;
    stopped=true; discovery->stop();
    delete scroll->takeWidget();
    for (auto *source:sources) obs_source_release(source);
    sources.clear(); available.clear();
}

void Mixer::load(obs_data_t *data)
{
    ++collectionRevision;
    const auto json=text(obs_data_get_string(data,"pecislav_group_mixer"));
    model=Model::fromJson(QJsonDocument::fromJson(json.toUtf8()).object());
    stripsAction->setChecked(!model.horizontal);rowsAction->setChecked(model.horizontal);
    requestRebuild();
}
void Mixer::save(obs_data_t *data) const
{
    const auto json=QJsonDocument(model.toJson()).toJson(QJsonDocument::Compact);
    obs_data_set_string(data,"pecislav_group_mixer",json.constData());
}
void Mixer::changed()
{
    requestRebuild();
    obs_frontend_save();
}
void Mixer::requestRebuild()
{
    if (pending || stopped) return;
    pending=true;
    QTimer::singleShot(0,this,[this] {
        pending=false;
        if (stopped || switching) return;
        if (activeDrags || QApplication::activeModalWidget() || QApplication::activePopupWidget()) {
            QTimer::singleShot(50,this,[this] { requestRebuild(); });
            return;
        }
        rebuild();
    });
}

void Mixer::refresh()
{
    if (stopped || switching || activeDrags || QApplication::activeModalWidget() || QApplication::activePopupWidget()) return;
    bool meterChanged=false;
    if(parentWidget()){
        if(auto *config=obs_frontend_get_profile_config()){
            double rate=config_get_double(config,"Audio","MeterDecayRate");if(rate<=0)rate=23.53;
            const bool truePeak=config_get_uint(config,"Audio","PeakMeterType")==1;
            meterChanged=property("meterDecayRate").toDouble()!=rate || property("truePeak").toBool()!=truePeak;
            setProperty("meterDecayRate",rate);setProperty("truePeak",truePeak);
        }
    }
    QHash<QString,obs_source_t *> next;
    obs_enum_sources([](void *param,obs_source_t *source) {
        if ((obs_source_get_output_flags(source)&OBS_SOURCE_AUDIO)==0 ||
             obs_source_get_type(source)!=OBS_SOURCE_TYPE_INPUT || obs_source_removed(source)) return true;
        const auto uuid=text(obs_source_get_uuid(source));
        if (!uuid.isEmpty()) static_cast<QHash<QString,obs_source_t *> *>(param)->insert(uuid,obs_source_get_ref(source));
        return true;
    },&next);
    // Global audio can be absent from the normal user-source enumeration.
    for (uint32_t i=0;i<6;++i) {
        auto *s=obs_get_output_source(i);
        if (!s) continue;
        const auto id=text(obs_source_get_uuid(s));
        if ((obs_source_get_output_flags(s)&OBS_SOURCE_AUDIO) && obs_source_get_type(s)==OBS_SOURCE_TYPE_INPUT &&
            !obs_source_removed(s) && !id.isEmpty() && !next.contains(id)) next.insert(id,s);
        else obs_source_release(s);
    }
    QStringList visible;
    for (auto it=next.cbegin();it!=next.cend();++it)
        if (!model.activeOnly || obs_source_active(it.value())) visible.append(it.key());
    std::sort(visible.begin(),visible.end(),[&](const QString &a,const QString &b) {
        return QString::localeAwareCompare(text(obs_source_get_name(next[a])),text(obs_source_get_name(next[b])))<0;
    });
    bool different=visible!=available || next.size()!=sources.size();
    if (!different) for(auto it=next.cbegin();it!=next.cend();++it)
        if (sources.value(it.key())!=it.value()) { different=true; break; }
    // Channel widgets own their own references, so exchanging this registry is safe.
    for(auto *s:sources)obs_source_release(s);
    sources=std::move(next); available=visible;
    if(different || meterChanged || !scroll->widget()) requestRebuild();
}

void Mixer::rebuild()
{
    hiddenButton->setText(ui("Hidden.Label","Hidden (%1)").arg(model.hidden.size()));
    scroll->setHorizontalScrollBarPolicy(model.horizontal?Qt::ScrollBarAlwaysOff:Qt::ScrollBarAsNeeded);
    auto *content=new QWidget;
    auto *layout=new QBoxLayout(model.horizontal?QBoxLayout::TopToBottom:QBoxLayout::LeftToRight,content);
    layout->setContentsMargins(model.horizontal?6:0,model.horizontal?6:0,0,0);
    layout->setSpacing(model.horizontal?7:0);
    for(const auto &g:model.groups)layout->addWidget(makeGroup(g.id,g.name,g.collapsed));
    layout->addWidget(makeGroup({},ui("Group.Ungrouped","Ungrouped"),false),model.horizontal?0:1);
    if(model.horizontal)layout->addStretch();
    QWidget *old=scroll->takeWidget();
    scroll->setWidget(content);
    delete old;
}

QWidget *Mixer::makeGroup(const QString &id,const QString &name,bool collapsed)
{
    QStringList shown=available;shown.removeIf([this](const QString &source){return model.hidden.contains(source);});
    const auto members=model.members(id,shown);
    const auto allMembers=model.members(id,sources.keys());
    auto *group=new DropFrame;
    group->setObjectName("gmGroup");
    group->setProperty("groupId",id);
    group->dropped=[this,id](const QString &source) { moveSource(source,id); };
    group->groupDropped=[this,id](const QString &moving){moveGroup(moving,id);};
    auto *layout=new QBoxLayout(model.horizontal?QBoxLayout::TopToBottom:QBoxLayout::LeftToRight,group);
    layout->setContentsMargins(0,0,0,0); layout->setSpacing(0);
    auto *header=new QWidget(group); header->setObjectName("gmGroupHeader");
    auto *head=new QBoxLayout(model.horizontal?QBoxLayout::LeftToRight:QBoxLayout::TopToBottom,header);
    head->setContentsMargins(0,0,0,0); head->setSpacing(1);
    QPushButton *fold;
    if (model.horizontal) {
        fold=new RailButton((id.isEmpty()?"":collapsed?"▸ ":"▾ ")+name+"  ("+QString::number(members.size())+")",header,id,true);
        fold->setFlat(true);fold->setMinimumWidth(0);fold->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);head->addWidget(fold,1);
    } else {
        auto *rail=new RailButton(name,header,id); rail->collapsed=collapsed; rail->free=id.isEmpty();
        fold=rail; head->addWidget(fold,1);
        auto *count=new QLabel(QString::number(members.size()),header); count->setAlignment(Qt::AlignCenter);
        head->addWidget(count);
    }
    if (!id.isEmpty()) {
        fold->setObjectName("gmGroupHandle");fold->setProperty("groupId",id);
        fold->setAccessibleName((collapsed?ui("Group.Expand","Expand %1"):ui("Group.Collapse","Collapse %1")).arg(name));
        fold->setToolTip(name+"\n"+ui("Drag.GroupTip","Drag to reorder groups. Click to expand/collapse. Right-click for more options."));
        connect(fold,&QPushButton::clicked,this,[this,id] {
            for(auto &g:model.groups)if(g.id==id)g.collapsed=!g.collapsed;
            changed();
        });
        fold->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(fold,&QWidget::customContextMenuRequested,this,[this,fold,id](const QPoint &p) { groupMenu(id,fold->mapToGlobal(p)); });
    } else fold->setToolTip(ui("Group.UngroupedTip","Channels outside groups. You can drag them here."));
    auto *mute=new QToolButton(header); mute->setIcon(volumeIcon(this,false));
    mute->setToolTip(ui("Group.MuteTip","Mute / unmute all channels in this group"));
    mute->setAccessibleName(ui("Group.Mute","Mute group %1").arg(name)); mute->setEnabled(!allMembers.isEmpty());
    head->addWidget(mute);
    connect(mute,&QToolButton::clicked,this,[this,allMembers] {
        bool allMuted=true;
        for(const auto &uuid:allMembers)if(auto *s=sources.value(uuid))allMuted=allMuted&&obs_source_muted(s);
        for(const auto &uuid:allMembers)if(auto *s=sources.value(uuid))obs_source_set_muted(s,!allMuted);
        obs_frontend_save();
    });
    layout->addWidget(header);
    if (!collapsed) {
        auto *channels=new QWidget(group);
        auto *channelLayout=new QBoxLayout(model.horizontal?QBoxLayout::TopToBottom:QBoxLayout::LeftToRight,channels);
        channelLayout->setContentsMargins(model.horizontal?6:0,model.horizontal?6:0,model.horizontal?6:0,model.horizontal?6:0);
        channelLayout->setSpacing(model.horizontal?5:0);
        for(const auto &uuid:members) {
            auto *s=sources.value(uuid);
            if (!s) continue;
            auto *target=new DropFrame(channels); target->setObjectName("gmChannel");
            if(!model.horizontal)target->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Expanding);
            target->dropped=[this,id,uuid](const QString &source) { moveSource(source,id,uuid); };
            auto *inner=new QVBoxLayout(target); inner->setContentsMargins(0,0,0,0);
            inner->addWidget(new Channel(this,s,uuid,model.horizontal,model,target));
            channelLayout->addWidget(target);
        }
        if(members.isEmpty()) {
            auto *empty=new QLabel(id.isEmpty()?ui("Group.NoChannels","No channels"):ui("Group.Empty","Empty group"),channels);
            empty->setObjectName("gmEmptyGroup");
            empty->setToolTip(id.isEmpty()?ui("Group.EmptyUngroupedTip","Visible channels not assigned to any group appear here."):ui("Group.EmptyTip","Drag a channel here by its name or use the Move menu."));
            empty->setWordWrap(true); empty->setAlignment(Qt::AlignCenter); empty->setMargin(9);
            if(!model.horizontal){empty->setMinimumWidth(96);empty->setMinimumHeight(390);}
            else empty->setMinimumHeight(36);
            channelLayout->addWidget(empty);
        }
        else if(!model.horizontal && id.isEmpty())channelLayout->addStretch();
        layout->addWidget(channels,id.isEmpty()?1:0);
    }
    if(!model.horizontal){group->setMinimumHeight(398);group->setSizePolicy(id.isEmpty()?QSizePolicy::Expanding:QSizePolicy::Fixed,QSizePolicy::Expanding);}
    else{group->setMinimumWidth(0);group->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);}
    return group;
}

void Mixer::moveSource(const QString &source,const QString &group,const QString &before)
{
    if(!sources.contains(source))return;
    if(model.move(source,group,before))changed();
}

void Mixer::moveGroup(const QString &group,const QString &before)
{
    if(model.moveGroup(group,before))changed();
}
void Mixer::hideSource(const QString &source)
{
    if(!sources.contains(source))return;
    model.hidden.insert(source);changed();
}
void Mixer::showHiddenSources()
{
    if(model.hidden.isEmpty())return;
    model.hidden.clear();changed();
}

void Mixer::newGroup()
{
    const auto revision=collectionRevision;
    QPointer<Mixer> guard(this);
    bool ok=false;
    const auto name=askText(this,ui("Group.New","New group"),ui("Group.NameLabel","Group name:"),{},&ok);
    if(ok&&guard&&!stopped&&revision==collectionRevision&&!model.addGroup(name).isEmpty())changed();
}

void Mixer::groupMenu(const QString &id,const QPoint &position)
{
    const auto revision=collectionRevision;
    QPointer<Mixer> guard(this);
    QMenu menu;
    menu.setObjectName("gmGroupMenu");
    if(testAttribute(Qt::WA_DontShowOnScreen))menu.setAttribute(Qt::WA_DontShowOnScreen);
    const auto group=std::find_if(model.groups.cbegin(),model.groups.cend(),[&](const Group &g){return g.id==id;});
    if(group==model.groups.cend())return;
    const QString currentName=group->name;
    menu.addSection(currentName);
    auto *rename=menu.addAction(ui("Group.RenameAction","Rename group…"));
    const int index=int(group-model.groups.cbegin());
    auto *earlier=menu.addAction(model.horizontal?ui("Move.Up","Move up"):ui("Move.Left","Move left"));
    auto *later=menu.addAction(model.horizontal?ui("Move.Down","Move down"):ui("Move.Right","Move right"));
    earlier->setEnabled(index>0);later->setEnabled(index+1<model.groups.size());
    menu.addSeparator();
    auto *remove=menu.addAction(ui("Group.Remove","Remove group (keep channels)"));
    const auto *action=menu.exec(position);
    if(!guard||stopped||revision!=collectionRevision)return;
    if(action==rename){
        bool ok=false;
        const auto name=askText(this,ui("Group.Rename","Rename group"),ui("Common.Name","Name:"),currentName,&ok).trimmed();
        if(ok&&guard&&!stopped&&revision==collectionRevision&&!name.isEmpty()){for(auto &g:model.groups)if(g.id==id)g.name=name.left(80);changed();}
    } else if(action==earlier && index>0){moveGroup(id,model.groups[index-1].id);}
    else if(action==later && index+1<model.groups.size()){moveGroup(model.groups[index+1].id,id);}
    else if(action==remove){model.removeGroup(id);changed();}
}

void Mixer::showSourceMenu(const QString &id,QWidget *anchor,const QPoint &position)
{
    Q_UNUSED(anchor);
    auto *source=sources.value(id);if(!source)return;
    source=obs_source_get_ref(source);if(!source)return;
    const auto releaseSource=[](obs_source_t *s){obs_source_release(s);};
    std::unique_ptr<obs_source_t,decltype(releaseSource)> sourceLifetime(source,releaseSource);
    const auto revision=collectionRevision;
    QPointer<Mixer> guard(this);
    QMenu menu;
    if(testAttribute(Qt::WA_DontShowOnScreen))menu.setAttribute(Qt::WA_DontShowOnScreen);
    menu.setObjectName("gmSourceMenu");
    menu.addSection(text(obs_source_get_name(source)));
    auto *move=menu.addMenu(ui("Channel.MoveGroup","Move to group"));
    for(const auto &g:model.groups){
        auto *a=move->addAction(g.name); a->setEnabled(model.membership.value(id)!=g.id);
        connect(a,&QAction::triggered,this,[this,id,target=g.id]{moveSource(id,target);});
    }
    auto *extract=menu.addAction(ui("Channel.Extract","↗ Remove from group"));extract->setEnabled(!model.membership.value(id).isEmpty());
    QStringList shown=available;shown.removeIf([this](const QString &sourceId){return model.hidden.contains(sourceId);});
    const auto members=model.members(model.membership.value(id),shown);
    const int index=members.indexOf(id);
    auto *earlier=menu.addAction(model.horizontal?ui("Move.Up","Move up"):ui("Move.Left","Move left"));
    auto *later=menu.addAction(model.horizontal?ui("Move.Down","Move down"):ui("Move.Right","Move right"));
    earlier->setEnabled(index>0);later->setEnabled(index>=0&&index+1<members.size());
    auto *hide=menu.addAction(ui("Channel.Hide","Hide channel (keep audio enabled)"));
    menu.addSeparator();
    auto *level=menu.addAction(ui("Channel.SetDb","Set volume in dB…"));
    auto *filters=menu.addAction(ui("Channel.Filters","Filters…"));
    auto *properties=menu.addAction(ui("Channel.Properties","Properties…"));
    auto *advancedAction=menu.addAction(ui("Channel.Advanced","Advanced audio settings…"));
    const auto *action=menu.exec(position);
    if(!guard||stopped||revision!=collectionRevision||obs_source_removed(source))return;
    if(action==extract)moveSource(id,{});
    else if(action==hide)hideSource(id);
    else if(action==earlier||action==later){
        const int other=index+(action==earlier?-1:1);
        if(other>=0&&other<members.size()){
            if(action==earlier)moveSource(id,model.membership.value(id),members[other]);
            else {
                // Put the next channel before this one, preserving the relative order of all others.
                moveSource(members[other],model.membership.value(id),id);
            }
        }
    } else if(action==filters)obs_frontend_open_source_filters(source);
    else if(action==properties)obs_frontend_open_source_properties(source);
    else if(action==advancedAction)advanced(source);
    else if(action==level){
        const float mul=obs_source_get_volume(source);
        bool ok=false;
        QInputDialog dialog(this);dialog.setWindowTitle(ui("Channel.Volume","Volume"));dialog.setLabelText(ui("Channel.DbLabel","Volume (dB):"));
        dialog.setInputMode(QInputDialog::DoubleInput);dialog.setDoubleRange(-96.0,0.0);dialog.setDoubleDecimals(1);
        dialog.setDoubleValue(mul>0?20.0*std::log10(mul):-96.0);
        dialog.setOkButtonText(ui("Common.Ok","OK"));dialog.setCancelButtonText(ui("Common.Cancel","Cancel"));
        ok=dialog.exec()==QDialog::Accepted;
        const double db=dialog.doubleValue();
        if(ok&&guard&&!stopped&&revision==collectionRevision&&!obs_source_removed(source)){obs_source_set_volume(source,float(std::pow(10.0,db/20.0)));obs_frontend_save();}
    }
}

void Mixer::advanced(obs_source_t *source)
{
    const auto revision=collectionRevision;
    QPointer<Mixer> guard(this);
    QDialog dialog(this); dialog.setWindowTitle(ui("Advanced.Title","Audio · %1").arg(text(obs_source_get_name(source))));
    auto *layout=new QVBoxLayout(&dialog); auto *form=new QFormLayout;
    auto *sync=new QSpinBox(&dialog);sync->setRange(-950,20000);sync->setSuffix(" ms");
    sync->setValue(int(obs_source_get_sync_offset(source)/1000000));
    auto *balance=new QSlider(Qt::Horizontal,&dialog);balance->setRange(-100,100);
    balance->setValue(qRound(obs_source_get_balance_value(source)*200.0f-100.0f));
    auto *monitor=new QCheckBox(ui("Advanced.Monitor","Enable audio monitoring"),&dialog);monitor->setChecked(obs_source_get_monitoring_enabled(source));
    auto *mono=new QCheckBox(ui("Advanced.Mono","Downmix to mono"),&dialog);mono->setChecked((obs_source_get_flags(source)&OBS_SOURCE_FLAG_FORCE_MONO)!=0);
    form->addRow(ui("Advanced.Sync","Sync offset:"),sync);form->addRow(ui("Advanced.Balance","Balance L / R:"),balance);
    form->addRow(monitor);form->addRow(mono);
    auto *tracks=new QWidget(&dialog);auto *trackLayout=new QHBoxLayout(tracks);trackLayout->setContentsMargins(0,0,0,0);
    std::array<QCheckBox *,6> checks;
    const uint32_t mask=obs_source_get_audio_mixers(source);
    for(int i=0;i<6;++i){checks[i]=new QCheckBox(QString::number(i+1),tracks);checks[i]->setChecked((mask&(1u<<i))!=0);trackLayout->addWidget(checks[i]);}
    form->addRow(ui("Advanced.Tracks","Audio tracks:"),tracks);layout->addLayout(form);
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(ui("Common.Ok","OK"));
    buttons->button(QDialogButtonBox::Cancel)->setText(ui("Common.Cancel","Cancel"));
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);layout->addWidget(buttons);
    if(dialog.exec()!=QDialog::Accepted||!guard||stopped||revision!=collectionRevision||obs_source_removed(source))return;
    obs_source_set_sync_offset(source,int64_t(sync->value())*1000000);
    obs_source_set_balance_value(source,float(balance->value()+100)/200.0f);
    obs_source_set_monitoring_enabled(source,monitor->isChecked());
    const uint32_t flags=obs_source_get_flags(source);
    obs_source_set_flags(source,mono->isChecked()?flags|OBS_SOURCE_FLAG_FORCE_MONO:flags&~OBS_SOURCE_FLAG_FORCE_MONO);
    uint32_t result=0;for(int i=0;i<6;++i)if(checks[i]->isChecked())result|=1u<<i;
    obs_source_set_audio_mixers(source,result);
    obs_frontend_save();
}
}
