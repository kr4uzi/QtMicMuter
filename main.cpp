#include "MicMuter.h"
#include <QApplication>
#include <QMessageBox>
#include <QSystemTrayIcon>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    if( !QSystemTrayIcon::isSystemTrayAvailable( ) )
    {
        QMessageBox::critical( 0, QObject::tr("Systray"), QObject::tr("Couldnt detect any systemtray on this system!" ) );
        return 1;
    }

    MicMuter muter;

    return app.exec( );
}
