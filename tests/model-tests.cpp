#include "model.hpp"
#include "ballistics.hpp"
#include <QCoreApplication>
#include <QJsonArray>
#include <iostream>
#include <stdexcept>

static void check(bool ok,const char *message) { if(!ok)throw std::runtime_error(message); }
int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv);
    try {
        gm::Model m;
        auto voice=m.addGroup("Hlas"), game=m.addGroup("Hra");
        check(m.addGroup("  ").isEmpty(),"empty group name");
        m.groups[1].collapsed=true;
        check(m.moveGroup(game,voice)&&m.groups[0].id==game&&m.groups[0].collapsed,"move group preserves collapse state");
        check(m.moveGroup(game)&&m.groups.back().id==game,"move group to end");
        check(!m.moveGroup(game,game)&&!m.moveGroup(game,"unknown"),"invalid group move");
        m.groups[1].collapsed=false;
        m.order={"mic","discord","game"};
        check(m.move("mic",voice),"move into group");
        check(m.move("discord",voice,"mic"),"move before channel");
        check(m.members(voice,{"mic","discord","game"})==QStringList({"discord","mic"}),"order within group");
        check(!m.move("mic","unknown"),"reject unknown group");
        check(!m.move("mic",game,"mic"),"self drop must not move");
        m.groups[1].collapsed=true;
        check(m.move("discord",game),"move between groups");
        check(!m.groups[1].collapsed,"open target group");
        check(m.move("discord",{}),"extract channel");
        check(m.members({}, {"discord"}).contains("discord"),"ungrouped visible");
        m.move("mic",game);m.removeGroup(game);
        check(m.membership.value("mic").isEmpty(),"remove group retains channels ungrouped");
        m.horizontal=true;m.activeOnly=true;m.groups[0].collapsed=true;m.hidden.insert("mic");
        auto roundtrip=gm::Model::fromJson(m.toJson());
        check(roundtrip.toJson()==m.toJson(),"state roundtrip");
        check(roundtrip.hidden.contains("mic"),"hidden state not persisted");
        check(roundtrip.membership==m.membership,"hide changed membership");
        auto old=m.toJson();old.remove("hidden");old["version"]=1;
        check(gm::Model::fromJson(old).hidden.isEmpty(),"v1 layout migration");
        auto broken=m.toJson();broken["groups"]=QJsonArray{QJsonObject{{"id","a"},{"name","A"}},QJsonObject{{"id","a"},{"name","Duplicate"}}};
        broken["membership"]=QJsonObject{{"mic","deleted"}};
        broken["order"]=QJsonArray{"mic","mic","discord"};
        auto repaired=gm::Model::fromJson(broken);
        check(repaired.groups.size()==1&&repaired.order.count("mic")==1,"deduplicate corrupted layout");
        check(repaired.membership.value("mic").isEmpty(),"recover orphan channel");
        check(repaired.members({}, {"discord"})==QStringList({"discord"}),"removed sources hidden");
        gm::MeterEnvelope smooth;
        smooth.advance(-6,-15,.016,23.53);check(smooth.peak==-6,"peak attack");
        smooth.advance(-60,-60,.016,23.53);
        check(std::abs(smooth.peak-(-6-.016*23.53))<.0001,"OBS dB-per-second decay");
        gm::MeterEnvelope fast,slow;fast.peak=slow.peak=-6;
        for(int i=0;i<60;++i)fast.advance(-60,-60,1.0/60.0,11.76);
        for(int i=0;i<10;++i)slow.advance(-60,-60,.1,11.76);
        check(std::abs(fast.peak-slow.peak)<.0001,"meter depends on frame rate");
        fast.advance(NAN,-INFINITY,.016,11.76);check(std::isfinite(fast.peak)&&std::isfinite(fast.magnitude),"invalid meter samples");
        std::cout<<"PASS: grouping, group order, extraction, hidden-state migration/persistence, deletion, OBS-style ballistics\n";
        return 0;
    } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
