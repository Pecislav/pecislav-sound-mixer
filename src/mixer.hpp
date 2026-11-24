#pragma once
#include "model.hpp"
#include <QHash>
#include <QWidget>
#include <obs-module.h>

class QBoxLayout;
class QComboBox;
class QScrollArea;
class QTimer;
class QToolButton;
class QAction;

namespace gm {
class Mixer final : public QWidget {
public:
    explicit Mixer(QWidget *parent = nullptr);
    ~Mixer() override;
    void load(obs_data_t *data);
    void save(obs_data_t *data) const;
    void shutdown();
    void beginCollectionChange();
    void finishCollectionChange();
    void refresh();
    void requestRebuild();
    void moveSource(const QString &source, const QString &group, const QString &before = {});
    void moveGroup(const QString &group, const QString &before = {});
    void hideSource(const QString &source);
    void showHiddenSources();
    void showSourceMenu(const QString &id, QWidget *anchor, const QPoint &position);

private:
    Model model;
    QHash<QString, obs_source_t *> sources;
    QStringList available;
    QScrollArea *scroll = nullptr;
    QAction *stripsAction = nullptr;
    QAction *rowsAction = nullptr;
    QTimer *discovery = nullptr;
    QToolButton *hiddenButton = nullptr;
    bool pending = false;
    bool stopped = false;
    bool switching = false;
    quint64 collectionRevision = 0;
    void changed();
    void rebuild();
    void newGroup();
    QWidget *makeGroup(const QString &id, const QString &name, bool collapsed);
    void groupMenu(const QString &id, const QPoint &position);
    void advanced(obs_source_t *source);
};
}
