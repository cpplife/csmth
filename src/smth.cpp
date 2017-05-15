#include <cassert>
#include <cstdlib>
#include <regex>
#include <iostream>
#include <cwchar>
#include <codecvt>
#include <locale>
#include <io.h>
#include <fcntl.h>
#include <conio.h>
#include <windows.h>

#include "net_util.h"
#include "tinyxml2.h"
#include "smth.h"


static struct {
} gsSmth;

static const std::string SMTH_DOMAIN = "m.newsmth.net";


static std::string Smth_ClearHtmlTags( const std::string& text )
{
	std::string s = text;
	std::smatch m;

	std::regex r( "<[^>]*?>", std::regex::ECMAScript );
	while ( std::regex_search( s, m, r ) ) {
		std::string pre = m.prefix();
		std::string suf = m.suffix();
		s = pre + suf;
	}
	return s;
}

static void Smth_GotoXY( int x, int y )
{
	if ( x < 0 || y < 0 ) return;

	COORD coord;
	coord.X = (SHORT)x;
	coord.Y = (SHORT)y;

	SetConsoleCursorPosition( GetStdHandle( STD_OUTPUT_HANDLE ),  coord );

}

static void Smth_SetPosMarker( int x, int y )
{
	Smth_GotoXY( x, y );
	wprintf( L">");
}

static void Smth_ClearPosMarker( int x, int y )
{
	Smth_GotoXY( x, y );
	wprintf( L" ");
}

static bool Smth_GetCursorXY( int& x, int& y )
{
	x = -1; y = -1;
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	if ( GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi) ) {

		x = csbi.dwCursorPosition.X;
		y = csbi.dwCursorPosition.Y;

		return true;
	}
	return false;
}

#if 0
static bool Smth_GetConsoleSize( int& columns, int& rows )
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}
#endif

enum SMTH_KEY {
	SK_NONE,
	SK_UP,
	SK_DOWN,
	SK_LEFT,
	SK_RIGHT,
	SK_ENTER,
	SK_H,
	SK_TAB,
	SK_QUIT,
	SK_CTRLC,
};

static int Smth_GetPressedKey( void )
{
	static int chars[ 16 ] = {};
	static int char_count = 0;

	chars[char_count] = _getch();
	char_count ++;

	if ( char_count == 2 ) {
		if ( chars[0] == 224 ) {
			char_count = 0;
			if ( chars[1] == 72 ) {
				return SK_UP;
			}
			if ( chars[1] == 80 ) {
				return SK_DOWN;
			}
			if ( chars[1] == 75 ) {
				return SK_LEFT;
			}
			if ( chars[1] == 77 ) {
				return SK_RIGHT;
			}
		}
	}
	else if ( char_count == 1 ) {
		if ( chars[0] != 224 && chars[0] != 0 ) {
			char_count = 0; 

			if ( chars[0] == 'H' ) return SK_H;
			if ( chars[0] == 0xD ) return SK_ENTER;
			if ( chars[0] == '!' ) return SK_QUIT;
			if ( chars[0] == 3 ) return SK_CTRLC;
			if ( chars[0] == 9 ) return SK_TAB;
		}
	}


	return SK_NONE;
}

std::wstring Smth_Utf8StringToWString( const std::string& text )
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes( text );
}

#if 0
static std::wstring Smth_ClearMetaTag( const std::wstring& text )
{
	std::wstring s = text;
	while ( true ) {
		size_t index = s.find( L"<meta" );
		if ( index != std::wstring::npos ) {

			size_t end = s.find( L">", index );
			assert( end != std::wstring::npos );

			s = s.substr( 0, index ) + s.substr( end + 1, s.length() - end - 1 );

		}
		else {
			break;
		}
	}

	return s;
}
#endif

static std::string Smth_ReplaceHtmlTags( const std::string& text )
{
	std::string s = text;

	static struct {
		const char* src;
		const char* dst;
	} tags[] = {
		{ "<br>",   "\n" },
		{ "<br/>",  "\n" },
		{ "<br />", "\n" },
		{ "&nbsp;", " "  },
	};


	int count = sizeof( tags ) / sizeof( tags[0] );

	for ( int i = 0; i < count; ++i ) {
		while ( true ) {
			size_t index = s.find( tags[i].src );
			if ( index != std::string::npos ) {

				size_t len = strlen( tags[i].src );

				s = s.replace( index, len, tags[i].dst );

			}
			else {
				break;
			}
		}
	}

	return s;
}

static std::vector<std::string> Smth_ExtractImgUrl( const std::string& text )
{
	std::vector<std::string> vec;

	std::string t = text;
	std::smatch m;
	std::regex r( "<img .+? src=\"(.+?)/middle\".*?/>", std::regex::ECMAScript );
	while ( std::regex_search( t, m, r ) ) {

		std::string imgUrl = m.str(1);
		vec.push_back( imgUrl );

		t = m.suffix();
	}

	return vec;
}

static std::string Smth_ProcessArticleContent( const std::string& text )
{
	std::string s = Smth_ReplaceHtmlTags( text );
	std::vector<std::string> urls = Smth_ExtractImgUrl( s );
	s = Smth_ClearHtmlTags( s );
	for ( size_t i = 0; i < urls.size(); ++i ) {
		s += urls[i] + "\n";
	}
	return s;
}

void Smth_OutputSectionPage( const SectionPage& page, LinkPositionState* state )
{
	std::wstring s = Smth_Utf8StringToWString(page.name);
	wprintf( L"=== %s ===\n", s.c_str() );

	for ( size_t i = 0; i < page.items.size(); ++i ) {
		s = Smth_Utf8StringToWString(page.items[i].title);
		int x, y;
		Smth_GetCursorXY( x, y );
		wprintf( L"  %s\n", s.c_str() );
		if ( state != nullptr && x >= 0 && y >= 0 ) {
			state->Append( x, y, SMTH_DOMAIN + page.items[i].url );
			state->posIndex = 0;
		}
	}

}

void Smth_OutputBoardPage( const BoardPage& page, LinkPositionState* state )
{
	std::wstring s = Smth_Utf8StringToWString(page.name_cn);
	std::wstring t = Smth_Utf8StringToWString(page.name_en);
	wprintf( L"=== %s(%s) ===\n", s.c_str(), t.c_str() );

	for ( size_t i = 0; i < page.items.size(); ++i ) {
		size_t index = page.items.size() - 1 - i;
		s = Smth_Utf8StringToWString(page.items[index].title);
		int x, y;
		Smth_GetCursorXY( x, y );
		wprintf( L"  %s\n", s.c_str() );

		if ( state != nullptr && x >= 0 && y >= 0 ) {
			state->Append( x, y, SMTH_DOMAIN + page.items[index].url );
			state->posIndex = 0;
		}
	}
}

void Smth_OutputArticlePage( const ArticlePage& page, LinkPositionState* state )
{
	std::wstring s = Smth_Utf8StringToWString(page.name);
	wprintf( L"=== %s ===\n", s.c_str() );
	int x, y;
	for ( size_t i = 0; i < page.items.size(); ++i ) {
		s = Smth_Utf8StringToWString(page.items[i].author);
		wprintf( L"-----------------------------------------------------------------------------\n" );
		Smth_GetCursorXY( x, y );
		wprintf( L"  %s\n", s.c_str() );
		wprintf( L"-----------------------------------------------------------------------------\n" );
		s = Smth_Utf8StringToWString(page.items[i].content);
		wprintf( L"%s\n", s.c_str() );
		wprintf( L"-----------------------------------------------------------------------------\n" );

		if ( state != nullptr && x >= 0 && y >= 0 ) {
			state->Append( x, y, SMTH_DOMAIN );
			state->posIndex = 0;
		}
	}
	// Added last line as the pos, so we can see last text when move cursor.
	Smth_GetCursorXY( x, y );
	if ( state != nullptr && x >= 0 && y >= 0 ) {
		state->Append( x, y, SMTH_DOMAIN );
		state->posIndex = 0;
	}
}

SectionPage Smth_GetSectionPage( const std::string& htmlText )
{
	SectionPage page;

	const char* beginTag = "<ul class=\"slist sec\">";
	const char* endTag = "</ul>";
	size_t index = htmlText.find( beginTag );
	if ( index != std::string::npos ) {
		size_t end = htmlText.find( endTag, index + 1 );
		if ( end != std::string::npos ) {
			
			std::string titleText = htmlText.substr( index, end - index );
			//
			std::smatch m;
			std::regex r( "<li[^>]*>.+?</li>", std::regex::ECMAScript );
			while ( std::regex_search( titleText, m, r ) ) {
				std::smatch um;
				std::regex title( "<li class=\"f\">(.*?)</li>", std::regex::ECMAScript );
				std::string mstr = m.str();
				if (std::regex_search( mstr, um, title ) ) {
					page.name = um.str(1);
				}
				else {
					TitleItem item;
					//std::regex rr( "<a href=\"(.*?)\">(.+?)(\\(.*?\\))</a>", std::regex::ECMAScript );
					std::regex rr( "<a href=\"(.*?)\">(.+?)</a>", std::regex::ECMAScript );
					std::string tt = m.str();
					if (std::regex_search( tt, um, rr ) ) {
						item.url   = um.str(1);
						item.title = Smth_ClearHtmlTags( um.str(2) );

						page.items.push_back( item );
					}
				}

				titleText = m.suffix();
			}
		}
	}
	return page;
}

BoardPage Smth_GetBoardPage( const std::string& htmlText )
{
	BoardPage page;

	const char* beginTag = "<ul class=\"list sec\">";
	const char* endTag = "</ul>";
	size_t index = htmlText.find( beginTag );
	if ( index != std::string::npos ) {
		size_t end = htmlText.find( endTag, index + 1 );
		if ( end != std::string::npos ) {
			
			std::string titleText = htmlText.substr( index, end - index );
			//
			std::smatch m;
			std::regex r( "<li[^>]*>.+?</li>", std::regex::ECMAScript );
			while ( std::regex_search( titleText, m, r ) ) {
				std::smatch um;
				std::string mstr = m.str();
				BoardItem item;
				r.assign( "<div><a href=\"(.*?)\".*?>(.+?)</a>", std::regex::ECMAScript );
				if (std::regex_search( mstr, um, r ) ) {
					item.url   = um.str(1);
					item.title = Smth_ClearHtmlTags( um.str(2) );
				}
				page.items.push_back( item );
				titleText = m.suffix();
			}
		}
	}
	return page;
}
ArticlePage Smth_GetArticlePage( const std::string& htmlText )
{
	ArticlePage page;

	const char* beginTag = "<ul class=\"list sec\">";
	const char* endTag = "</ul>";
	size_t index = htmlText.find( beginTag );
	if ( index != std::string::npos ) {
		size_t end = htmlText.find( endTag, index + 1 );
		if ( end != std::string::npos ) {
			std::string titleText = htmlText.substr( index, end - index );
			//
			std::smatch m;
			std::regex r( "<li[^>]*>.+?</li>", std::regex::ECMAScript );
			while ( std::regex_search( titleText, m, r ) ) {
				std::smatch um;
				std::regex title( "<li class=\"f\">(.*?)</li>", std::regex::ECMAScript );
				std::string mstr = m.str();
				if (std::regex_search( mstr, um, title ) ) {
					page.name = um.str(1);
				}
				else {
					ArticleItem item;
					std::string tt = m.str();
					std::regex rr( "<div><a class=\"plant\">(.+?)</div>", std::regex::ECMAScript );
					if (std::regex_search( tt, um, rr ) ) {
						item.author = Smth_ClearHtmlTags( um.str(1) );
					}
					rr.assign( "<div class=\"sp\">(.+?)</div>", std::regex::ECMAScript );
					if (std::regex_search( tt, um, rr ) ) {
						//item.content = Smth_ReplaceHtmlTags( um.str(1) );
						item.content = Smth_ProcessArticleContent( um.str(1) );
					}
					page.items.push_back( item );
				}
				titleText = m.suffix();
			}
		}
	}
	return page;
}

static std::string Smth_GetUrlCategory( const std::string& fullUrl )
{
	size_t index = fullUrl.find( SMTH_DOMAIN );
	if ( index != std::string::npos ) {
		size_t begin = index + SMTH_DOMAIN.length() + 1;
		size_t end = fullUrl.find( "/", begin );
		if ( end != std::string::npos ) {
			return fullUrl.substr( begin, end - begin );
		}
	}
	return "";
}

static void Smth_GotoUrl( const std::string& fullUrl, LinkPositionState* state=nullptr )
{
	system( "cls" );
	std::string cat = Smth_GetUrlCategory( fullUrl );
	std::string result;

	if ( state != nullptr ) {
		state->Clear();
	}

	if ( cat == "board" ) { 
		result = Net_Get( fullUrl );
		Smth_OutputBoardPage( Smth_GetBoardPage( result ), state );
	}
	else if ( cat == "article" ) { 
		result = Net_Get( fullUrl );
		Smth_OutputArticlePage( Smth_GetArticlePage( result ), state );
	}
	else {
		result = Net_Get( fullUrl );
		Smth_OutputSectionPage( Smth_GetSectionPage( result ), state );
	}
}

bool Smth_Init( void )
{
	if ( Net_Init() ) {
		_setmode(_fileno(stdout), _O_U16TEXT);
		return true;
	}
	return false;
}

void Smth_Deinit( void )
{
	Net_Deinit();
}

void Smth_Update( void )
{
	int c = 0;
	int i = 0;

	bool quit = false;

	LinkPositionState linkState;

	do {
		std::string result;
		switch ( i ) {
		case 0:
			Smth_GotoUrl( "m.newsmth.net", &linkState );
			break;
		case 1:
			Smth_GotoUrl( "m.newsmth.net/hot/1", &linkState );
			break;
		case 2:
			Smth_GotoUrl( "m.newsmth.net/hot/1", &linkState );
			wprintf(L"\n");
			break;
		case 3:
			Smth_GotoUrl( "http://m.newsmth.net/board/Universal", &linkState );
			wprintf(L"\n");
			break;
		case 4:
			system( "cls" );
			Smth_GotoUrl( "http://m.newsmth.net/article/Picture/1659069", &linkState );
			break;
		case 0xFF:
			Smth_GotoUrl( linkState.Url(), &linkState );
			break;
		default:
			break;
		}

		Smth_SetPosMarker( linkState.PosX(), linkState.PosY() );

		c = Smth_GetPressedKey();
		switch( c ) {
		case SK_H:
			i = 0;
			break;
		case SK_UP:
			Smth_ClearPosMarker( linkState.PosX(), linkState.PosY() );
			linkState.PrevPos();
			break;
		case SK_DOWN:
			Smth_ClearPosMarker( linkState.PosX(), linkState.PosY() );
			linkState.NextPos();
			break;
		case SK_LEFT:
			break;
		case SK_RIGHT:
			break;
		case SK_TAB:
			i += 1;
			i = i % 5;
			break;
		case SK_QUIT:
		case SK_CTRLC:
			quit = true;
			break;
		case SK_ENTER:
			if ( linkState.posIndex >= 0 ) {
				i = 0xFF;
			}
			break;
		default:
			i = -1;
			break;
		}

	} while ( !quit );
}
