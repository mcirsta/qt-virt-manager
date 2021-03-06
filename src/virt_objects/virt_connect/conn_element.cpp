#include "conn_element.h"

#define TIMEOUT 10
#define TO_RUN true
#define TO_STOP false

ConnElement::ConnElement(QObject *parent) :
    QObject(parent)
{
    waitTimerId = 0;
    connAliveThread = new ConnAliveThread(this);
    connect(connAliveThread, SIGNAL(connMsg(QString)),
            this, SLOT(receiveConnMessage(QString)));
    connect(connAliveThread, SIGNAL(changeConnState(CONN_STATE)),
            this, SLOT(setConnectionState(CONN_STATE)));
    connect(connAliveThread, SIGNAL(authRequested(QString&)),
            this, SLOT(getAuthCredentials(QString&)));
    connect(connAliveThread, SIGNAL(domStateChanged(Result)),
            this, SIGNAL(domStateChanged(Result)));
    connect(connAliveThread, SIGNAL(netStateChanged(Result)),
            this, SIGNAL(netStateChanged(Result)));
    connect(connAliveThread, SIGNAL(connClosed(bool)),
            this, SLOT(forwardConnClosedSignal(bool)));
    connect(connAliveThread, SIGNAL(errorMsg(QString&,uint)),
            this, SLOT(writeErrorToLog(QString&,uint)));
    // change element state in the thread's state changed case only
    connect(connAliveThread, SIGNAL(started()),
            this, SLOT(connAliveThreadStarted()));
    connect(connAliveThread, SIGNAL(finished()),
            this, SLOT(connAliveThreadFinished()));
    connect(connAliveThread, SIGNAL(domainEnd(QString&)),
            this, SLOT(emitDomainKeyToCloseViewer(QString&)));
}
void ConnElement::buildURI()
{
    settings.beginGroup("Connects");
    settings.beginGroup(name);
    checkTimeout = settings.value("TimeOut", TIMEOUT).toInt();
    QString Driver = settings.value("Driver", "").toString();
    QString Transport = settings.value("Transport", "").toString();
    Host = settings.value("Host", "").toString();
    QString Path = settings.value("Path", "").toString();
    QString Extra = settings.value("Extra", "").toString();
    settings.endGroup();
    settings.endGroup();

    // URI building
    QStringList _uri;
    QString _driver, _drv_row;
    _drv_row = Driver.toLower().split("/").first();
    if ( _drv_row.startsWith("vmware") ) {
        if ( _drv_row.endsWith("player") ) {
            _driver.append("vmwareplayer");
        } else if ( _drv_row.endsWith("workstation") ) {
            _driver.append("vmwarews");
        } else if ( _drv_row.endsWith("fusion") ) {
            _driver.append("vmwarefusion");
        } else if ( _drv_row.endsWith("esx") ) {
            _driver.append("esx");
        } else if ( _drv_row.endsWith("gsx") ) {
            _driver.append("gsx");
        } else if ( _drv_row.endsWith("vpx") ) {
            _driver.append("vpx");
        };
    } else
        _driver = _drv_row;
    _uri.append(_driver);
    if ( !Transport.isEmpty() ) {
        _uri.append("+");
        _uri.append(Transport.toLower());
    };
    _uri.append("://");
    if ( !Host.isEmpty() ) {
        _uri.append(Host);
    };
    _uri.append("/");
    if ( !Path.isEmpty() ) {
        _uri.append(Path);
    };
    if ( !Extra.isEmpty() ) {
        _uri.append("?");
        _uri.append(Extra);
    };
    URI = _uri.join("");
    own_index->setURI(URI);
}

/* public slots */
void ConnElement::setItemReference(ConnItemModel *model, ConnItemIndex *idx)
{
    own_model = model;
    own_index = idx;
    name = idx->getName();
    conn_Status.insert("availability", QVariant(AVAILABLE));
    conn_Status.insert("isRunning", QVariant(CLOSED));
    conn_Status.insert("initName", QVariant(name));
    idx->setData(conn_Status);
    settings.beginGroup("Connects");
    settings.beginGroup(name);
    bool atStart = settings.value("AtStart", QVariant(false)).toBool();
    settings.endGroup();
    settings.endGroup();
    buildURI();
    if ( atStart ) openConnection();
}
void ConnElement::setItemReferenceForLocal(ConnItemModel *model, ConnItemIndex *idx)
{
    own_model = model;
    own_index = idx;
    name = idx->getName();
    conn_Status.insert("availability", QVariant(AVAILABLE));
    conn_Status.insert("isRunning", QVariant(CLOSED));
    conn_Status.insert("initName", QVariant(name));
    idx->setData(conn_Status);
    openConnection();
}
void ConnElement::openConnection()
{
    conn_Status.insert("availability", QVariant(NOT_AVAILABLE));
    own_index->setData(conn_Status);
    connAliveThread->setData(URI);
    if ( !connAliveThread->isRunning() ) connAliveThread->start();
    if (!waitTimerId) {
        _diff = 0;
        waitTimerId = startTimer(1000);
    } else {
        addMsgToLog(QString("Connection '%1'").arg(name),
            QString("Wait Timer is running: %1").arg(waitTimerId));
    };
}
void ConnElement::closeConnection()
{
    if ( connAliveThread->isRunning() ) connAliveThread->closeConnection();
}
void ConnElement::overviewConnection()
{
    virConnectPtr *_connPtrPtr = connAliveThread->getPtr_connectionPtr();
    //qDebug()<<"overviewConnection"<<(*_connPtrPtr);
    emit connPtrPtr(_connPtrPtr, name);
    int row = own_model->connItemDataList.indexOf(own_index);
    own_model->setData(own_model->index(row, 0), true, Qt::DecorationRole);
}
virConnectPtr* ConnElement::getPtr_connectionPtr() const
{
    return connAliveThread->getPtr_connectionPtr();
}
void ConnElement::setAuthCredentials(QString &crd, QString &text)
{
    if ( connAliveThread!=NULL ) {
        connAliveThread->setAuthCredentials(crd, text);
    }
}
QString ConnElement::getName() const
{
    return name;
}
QString ConnElement::getURI() const
{
    return URI;
}
void ConnElement::setName(QString &_name)
{
    name = _name;
}
void ConnElement::setURI(QString &_uri)
{
    URI = _uri;
}
void ConnElement::setOnViewConnAliveThread(bool state)
{
    connAliveThread->onView = state;
}

/* private slots */
void ConnElement::setConnectionState(CONN_STATE status)
{
    //qDebug()<<"setConnectionState0"<<status;
    if (waitTimerId) {
        killTimer(waitTimerId);
        waitTimerId = 0;
    };
    conn_Status.insert("availability", QVariant(AVAILABLE));
    conn_Status.insert("isRunning", QVariant(status));
    conn_Status.insert("onView", QVariant(false));
    own_index->setData(conn_Status);
    int row = own_model->connItemDataList.indexOf(own_index);
    QString data;
    for (int i=0; i<own_model->columnCount(); i++) {
        switch (i) {
        case 0:
            data = name;
            break;
        case 1:
            data = URI;
            break;
        case 2:
            switch (status) {
            case FAILED:
                data = "FAILED";
                break;
            case CLOSED:
                data = "CLOSED";
                break;
            case RUNNING:
                data = "OPENED";
                break;
            default:
                break;
            };
            break;
        default:
            break;
        };
        own_model->setData(own_model->index(row, i), data, Qt::EditRole);
    };
    //qDebug()<<"setConnectionState1"<<status;
}
void ConnElement::timerEvent(QTimerEvent *event)
{
    int percent = 0;
    int _timerId = event->timerId();
    if ( _timerId && waitTimerId==_timerId ) {
        percent = int ((float(_diff)/checkTimeout)*100.0);
        QModelIndex _idx = own_model->index(
                    own_model->connItemDataList.indexOf( own_index ), 2);
        own_model->setData(_idx, QString::number(percent), Qt::EditRole);
        _diff++;
    };
}
void ConnElement::receiveConnMessage(QString msg)
{
    addMsgToLog( QString("Connection '%1'").arg(name), msg );
}
void ConnElement::addMsgToLog(QString title, QString msg)
{
    QString time = QTime::currentTime().toString();
    QString errorMsg = QString("<b>%1 %2:</b><br><font color='blue'><b>EVENT</b></font>: %3")
            .arg(time).arg(title).arg(msg);
    sendWarning(errorMsg);
    mainWindowUp();
}
void ConnElement::sendWarning(QString &msg)
{
    emit warning(msg);
}
void ConnElement::writeErrorToLog(QString &msg, uint _num)
{
    Q_UNUSED(_num);
    QString time = QTime::currentTime().toString();
    QString title(QString("Connection '%1'").arg(name));
    QString errorMsg = QString("<b>%1 %2:</b><br><font color='red'><b>ERROR</b></font>: %3")
            .arg(time).arg(title).arg(msg);
    sendWarning(errorMsg);
}
void ConnElement::mainWindowUp()
{
    emit warningShowed();
}
void ConnElement::getAuthCredentials(QString &crd)
{
    emit authRequested(crd);
}
void ConnElement::forwardConnClosedSignal(bool onView)
{
    emit connClosed(onView, name);
}
void ConnElement::connAliveThreadStarted()
{
    conn_Status.insert("isRunning", QVariant(CONNECT));
    own_index->setData(conn_Status);
}
void ConnElement::connAliveThreadFinished()
{
    conn_Status.insert("isRunning", QVariant(CLOSED));
    own_index->setData(conn_Status);
}
void ConnElement::emitDomainKeyToCloseViewer(QString &_domName)
{
    QString key;
    key = QString("%1_%2").arg(name).arg(_domName);
    emit domainEnd(key);
}
