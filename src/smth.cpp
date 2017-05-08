#include <cassert>
#include <cstdlib>

#include "net_util.h"
#include "tinyxml2.h"
#include "smth.h"


static struct {
} gsSmth;

static std::wstring Smth_ClearMetaTag( const std::wstring& text )
{
	std::wstring s = text;
	while ( true ) {
		int index = s.find( L"<meta" );
		if ( index != std::wstring::npos ) {

			int end = s.find( L">", index );
			assert( end != std::wstring::npos );

			s = s.substr( 0, index ) + s.substr( end + 1, s.length() - end - 1 );

		}
		else {
			break;
		}
	}

	return s;
}


bool Smth_Init( void )
{
	if ( Net_Init() ) {
		std::wstring result = Net_Get( "m.newsmth.net" );
		result = Smth_ClearMetaTag( result );

		tinyxml2::XMLDocument doc;
		doc.Parse( (const char*)result.c_str(), result.length()*2 );
		tinyxml2::XMLPrinter printer( 0, true );
		doc.Print( &printer );
		doc.SaveFile( "m.html" );


		printf( printer.CStr() );
		

		return true;
	}
	return false;
}

void Smth_Deinit( void )
{
	Net_Deinit();
}
