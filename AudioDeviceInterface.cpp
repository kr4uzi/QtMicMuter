#include "AudioDeviceInterface.h"
#include <CoreAudio/CoreAudio.h>
#include <cstring>
using namespace std;

AudioDeviceInterface * AudioDeviceInterface::m_Instance = nullptr;

AudioDeviceInterface * AudioDeviceInterface::GetInstance( )
{
    if( m_Instance == nullptr )
        m_Instance = new AudioDeviceInterface( );
    
    return m_Instance;
}

class DefaultInputDeviceListener
{
private:
    AudioDeviceInterface * m_This;
    
public:
    static const AudioObjectPropertyAddress DeviceChangePropertyAddress;
    
    DefaultInputDeviceListener( AudioDeviceInterface * nThis )
        : m_This( nThis )
    {
        auto result = AudioObjectAddPropertyListener(kAudioObjectSystemObject, &DeviceChangePropertyAddress, &OnDefaultDeviceChanged, this);
        if( result != noErr )
            throw AudioInterfaceException( "Unable to add DefaultInputDeviceChanged listener!" );
    }
    
    DefaultInputDeviceListener( )
    {
        auto result = AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &DeviceChangePropertyAddress, &OnDefaultDeviceChanged, this );
        if( result != noErr )
            throw AudioInterfaceException( "Unable to remove DefaultInputDeviceChanged listener!" );
    }
    
private:
    static OSStatus OnDefaultDeviceChanged( AudioObjectID object, UInt32 num_addresses, const AudioObjectPropertyAddress addresses[], void * context )
    {
        if( object != kAudioObjectSystemObject )
            return noErr;
        
        for( UInt32 i = 0; i < num_addresses; ++i )
        {
            if( addresses[i].mSelector == DeviceChangePropertyAddress.mSelector &&
               addresses[i].mScope == DeviceChangePropertyAddress.mScope &&
               addresses[i].mElement == DeviceChangePropertyAddress.mElement &&
               context )
            {
                auto self = static_cast<DefaultInputDeviceListener *>(context);
                self->m_This->OnDefaultInputDeviceChanged( );
                
                break;
            }
        }
        
        return noErr;
    }
};

const AudioObjectPropertyAddress DefaultInputDeviceListener::DeviceChangePropertyAddress = {
    kAudioHardwarePropertyDefaultInputDevice,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster
};

class AudioInputDevice : public IMicrophone
{
private:
    AudioDeviceID m_DeviceID;
	Float32 m_Volume;
    
    static const AudioObjectPropertyAddress AudioInputDeviceMutedAddress;
	static const AudioObjectPropertyAddress AudioInputDeviceVolumeAddress;
    
    static OSStatus OnAudioInputDeviceMuted( AudioObjectID object, UInt32 num_addresses, const AudioObjectPropertyAddress addresses[], void * context )
    {
        auto self = static_cast<AudioInputDevice *>( context );
        
        if( object != self->m_DeviceID )
            return noErr;
        
        self->OnMuted( self->IsMuted( ) );
        
        return noErr;
    }

	static OSStatus OnAudioInputDeviceVolumeChanged( AudioObjectID object, UInt32 num_addresses, const AudioObjectPropertyAddress addresses[], void * context )
	{
		auto self = static_cast<AudioInputDevice *>(context);

		if( object != self->m_DeviceID )
			return noErr;

		self->m_Volume = self->GetVolume( );

		return noErr;
	}
    
public:
    AudioInputDevice( AudioDeviceID nDeviceID )
		: m_DeviceID( nDeviceID ), m_Volume( GetVolume( ) )
    {
		auto result = AudioObjectAddPropertyListener( m_DeviceID, &AudioInputDeviceMutedAddress, &OnAudioInputDeviceMuted, this );
        if( result != noErr )
            throw AudioInterfaceException( "Unable to add InputDeviceMuted listener!" );

		result = AudioObjectAddPropertyListener( m_DeviceID, &AudioInputDeviceVolumeAddress, &OnAudioInputDeviceVolumeChanged, this );
		if( result != noErr )
			throw AudioInterfaceException( "Unable to add InputDeviceVolume listener!" );
    }
    
    ~AudioInputDevice( )
    {
        auto result = AudioObjectRemovePropertyListener(m_DeviceID, &AudioInputDeviceMutedAddress, &OnAudioInputDeviceMuted, this );
        if( result != noErr )
            throw AudioInterfaceException( "Unable to remove InputDeviceMuted listener!" );

		result = AudioObjectRemovePropertyListener( m_DeviceID, &AudioInputDeviceVolumeAddress, &OnAudioInputDeviceVolumeChanged, this );
		if( result != noErr )
			throw AudioInterfaceException( "Unable to remove InputDeviceVolume listener!" );
    }

	Float32 GetVolume( ) const
	{
		Float32 volume;
		UInt32 volume_size = sizeof( UInt32 );

		auto error = AudioObjectGetPropertyData( m_DeviceID, &AudioInputDeviceVolumeAddress, 0, nullptr, &volume_size, &volume );
		if( error != noErr )
			throw AudioInterfaceException( "Could not detect input device volume!" );

		return volume;
	}

	void SetVolume( Float32 volume )
	{
		auto error = AudioObjectSetPropertyData( m_DeviceID, &address, 0, nullptr, sizeof( Float32 ), &volume );
		if( error != noErr )
			throw AudioInterfaceException( "Could not mute InputDevice!" );
	}
    
    bool IsMuted( ) const override
    {
        UInt32 muted = 0;
        UInt32 bool_size = sizeof(UInt32);
        
        AudioObjectPropertyAddress address = {
            kAudioDevicePropertyMute,
            kAudioDevicePropertyScopeInput
        };
        
        auto error = AudioObjectGetPropertyData( m_DeviceID, &address, 0, nullptr, &bool_size, &muted );
        if( error != noErr )
            throw AudioInterfaceException( "Could not detect input device muted state!" );
        
        return muted;
    }
    
    void SetMuted( bool mute ) override
    {
		if( mute )
		{
			m_Volume = GetVolume( );
			SetVolume( 0.0 );
		}
		else
			SetVolume( m_Volume );
    }
    
    boost::signals2::signal<void(bool)> OnMuted;
};

const AudioObjectPropertyAddress AudioInputDevice::AudioInputDeviceMutedAddress = {
    kAudioDevicePropertyMute,
    kAudioDevicePropertyScopeInput
};

const AudioObjectPropertyAddress AudioInputDevice::AudioInputDeviceVolumeAddress = {
	kAudioDevicePropertyVolumeScalar,
	kAudioDevicePropertyScopeInput
}


AudioDeviceInterface::AudioDeviceInterface( )
{
    m_Listener = new DefaultInputDeviceListener( this );
}

AudioDeviceInterface::~AudioDeviceInterface( )
{
    if( m_Listener )
        delete (DefaultInputDeviceListener *)m_Listener;
}

shared_ptr<IMicrophone> AudioDeviceInterface::GetDefaultInputDevice( )
{
    AudioDeviceID DefaultInputDeviceID = 0;
    UInt32 DeviceIDSize = sizeof(AudioDeviceID);
    
    auto error = AudioObjectGetPropertyData( kAudioObjectSystemObject, &DefaultInputDeviceListener::DeviceChangePropertyAddress, 0, NULL, &DeviceIDSize, &DefaultInputDeviceID );
    if( error != noErr )
        throw AudioInterfaceException( "Could not detect Default Input Device" );
    
    return make_shared<AudioInputDevice>( DefaultInputDeviceID );
}

AudioInterfaceException::AudioInterfaceException( const char * nWhat )
    : m_What( nullptr )
{
    if( nWhat )
    {
        auto len = strlen(nWhat);
        m_What = new char[ len + 1 ];
        strcpy( m_What, nWhat );
        m_What[len] = '\0';
    }
}

AudioInterfaceException::~AudioInterfaceException( )
{
    delete m_What;
}

const char * AudioInterfaceException::what( ) const throw( )
{
    if( m_What )
        return m_What;
    
    return "";
}


