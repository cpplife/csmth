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

struct BoardItem {
	std::string url;
	std::string title;
	std::string author;
	std::string last_replier;
};

struct BoardPage {
	std::string name_cn;
	std::string name_en;
	std::vector<BoardItem> items;
};

struct ArticleItem {
	std::string author;
	std::string content;
};

struct ArticlePage {
	std::string name;
	std::vector<ArticleItem> items;
};

struct LinkPos {
	int x;
	int y;
	std::string url;
};

struct LinkPositionState {
	int posIndex;
	std::vector<LinkPos> linkPositions;

	void Clear( void ) {
		posIndex = -1;
		linkPositions.clear();
	}

	void Append( const LinkPos& linkPos ) {
		linkPositions.push_back( linkPos );
	}

	void Append( int x, int y, const std::string& url ) {
		LinkPos p;
		p.x = x; p.y = y;
		p.url = url;
		linkPositions.push_back( p );
	}

	int PosX() const {
		return posIndex >= 0 ? linkPositions[posIndex].x : -1;
	}

	int PosY() const {
		return posIndex >= 0 ? linkPositions[posIndex].y : -1;
	}

	void NextPos() {
		posIndex++;
		if ( posIndex >= (int)linkPositions.size() ) {
			posIndex = 0;
		}
	}

	void PrevPos() {
		posIndex--;
		if ( posIndex < 0 ) {
			posIndex = (int)(linkPositions.size() - 1);
		}
	}

	std::string Url() const {
		return posIndex >= 0 ? linkPositions[posIndex].url : "";
	}

};


SectionPage Smth_GetSectionPage( const std::string& htmlText );

BoardPage   Smth_GetBoardPage(   const std::string& htmlText );

ArticlePage Smth_GetArticlePage( const std::string& htmlText );

void Smth_OutputSectionPage( const SectionPage& page, LinkPositionState* state=nullptr );
void Smth_OutputBoardPage( const BoardPage& page, LinkPositionState* state=nullptr );
void Smth_OutputArticlePage( const ArticlePage& page, LinkPositionState* state=nullptr );


std::wstring Smth_Utf8StringToWString( const std::string& text );

bool Smth_Init( void );
void Smth_Deinit( void );

void Smth_Update( void );

#endif // #ifndef SMTH_H_170505112940
