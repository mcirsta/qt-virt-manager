#ifndef _VIRT_THREAD_H
#define _VIRT_THREAD_H

#include <QThread>
#include <QString>
#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include <QDebug>

class _VirtThread : public QThread
{
    Q_OBJECT
public:
    explicit _VirtThread(
            QObject        *parent      = NULL,
            virConnectPtr  *connPtrPtr  = NULL);
    virtual ~_VirtThread();
    bool             keep_alive;
    uint             number;
    virConnectPtr   *ptr_ConnPtr;
    virErrorPtr      virtErrors;

    virtual QString  sendConnErrors();
    virtual QString  sendGlobalErrors();

signals:
    void             errorMsg(QString&, uint);
    void             ptrIsNull();
public slots:
    virtual void     stop();
};

#endif // _VIRT_THREAD_H
