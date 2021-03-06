#ifndef _StatsManager_H_
#define _StatsManager_H_

#include <fstream>

#include <crustacore/basics.h>
#include <crusta/DataManager.h>
#include <crusta/Timer.h>

#define CRUSTA_RECORD_STATS 0


namespace crusta {


class StatsManager
{
public:
    enum Stat
    {
        INHERITSHAPECOVERAGE=1,
        UPDATELINEDATA,
        PROCESSVERTICALSCALE,
        EDITLINE
    };

    ~StatsManager();

    static void newFrame();

    static void start(Stat stat);
    static void stop(Stat stat);

    static void extractTileStats(const SurfaceApproximation& surface);
    static void incrementDataUpdated();

protected:
    static const int     numTimers = 5;
    static Timer         timers[numTimers];
    static std::ofstream file;
    
    static int numTiles;
    static int numSegments;
    static int maxSegmentsPerTile;
    static int numData;
    static int numDataUpdated;
};


extern StatsManager statsMan;


} //namespace crusta


#endif //_StatsManager_H_
