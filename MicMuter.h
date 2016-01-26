#pragma once

#include "Microphone.h"
#include "KeyboardHook.h"
#include <mutex>

class MicMuterGUI;

class MicMuter
{
private:
	std::mutex m_Mutex;
    std::shared_ptr<IMicrophone> m_Microphone;

    MicMuterGUI * m_GUI;
	IMicrophoneManager * m_MicManager;

public:
    MicMuter( );
    ~MicMuter( );

private:
	void DefaultMicrophoneChanged( );
	void MicrophoneMuted( bool muted );

	//std::shared_ptr<IMicrophone> GetDefaultMicrophone( );
};
