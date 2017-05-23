#include <cassert>
#include <cstdlib>
#include <regex>
#include <iostream>
#include <cwchar>
#include <codecvt>
#include <locale>
#include <stack>
#include <io.h>
#include <fcntl.h>
#include <conio.h>
#include <windows.h>

#include "net_util.h"
#include "tinyxml2.h"
#include "smth.h"


static const char* SMTH_HOMEPAGES[] = {
	"m.newsmth.net/section",
	"m.newsmth.net",
	"m.newsmth.net/hot/1",
	"m.newsmth.net/hot/2",
	"m.newsmth.net/hot/3",
	"m.newsmth.net/hot/4",
	"m.newsmth.net/hot/5",
	"m.newsmth.net/hot/6",
	"m.newsmth.net/hot/7",
	"m.newsmth.net/hot/8",
	"m.newsmth.net/hot/9",
};

static const int SMTH_HOMEPAGE_COUNT = sizeof(SMTH_HOMEPAGES)/sizeof(SMTH_HOMEPAGES[0]);

static struct SmthModule {
	std::stack<std::string> urlStack;
	std::string gotoUrl;

	ArticlePage article;
	BoardPage   board;
	SectionPage section;

	PageView    view;

	// Added class name and ctor/dtor to avoid compiling error (c2280 in windows)
	SmthModule() {
	}
	~SmthModule() {
	}

} gsSmth;

static const std::string SMTH_DOMAIN = "m.newsmth.net";


static int Smth_GetHomePageIndex( const std::string& url )
{
	for ( int i = 0; i < SMTH_HOMEPAGE_COUNT; ++i ) {
		if ( url == SMTH_HOMEPAGES[i] ) {
			return i;
		}
	}
	return -1;
}

static bool Smth_IsHomePageUrl( const std::string& url )
{
	for ( int i = 0; i < SMTH_HOMEPAGE_COUNT; ++i ) {
		if ( url == SMTH_HOMEPAGES[i] ) {
			return true;
		}
	}
	return false;
}

static bool Smth_IsSubPageUrl( const std::string& fullUrl )
{
	size_t index = fullUrl.rfind( "?p=" );
	if ( index != std::string::npos ) {
		return true;
	}
	return false;
}


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
	SK_SPACE,
	SK_H,
	SK_TAB,
	SK_STAB,
	SK_QUIT,
	SK_CTRLC,
	SK_NEXTPAGE,
	SK_PREVPAGE,
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

static int Smth_CheckPressedKey( void ) {
	INPUT_RECORD record;
	DWORD recNum;
	if ( ReadConsoleInput( GetStdHandle(STD_INPUT_HANDLE), &record, 1, &recNum ) ) {
		if ( record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown ) {
			WORD key = record.Event.KeyEvent.wVirtualKeyCode;
			bool ctrlPressed = ( (record.Event.KeyEvent.dwControlKeyState) & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
			bool altPressed = ( (record.Event.KeyEvent.dwControlKeyState) & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) != 0;
			bool capsOn = ( (record.Event.KeyEvent.dwControlKeyState) & (CAPSLOCK_ON)) != 0;
			bool shiftPressed = ( (record.Event.KeyEvent.dwControlKeyState) & (SHIFT_PRESSED)) != 0;

			if ( !ctrlPressed && !altPressed ) {
				switch( key )
				{
				case 0x48:
					if ( ( (!capsOn && shiftPressed) || (capsOn && !shiftPressed) ) ) {
						return SK_H;
					}
					break;
				case VK_SPACE:
					return SK_SPACE;
				case VK_RETURN:
					return SK_ENTER;
				case VK_TAB:
					return shiftPressed ? SK_STAB : SK_TAB;
				case VK_UP:
					return SK_UP;
				case VK_DOWN:
					return SK_DOWN;
				case VK_LEFT:
					return SK_LEFT;
				case VK_RIGHT:
					return SK_RIGHT;
				case VK_NEXT:
					return SK_NEXTPAGE;
				case VK_PRIOR:
					return SK_PREVPAGE;
				case 0x31:
					if ( shiftPressed ) { // ! key
						return SK_QUIT;
					}
					break;
				case 0x43:
					if ( ctrlPressed ) { // ctrl c
						return SK_QUIT;
					}
					break;
				default:
					break;
				}
			}
		}
	}

	return SK_NONE;

};

static void Smth_PrintLn( VIEWLINE_TYPE type, const std::wstring& text )
{
	HANDLE h = GetStdHandle( STD_OUTPUT_HANDLE );
	WORD wOldColorAttrs;
	CONSOLE_SCREEN_BUFFER_INFO csbiInfo;

	GetConsoleScreenBufferInfo(h, &csbiInfo);
	wOldColorAttrs = csbiInfo.wAttributes;

	switch( type ) {
	case FROM: // Dark red
		SetConsoleTextAttribute( h, FOREGROUND_RED );
		break;
	case REFER_AUTHOR: // Dark yellow
		SetConsoleTextAttribute( h, FOREGROUND_RED | FOREGROUND_GREEN );
		break;
	case REFER:
	case REFER_MORE: // Dark magenta
		SetConsoleTextAttribute( h, FOREGROUND_GREEN | FOREGROUND_BLUE );
		break;
	case ITEM_TOP:
		SetConsoleTextAttribute( h, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN );
		break;
	default:
		break;
	}

	wprintf( L"  %s\n", text.c_str() );
	SetConsoleTextAttribute( h, wOldColorAttrs );
}

std::wstring Smth_Utf8StringToWString( const std::string& text )
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes( text );
}

static std::string Smth_IntToUtf8String( int c )
{
	std::string s;
	if ( c < 0x80 ) {
		s.push_back( ( char)c );
	} else if ( c < 0x800 ) { // 11 bits
		s.push_back( (char)( 0xC0 | ( c >> 6 ) ) );
		s.push_back( (char)( 0x80 | ( c & 0x3F ) ) );
	} else if ( c < 0x10000 ) { // 16 bits
		s.push_back( (char)( 0xE0 | ( c >> 12 ) ) );
		s.push_back( (char)( 0x80 | ( ( c >> 6 ) & 0x3F ) ) );
		s.push_back( (char)( 0x80 | ( c & 0x3F ) ) );
	} else if ( c < 0x200000 ) { // 21 bits
		s.push_back( (char)( 0xF0 | ( c >> 18 ) ) );
		s.push_back( (char)( 0x80 | ( ( c >> 12 ) & 0x3F ) ) );
		s.push_back( (char)( 0x80 | ( ( c >> 6 ) & 0x3F ) ) );
		s.push_back( (char)( 0x80 | ( c & 0x3F ) ) );
	} else {
		s.push_back( '?' );
	}
	return s;
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

static std::string Smth_ReplaceHtmlEntities( const std::string& text )
{
	std::string s = text;

	static struct {
		const char* src;
		const char* dst;
	} tags[] = {
		{ "&nbsp;", " "  },
		{ "&lt;",   "<"  },
		{ "&gt;",   ">"  },
		{ "&amp;",  "&"  },
		{ "&quot;", "\"" },
		{ "&apos;", "'"  },
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

	while ( true ) {
		size_t index = s.find( "&#" );
		if ( index != std::string::npos ) {
			size_t end = s.find( ";", index );
			if ( end != std::string::npos ) {
				std::string n = s.substr( index + 2, end + 1 - index - 2 );
				int c = std::stoi( n );
				s = s.replace( index, end + 1 - index, Smth_IntToUtf8String( c ) );
			}
		}
		else {
			break;
		}
	}

	return s;
}

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

static std::string Smth_ParseHtml( const std::string& text )
{
	return Smth_ReplaceHtmlTags( Smth_ReplaceHtmlEntities( text ) );
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
	std::string s = Smth_ParseHtml( text );
	std::vector<std::string> urls = Smth_ExtractImgUrl( s );
	s = Smth_ClearHtmlTags( s );
	for ( size_t i = 0; i < urls.size(); ++i ) {
		s += urls[i] + "\n";
	}
	return s;
}

void Smth_CreateViewFromArticlePage( const ArticlePage& page, PageView& view )
{
	view.Clear();
	for ( size_t i = 0; i < page.items.size(); ++i ) {
		view.ParseArticle( page.items[i].author + "\n\n" + page.items[i].content );
	}
	view.SetItemIndex( 0 );
}


void Smth_OutputSectionPage( const SectionPage& page, LinkPositionState* state )
{
	std::wstring s = Smth_Utf8StringToWString(page.name);
	wprintf( L"  === %s ===\n", s.c_str() );

	for ( size_t i = 0; i < page.items.size(); ++i ) {
		s = Smth_Utf8StringToWString(page.items[i].title);
		int x, y;
		Smth_GetCursorXY( x, y );
		std::wstring t = L"";
		if ( page.items[i].type == "section" ) {
			t = L"\uFF0B";
			//t = L"+";
		}
		else if ( page.items[i].type == "board" ) {
			t = L"\u25C6";
			//t = L"*";
		}
		wprintf( L"  %1s  %s\n", t.c_str(), s.c_str() );
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
	wprintf( L"  === %s(%s) ===\n", s.c_str(), t.c_str() );

	for ( size_t i = 0; i < page.items.size(); ++i ) {
		//size_t index = page.items.size() - 1 - i;
		size_t index = i;
		s = Smth_Utf8StringToWString(page.items[index].title);
		int x, y;
		Smth_GetCursorXY( x, y );
		std::wstring time = Smth_Utf8StringToWString(page.items[index].replier_time);
		t = Smth_Utf8StringToWString(page.items[index].author);
		if ( t.length() == 5 ) {
			if ( (int)t[0] == 0x539F && (int)t[1] == 0X5E16 
					&& (int)t[2] == 0x5DF2 && (int)t[3] == 0x5220 && (int)t[4] == 0x9664 ) {
				t = L"[DELETED]";
			}
		}
		std::wstring top = page.items[index].is_top ? L"*" : L"";
		wprintf( L"  %1s %-12s %-10s %s\n", top.c_str(), t.c_str(), time.c_str(), s.c_str() );

		if ( state != nullptr && x >= 0 && y >= 0 ) {
			state->Append( x, y, SMTH_DOMAIN + page.items[index].url );
			state->posIndex = 0;
		}
	}
}

void Smth_OutputArticlePage( const ArticlePage& page, LinkPositionState* state )
{
	std::wstring s = Smth_Utf8StringToWString(page.name);
	wprintf( L"  === %s ===\n", s.c_str() );
	int x, y;
	for ( size_t i = 0; i < page.items.size(); ++i ) {
		s = Smth_Utf8StringToWString(page.items[i].author);
		wprintf( L"--------------------------------------------\n" );
		Smth_GetCursorXY( x, y );
		wprintf( L"  %s\n", s.c_str() );
		wprintf( L"--------------------------------------------\n" );
		s = Smth_Utf8StringToWString(page.items[i].content);
		wprintf( L"%s\n", s.c_str() );
		wprintf( L"--------------------------------------------\n" );

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

	// Get the board name
	std::smatch m;
	std::regex r( "<div class=\"menu sp\">.*?</a>\\|(.*?)</div>", std::regex::ECMAScript );
	if ( std::regex_search( htmlText, m, r ) ) {
		page.name = m.str(1);
	}

	const char* beginTag = "<ul class=\"slist sec\">";
	const char* endTag = "</ul>";
	size_t index = htmlText.find( beginTag );
	if ( index != std::string::npos ) {
		size_t end = htmlText.find( endTag, index + 1 );
		if ( end != std::string::npos ) {
			
			std::string titleText = htmlText.substr( index, end - index );
			//
			r.assign( "<li[^>]*>.+?</li>", std::regex::ECMAScript );
			while ( std::regex_search( titleText, m, r ) ) {
				std::smatch um;
				std::regex title( "<li class=\"f\">(.*?)</li>", std::regex::ECMAScript );
				std::string mstr = m.str();
				if (std::regex_search( mstr, um, title ) ) {
					page.name = um.str(1);
				}
				else {
					SectionItem item;
					//std::regex rr( "<a href=\"(.*?)\">(.+?)(\\(.*?\\))</a>", std::regex::ECMAScript );
					std::regex rr( "<a href=\"(.*?)\">(.+?)</a>", std::regex::ECMAScript );
					std::string tt = m.str();
					if (std::regex_search( tt, um, rr ) ) {
						item.url   = um.str(1);
						item.title = Smth_ParseHtml( Smth_ClearHtmlTags( um.str(2) ) );
						static const char* SectionTypes[] = {
							"section",
							"board",
							"article",
						};
						for ( size_t k = 0; k < sizeof( SectionTypes ) / sizeof( SectionTypes[0] ); ++k ) {
							if ( item.url.find( SectionTypes[k] ) != std::string::npos ) {
								item.type = SectionTypes[k];
								break;
							}
						}

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

	// Get the board name
	std::smatch m;
	std::regex r( "<div class=\"menu sp\">.*?</a>\\|(.*?)</div>", std::regex::ECMAScript );
	if ( std::regex_search( htmlText, m, r ) ) {
		std::string s = m.str(1);
		size_t i = s.find( '-' );
		if ( i != std::string::npos ) {
			size_t j = s.find( '(' );
			size_t k = s.find( ')' );
			page.name_cn = s.substr( i + 1, j - i - 1 );
			page.name_en = s.substr( j + 1, k - j - 1 );
		}
	}

	// Get page count.
	r.assign( "<form action.+</a>\\|<a class=\"plant\">(\\d+)/(\\d+)</a>", std::regex::ECMAScript );
	if ( std::regex_search( htmlText, m, r ) ) {
		page.pageIndex = std::stoi( m.str(1) );
		page.pageCount = std::stoi( m.str(2) );
	}

	const char* beginTag = "<ul class=\"list sec\">";
	const char* endTag = "</ul>";
	size_t index = htmlText.find( beginTag );
	if ( index != std::string::npos ) {
		size_t end = htmlText.find( endTag, index + 1 );
		if ( end != std::string::npos ) {
			
			std::string titleText = htmlText.substr( index, end - index );
			//
			r.assign( "<li[^>]*>.+?</li>", std::regex::ECMAScript );
			while ( std::regex_search( titleText, m, r ) ) {
				std::smatch um;
				std::string mstr = m.str();
				BoardItem item;
				item.is_top = false;
				std::regex rr( "<div><a href=\"(.*?)\".*?>(.+?)</a>", std::regex::ECMAScript );
				if (std::regex_search( mstr, um, rr ) ) {
					item.url   = um.str(1);
					item.title = Smth_ParseHtml( Smth_ClearHtmlTags( um.str(2) ) );
				}
				if ( mstr.find( "class=\"top\"" ) != std::string::npos ) {
					item.is_top = true;
				}

				static const char* PATERN[] = {
					"<div>(\\d{4}-\\d{2}-\\d{2})[&]nbsp;<a href=\"(.*?)\">(.+?)</a>\\|(\\d{4}-\\d{2}-\\d{2})[&]nbsp;<a href=\"(.+?)\">(.+?)</a></div>",
					"<div>(\\d{2}:\\d{2}:\\d{2})[&]nbsp;<a href=\"(.*?)\">(.+?)</a>\\|(\\d{2}:\\d{2}:\\d{2})[&]nbsp;<a href=\"(.+?)\">(.+?)</a></div>",
					"<div>(\\d{4}-\\d{2}-\\d{2})[&]nbsp;<a href=\"(.*?)\">(.+?)</a>\\|(\\d{2}:\\d{2}:\\d{2})[&]nbsp;<a href=\"(.+?)\">(.+?)</a></div>",
				};
				for ( size_t k = 0; k < sizeof( PATERN ) / sizeof( PATERN[0] ); ++k ) {
					rr.assign( PATERN[k], std::regex::ECMAScript );
					if (std::regex_search( mstr, um, rr ) ) {
						item.author_time  = um.str(1);
						item.author       = um.str(3);
						item.replier_time = um.str(4);
						item.last_replier = um.str(6);
					}
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

	// Get the board name
	std::smatch m;
	std::regex r( "<div class=\"menu sp\">.*?</a>\\|(.*?)</div>", std::regex::ECMAScript );
	if ( std::regex_search( htmlText, m, r ) ) {
		page.boardName = m.str(1);
	}
	// Get page count.
	r.assign( "<form action.+</a>\\|<a class=\"plant\">(\\d+)/(\\d+)</a>", std::regex::ECMAScript );
	if ( std::regex_search( htmlText, m, r ) ) {
		page.pageIndex = std::stoi( m.str(1) );
		page.pageCount = std::stoi( m.str(2) );
	}


	const char* beginTag = "<ul class=\"list sec\">";
	const char* endTag = "</ul>";
	size_t index = htmlText.find( beginTag );
	if ( index != std::string::npos ) {
		size_t end = htmlText.find( endTag, index + 1 );
		if ( end != std::string::npos ) {
			std::string titleText = htmlText.substr( index, end - index );
			//
			r.assign( "<li[^>]*>.+?</li>", std::regex::ECMAScript );
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

static std::string Smth_GetNextPageUrl( const std::string& fullUrl )
{
	size_t index = fullUrl.rfind( "?p=" );
	if ( index != std::string::npos ) {
		int currentPage = std::stoi( fullUrl.substr(index + 3) );
		return fullUrl.substr(0, index + 3) + std::to_string( currentPage + 1 );
	}
	return fullUrl + "?p=2";
}

static std::string Smth_GetPrevPageUrl( const std::string& fullUrl )
{
	size_t index = fullUrl.rfind( "?p=" );
	if ( index != std::string::npos ) {
		int currentPage = std::stoi( fullUrl.substr(index + 3) );
		if ( currentPage - 1 > 1 ) {
			return fullUrl.substr(0, index + 3) + std::to_string( currentPage - 1 );
		}
		else {
			return fullUrl.substr(0, index);
		}
	}
	return fullUrl;
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
		gsSmth.board = Smth_GetBoardPage( result );
		Smth_OutputBoardPage( gsSmth.board, state );
	}
	else if ( cat == "article" ) { 
		result = Net_Get( fullUrl );
		gsSmth.article = Smth_GetArticlePage( result );
		Smth_CreateViewFromArticlePage( gsSmth.article, gsSmth.view );
	}
	else {
		result = Net_Get( fullUrl );
		gsSmth.section = Smth_GetSectionPage( result );
		Smth_OutputSectionPage( gsSmth.section, state );
	}

	if ( gsSmth.urlStack.size() > 0 && gsSmth.urlStack.top() != fullUrl ) {
		if ( Smth_IsHomePageUrl( fullUrl ) ) {
			gsSmth.urlStack.pop();
		}
		if ( !Smth_IsSubPageUrl( fullUrl ) ) {
			gsSmth.urlStack.push( fullUrl );
		}
	}
	else if ( gsSmth.urlStack.size() == 0 ) {
		gsSmth.urlStack.push( fullUrl );
	}
}

bool Smth_Init( void )
{
	if ( Net_Init() ) {
		_setmode(_fileno(stdout), _O_U16TEXT);

		gsSmth.gotoUrl = SMTH_HOMEPAGES[0];
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

	bool quit = false;

	int artileIndex = -1;

	LinkPositionState linkState;

	std::string curUrl = "";
	std::string cat = "";
	do {

		if ( gsSmth.gotoUrl.length() > 0 && gsSmth.gotoUrl != curUrl ) {
			curUrl = gsSmth.gotoUrl;
			Smth_GotoUrl(gsSmth.gotoUrl, &linkState );
			gsSmth.gotoUrl = "";

			cat = Smth_GetUrlCategory( curUrl );
			if ( cat == "article" ) {
				artileIndex = 0;
			}
			else {
				artileIndex = -1;
			}
		}

		if ( artileIndex >= 0 ) {
			gsSmth.view.Output( &linkState );
			artileIndex = -1;
		}

		Smth_SetPosMarker( linkState.PosX(), linkState.PosY() );

		c = Smth_CheckPressedKey();
		switch( c ) {
		case SK_H:
			gsSmth.gotoUrl = SMTH_HOMEPAGES[0];
			break;
		case SK_UP:
			Smth_ClearPosMarker( linkState.PosX(), linkState.PosY() );
			linkState.PrevPos();
			if ( cat == "article" ) {
				if ( gsSmth.view.ItemIndex() > 0 ) {
					gsSmth.view.SetItemIndex( gsSmth.view.ItemIndex() - 1 );
					artileIndex = gsSmth.view.ItemIndex();
				}
				else {
					if ( gsSmth.article.pageIndex > 1 ) {
						gsSmth.gotoUrl = Smth_GetPrevPageUrl( curUrl );
					}
				}
			}
			break;
		case SK_SPACE:
		case SK_DOWN:
			Smth_ClearPosMarker( linkState.PosX(), linkState.PosY() );
			linkState.NextPos();
			if ( cat == "article" ) {
				if ( gsSmth.view.ItemIndex() < gsSmth.view.ItemCount() - 1 ) {
					gsSmth.view.SetItemIndex( gsSmth.view.ItemIndex() + 1 );
					artileIndex = gsSmth.view.ItemIndex();
				}
				else {
					if (gsSmth.article.pageIndex < gsSmth.article.pageCount) {
						gsSmth.gotoUrl = Smth_GetNextPageUrl( curUrl );
					}
				}
			}
			break;
		case SK_LEFT:
			{
				if ( gsSmth.urlStack.size() > 1 ) {
					gsSmth.urlStack.pop();
					gsSmth.gotoUrl = gsSmth.urlStack.top();
				}
			}
			break;
		case SK_RIGHT:
			break;
		case SK_STAB:
			{
				int index = Smth_GetHomePageIndex( curUrl );
				if ( index >= 0 ) {
					index--;
					if ( index < 0 ) {
						index = SMTH_HOMEPAGE_COUNT - 1;
					}
					gsSmth.gotoUrl = SMTH_HOMEPAGES[index];
				}
			}
			break;
		case SK_TAB:
			{
				int index = Smth_GetHomePageIndex( curUrl );
				if ( index >= 0 ) {
					index++;
					if ( index >= SMTH_HOMEPAGE_COUNT ) {
						index = 0;
					}
					gsSmth.gotoUrl = SMTH_HOMEPAGES[index];
				}
			}
			break;
		case SK_PREVPAGE:
			{
				if ( cat == "article" ) {
					if ( gsSmth.article.pageIndex > 1 ) {
						gsSmth.gotoUrl = Smth_GetPrevPageUrl( curUrl );
					}
				}
				if ( cat == "board" ) {
					if ( gsSmth.board.pageIndex > 1 ) {
						gsSmth.gotoUrl = Smth_GetPrevPageUrl( curUrl );
					}
				}
			}
			break;
		case SK_NEXTPAGE:
			{
				if ( cat == "article" ) {
					if ( gsSmth.article.pageIndex < gsSmth.article.pageCount ) {
						gsSmth.gotoUrl = Smth_GetNextPageUrl( curUrl );
					}
				}
				if ( cat == "board" ) {
					if ( gsSmth.board.pageIndex < gsSmth.board.pageCount ) {
						gsSmth.gotoUrl = Smth_GetNextPageUrl( curUrl );
					}
				}
			}
			break;
		case SK_QUIT:
		case SK_CTRLC:
			quit = true;
			break;
		case SK_ENTER:
			if ( linkState.posIndex >= 0 ) {
				gsSmth.gotoUrl = linkState.Url();
			}
			break;
		default:
			break;
		}

	} while ( !quit );
}


/////////////////////////////////////////////////////////////////////////////
PageView::PageView( size_t w, size_t h )
	: width( w ), height( h )
{
	itemIndex = -1;
}

void PageView::ParseArticle( const std::string& text )
{
	std::wstring wt = Smth_Utf8StringToWString( text );
	
	PageViewItem item;
	ViewLine line( TEXT );
	VIEWLINE_TYPE prevLineType = TEXT;
	size_t charCount = 0;
	for ( size_t i = 0; i < wt.length(); ++i ) {
		wchar_t c = wt[i];
		int charSize = 1;
		if ( c > 127 ) {
			// Unicode char as 2 chars
			// TODO: This is not right, need to polish.
			charSize = 2;
		}
		if ( c == L'\n' ) {

			line.SetType( AdjustLineType( line, prevLineType ) );
			prevLineType = line.Type();
			item.Append( line );
			if ( item.LineCount() == height ) {
				items.push_back( item );
				item.Clear();
			}

			line.Clear();
			charCount = 0;
			continue;
		}
		if ( charCount + charSize > width ) {

			line.SetType( AdjustLineType( line, prevLineType ) );
			prevLineType = line.Type();
			item.Append( line );
			if ( item.LineCount() == height ) {
				items.push_back( item );
				item.Clear();
			}

			line.Clear();
			line.SetType( TEXT_MORE );
			line.Append( c );
			charCount = charSize;
			continue;
		}
		line.Append( c );
		charCount += charSize;

	}
	if ( line.Length() > 0 ) {
		line.SetType( AdjustLineType( line, prevLineType ) );
		prevLineType = line.Type();
		item.Append( line );
	}
	if ( item.LineCount() > 0 ) {
		items.push_back( item );
	}
}

void PageView::Output( LinkPositionState* state ) const
{
	if ( state != nullptr ) {
		state->Clear();
	}

	system( "cls" );
	if ( itemIndex >= 0 && itemIndex < (int)items.size() ) {
		items[itemIndex].Output();
	}

	int x, y;
	Smth_GetCursorXY( x, y );
	if ( state != nullptr && x >= 0 && y >= 0 ) {
		state->Append( x, y, SMTH_DOMAIN );
		state->posIndex = 0;
	}
}

VIEWLINE_TYPE PageView::AdjustLineType( const ViewLine& line, VIEWLINE_TYPE prevLineType ) const
{
	std::wstring ln = line.Text();
	if ( ln.find(L"FROM") == 0 ) {
		return FROM;
	}
	if ( ln.length() > 0 && (int)ln[0] == 12304 ) {
		return REFER_AUTHOR;
	}
	if ( ln.length() > 0 && ln[0] == L':' ) {
		return REFER;
	}
	if ( line.Type() == TEXT_MORE && ( prevLineType == REFER || prevLineType == REFER_MORE ) ) {
		return REFER_MORE;
	}

	return line.Type();
}

/////////////////////////////////////////////////////////////////////////////
void ViewLine::Append( wchar_t c )
{
	content.push_back( c );
}

void ViewLine::Output( void ) const
{
	Smth_PrintLn( type, content );
}
