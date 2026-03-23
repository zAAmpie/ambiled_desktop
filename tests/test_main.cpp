#include <gtest/gtest.h>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    // QCoreApplication is sufficient for QImage operations — no platform
    // plugin required, which simplifies test discovery and CI execution.
    QCoreApplication app(argc, argv);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
