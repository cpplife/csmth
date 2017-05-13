#ifndef SMTH_H_170505112940
#define SMTH_H_170505112940


#include <string>
#include <vector>

struct TitleItem {
	std::string url;
	std::string title;
};

struct SectionPage {
	std::string name;
	std::vector<TitleItem> items;
};

struct ArticleItem {
	std::string author;
	std::string content;
};

struct ArticlePage {
	std::string name;
	std::vector<ArticleItem> items;
};

SectionPage Smth_GetSectionPage( const std::string& htmlText );
ArticlePage Smth_GetArticlePage( const std::string& htmlText );


std::wstring Smth_Utf8StringToWString( const std::string& text );

bool Smth_Init( void );
void Smth_Deinit( void );

void Smth_Update( void );

#endif // #ifndef SMTH_H_170505112940
