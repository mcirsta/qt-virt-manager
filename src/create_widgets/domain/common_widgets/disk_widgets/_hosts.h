#ifndef _HOSTS_H
#define _HOSTS_H

#include <QWidget>
#include <QCheckBox>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>

class _Hosts : public QWidget
{
    Q_OBJECT
public:
    explicit _Hosts(QWidget *parent = NULL);

signals:
    void             dataChanged();

private:
    QCheckBox       *useHosts;
    QListWidget     *hosts;
    QPushButton     *add;
    QPushButton     *del;
    QLineEdit       *name, *port;
    QLabel          *colon;
    QHBoxLayout     *panelLayout;
    QWidget         *panel;
    QVBoxLayout     *baseLayout;
    QWidget         *baseWdg;
    QVBoxLayout     *commonLayout;

    bool             hostMode = false;
    bool             oneHostMode = false;

public slots:
    bool             isUsed() const;
    void             setUsage(bool);
    void             setFullHostMode(bool);
    QStringList      getHostsList() const;
    void             setOneHostMode(bool);
    void             setHostItem(QString&);
    void             clearHostList();
    void             setHostPlaceholderText(const QString&);
    void             setPortPlaceholderText(const QString&);

private slots:
    void             addHost();
    void             delHost();
};

#endif // _HOSTS_H
