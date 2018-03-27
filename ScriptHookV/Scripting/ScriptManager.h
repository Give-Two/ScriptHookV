#ifndef __SCRIPT_MANAGER_H__
#define __SCRIPT_MANAGER_H__

#include "ScriptEngine.h"
#include "NativeInvoker.h"

class Script {
public:

	inline Script( void( *function )( ) ) {

		scriptFiber = nullptr;
		callbackFunction = function;
		wakeAt = timeGetTime();
	}

	inline ~Script() {

		if ( scriptFiber ) {
			DeleteFiber( scriptFiber );
		}
	}

	void Tick();

	void Wait( uint32_t time );

	inline void( *GetCallbackFunction() )( ) {

		return callbackFunction;
	}

private:

	HANDLE			scriptFiber;
	uint32_t		wakeAt;
	void( *callbackFunction )( );

	void			Run();
};

typedef std::map<HMODULE,std::shared_ptr<Script>> scriptMap;

class ScriptManagerThread {
private:

	scriptMap				m_scripts;
	scriptMap				m_additional;

public:

	virtual void			DoRun();
	virtual void			Reset();
	void					AddScript( HMODULE module, void( *fn )( ));
	void					AddAdditional(HMODULE module, void(*fn)());
	void					RemoveScript( void( *fn )( ) );
	void					RemoveScript( HMODULE module);
	void					RemoveAllScripts();
};

namespace ScriptManager {

	void					WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
}

extern ScriptManagerThread	g_ScriptManagerThread;
extern HANDLE				g_MainFiber;
#endif // __SCRIPT_MANAGER_H__
