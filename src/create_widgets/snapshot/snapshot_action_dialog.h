#ifndef SNAPSHOT_ACTION_DIALOG_H
#define SNAPSHOT_ACTION_DIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTreeView>
#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDesktopServices>
#include <QTemporaryFile>
#include <QUrl>
#include <QDir>
#include <QDomDocument>
#include <QDateTime>
#include <QSettings>
#include "virt_objects/virt_entity_config.h"
#include "model/snapshot_tree_model.h"
#include "revert_snapshot_flags.h"
#include "delete_snapshot_flags.h"

class SnapshotActionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SnapshotActionDialog(
            QWidget        *parent      = NULL,
            virConnectPtr*  ptr_ConnPtr = NULL,
            QString         _domName    = QString());
    ~SnapshotActionDialog();

signals:
    void                 ptrIsNull();

private:
    virConnectPtr*       ptr_ConnPtr;
    virDomainPtr         domain = NULL;
    const QString        domName;
    QStringList          params;
    int                  flags = 0;
    SnapshotTreeModel   *model;
    QAction             *revertAction;
    RevertSnapshotFlags *revertFlagsMenu;
    QAction             *deleteAction;
    DeleteSnapshotFlags *deleteFlagsMenu;
    QAction             *refreshAction;
    QAction             *getXMLDescAction;
    QTreeView           *snapshotTree;
    QToolBar            *toolBar;
    QHBoxLayout         *buttonsLayout;
    QVBoxLayout         *commonLayout;
    QLabel              *info;
    QPushButton         *ok;
    QPushButton         *cancel;
    QPushButton         *closeWdg;
    QWidget             *buttonsWdg;
    QSettings            settings;

public slots:
    QStringList          getParameters() const;
    uint                 getSnapshotFlags() const;

private slots:
    void                 clearSnapshotTree();
    void                 addSnapshotChild(int, const QModelIndex&, char*);
    void                 setDomainSnapshots();
    void                 accept();
    void                 reject();
    void                 cancelled();
    void                 changeDialogState(bool);
    void                 detectTriggeredAction(QAction*);
    void                 showSnapsotXMLDesc();
};

#endif // SNAPSHOT_ACTION_DIALOG_H
