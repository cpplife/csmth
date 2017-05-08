#include <windows.h>
#include <thread>
#include <vector>
#include <codecvt>
#include <locale>

#include "curl.h"
#include "net_util.h"

#define CURL_APIENTRY

typedef CURL* (CURL_APIENTRY* PFN_CURL_EASY_INIT) ( void );
typedef void  (CURL_APIENTRY* PFN_CURL_EASY_CLEANUP) ( CURL* handle );
typedef CURLcode (CURL_APIENTRY* PFN_CURL_EASY_SETOPT) (CURL *handle, CURLoption option, ...);
typedef CURLcode (CURL_APIENTRY* PFN_CURL_EASY_PERFORM) (CURL * easy_handle );
typedef struct curl_slist*(CURL_APIENTRY* PFN_CURL_SLIST_APPEND) (struct curl_slist * list, const char * string );
typedef void (CURL_APIENTRY* PFN_CURL_SLIST_FREE_ALL) (struct curl_slist * list);

static struct {

	void* libcurl;

	PFN_CURL_EASY_INIT    curl_easy_init;
	PFN_CURL_EASY_CLEANUP curl_easy_cleanup;
	PFN_CURL_EASY_SETOPT  curl_easy_setopt;
	PFN_CURL_EASY_PERFORM curl_easy_perform;

	PFN_CURL_SLIST_APPEND   curl_slist_append;
	PFN_CURL_SLIST_FREE_ALL curl_slist_free_all;


} gsNetInst;

bool Net_Init( void )
{
	gsNetInst.libcurl = LoadLibraryA( "libcurl.dll" );
	if ( gsNetInst.libcurl == nullptr ) {
		return false;
	}

	gsNetInst.curl_easy_init    = (PFN_CURL_EASY_INIT)GetProcAddress(    (HMODULE)gsNetInst.libcurl, "curl_easy_init"    );
	gsNetInst.curl_easy_cleanup = (PFN_CURL_EASY_CLEANUP)GetProcAddress( (HMODULE)gsNetInst.libcurl, "curl_easy_cleanup" );
	gsNetInst.curl_easy_setopt  = (PFN_CURL_EASY_SETOPT)GetProcAddress(  (HMODULE)gsNetInst.libcurl, "curl_easy_setopt"  );
	gsNetInst.curl_easy_perform = (PFN_CURL_EASY_PERFORM)GetProcAddress( (HMODULE)gsNetInst.libcurl, "curl_easy_perform" );

	gsNetInst.curl_slist_append   = (PFN_CURL_SLIST_APPEND)GetProcAddress(   (HMODULE)gsNetInst.libcurl, "curl_slist_append"   );
	gsNetInst.curl_slist_free_all = (PFN_CURL_SLIST_FREE_ALL)GetProcAddress( (HMODULE)gsNetInst.libcurl, "curl_slist_free_all" );


	return true;
}

void Net_Deinit( void )
{
	gsNetInst.curl_easy_init    = nullptr;
	gsNetInst.curl_easy_cleanup = nullptr;
	gsNetInst.curl_easy_setopt  = nullptr;
	gsNetInst.curl_easy_perform = nullptr;

	gsNetInst.curl_slist_append   = nullptr;
	gsNetInst.curl_slist_free_all = nullptr;

	if ( gsNetInst.libcurl != nullptr ) {
		FreeLibrary( (HMODULE)gsNetInst.libcurl );
	}
	gsNetInst.libcurl = nullptr;
}

static size_t Net_CurlWriteCallback( char* ptr, size_t size, size_t nmemb, void* userdata )
{
	std::vector<char>* pBuffer = (std::vector<char>*)userdata;
	pBuffer->insert( pBuffer->end(), ptr, ptr + size*nmemb );

	return size*nmemb;
}

std::wstring Net_Get( const std::string& url )
{
	if (gsNetInst.libcurl == nullptr) return std::wstring();

	std::vector<char> data;

	CURL* curl = gsNetInst.curl_easy_init();
	if ( curl != nullptr ) {
		curl_slist* chunk = nullptr;
		chunk = gsNetInst.curl_slist_append( chunk, "Accept:" );

		gsNetInst.curl_easy_setopt( curl, CURLOPT_HTTPHEADER, chunk );
		gsNetInst.curl_easy_setopt( curl, CURLOPT_URL, url.c_str() );
		gsNetInst.curl_easy_setopt( curl, CURLOPT_VERBOSE, 0 );

		gsNetInst.curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, Net_CurlWriteCallback );
		gsNetInst.curl_easy_setopt( curl, CURLOPT_WRITEDATA, &data );

		CURLcode res = gsNetInst.curl_easy_perform( curl );

		gsNetInst.curl_slist_free_all( chunk );

	}
	gsNetInst.curl_easy_cleanup( curl );

	std::string utf8_text = std::string( data.begin(), data.end() );

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes( utf8_text );
}
