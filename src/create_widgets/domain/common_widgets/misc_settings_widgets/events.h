#ifndef EVENTS_H
#define EVENTS_H

#include <QWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>

class Events : public QWidget
{
    Q_OBJECT
public:
    explicit Events(QWidget *parent = 0);
    QCheckBox       *powerOffLabel, *rebootLabel,
                    *crashLabel, *lockFailureLabel;
    QComboBox       *on_powerOff, *on_reboot,
                    *on_crash, *on_lockfailure;

signals:
    void             dataChanged();

private:
    QGridLayout     *commonLayout;
};

#endif // EVENTS_H
