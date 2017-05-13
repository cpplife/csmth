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

#include "net_util.h"
#include "tinyxml2.h"
#include "smth.h"


static struct {
} gsSmth;


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

std::wstring Smth_Utf8StringToWString( const std::string& text )
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes( text );
}

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
			int index = s.find( tags[i].src );
			if ( index != std::string::npos ) {

				int len = strlen( tags[i].src );

				s = s.replace( index, len, tags[i].dst );

			}
			else {
				break;
			}
		}
	}

	return s;
}

SectionPage Smth_GetSectionPage( const std::string& htmlText )
{
	SectionPage page;

	const char* beginTag = "<ul class=\"slist sec\">";
	const char* endTag = "</ul>";
	int index = htmlText.find( beginTag );
	if ( index != std::string::npos ) {
		int end = htmlText.find( endTag, index + 1 );
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
					std::wstring t = Smth_Utf8StringToWString(page.name);
					wprintf(L"=== %s ===\n", t.c_str() );
				}
				else {
					TitleItem item;

					//std::regex rr( "<a href=\"(.*?)\">(.+?)(\\(.*?\\))</a>", std::regex::ECMAScript );
					std::regex rr( "<a href=\"(.*?)\">(.+?)</a>", std::regex::ECMAScript );
					std::string mstr = m.str();
					if (std::regex_search( mstr, um, rr ) ) {
						item.url   = um.str(1);
						item.title = Smth_ClearHtmlTags( um.str(2) );

						std::wstring t = Smth_Utf8StringToWString(item.title);
						const wchar_t* s = t.c_str();
						wprintf(L"%s\n", s);
					}

					page.items.push_back( item );
				}

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
	int index = htmlText.find( beginTag );
	if ( index != std::string::npos ) {
		int end = htmlText.find( endTag, index + 1 );
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
					std::wstring t = Smth_Utf8StringToWString(page.name);
					wprintf(L"=== %s ===\n", t.c_str() );
				}
				else {
					ArticleItem item;

					std::string mstr = m.str();

					std::regex r( "<div><a class=\"plant\">(.+?)</div>", std::regex::ECMAScript );
					if (std::regex_search( mstr, um, r ) ) {

						item.author = Smth_ClearHtmlTags( um.str(1) );

						std::wstring t = Smth_Utf8StringToWString(item.author);
						const wchar_t* s = t.c_str();
						wprintf(L"-----------------------------------------------------------------------------\n");
						wprintf(L"%s\n", s);
						wprintf(L"-----------------------------------------------------------------------------\n");
					}
					r.assign( "<div class=\"sp\">(.+?)</div>", std::regex::ECMAScript );
					if (std::regex_search( mstr, um, r ) ) {

						item.content = Smth_ReplaceHtmlTags( um.str(1) );

						std::wstring t = Smth_Utf8StringToWString(item.content);
						const wchar_t* s = t.c_str();
						wprintf(L"%s\n", s);
						wprintf(L"-----------------------------------------------------------------------------\n");
					}

					page.items.push_back( item );
				}

				titleText = m.suffix();
			}
		}
	}
	return page;
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
	char c = 0;
	int i = 0;
	do {

		std::string result;
		switch ( i ) {
		case 0:
			system( "cls" );
			result = Net_Get( "m.newsmth.net" );
			Smth_GetSectionPage( result );
			wprintf(L"\n"); 
			break;
		case 1:
			system( "cls" );
			result = Net_Get( "m.newsmth.net/hot/1" );
			Smth_GetSectionPage( result );
			wprintf(L"\n");
			break;
		case 2:
			system( "cls" );
			result = Net_Get( "m.newsmth.net/hot/2" );
			Smth_GetSectionPage( result );
			wprintf(L"\n");
			break;
		case 3:
			system( "cls" );
			result = Net_Get( "http://m.newsmth.net/article/AutoTravel/13459460" );
			Smth_GetArticlePage( result );
			break;

		default:
			break;
		}

		c = _getch();
		i += 1;

	} while ( c != '!' && c != 3 );
}
