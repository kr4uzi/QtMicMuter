#pragma once

#include <QDialog>
#include <boost/signals2.hpp>

#include "KeyboardHook.h"

QT_BEGIN_NAMESPACE
class QSystemTrayIcon;
class QSettings;
class QAction;
class QMenu;
class QPushButton;
class QCheckBox;
QT_END_NAMESPACE

class MicMuterGUI : public QDialog
{
private:
    int m_QtHotkey;
	bool m_Recording = false;
    KeyboardHook::Keycode m_Hotkey;
	QSettings * m_Settings;
    QAction * m_QuitAction;
    QAction * m_OptionAction;

    QSystemTrayIcon * m_TrayIcon;
    QMenu * m_ContextMenu;

    QPushButton * m_SaveButton;
    QPushButton * m_RecordButton;
    QCheckBox * m_SetScrollLock;

public:
	MicMuterGUI( );
	~MicMuterGUI( );

	void SetIcon( bool muted );
	boost::signals2::signal<void( )> OnSaveButtonPressed;
	boost::signals2::signal<void( )> OnRecordButtonPressed;

    bool IsScrollLockSet( ) const;
	KeyboardHook::Keycode GetHotKey( ) const;

protected:
	void closeEvent( QCloseEvent * event ) override;
	void keyPressEvent( QKeyEvent *event ) override;

private:
	void CreateDialog( );
};
