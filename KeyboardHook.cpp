#include "KeyboardHook.h"
using namespace std;

#if defined(WIN32)
#include <Windows.h>

LRESULT CALLBACK LowLevelKeyboardProc( int nCode, WPARAM wParam, LPARAM lParam )
{
	if( nCode == HC_ACTION && !(((PKBDLLHOOKSTRUCT)lParam)->flags & LLKHF_INJECTED) )
	{
		KeyboardHook::KeyboardMessage msg;

		switch( wParam )
		{
		case WM_KEYDOWN:
			msg = KeyboardHook::KeyboardMessage::KEYDOWN;
			break;
		case WM_SYSKEYDOWN:
			msg = KeyboardHook::KeyboardMessage::SYSKEYDOWN;
			break;
		case WM_KEYUP:
			msg = KeyboardHook::KeyboardMessage::KEYUP;
			break;
		case WM_SYSKEYUP:
		default:
			msg = KeyboardHook::KeyboardMessage::SYSKEYUP;
			break;
		}

        KeyboardHook::GetInstance( )->OnKeyPress( msg, ((PKBDLLHOOKSTRUCT)lParam)->vkCode );
	}

	return CallNextHookEx( nullptr, nCode, wParam, lParam );
}
#elif defined(__APPLE__)
#include <ApplicationServices/ApplicationServices.h>
CGEventRef KeyboardHookCallback( CGEventTapProxy proxy, CGEventType type, CGEventRef event, void * refcon )
{
    KeyboardHook::KeyboardMessage msg;

    switch( type )
    {
    case kCGEventKeyDown:
        msg = KeyboardHook::KeyboardMessage::KEYDOWN;
        break;
    case kCGEventKeyUp:
    default:
        msg = KeyboardHook::KeyboardMessage::KEYUP;
        break;
    }

    auto keycode = CGEventGetIntegerValueField( event, kCGKeyboardEventKeycode );
    KeyboardHook::GetInstance( )->OnKeyPress( msg, (KeyboardHook::Keycode)keycode );

    return event;
}
#endif

KeyboardHook * KeyboardHook::m_Instance = nullptr;

KeyboardHook * KeyboardHook::GetInstance( )
{
	if( m_Instance == nullptr )
		m_Instance = new KeyboardHook( );

	return m_Instance;
}

KeyboardHook::KeyboardHook( )
	: m_Hook( nullptr )
{
	m_Thread = thread( &KeyboardHook::Loop, this );
}

KeyboardHook::~KeyboardHook( )
{
	Stop( );

#ifdef WIN32
	if( m_Hook )
		UnhookWindowsHookEx( (HHOOK)m_Hook );
#endif
}

void KeyboardHook::Start( )
{
	unique_lock<mutex> lock( m_Mutex );
	m_Ready = true;
	m_Condition.notify_all( );
}

void KeyboardHook::Stop( )
{
	m_Ready = false;
}

void KeyboardHook::Loop( )
{
	unique_lock<mutex> lock( m_Mutex );
	while( !m_Ready )
		m_Condition.wait( lock );

#if defined(WIN32)
	m_Hook = SetWindowsHookEx( WH_KEYBOARD_LL, LowLevelKeyboardProc, nullptr, 0 );
	if( m_Hook )
	{
		MSG msg;

		while( m_Ready && GetMessage( &msg, nullptr, 0, 0 ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
	}
#elif defined(__APPLE__)
    CGEventMask eventMask = CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp);
    CFMachPortRef eventTap = CGEventTapCreate( kCGSessionEventTap, kCGHeadInsertEventTap, 0, eventMask, KeyboardHookCallback, NULL );

    if( !eventTap )
    {
        fprintf(stderr, "failed to create eventtap!");
        return;
    }


    CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource( kCFAllocatorDefault, eventTap, 0) ;
    CFRunLoopAddSource( CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes );


    CGEventTapEnable(eventTap, true);


    CFRunLoopRun();
#endif
}

bool KeyboardHook::GetScrollLock( )
{
#ifdef WIN32
	return GetKeyState( VK_SCROLL ) == 1;
#else
	return false;
#endif
}

void KeyboardHook::SetScrollLock( bool flag )
{
	bool locked = GetScrollLock( );

	if( flag ^ locked )
	{
#if defined( WIN32 )
        INPUT input[2];
        ZeroMemory( input, sizeof( input ) );
        input[0].type = input[1].type = INPUT_KEYBOARD;
        input[0].ki.wVk = input[1].ki.wVk = VK_SCROLL;
        input[1].ki.dwFlags = KEYEVENTF_KEYUP;

        SendInput( 2, input, sizeof( INPUT ) );
#elif defined( __APPLE__ )
        CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateCombinedSessionState);
        CGEventRef saveCommandDown = CGEventCreateKeyboardEvent(source, (CGKeyCode)0x91, true);
        CGEventRef saveCommandUp = CGEventCreateKeyboardEvent(source, (CGKeyCode)0x91, false);

        CGEventPost(kCGAnnotatedSessionEventTap, saveCommandDown);
        CGEventPost(kCGAnnotatedSessionEventTap, saveCommandUp);

        CFRelease(saveCommandUp);
        CFRelease(saveCommandDown);
        CFRelease(source);
#endif
	}
}
