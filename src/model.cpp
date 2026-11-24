#include "model.hpp"
#include <QJsonArray>
#include <QSet>
#include <QUuid>
#include <algorithm>

namespace gm {
QString Model::addGroup(const QString &raw)
{
    const QString name = raw.trimmed().left(80);
    if (name.isEmpty()) return {};
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    groups.push_back({id, name, false});
    return id;
}

void Model::removeGroup(const QString &id)
{
    groups.erase(std::remove_if(groups.begin(), groups.end(), [&](const Group &g) {
        return g.id == id;
    }), groups.end());
    for (auto it = membership.begin(); it != membership.end(); ++it)
        if (it.value() == id) it.value().clear();
}

bool Model::moveGroup(const QString &id,const QString &before)
{
    if(id.isEmpty() || id==before)return false;
    auto it=std::find_if(groups.begin(),groups.end(),[&](const Group &g){return g.id==id;});
    if(it==groups.end())return false;
    if(!before.isEmpty() && std::none_of(groups.cbegin(),groups.cend(),[&](const Group &g){return g.id==before;}))return false;
    const Group group=*it;
    groups.erase(it);
    auto target=std::find_if(groups.begin(),groups.end(),[&](const Group &g){return g.id==before;});
    groups.insert(target,group);
    return true;
}

bool Model::move(const QString &source, const QString &group, const QString &before)
{
    if (source.isEmpty() || source == before) return false;
    if (!group.isEmpty() && std::none_of(groups.begin(), groups.end(), [&](const Group &g) {
        return g.id == group;
    })) return false;
    membership[source] = group;
    order.removeAll(source);
    const int index = order.indexOf(before);
    if (index < 0) order.append(source); else order.insert(index, source);
    for (Group &g : groups)
        if (g.id == group) g.collapsed = false;
    return true;
}

QStringList Model::members(const QString &group, const QStringList &available) const
{
    const QSet<QString> allowed(available.begin(), available.end());
    QStringList result;
    for (const QString &id : order)
        if (allowed.contains(id) && membership.value(id) == group && !result.contains(id))
            result.append(id);
    for (const QString &id : available)
        if (membership.value(id) == group && !result.contains(id)) result.append(id);
    return result;
}

QJsonObject Model::toJson() const
{
    QJsonArray gs;
    for (const Group &g : groups)
        gs.append(QJsonObject{{"id", g.id}, {"name", g.name}, {"collapsed", g.collapsed}});
    QJsonObject ms;
    for (auto it = membership.cbegin(); it != membership.cend(); ++it)
        ms.insert(it.key(), it.value());
    QStringList hiddenIds=hidden.values();hiddenIds.sort();
    return {{"version", 2}, {"horizontal", horizontal}, {"activeOnly", activeOnly},
            {"groups", gs}, {"order", QJsonArray::fromStringList(order)}, {"membership", ms},
            {"hidden",QJsonArray::fromStringList(hiddenIds)}};
}

Model Model::fromJson(const QJsonObject &json)
{
    Model m;
    m.horizontal = json.value("horizontal").toBool();
    m.activeOnly = json.value("activeOnly").toBool();
    for(const auto &v:json.value("hidden").toArray())if(!v.toString().isEmpty())m.hidden.insert(v.toString());
    QSet<QString> ids;
    for (const QJsonValue &v : json.value("groups").toArray()) {
        const auto o = v.toObject();
        const auto id = o.value("id").toString(), name = o.value("name").toString().trimmed();
        if (id.isEmpty() || name.isEmpty() || ids.contains(id)) continue;
        ids.insert(id);
        m.groups.append({id, name.left(80), o.value("collapsed").toBool()});
    }
    for (const QJsonValue &v : json.value("order").toArray()) {
        const auto id = v.toString();
        if (!id.isEmpty() && !m.order.contains(id)) m.order.append(id);
    }
    const auto memberships = json.value("membership").toObject();
    for (auto it = memberships.begin(); it != memberships.end(); ++it)
        m.membership.insert(it.key(), ids.contains(it.value().toString()) ? it.value().toString() : QString());
    return m;
}
}
