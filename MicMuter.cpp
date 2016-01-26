#include "MicMuter.h"
#include "MicMuterGUI.h"
#include <functional>

#if defined(WIN32)
#include "ComInterface.h"
#elif defined(__APPLE__)
#include "AudioDeviceInterface.h"
#endif

MicMuter::MicMuter( )
    : m_GUI( new MicMuterGUI( ) )
{
	auto keyhook = KeyboardHook::GetInstance( );
	keyhook->OnKeyPress.connect( [=]( KeyboardHook::KeyboardMessage msg, KeyboardHook::Keycode key )
	{
        if( m_Microphone && msg == KeyboardHook::KeyboardMessage::KEYDOWN && key == m_GUI->GetHotKey( ) )
			m_Microphone->SetMuted( !m_Microphone->IsMuted( ) );
	} );

#ifdef WIN32
	m_MicManager = ComInterface::GetInstance( );
#elif defined(__APPLE__)
	m_MicManager = AudioDeviceInterface::GetInstance( );
#endif

	m_MicManager->OnDefaultInputDeviceChanged.connect( std::bind( &MicMuter::DefaultMicrophoneChanged, this ) );
    DefaultMicrophoneChanged( );

	keyhook->Start( );
}

MicMuter::~MicMuter( )
{
	KeyboardHook::GetInstance( )->OnKeyPress.disconnect_all_slots( );

	m_MicManager->OnDefaultInputDeviceChanged.disconnect_all_slots( );
	if( m_Microphone )
		m_Microphone->OnMuted.disconnect_all_slots( );

	delete m_GUI;
}

/*
std::shared_ptr<IMicrophone> MicMuter::GetDefaultMicrophone( )
{	
#ifdef WIN32
	auto com = ComInterface::GetInstance( );
	if( com )
	{
		auto enumerator = com->GetDeviceEnumerator( );
		if( enumerator )
		{
            enumerator->OnDefaultDeviceChanged.connect( [=]( ComIMMDeviceEnumerator::DataFlow flow, ComIMMDeviceEnumerator::Role role )
			{
                if( flow == ComIMMDeviceEnumerator::DataFlow::Capture && role == ComIMMDeviceEnumerator::Role::Communications )
				{
                    OnDefaultMicrophoneChanged( enumerator->GetDefaultAudioEndpoint( flow, role ) );
				}
			} );

			return enumerator->GetDefaultAudioEndpoint( ComIMMDeviceEnumerator::DataFlow::Capture, ComIMMDeviceEnumerator::Role::Communications );
		}
	}
#elif defined(__APPLE__)
    auto com = AudioDeviceInterface::GetInstance( );
    if( com )
    {
		com->OnDefaultInputDeviceChanged.connect( [=]( )
        {
            auto microphone = com->GetDefaultInputDevice();
            OnDefaultMicrophoneChanged( microphone );
        } );
    }
#endif	

	return nullptr;
}
*/

void MicMuter::DefaultMicrophoneChanged( )
{
    m_Microphone = m_MicManager->GetDefaultMicrophone( );
    
	if( m_Microphone )
	{
        m_Microphone->OnMuted.connect( std::bind( &MicMuter::MicrophoneMuted, this, std::placeholders::_1 ) );
		MicrophoneMuted( m_Microphone->IsMuted( ) );
	}
}

void MicMuter::MicrophoneMuted( bool muted )
{
    if( m_GUI->IsScrollLockSet( ) )
		KeyboardHook::GetInstance( )->SetScrollLock( muted );

    m_GUI->SetIcon( muted );
}
