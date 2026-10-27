#pragma once

#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QString>
#include <QVector>

//Enable with QT_LOGGING_RULES="ambiled.perf.debug=true" - disabled (and near zero-cost) otherwise
Q_DECLARE_LOGGING_CATEGORY(lcPerf)

class PerfStats
{
    /* Rolling timing statistics for one pipeline stage. Periodically logs count, rate, mean, p95
     * and max to lcPerf. Not thread-safe - each instance must only be fed from a single thread. */
public:
    explicit PerfStats(QString name, int reportIntervalMs = 5000);

    //Record one sample, and log + reset if the report interval has elapsed
    void addSample(qint64 nsecs);

private:
    void report();

    QString pName;
    int pReportIntervalMs;
    QElapsedTimer pIntervalTimer;
    QVector<qint64> pSamples;
};

class PerfTimer
{
    /* Scoped timer that feeds its lifetime into a PerfStats, if lcPerf is enabled */
public:
    explicit PerfTimer(PerfStats &stats);
    ~PerfTimer();

private:
    PerfStats &pStats;
    QElapsedTimer pTimer;
};
