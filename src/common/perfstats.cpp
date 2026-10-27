#include "perfstats.h"
#include <algorithm>

Q_LOGGING_CATEGORY(lcPerf, "ambiled.perf", QtWarningMsg)

//Constructor
PerfStats::PerfStats(QString name, int reportIntervalMs) : pName(std::move(name)), pReportIntervalMs(reportIntervalMs)
{
    pIntervalTimer.start();
}

//Record one sample, and log + reset if the report interval has elapsed
void PerfStats::addSample(qint64 nsecs)
{
    pSamples.append(nsecs);
    if (pIntervalTimer.elapsed() >= pReportIntervalMs)
        report();
}

//Count an untimed event, if lcPerf is enabled
void PerfStats::addEvent()
{
    if (lcPerf().isDebugEnabled())
        addSample(0);
}

//Log the statistics gathered over the last interval and start a new one
void PerfStats::report()
{
    qint64 intervalMs = pIntervalTimer.restart();
    if (pSamples.isEmpty())
        return;

    std::sort(pSamples.begin(), pSamples.end());
    qint64 total = 0;
    for (qint64 sample : pSamples)
        total += sample;

    int count = pSamples.size();
    double meanMs = static_cast<double>(total) / count / 1e6;
    double p95Ms = static_cast<double>(pSamples.at(qMin(count - 1, (count * 95) / 100))) / 1e6;
    double maxMs = static_cast<double>(pSamples.last()) / 1e6;
    double rate = count * 1000.0 / qMax<qint64>(1, intervalMs);

    qCDebug(lcPerf).noquote() << QStringLiteral("%1: n=%2 (%3/s) mean=%4ms p95=%5ms max=%6ms")
        .arg(pName, -18).arg(count).arg(rate, 0, 'f', 1).arg(meanMs, 0, 'f', 3).arg(p95Ms, 0, 'f', 3).arg(maxMs, 0, 'f', 3);

    pSamples.clear();
}

//Constructor - only starts timing if perf logging is enabled
PerfTimer::PerfTimer(PerfStats &stats) : pStats(stats)
{
    if (lcPerf().isDebugEnabled())
        pTimer.start();
}

//Destructor - feeds elapsed time into the stats
PerfTimer::~PerfTimer()
{
    if (pTimer.isValid())
        pStats.addSample(pTimer.nsecsElapsed());
}
