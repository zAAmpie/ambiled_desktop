#pragma once

#include <QObject>
#include <QMap>
#include <QVariantMap>
#include "parameterspec.h"

class QFormLayout;
class QSpinBox;
class QComboBox;

class ParameterForm : public QObject
{
    /* Builds one editor row per ParameterSpec (a spin box for IntValue, a combo box for Choice) into
     * an existing form layout, and reads/writes their values as a QVariantMap keyed by spec key -
     * so config UI for processes, test patterns etc. needs no per-type widget code. */
    Q_OBJECT
public:
    //Rows are appended to `layout`; the widgets are parented to the layout's parent widget
    ParameterForm(const QList<ParameterSpec> &specs, QFormLayout *layout, QObject *parent = nullptr);

    QVariantMap values() const;
    //Updates the editors for any keys present, without emitting valuesChanged
    void setValues(const QVariantMap &values);

signals:
    //The user changed any editor
    void valuesChanged();

private:
    QMap<QString, QSpinBox *> pSpinBoxes;
    QMap<QString, QComboBox *> pComboBoxes;
};
