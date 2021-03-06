#include "qspicewidget.h"

#include <QApplication>
#include <QImage>
#include <QPixmap>
#include <QMouseEvent>
#include <QClipboard>
#include <spice/vd_agent.h>
#include <spice-session.h>

#define MARGIN 5

QSpiceWidget::QSpiceWidget(QWidget *parent) :
    QWidget(parent)
{
    spiceSession = new QSpiceSession(this);
    display = NULL;
    inputs = NULL;
    cursor = NULL;
    smartcard = NULL;
    usbredir = NULL;
    webdav = NULL;
    playback = NULL;
    usbDevManager = NULL;
    smartcardManager = NULL;
    spiceAudio = NULL;
    _width = 0;
    _height = 0;
    init_h = 0;
    init_w = 0;
    zoom = 1.0;

    tr_mode = Qt::SmoothTransformation;
    img = NULL;
    m_Image = new QLabel(this);
    m_Image->setAlignment(Qt::AlignTop | Qt::AlignJustify);
    m_Image->setContentsMargins(0,0,0,0);

    commonLayout = new QHBoxLayout(this);
    commonLayout->addWidget(m_Image, -1, Qt::AlignJustify);
    commonLayout->setContentsMargins(0,0,0,0);
    setLayout(commonLayout);

    resizeTimer.setSingleShot( true );
    connect(&resizeTimer, SIGNAL(timeout()), SLOT(resizeDone()));

    connect(spiceSession, SIGNAL(channelNew(QSpiceChannel*)),
            SLOT(ChannelNew(QSpiceChannel*)));
    m_Image->setMouseTracking(true);
    m_Image->setFocusPolicy(Qt::StrongFocus);
    m_Image->installEventFilter( this );
    setContentsMargins(MARGIN,MARGIN,MARGIN,MARGIN);
}
QSpiceWidget::~QSpiceWidget()
{
    Disconnect();
}

bool QSpiceWidget::Connect(QString uri)
{
    spiceSession->setUri(uri);
    return spiceSession->Connect();
}
void QSpiceWidget::Disconnect()
{
    spiceSession->Disconnect();
}
void QSpiceWidget::SendKeySequience(Qt::Key key)
{
    if ( NULL!=inputs ) {
        // inputs->inputsKeyPressAndRelease()
        // does not give the desired effect
        inputs->inputsQKeyPress(key);
        inputs->inputsQKeyRelease(key);
    };
}

void QSpiceWidget::mainFileCopyAsync(QStringList &fileNames)
{
    main->mainFileCopyAsync(fileNames);
}

void QSpiceWidget::copyClipboardFromGuest()
{
    main->mainClipboardSelectionRequest();
}

void QSpiceWidget::sendClipboardDataToGuest(uint type, const uchar *_data, uint _size)
{
    main->mainClipboardSelectionNotify(type, _data, _size);
}

/* private slots */
void QSpiceWidget::ChannelNew(QSpiceChannel *channel)
{
    QSpiceMainChannel * _main = dynamic_cast<QSpiceMainChannel *>(channel);
    if (_main)
    {
        main = _main;
        connect(main, SIGNAL(channelDestroyed()),
                SLOT(channelDestroyed()));
        connect(main, SIGNAL(main_AgentUpdate()),
                SLOT(mainAgentUpdate()));
        connect(main, SIGNAL(main_ClipboardSelection(uint,void*,uint)),
                SLOT(mainClipboardSelection(uint,void*,uint)));
        connect(main, SIGNAL(main_ClipboardSelectionGrab(uint,void*,uint)),
                SLOT(mainClipboardSelectionGrab()));
        connect(main, SIGNAL(main_ClipboardSelectionRelease(uint)),
                SLOT(mainClipboardSelectionRelease(uint)));
        connect(main, SIGNAL(main_ClipboardSelectionRequest(uint,uint)),
                SLOT(mainClipboardSelectionRequest(uint,uint)));
        connect(main, SIGNAL(main_MouseUpdate()),
                SLOT(mainMouseUpdate()));
        connect(main, SIGNAL(downloaded(int,int)),
                this, SIGNAL(downloaded(int,int)));
        main->Connect();
        return;
    }

    QSpiceDisplayChannel *_display = dynamic_cast<QSpiceDisplayChannel *>(channel);
    if (_display)
    {
        display = _display;
        connect(display, SIGNAL(displayPrimaryCreate(int,int,int,int,int,void*)),
                SLOT(displayPrimaryCreate(int,int,int,int,int,void*)));
        connect(display, SIGNAL(displayInvalidate(int,int,int,int)),
                SLOT(displayInvalidate(int,int,int,int)));
        connect(display, SIGNAL(displayPrimaryDestroy()),
                SLOT(displayPrimaryDestroy()));
        connect(display, SIGNAL(displayMark(int)),
                SLOT(displayMark(int)));
        connect(display, SIGNAL(channelDestroyed()),
                SLOT(displayPrimaryDestroy()));

        bool online = display->Connect();
        emit displayChannelChanged(online);
        return;
    }

    QSpiceInputsChannel * _inputs = dynamic_cast<QSpiceInputsChannel *>(channel);
    if (_inputs)
    {
        inputs = _inputs;
        connect(inputs, SIGNAL(channelDestroyed()), SLOT(channelDestroyed()));
        bool online = inputs->Connect();
        emit inputsChannelChanged(online);
        return;
    }

    QSpiceCursorChannel * _cursor = dynamic_cast<QSpiceCursorChannel *>(channel);
    if (_cursor)
    {
        cursor = _cursor;
        connect(cursor, SIGNAL(channelDestroyed()), SLOT(channelDestroyed()));
        connect(cursor, SIGNAL(cursorSet(int,int,int,int,void*)),
                SLOT(cursorSet(int,int,int,int,void*)));
        bool online = cursor->Connect();
        emit cursorChannelChanged(online);
        return;
    }

    QSpiceSmartcardChannel * _smartcard = dynamic_cast<QSpiceSmartcardChannel *>(channel);
    if (_smartcard)
    {
        smartcard = _smartcard;
        bool online = smartcard->Connect();
        if ( online ) {
            smartcardManager = new QSpiceSmartcardManager(this);
            if ( smartcardManager ) {
                connect(smartcardManager, SIGNAL(cardInserted(QString&)),
                        this, SLOT(cardInserted(QString&)));
                connect(smartcardManager, SIGNAL(cardRemoved(QString&)),
                        this, SLOT(cardRemoved(QString&)));
                connect(smartcardManager, SIGNAL(readerAdded(QString&)),
                        this, SLOT(readerAdded(QString&)));
                connect(smartcardManager, SIGNAL(readerRemoved(QString&)),
                        this, SLOT(readerRemoved(QString&)));
            };
        };
        emit smartcardChannelChanged(online);
        return;
    }

    QSpiceUSBRedirChannel * _usbredir = dynamic_cast<QSpiceUSBRedirChannel *>(channel);
    if (_usbredir)
    {
        usbredir = _usbredir;
        bool online = usbredir->Connect();
        if ( online ) {
            usbDevManager = new QSpiceUsbDeviceManager(this, spiceSession);
            if ( usbDevManager ) {
                connect(usbDevManager, SIGNAL(autoConnectFailed(QString&,QString&)),
                        this, SLOT(usbDevAutoConnectFailed(QString&,QString&)));
                connect(usbDevManager, SIGNAL(deviceAdded(QString&)),
                        this, SLOT(usbDevAdded(QString&)));
                connect(usbDevManager, SIGNAL(deviceInfo(QString&,QString&)),
                        this, SLOT(usbDevError(QString&,QString&)));
                connect(usbDevManager, SIGNAL(deviceRemoved(QString&)),
                        this, SLOT(usbDevRemoved(QString&)));
            };
        };
        emit usbredirChannelChanged(online);
        return;
    }

    QSpiceWebDAVChannel * _webdav = dynamic_cast<QSpiceWebDAVChannel *>(channel);
    if (_webdav)
    {
        webdav = _webdav;
        bool online = webdav->Connect();
        emit webdavChannelChanged(online);
        return;
    }

    QSpicePlaybackChannel * _playback = dynamic_cast<QSpicePlaybackChannel *>(channel);
    if (_playback)
    {
        playback = _playback;
        bool online = playback->Connect();
        if ( online && !spiceAudio ) {
            spiceAudio = new QSpiceAudio(
                        this, (SpiceSession*)spiceSession->gobject);
            //qDebug()<<"spiceAudio is associated:"<<spiceAudio->isAssociated();
            if ( spiceAudio->isAssociated() ) {
                // reserved for some work
            };
        };
        emit playbackChannelChanged(online);
        return;
    }

    QSpiceRecordChannel * _record = dynamic_cast<QSpiceRecordChannel *>(channel);
    if (_record)
    {
        record = _record;
        bool online = record->Connect();
        if ( online && !spiceAudio ) {
            spiceAudio = new QSpiceAudio(
                        this, (SpiceSession*)spiceSession->gobject);
            //qDebug()<<"spiceAudio is associated:"<<spiceAudio->isAssociated();
            if ( spiceAudio->isAssociated() ) {
                // reserved for some work
            };
        };
        emit recordChannelChanged(online);
        return;
    }

}

void QSpiceWidget::channelDestroyed()
{
    if (QObject::sender() == main) {
        main = NULL;
    } else if (QObject::sender() == inputs) {
        inputs = NULL;
        emit inputsChannelChanged(false);
    } else if (QObject::sender() == cursor) {
        m_Image->setCursor(Qt::ArrowCursor);
        cursor = NULL;
        emit cursorChannelChanged(false);
    } else if (QObject::sender() == display) {
        display = NULL;
        emit displayChannelChanged(false);
    } else if (QObject::sender() == smartcard) {
        if (smartcardManager) {
            delete smartcardManager;
            smartcardManager = NULL;
        };
        smartcard = NULL;
        emit smartcardChannelChanged(false);
    } else if (QObject::sender() == usbredir) {
        if ( usbDevManager ) {
            delete usbDevManager;
            usbDevManager = NULL;
        };
        usbredir = NULL;
        emit usbredirChannelChanged(false);
    } else if (QObject::sender() == webdav) {
        webdav = NULL;
        emit webdavChannelChanged(false);
    } else if (QObject::sender() == playback) {
        playback = NULL;
        emit playbackChannelChanged(false);
        if ( spiceAudio ) {
            delete spiceAudio;
            spiceAudio = NULL;
        };
    } else if (QObject::sender() == record) {
        record = NULL;
        emit recordChannelChanged(false);
        if ( spiceAudio ) {
            delete spiceAudio;
            spiceAudio = NULL;
        };
    }
}


void QSpiceWidget::mainAgentUpdate()
{
    //qDebug()<<"main: AgentUpdate";
}

void QSpiceWidget::mainClipboardSelection(uint type, void *_data, uint _size)
{
    //qDebug()<<"main: ClipboardSelection";
    QImage _img;
    bool res;
    switch (type) {
    case VD_AGENT_CLIPBOARD_NONE:
        break;
    case VD_AGENT_CLIPBOARD_UTF8_TEXT:
        QApplication::clipboard()->setText(
                    QString::fromUtf8((char*)_data, _size));
        break;
    case VD_AGENT_CLIPBOARD_IMAGE_PNG:
        res = _img.loadFromData((uchar*)_data, _size, "PNG");
        if (res) QApplication::clipboard()->setImage(_img);
        break;
    case VD_AGENT_CLIPBOARD_IMAGE_BMP:
        res = _img.loadFromData((uchar*)_data, _size, "BMP");
        if (res) QApplication::clipboard()->setImage(_img);
        break;
    case VD_AGENT_CLIPBOARD_IMAGE_JPG:
        res = _img.loadFromData((uchar*)_data, _size, "JPG");
        if (res) QApplication::clipboard()->setImage(_img);
        break;
    case VD_AGENT_CLIPBOARD_IMAGE_TIFF:
        res = _img.loadFromData((uchar*)_data, _size, "TIFF");
        if (res) QApplication::clipboard()->setImage(_img);
        break;
    default:
        break;
    };
}

void QSpiceWidget::mainClipboardSelectionGrab()
{
    qDebug()<<"main: ClipboardSelectionGrab";
}

void QSpiceWidget::mainClipboardSelectionRelease(uint selection)
{
    switch (selection) {
    case VD_AGENT_CLIPBOARD_SELECTION_CLIPBOARD:
        qDebug()<<"guestClipboard_CLIPBOARDSelectionRelease";
        break;
    case VD_AGENT_CLIPBOARD_SELECTION_PRIMARY:
        qDebug()<<"guestClipboard_PRIMARYSelectionRelease";
        break;
    case VD_AGENT_CLIPBOARD_SELECTION_SECONDARY:
        qDebug()<<"guestClipboard_SECONDARYSelectionRelease";
        break;
    default:
        break;
    }
}

void QSpiceWidget::mainClipboardSelectionRequest(uint selection, uint type)
{
    QString dataType;
    switch (type) {
    case VD_AGENT_CLIPBOARD_NONE:
        dataType.append("None");
        break;
    case VD_AGENT_CLIPBOARD_UTF8_TEXT:
        dataType.append("UTF8_TEXT");
        break;
    case VD_AGENT_CLIPBOARD_IMAGE_PNG:
        dataType.append("IMAGE_PNG");
        break;
    case VD_AGENT_CLIPBOARD_IMAGE_BMP:
        dataType.append("IMAGE_BMP");
        break;
    case VD_AGENT_CLIPBOARD_IMAGE_JPG:
        dataType.append("IMAGE_JPG");
        break;
    case VD_AGENT_CLIPBOARD_IMAGE_TIFF:
        dataType.append("IMAGE_TIFF");
        break;
    default:
        break;
    };
    switch (selection) {
    case VD_AGENT_CLIPBOARD_SELECTION_CLIPBOARD:
        qDebug()<<"guestClipboard_CLIPBOARDSelectionRequest"<<dataType;
        break;
    case VD_AGENT_CLIPBOARD_SELECTION_PRIMARY:
        qDebug()<<"guestClipboard_PRIMARYSelectionRequest"<<dataType;
        break;
    case VD_AGENT_CLIPBOARD_SELECTION_SECONDARY:
        qDebug()<<"guestClipboard_SECONDARYSelectionRequest"<<dataType;
        break;
    default:
        break;
    }
}

void QSpiceWidget::mainMouseUpdate()
{
    //qDebug()<<"main: MouseUpdate";
}

void QSpiceWidget::usbDevAutoConnectFailed(QString &dev, QString &err)
{
    QString _msg = QString("<font color='red'>UsbDevAutoConnectFailed</font>: %1 : %2")
            .arg(dev).arg(err);
    emit errMsg(_msg);
    //qDebug()<<"usbDevAutoConnectFailed:"<< dev<< err;
}

void QSpiceWidget::usbDevAdded(QString &dev)
{
    QString _msg = QString("<font color='blue'>UsbDevAdded</font>: %1")
            .arg(dev);
    emit errMsg(_msg);
    //qDebug()<<"usbDevAdded:"<< dev;
}

void QSpiceWidget::usbDevError(QString &dev, QString &err)
{
    QString _msg = QString("<font color='blue'>UsbDevInfo</font>: %1 : %2")
            .arg(dev).arg(err);
    emit errMsg(_msg);
    //qDebug()<<"usbDevError"<< dev<< err;
}

void QSpiceWidget::usbDevRemoved(QString &dev)
{
    QString _msg = QString("<font color='blue'>UsbDevRemoved</font>: %1")
            .arg(dev);
    emit errMsg(_msg);
    //qDebug()<<"usbDevRemoved:"<< dev;
}

void QSpiceWidget::cardInserted(QString &_vcard)
{
    qDebug()<<"cardInserted"<<_vcard;
}

void QSpiceWidget::cardRemoved(QString &_vcard)
{
    qDebug()<<"cardRemoved"<<_vcard;
}

void QSpiceWidget::readerAdded(QString &_reader)
{
    qDebug()<<"readerAdded"<<_reader;
}

void QSpiceWidget::readerRemoved(QString &_reader)
{
    qDebug()<<"readerRemoved"<<_reader;
}

void QSpiceWidget::displayPrimaryCreate(
     int                 format,
     int                 width,
     int                 height,
     int                 stride,
     int                 shmid,
     void  *             imgdata)
{
    Q_UNUSED(shmid);
    m_Image->setUpdatesEnabled(false);

    //qDebug() << "Display Create(" << width << ", " << height << ")";
    init_h = height; init_w = width;
    QImage::Format _format = QImage::Format_Invalid;

    if (img) {
        delete img;
        img = NULL;
    };
    switch(format) {
    case SPICE_SURFACE_FMT_32_xRGB:
        _format = QImage::Format_RGB32;
        break;

    case SPICE_SURFACE_FMT_16_555:
        _format = QImage::Format_RGB555;
        break;

    default:
        qDebug() << "Unknown display format " << _format;
    };
    if ( _format!=QImage::Format_Invalid ) {
        img = new QImage(
                    static_cast<uchar *>(imgdata),
                    init_w,
                    init_h,
                    stride,
                    _format);
    };

    if (img) {
        m_Image->setUpdatesEnabled(false);
        QPixmap pix =  QPixmap::fromImage(*img);
        m_Image->setPixmap(pix);
        QSize _size(init_w+2*MARGIN, init_h+2*MARGIN+4);
        //qDebug()<<_size<<"emit";
        emit DisplayResize(_size);
    }
}


void QSpiceWidget::displayInvalidate(
    int                 x,
    int                 y,
    int                 width,
    int                 height)
{
    //qDebug()<<"displayInvalidate"<<x<<y<<width<<height<<":"<<x*zoom<<y*zoom<<width*zoom<<height*zoom;
    // for optimal processing
    if ( zoom == 1.0 ) {
        // faster
        m_Image->update(x, y, width, height);
    } else {
        // slower
        QPixmap pix = QPixmap::fromImage(
                    img->scaled(
                        _width,
                        _height,
                        Qt::KeepAspectRatio,
                        tr_mode));
        m_Image->setPixmap(pix);
    };
}

void QSpiceWidget::displayPrimaryDestroy()
{
    //qDebug() << "Display Destroyed";
    m_Image->setPixmap(QPixmap());
    m_Image->setUpdatesEnabled(false);
}

void QSpiceWidget::displayMark(int mark)
{
    //qDebug() << "Display Mark " << mark;
    m_Image->setUpdatesEnabled(mark != 0);
}

void QSpiceWidget::cursorSet(
        int                width,
        int                height,
        int                hot_x,
        int                hot_y,
        void *             rgba)
{
    QImage c_img((uchar *) rgba, width, height, QImage::Format_ARGB32);
    QPixmap pix = QPixmap::fromImage(c_img);
    QCursor c(pix, hot_x, hot_y);
    m_Image->setCursor(c);
}


int QtButtonToSpice(QMouseEvent *ev)
{
    switch (ev->button())
    {
    case Qt::LeftButton:
        return SPICE_MOUSE_BUTTON_LEFT;

    case Qt::MiddleButton:
        return SPICE_MOUSE_BUTTON_MIDDLE;

    case Qt::RightButton:
        return SPICE_MOUSE_BUTTON_RIGHT;

    default:
        return SPICE_MOUSE_BUTTON_INVALID;
    }
}

int QtButtonsMaskToSpice(Qt::MouseButtons buttons)
{
    int mask = 0;
    if (buttons & Qt::LeftButton)
        mask |= SPICE_MOUSE_BUTTON_MASK_LEFT;

    if (buttons & Qt::MiddleButton)
        mask |= SPICE_MOUSE_BUTTON_MASK_MIDDLE;

    if (buttons & Qt::RightButton)
        mask |= SPICE_MOUSE_BUTTON_MASK_RIGHT;

    return mask;
}

int QtButtonsMaskToSpice(QMouseEvent *ev)
{
    return QtButtonsMaskToSpice(ev->buttons());
}

bool QSpiceWidget::eventFilter(QObject *object, QEvent *event)
{
    Q_UNUSED(object);

    if (! (display && inputs))
        return false;

    if (event->type() == QEvent::MouseMove )
    {
        QMouseEvent *ev = (QMouseEvent *) event;
        //qDebug()<<ev->x()<<ev->y()<<":"<<ev->x()*zoom<<ev->y()*zoom<<":"<<zoom;
        inputs->inputsPosition(ev->x()*zoom, ev->y()*zoom,
                               display->getId(), QtButtonsMaskToSpice(ev));
        return true;
    }
    else if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *ev = (QMouseEvent *) event;
        inputs->inputsButtonPress(QtButtonToSpice(ev), QtButtonsMaskToSpice(ev));
        return true;
    }
    else if (event->type() == QEvent::MouseButtonRelease)
    {
        QMouseEvent *ev = (QMouseEvent *) event;
        inputs->inputsButtonRelease(QtButtonToSpice(ev), QtButtonsMaskToSpice(ev));
        return true;
    }
    else if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *ev = (QKeyEvent *) event;
        inputs->inputsQKeyPress(ev->key());
        return true;
    }
    else if (event->type() == QEvent::KeyRelease)
    {
        QKeyEvent *ev = (QKeyEvent *) event;
        inputs->inputsQKeyRelease(ev->key());
        return true;
    }
    else if (event->type() == QEvent::MouseButtonDblClick)
    {
        inputs->inputsButtonPress(SPICE_MOUSE_BUTTON_LEFT, SPICE_MOUSE_BUTTON_MASK_LEFT);
        inputs->inputsButtonRelease(SPICE_MOUSE_BUTTON_LEFT, 0);
        return true;
    }
    else if (event->type() == QEvent::Wheel)
    {
        QWheelEvent *ev = (QWheelEvent *) event;
        if (ev->delta() > 0)
        {
            inputs->inputsButtonPress(SPICE_MOUSE_BUTTON_UP, QtButtonsMaskToSpice(QApplication::mouseButtons()));
            inputs->inputsButtonRelease(SPICE_MOUSE_BUTTON_UP, QtButtonsMaskToSpice(QApplication::mouseButtons()));
        }
        else
        {
            inputs->inputsButtonPress(SPICE_MOUSE_BUTTON_DOWN, QtButtonsMaskToSpice(QApplication::mouseButtons()));
            inputs->inputsButtonRelease(SPICE_MOUSE_BUTTON_DOWN, QtButtonsMaskToSpice(QApplication::mouseButtons()));

        }
        ev->accept();
        return true;
    }


    return false;

}

void QSpiceWidget::resizeEvent ( QResizeEvent * event )
{
    Q_UNUSED(event)

    if (m_Image->updatesEnabled() && main && display)
        resizeTimer.start(500);
}

void QSpiceWidget::reloadUsbDevList(void *obj)
{
    SpiceUsbDeviceWidget *usbDevWdg = static_cast<SpiceUsbDeviceWidget*>(obj);
    if ( usbDevWdg ) {
        usbDevWdg->setEnabled(false);
        usbDevWdg->clearList();
        QStringList _devList =
                usbDevManager->spiceUsbDeviceManager_get_devices();
        foreach (QString _dev, _devList) {
            QString _name, _desc;
            QStringList _split = _dev.split("<||>");
            _name = _split.first();
            _desc = _split.last();
            QString _id = QString("%1 %2").arg(_name).arg(_desc);
            bool connected = usbDevManager->
                    spiceUsbDeviceManager_is_device_connected(_id);
            QListWidgetItem *item = new QListWidgetItem();
            item->setText(_name);
            item->setData(Qt::UserRole, _desc);
            item->setCheckState(connected? Qt::Checked:Qt::Unchecked);
            usbDevWdg->addItem(item);
        };
        usbDevWdg->setEnabled(true);
    };
}

void QSpiceWidget::reloadSmartcardList(void *obj)
{
    SpiceSmartcardWidget *smartcardWdg = static_cast<SpiceSmartcardWidget*>(obj);
    if ( smartcardWdg ) {
        smartcardWdg->setEnabled(false);
        smartcardWdg->clearList();
        QStringList _cardList =
                smartcardManager->spiceSmartcardManager_get_readers();
        foreach (QString _card, _cardList) {
            QListWidgetItem *item = new QListWidgetItem();
            item->setText(_card);
            item->setCheckState(Qt::Checked);
            smartcardWdg->addItem(item);
        };
        smartcardWdg->setEnabled(true);
    };
}

void QSpiceWidget::resizeDone()
{
    if (m_Image->updatesEnabled() && main && display)
    {
        main->mainSetDisplay(
                    display->getId(),
                    0,
                    0,
                    _width,
                    _height);
        main->mainSetDisplayEnabled(display->getId(), true);
        //qDebug()<<"configured"<<
        main->mainSendMonitorConfig();
    }

}

/* public slots */
void QSpiceWidget::setGuestName(QString &_name)
{
    guestName = _name;
}

void QSpiceWidget::setNewSize(int _w, int _h)
{
    _width  = _w-2*MARGIN;
    _height = _h-2*MARGIN-4;
    //qDebug()<<"setNewSize"<<_width<<_height;
    // for optimal processing
    if ( _width==init_w && _height==init_h ) {
        // faster
        zoom = 1.0;
        return;
    };
    if ( _width-2*MARGIN && init_w ) {
        // slower
        if ( img ) {
            QPixmap pix = QPixmap::fromImage(
                        img->scaled(
                            _width,
                            _height,
                            Qt::KeepAspectRatio,
                            tr_mode));
            m_Image->setUpdatesEnabled(true);
            m_Image->setPixmap(pix);
            zoom = init_w/(qreal)pix.width();
        };
    };
}

void QSpiceWidget::showUsbDevWidget()
{
    SpiceUsbDeviceWidget *usbDevWdg = new SpiceUsbDeviceWidget(this);
    if ( usbDevManager ) {
        connect(usbDevManager, SIGNAL(deviceAdded(QString&)),
                usbDevWdg, SLOT(addDevice(QString&)));
        connect(usbDevManager, SIGNAL(deviceRemoved(QString&)),
                usbDevWdg, SLOT(removeDevice(QString&)));
        connect(usbDevWdg, SIGNAL(connectDevice(QString&)),
                usbDevManager, SLOT(spiceUsbDeviceManager_connect_device(QString&)));
        connect(usbDevWdg, SIGNAL(disconnectDevice(QString&)),
                usbDevManager, SLOT(spiceUsbDeviceManager_disconnect_device(QString&)));
        connect(usbDevWdg, SIGNAL(devicesChanged(void*)),
                this, SLOT(reloadUsbDevList(void*)));
        reloadUsbDevList(usbDevWdg);
    };
    usbDevWdg->exec();
    delete usbDevWdg;
    usbDevWdg = NULL;
}

void QSpiceWidget::showSmartCardWidget()
{
    SpiceSmartcardWidget *smartcardWdg = new SpiceSmartcardWidget(this);
    if ( smartcardManager ) {
        connect(smartcardManager, SIGNAL(cardInserted(QString&)),
                smartcardWdg, SLOT(addCard(QString&)));
        connect(smartcardManager, SIGNAL(cardRemoved(QString&)),
                smartcardWdg, SLOT(removeCard(QString&)));
        //connect(smartcardWdg, SIGNAL(connectCard(QString&)),
        //        smartcardManager, SLOT(spiceSmartcardReader_insert_card(QString&)));
        //connect(smartcardWdg, SIGNAL(disconnectCard(QString&)),
        //        smartcardManager, SLOT(spiceSmartcardReader_remove_card(QString&)));
        connect(smartcardWdg, SIGNAL(cardsChanged(void*)),
                this, SLOT(reloadSmartcardList(void*)));
        reloadSmartcardList(smartcardWdg);
    };
    smartcardWdg->exec();
    delete smartcardWdg;
    smartcardWdg = NULL;
}

void QSpiceWidget::getScreenshot()
{
    // WARNING: used %1%2%3.snapshot template,
    // because filter will added to tail the template
    // after last dot.
    QString fileName = QFileDialog::getSaveFileName(
                this,
                "Save Image to",
                QString("%1%2%3_%4_%5.snapshot")
                    .arg(QDir::homePath())
                    .arg(QDir::separator())
                    .arg(guestName)
                    .arg(QDate::currentDate().toString("dd.MM.yyyy"))
                    .arg(QTime::currentTime().toString()),
                "Images (*.png)");
    if ( !fileName.isNull() ) {
        img->save(fileName, "png");
    };
}

void QSpiceWidget::setTransformationMode(Qt::TransformationMode _mode)
{
    tr_mode = _mode;
}
