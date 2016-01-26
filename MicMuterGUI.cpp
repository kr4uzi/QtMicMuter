#include "MicMuterGUI.h"

#include <QSystemTrayIcon>
#include <QSettings>
#include <QLabel>
#include <QGridLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QKeyEvent>
#include <QKeySequence>
#include <QAction>
#include <QMenu>
#include <QCoreApplication>
using namespace std;

bool MicMuterGUI::IsScrollLockSet( ) const
{
    return m_SetScrollLock->isChecked( );
}

KeyboardHook::Keycode MicMuterGUI::GetHotKey( ) const
{
    return m_Hotkey;
}

void MicMuterGUI::CreateDialog( )
{    
    setWindowIcon( QIcon( ":/images/micon.svg" ) );
    setWindowFlags( Qt::WindowTitleHint | Qt::WindowCloseButtonHint );

    m_SaveButton = new QPushButton( tr("&Save") );
    m_RecordButton = new QPushButton( tr("&Record") );
    m_SetScrollLock = new QCheckBox( );

	m_RecordButton->setText( m_Settings->value( "hotkey_text" ).toString( ) );
	m_SetScrollLock->setChecked( m_Settings->value( "scrolllock" ).toBool( ) );

	connect( m_SaveButton, &QPushButton::clicked, [this]( ) { OnSaveButtonPressed( ); } );
	connect( m_RecordButton, &QPushButton::clicked, 
		[this]( )
	{
		m_Recording = true;
		m_RecordButton->setText( "..." );

		OnRecordButtonPressed( ); 
	} );

    auto layout = new QGridLayout( );
    layout->addWidget( new QLabel( tr("Hotkey") ), 0, 0 );
    layout->addWidget( m_RecordButton, 0, 1 );
    layout->addWidget( new QLabel( tr("Set Scrolllock") ), 1, 0 );
    layout->addWidget( m_SetScrollLock, 1, 1 );
    layout->addWidget( m_SaveButton, 2, 0, 1, 2 );

    setLayout( layout );
}

MicMuterGUI::MicMuterGUI( )
{
	m_Settings = new QSettings( "Krauzi", "QtMicMuter" );
    m_Hotkey = m_Settings->value( "hotkey" ).toUInt( );

	CreateDialog( );

	OnSaveButtonPressed.connect( [this]( )
	{
		m_Settings->setValue( "scrolllock", IsScrollLockSet( ) );
        m_Settings->setValue( "hotkey", m_Hotkey );
        m_Settings->setValue( "hotkey_text", m_RecordButton->text( ) );
		setVisible( false );
	} );

    m_OptionAction = new QAction( tr("&Options"), this );
    m_QuitAction = new QAction( tr("&Quit"), this );
	connect( m_OptionAction, &QAction::triggered, [this]( ) { setVisible( true ); } );
	connect( m_QuitAction, &QAction::triggered, QCoreApplication::instance( ), &QCoreApplication::quit );

    m_ContextMenu = new QMenu( );
    m_ContextMenu->addAction( m_OptionAction );
    m_ContextMenu->addSeparator( );
    m_ContextMenu->addAction( m_QuitAction );

    m_TrayIcon = new QSystemTrayIcon( );
    m_TrayIcon->setContextMenu( m_ContextMenu );
    SetIcon( false );

    m_TrayIcon->setVisible( true );
}

MicMuterGUI::~MicMuterGUI( )
{
    delete m_TrayIcon;

    delete m_ContextMenu;
}

void MicMuterGUI::SetIcon( bool muted )
{
    if( muted )
        m_TrayIcon->setIcon( QIcon( ":/images/micoff.svg" ) );
    else
        m_TrayIcon->setIcon( QIcon( ":/images/micon.svg" ) );
}

void MicMuterGUI::closeEvent( QCloseEvent * event )
{
    setVisible( false );
	m_Recording = false;
    event->ignore( );
}

void MicMuterGUI::keyPressEvent( QKeyEvent * event )
{
    if( m_Recording )
    {
		m_Hotkey = event->nativeVirtualKey( );
        m_RecordButton->setText( QKeySequence( event->key( ) ).toString( QKeySequence::NativeText ) );
        m_Recording = false;
    }
}
