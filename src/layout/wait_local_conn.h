#ifndef WAIT_LOCAL_CONN_H
#define WAIT_LOCAL_CONN_H

#include <QThread>
#include <QRegExp>
#include "conn_item_model.h"
#include "virt_objects/virt_connect/conn_element.h"
#include <QDebug>

typedef QMap<QString, ConnElement*> CONN_LIST;

class WaitLocalConn : public QThread
{
    Q_OBJECT
public:
    explicit WaitLocalConn(
            QObject         *parent = NULL,
            CONN_LIST       *_conns = NULL,
            ConnItemModel   *_model = NULL);
    CONN_LIST       *connections;
    ConnItemModel   *connItemModel;
    void             run();

signals:
    void             uriToCompare(QString&);
private:
    bool             localConnsClosed;
};

#endif // WAIT_LOCAL_CONN_H
