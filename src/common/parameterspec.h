#pragma once

#include <QList>
#include <QPair>
#include <QString>
#include <QVariant>

//Describes one user-configurable parameter (of an image process, test pattern, ...), for building
//generic config UI - see ParameterForm
struct ParameterSpec
{
    enum Kind {IntValue, Choice};

    Kind kind = IntValue;
    QString key;
    QString label;
    QVariant defaultValue; //Optional - for types that take their defaults from their specs

    //IntValue
    int minimum = 0;
    int maximum = 100;
    int step = 1;
    QString suffix;

    //Choice - (label, value) pairs
    QList<QPair<QString, int>> choices;
};
