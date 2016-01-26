#pragma once
#include <boost/signals2.hpp>
#include <vector>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>

class KeyboardHook
{
public:
	enum class KeyboardMessage
	{
		KEYDOWN,
		KEYUP,
		SYSKEYDOWN,
		SYSKEYUP
	};

	typedef unsigned int Keycode;

private:
	KeyboardHook( );
    ~KeyboardHook( );

	void * m_Hook;

	std::thread m_Thread;
    bool m_Ready = false;
	std::condition_variable m_Condition;
	std::mutex m_Mutex;

	static KeyboardHook * m_Instance;

public:
	static KeyboardHook * GetInstance( );

    boost::signals2::signal<void(KeyboardMessage, Keycode)> OnKeyPress;

	void Start( );
	void Stop( );

	bool GetScrollLock( );
	void SetScrollLock( bool flag );

private:
	void Loop( );
};
