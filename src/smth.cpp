#include <windows.h>

#include "curl.h"
#include "smth.h"

#define CURL_APIENTRY

typedef CURL* (CURL_APIENTRY* PFN_CURL_EASY_INIT) ( void );
typedef void  (CURL_APIENTRY* PFN_CURL_EASY_CLEANUP) ( CURL* handle );
typedef CURLcode (CURL_APIENTRY* PFN_CURL_EASY_SETOPT) (CURL *handle, CURLoption option, ...);
typedef CURLcode (CURL_APIENTRY* PFN_CURL_EASY_PERFORM) (CURL * easy_handle );
typedef struct curl_slist*(CURL_APIENTRY* PFN_CURL_SLIST_APPEND) (struct curl_slist * list, const char * string );
typedef void (CURL_APIENTRY* PFN_CURL_SLIST_FREE_ALL) (struct curl_slist * list);

static struct {

	void* module;

	PFN_CURL_EASY_INIT    curl_easy_init;
	PFN_CURL_EASY_CLEANUP curl_easy_cleanup;
	PFN_CURL_EASY_SETOPT  curl_easy_setopt;
	PFN_CURL_EASY_PERFORM curl_easy_perform;

	PFN_CURL_SLIST_APPEND   curl_slist_append;
	PFN_CURL_SLIST_FREE_ALL curl_slist_free_all;
} gsCurl;

static struct {
	CURL* curl;
} gsSmth;

static bool Smth_InitCurl( void )
{
	gsCurl.module = LoadLibraryA( "libcurl.dll" );
	if ( gsCurl.module == nullptr ) {
		return false;
	}

	gsCurl.curl_easy_init    = (PFN_CURL_EASY_INIT)GetProcAddress(    (HMODULE)gsCurl.module, "curl_easy_init"    );
	gsCurl.curl_easy_cleanup = (PFN_CURL_EASY_CLEANUP)GetProcAddress( (HMODULE)gsCurl.module, "curl_easy_cleanup" );
	gsCurl.curl_easy_setopt  = (PFN_CURL_EASY_SETOPT)GetProcAddress(  (HMODULE)gsCurl.module, "curl_easy_setopt"  );
	gsCurl.curl_easy_perform = (PFN_CURL_EASY_PERFORM)GetProcAddress( (HMODULE)gsCurl.module, "curl_easy_perform" );

	gsCurl.curl_slist_append   = (PFN_CURL_SLIST_APPEND)GetProcAddress(   (HMODULE)gsCurl.module, "curl_slist_append"   );
	gsCurl.curl_slist_free_all = (PFN_CURL_SLIST_FREE_ALL)GetProcAddress( (HMODULE)gsCurl.module, "curl_slist_free_all" );

	return true;
}

static void Smth_DeinitCurl( void )
{
	gsCurl.curl_easy_init    = nullptr;
	gsCurl.curl_easy_cleanup = nullptr;
	gsCurl.curl_easy_setopt  = nullptr;
	gsCurl.curl_easy_perform = nullptr;

	gsCurl.curl_slist_append   = nullptr;
	gsCurl.curl_slist_free_all = nullptr;

	if ( gsCurl.module != nullptr ) {
		FreeLibrary( (HMODULE)gsCurl.module );
	}
	gsCurl.module = nullptr;
}


bool Smth_Init( void )
{
	if ( Smth_InitCurl() ) {
		gsSmth.curl = gsCurl.curl_easy_init();
		if ( gsSmth.curl != nullptr ) {
			curl_slist* chunk = nullptr;
			chunk = gsCurl.curl_slist_append( chunk, "Accept:" );

			gsCurl.curl_easy_setopt( gsSmth.curl, CURLOPT_HTTPHEADER, chunk );
			gsCurl.curl_easy_setopt( gsSmth.curl, CURLOPT_URL, "m.newsmth.net" );
			gsCurl.curl_easy_setopt( gsSmth.curl, CURLOPT_VERBOSE, 0 );

			CURLcode res = gsCurl.curl_easy_perform( gsSmth.curl );

			gsCurl.curl_slist_free_all( chunk );

			return true;
		}
	}
	return false;
}

void Smth_Deinit( void )
{
	if ( gsSmth.curl != nullptr ) {
		gsCurl.curl_easy_cleanup( gsSmth.curl );
	}
	Smth_DeinitCurl();
}
