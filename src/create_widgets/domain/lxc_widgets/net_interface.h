#ifndef LXC_NET_INTERFACE_H
#define LXC_NET_INTERFACE_H

#include "create_widgets/domain/_qwidget.h"
#include "libvirt/libvirt.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

class LXC_NetInterface : public _QWidget
{
    Q_OBJECT
public:
    explicit LXC_NetInterface(QWidget *parent = 0, virNetworkPtr *nets = NULL);
    ~LXC_NetInterface();

signals:

private:
    virNetworkPtr           *existNetwork;
    QCheckBox               *useExistNetwork;
    QComboBox               *networks;

    QLineEdit               *bridgeName;
    QLabel                  *macLabel;
    QLineEdit               *mac;
    QGridLayout             *netDescLayout;
    QWidget                 *netDescWdg;

    QVBoxLayout             *commonLayout;

public slots:

private slots:
    void changeUsedNetwork(bool);
    void changeUsedNetwork(QString);

};

#endif // LXC_NET_INTERFACE_H