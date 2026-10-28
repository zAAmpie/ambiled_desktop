#include "parameterform.h"
#include <QComboBox>
#include <QFormLayout>
#include <QSignalBlocker>
#include <QSpinBox>

//Constructor - appends one editor row per spec to the layout
ParameterForm::ParameterForm(const QList<ParameterSpec> &specs, QFormLayout *layout, QObject *parent) : QObject(parent)
{
    QWidget *owner = layout->parentWidget();
    for (const ParameterSpec &spec : specs)
    {
        switch (spec.kind)
        {
        case ParameterSpec::IntValue:
        {
            QSpinBox *spinBox = new QSpinBox(owner);
            spinBox->setRange(spec.minimum, spec.maximum);
            spinBox->setSingleStep(spec.step);
            spinBox->setSuffix(spec.suffix);
            if (spec.defaultValue.isValid())
                spinBox->setValue(spec.defaultValue.toInt());
            connect(spinBox, &QSpinBox::valueChanged, this, &ParameterForm::valuesChanged);
            layout->addRow(spec.label, spinBox);
            pSpinBoxes.insert(spec.key, spinBox);
            break;
        }
        case ParameterSpec::Choice:
        {
            QComboBox *comboBox = new QComboBox(owner);
            for (const QPair<QString, int> &choice : spec.choices)
                comboBox->addItem(choice.first, choice.second);
            if (spec.defaultValue.isValid())
                comboBox->setCurrentIndex(comboBox->findData(spec.defaultValue.toInt()));
            connect(comboBox, &QComboBox::currentIndexChanged, this, &ParameterForm::valuesChanged);
            layout->addRow(spec.label, comboBox);
            pComboBoxes.insert(spec.key, comboBox);
            break;
        }
        }
    }
}

//Current editor values, keyed by spec key
QVariantMap ParameterForm::values() const
{
    QVariantMap values;
    for (auto it = pSpinBoxes.constBegin(); it != pSpinBoxes.constEnd(); ++it)
        values.insert(it.key(), it.value()->value());
    for (auto it = pComboBoxes.constBegin(); it != pComboBoxes.constEnd(); ++it)
        values.insert(it.key(), it.value()->currentData());
    return values;
}

//Updates the editors for any keys present, without emitting valuesChanged
void ParameterForm::setValues(const QVariantMap &values)
{
    for (auto it = pSpinBoxes.constBegin(); it != pSpinBoxes.constEnd(); ++it)
    {
        if (!values.contains(it.key()))
            continue;
        QSignalBlocker blocker(it.value());
        it.value()->setValue(values.value(it.key()).toInt());
    }
    for (auto it = pComboBoxes.constBegin(); it != pComboBoxes.constEnd(); ++it)
    {
        if (!values.contains(it.key()))
            continue;
        QSignalBlocker blocker(it.value());
        it.value()->setCurrentIndex(it.value()->findData(values.value(it.key()).toInt()));
    }
}
