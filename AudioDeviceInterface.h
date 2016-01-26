#pragma once
#include "Microphone.h"
#include <boost/signals2.hpp>
#include <vector>
#include <memory>
#include <exception>

class AudioInterfaceException : public std::exception
{
private:
    char * m_What;
    
public:
    AudioInterfaceException( const char * nWhat );
    ~AudioInterfaceException( );
    
    const char * what( ) const throw( ) override;
};

class AudioDeviceInterface
{
private:
    void * m_Listener;
    
    AudioDeviceInterface( );
    ~AudioDeviceInterface( );
    
    static AudioDeviceInterface * m_Instance;
    friend class DefaultInputDeviceListener;
    
public:
    static AudioDeviceInterface * GetInstance( );
    
    boost::signal<void()> OnDefaultInputDeviceChanged;
    std::shared_ptr<IMicrophone> GetDefaultInputDevice( );
};
