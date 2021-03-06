#ifndef MEMBALLOON_H
#define MEMBALLOON_H

#include "create_widgets/domain/_qwidget.h"
#include "device_address.h"

class MemBalloon : public _QWidget
{
    Q_OBJECT
public:
    explicit MemBalloon(
            QWidget        *parent      = NULL,
            virConnectPtr*  connPtrPtr  = NULL);
    QComboBox       *model;
    QCheckBox       *periodLabel;
    QSpinBox        *period;
    DeviceAddress   *addr;

private:
    QLabel          *modelLabel;
    QGridLayout     *baseLayout;
    QWidget         *baseWdg;
    QVBoxLayout     *commonLayout;

public slots:
    QDomDocument     getDataDocument() const;

private slots:
    void             modelChanged(int);
};

#endif // MEMBALLOON_H
