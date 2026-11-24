#pragma once
#include <QJsonObject>
#include <QHash>
#include <QStringList>
#include <QVector>
#include <QSet>

namespace gm {
struct Group {
    QString id;
    QString name;
    bool collapsed = false;
};

// Empty group ID is an ungrouped channel. Audio settings never live here.
class Model {
public:
    QVector<Group> groups;
    QStringList order;
    QHash<QString, QString> membership;
    QSet<QString> hidden;
    bool horizontal = false;
    bool activeOnly = false;

    QString addGroup(const QString &name);
    void removeGroup(const QString &id);
    bool moveGroup(const QString &id, const QString &before = {});
    bool move(const QString &source, const QString &group, const QString &before = {});
    QStringList members(const QString &group, const QStringList &available) const;
    QJsonObject toJson() const;
    static Model fromJson(const QJsonObject &json);
};
}
