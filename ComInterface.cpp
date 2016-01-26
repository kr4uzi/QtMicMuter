#include "ComInterface.h"
#include <cstring>
#include <Objbase.h>
#include <Mmdeviceapi.h>
#include <Endpointvolume.h>
using namespace std;

ComIMMDeviceEnumerator::Role Role_from_ERole( ERole role )
{
	switch( role )
	{
	case ERole::eCommunications:
		return ComIMMDeviceEnumerator::Role::Communications;
	case ERole::eConsole:
		return ComIMMDeviceEnumerator::Role::Console;
	case ERole::eMultimedia:
	default:
		return ComIMMDeviceEnumerator::Role::Multimedia;
	}
}

ComIMMDeviceEnumerator::DataFlow DataFlow_from_EDataFlow( EDataFlow flow )
{
	switch( flow )
	{
	case EDataFlow::eCapture:
		return ComIMMDeviceEnumerator::DataFlow::Capture;
	case EDataFlow::eRender:
		return ComIMMDeviceEnumerator::DataFlow::Render;
	case EDataFlow::eAll:
	default:
		return ComIMMDeviceEnumerator::DataFlow::All;
	}
}

ERole ERole_from_Role( ComIMMDeviceEnumerator::Role role )
{
	switch( role )
	{
	case ComIMMDeviceEnumerator::Role::Communications:
		return ERole::eCommunications;
	case ComIMMDeviceEnumerator::Role::Console:
		return ERole::eConsole;
	case ComIMMDeviceEnumerator::Role::Multimedia:
	default:
		return ERole::eMultimedia;
	}
}

EDataFlow EDataFlow_from_DataFlow( ComIMMDeviceEnumerator::DataFlow flow )
{
	switch( flow )
	{
	case ComIMMDeviceEnumerator::DataFlow::Capture:
		return EDataFlow::eCapture;
	case ComIMMDeviceEnumerator::DataFlow::Render:
		return EDataFlow::eRender;
	case ComIMMDeviceEnumerator::DataFlow::All:
	default:
		return EDataFlow::eAll;
	}
}

class CIAudioEndpointVolumeCallback : public IAudioEndpointVolumeCallback
{
private:
	ComIAudioEndpointVolume * m_This;
	ULONG m_Ref;

public:
	CIAudioEndpointVolumeCallback( ComIAudioEndpointVolume * nThis )
		: m_This( nThis ), m_Ref( 1 )
	{
	
	}

	~CIAudioEndpointVolumeCallback( )
	{
	
	}

	ULONG STDMETHODCALLTYPE AddRef( )
	{
		return InterlockedIncrement( &m_Ref );
	}

	ULONG STDMETHODCALLTYPE Release( )
	{
		ULONG ref = InterlockedDecrement( &m_Ref );

		if( ref == 0 )
			delete this;

		return ref;
	}

	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, VOID **ppvInterface )
	{
		if( IID_IUnknown == riid )
		{
			AddRef( );
			*ppvInterface = (IUnknown*)this;
		}
		else if( __uuidof(IAudioEndpointVolumeCallback) == riid )
		{
			AddRef( );
			*ppvInterface = (IAudioEndpointVolumeCallback*)this;
		}
		else
		{
			*ppvInterface = NULL;
			return E_NOINTERFACE;
		}

		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnNotify( PAUDIO_VOLUME_NOTIFICATION_DATA pNotify )
	{
        m_This->OnMuted( pNotify->bMuted ? true : false );
		return S_OK;
	}
};

class CIMMNotificationClient : public IMMNotificationClient
{
private:
	ComIMMDeviceEnumerator * m_This;
	ULONG m_Ref;

public:
	CIMMNotificationClient( ComIMMDeviceEnumerator * nThis )
		: m_This( nThis ), m_Ref( 1 )
	{
	
	}

	~CIMMNotificationClient( )
	{
	
	}

	ULONG STDMETHODCALLTYPE AddRef( )
	{
		return InterlockedIncrement( &m_Ref );
	}

	ULONG STDMETHODCALLTYPE Release( )
	{
		ULONG ref = InterlockedDecrement( &m_Ref );

		if( ref == 0 )
			delete this;

		return ref;
	}

	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, VOID **ppvInterface )
	{
		if( IID_IUnknown == riid )
		{
			AddRef( );
			*ppvInterface = (IUnknown*)this;
		}
		else if( __uuidof(IMMNotificationClient) == riid )
		{
			AddRef( );
			*ppvInterface = (IMMNotificationClient*)this;
		}
		else
		{
			*ppvInterface = NULL;
			return E_NOINTERFACE;
		}

		return S_OK;
	}

    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged( LPCWSTR, DWORD )
	{
		return S_OK;
	}

    HRESULT STDMETHODCALLTYPE OnDeviceAdded( LPCWSTR )
	{
		return S_OK;
	}

    HRESULT STDMETHODCALLTYPE OnDeviceRemoved( LPCWSTR )
	{
		return S_OK;
	}

    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged( EDataFlow flow, ERole role, LPCWSTR )
	{

        m_This->OnDefaultDeviceChanged( DataFlow_from_EDataFlow( flow ), Role_from_ERole( role ) );

		return S_OK;
	}

    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged( LPCWSTR, const PROPERTYKEY )
	{
		return S_OK;
	}
};

ComInterface* ComInterface::m_Interface = nullptr;

ComInterface::ComInterface( )
{
	if( FAILED( CoInitialize( nullptr ) ) )
		throw COMException( "ComInterface::ComInterface: Could not initialize COM interface!" );

	IMMDeviceEnumerator * deviceEnumerator = nullptr;
	if( FAILED( CoCreateInstance( __uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator ) ) )
		throw COMException( "ComInterface::GetDeviceEnumerator: Could not initialize device enumerator!" );

	m_DeviceEnumerator = std::shared_ptr<ComIMMDeviceEnumerator>( new ComIMMDeviceEnumerator( deviceEnumerator ) );
	m_DeviceEnumerator->OnDefaultDeviceChanged.connect( std::bind( &ComInterface::DefaultInputDeviceChangedProxy, this, std::placeholders::_1, std::placeholders::_2 ) );
}

ComInterface::~ComInterface( )
{
	CoUninitialize( );
}

void ComInterface::DefaultInputDeviceChangedProxy( ComIMMDeviceEnumerator::DataFlow flow, ComIMMDeviceEnumerator::Role role )
{
	if( flow == ComIMMDeviceEnumerator::DataFlow::Capture && role == ComIMMDeviceEnumerator::Role::Communications )
		OnDefaultInputDeviceChanged( );
}

ComInterface * ComInterface::GetInstance( )
{
	if( m_Interface == nullptr )
		m_Interface = new ComInterface( );

	return m_Interface;
}

shared_ptr<ComIMMDeviceEnumerator> ComInterface::GetDeviceEnumerator( )
{
	return m_DeviceEnumerator;
}

shared_ptr<IMicrophone> ComInterface::GetDefaultMicrophone( )
{
	return m_DeviceEnumerator->GetDefaultAudioEndpoint( ComIMMDeviceEnumerator::DataFlow::Capture, ComIMMDeviceEnumerator::Role::Communications );
}

ComIMMDeviceEnumerator::ComIMMDeviceEnumerator( void * nDeviceEnumerator )
	: m_DeviceEnumerator( nDeviceEnumerator )
{
	auto notificator = new CIMMNotificationClient( this );
	if( FAILED( static_cast<IMMDeviceEnumerator *>(m_DeviceEnumerator)->RegisterEndpointNotificationCallback( notificator ) ) )
	{
		delete notificator;
		
		throw COMException( "ComIMMDeviceEnumerator::ComIMMDeviceEnumerator: Could not register callback!" );
	}

	m_Notificator = notificator;
}

ComIMMDeviceEnumerator::~ComIMMDeviceEnumerator( )
{
	if( m_DeviceEnumerator )
	{
		auto notificator = (CIMMNotificationClient *)m_Notificator;
		if( notificator )
		{
			static_cast<IMMDeviceEnumerator *>(m_DeviceEnumerator)->UnregisterEndpointNotificationCallback( notificator );
			delete m_Notificator;
		}
	}
}

shared_ptr<ComIAudioEndpointVolume> ComIMMDeviceEnumerator::GetDefaultAudioEndpoint( DataFlow _flow, Role _role )
{
	ERole role = ERole_from_Role( _role );
	EDataFlow flow = EDataFlow_from_DataFlow( _flow );

	IAudioEndpointVolume * endpointVolume = nullptr;

	IMMDevice * defaultDevice;
	if( FAILED( static_cast<IMMDeviceEnumerator *>(m_DeviceEnumerator)->GetDefaultAudioEndpoint( flow, role, &defaultDevice ) ) )
		throw COMException( "ComIMMDeviceEnumerator::GetDefaultAudioEndpoint: Could not detect default input device!" );
	else
	{
		if( FAILED( defaultDevice->Activate( __uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, nullptr, (LPVOID *)&endpointVolume ) ) )
		{
			defaultDevice->Release( );
			throw COMException( "ComIMMDeviceEnumerator::GetDefaultAudioEndpoint: Could not initialize audio endpoint!" );
		}
	}

	if( FAILED( defaultDevice->Release( ) ) )
		throw COMException( "ComIMMDeviceEnumerator::GetDefaultAudioEndpoint: Could not cleanup!" );

	return shared_ptr<ComIAudioEndpointVolume>( new ComIAudioEndpointVolume( endpointVolume ) );
}

ComIAudioEndpointVolume::ComIAudioEndpointVolume( void * nEndpointVolume )
	: m_AudioEndpointVolume( nEndpointVolume )
{
	auto notificator = new CIAudioEndpointVolumeCallback( this );
	if( FAILED( static_cast<IAudioEndpointVolume *>(m_AudioEndpointVolume)->RegisterControlChangeNotify( notificator ) ) )
	{
		delete notificator;

		throw COMException( "Could not detect microphone\'s muted state!" );
	}

	m_Notificator = notificator;
}

ComIAudioEndpointVolume::~ComIAudioEndpointVolume( )
{
	if( m_AudioEndpointVolume )
	{
		auto notificator = static_cast<CIAudioEndpointVolumeCallback *>( m_Notificator );
		if( notificator )
		{
			static_cast<IAudioEndpointVolume *>( m_AudioEndpointVolume )->UnregisterControlChangeNotify( notificator );
			delete notificator;
		}
	}
}

bool ComIAudioEndpointVolume::IsMuted( ) const
{
	BOOL muted;
	if( FAILED( static_cast<IAudioEndpointVolume *>(m_AudioEndpointVolume)->GetMute( &muted ) ) )
		throw COMException( "Could not detect microphone\'s muted state!" );

	return muted ? true : false;
}

void ComIAudioEndpointVolume::SetMuted( bool muted )
{
	if( FAILED( static_cast<IAudioEndpointVolume *>(m_AudioEndpointVolume)->SetMute( muted, nullptr ) ) )
		throw COMException( "Could not set microphone\'s muted state!" );
}

COMException::COMException( const char * nWhat )
	: m_What( nullptr )
{
	if( nWhat )
	{
		auto len = strlen( nWhat );
		m_What = new char[ len + 1 ];
		strcpy_s( m_What, len + 1, nWhat );
		m_What[len] = '\0';
	}
}

COMException::~COMException( )
{
	delete m_What;
}

const char * COMException::what( ) const throw()
{
	return m_What ? m_What : "";
}
