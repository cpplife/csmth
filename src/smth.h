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
	std::string boardName;
	std::string name;
	size_t pageIndex;
	size_t pageCount;
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
			posIndex = (int)linkPositions.size() - 1;
		}
	}

	void PrevPos() {
		posIndex--;
		if ( posIndex < 0 ) {
			posIndex = 0;
		}
	}

	std::string Url() const {
		return posIndex >= 0 ? linkPositions[posIndex].url : "";
	}

};

enum VIEWLINE_TYPE {
	TEXT,
	TEXT_MORE,
};
class ViewLine
{
public:
	ViewLine( VIEWLINE_TYPE t )
		: type( t )
	{
	}
	void SetType( VIEWLINE_TYPE t )
	{
		type = t;
	}
	VIEWLINE_TYPE Type() const
	{
		return type;
	}

	void Clear()
	{
		type = TEXT;
		content.clear();
	}
	int  Length() const
	{
		return (int)content.length();
	}

	void Append( wchar_t c );
	void Output() const;

private:
	VIEWLINE_TYPE type; 
	std::wstring content;
};

class PageViewItem
{
public:
	void Clear()
	{
		lines.clear();
	}
	void Append( const ViewLine& ln )
	{
		lines.push_back( ln );
	}
	size_t LineCount() const
	{
		return lines.size();
	}
	void Output() const
	{
		for ( size_t i = 0; i < lines.size(); ++i ) {
			lines[i].Output();
		}
	}

private:
	std::vector<ViewLine> lines;
};

class PageView
{
public:
	PageView( size_t w=80, size_t h=24 );
	void Parse( const std::string& text );
	void Output( LinkPositionState* state = nullptr ) const;
	void SetItemIndex( int idx )
	{
		itemIndex = idx;
	}
	int ItemIndex() const
	{
		return itemIndex;
	}
	int ItemCount() const
	{
		return (int)items.size();
	}
	void Clear()
	{
		itemIndex = -1;
		items.clear();
	}

private:
	std::vector<PageViewItem> items;
	int                       itemIndex;
	size_t                    width;
	size_t                    height;
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
