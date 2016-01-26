#pragma once
#include <memory>
#include <boost/signals2.hpp>

class IMicrophone
{
public:
	virtual bool IsMuted( ) const = 0;
	virtual void SetMuted( bool muted ) = 0;

    boost::signals2::signal<void(bool)> OnMuted;
};

class IMicrophoneManager
{
public:
	virtual std::shared_ptr<IMicrophone> GetDefaultMicrophone( ) = 0;

	boost::signals2::signal<void( )> OnDefaultInputDeviceChanged;
};
