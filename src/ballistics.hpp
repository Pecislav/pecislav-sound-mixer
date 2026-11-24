#pragma once
#include <algorithm>
#include <cmath>

namespace gm {
// Same peak attack / dB-per-second fall and VU integration approach as
// OBS frontend/components/VolumeMeter.cpp. Decay comes from the OBS profile.
struct MeterEnvelope {
    double peak=-60.0;
    double magnitude=-60.0;
    double held=-60.0;
    double holdAge=0.0;
    void advance(double inputPeak,double inputMagnitude,double seconds,double decayRate)
    {
        const auto bounded=[](double v){return std::isfinite(v)?std::clamp(v,-60.0,0.0):-60.0;};
        inputPeak=bounded(inputPeak);inputMagnitude=bounded(inputMagnitude);
        const double dt=std::clamp(seconds,0.0,0.25);
        peak=std::max(inputPeak,peak-std::max(0.0,decayRate)*dt);
        magnitude=bounded(magnitude+(inputMagnitude-magnitude)*std::min(1.0,dt/0.3*0.99));
        holdAge+=dt;
        if(inputPeak>=held || holdAge>=20.0){held=inputPeak;holdAge=0.0;}
    }
};
}
