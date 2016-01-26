#pragma once

#include "Microphone.h"
#include <memory>
#include <exception>
#include <boost/signals2.hpp>

class ComInterface;
class ComIMMDeviceEnumerator;

class COMException : public std::exception
{
private:
	char * m_What;

public:
	COMException( const char * nWhat );
	~COMException( );

	const char * what( ) const throw() override;
};

class ComIAudioEndpointVolume : public IMicrophone
{
private:
	void * m_AudioEndpointVolume, * m_Notificator;

private:
	friend class ComIMMDeviceEnumerator;
	ComIAudioEndpointVolume( void * nAudioEndpointVolume );

public:
	~ComIAudioEndpointVolume( );

	bool IsMuted( ) const override;
    void SetMuted( bool muted ) override;
};

class ComIMMDeviceEnumerator
{
public:
	enum class Role
	{
		Console,		//Games, system notification sounds, and voice commands.
		Multimedia,		//Music, movies, narration, and live music recording.
		Communications	//Voice communications (talking to another person).
	};

	enum class DataFlow
	{
		Render,			//Audio rendering stream. Audio data flows from the application to the audio endpoint device, which renders the stream.
		Capture,		//Audio capture stream. Audio data flows from the audio endpoint device that captures the stream, to the application.
		All				//Audio rendering or capture stream. Audio data can flow either from the application to the audio endpoint device, or from the audio endpoint device to the application.
	};

private:
    void * m_DeviceEnumerator, * m_Notificator;

private:
	friend class ComInterface;
	ComIMMDeviceEnumerator( void * nDeviceEnumerator );

public:
	~ComIMMDeviceEnumerator( );

	std::shared_ptr<ComIAudioEndpointVolume> GetDefaultAudioEndpoint( DataFlow flow, Role role );
    boost::signals2::signal<void( DataFlow, Role )> OnDefaultDeviceChanged;
};

class ComInterface : public IMicrophoneManager
{
private:
	static ComInterface * m_Interface;

	std::shared_ptr<ComIMMDeviceEnumerator> m_DeviceEnumerator;

private:
	ComInterface( );
	~ComInterface( );

	void DefaultInputDeviceChangedProxy( ComIMMDeviceEnumerator::DataFlow flow, ComIMMDeviceEnumerator::Role role );

public:
	static ComInterface * GetInstance( );

	std::shared_ptr<ComIMMDeviceEnumerator> GetDeviceEnumerator( );

	virtual std::shared_ptr<IMicrophone> GetDefaultMicrophone( ) override;
};
