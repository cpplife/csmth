#ifndef NET_UTIL_H_170508100647
#define NET_UTIL_H_170508100647

#include <string>

bool Net_Init( void );
void Net_Deinit( void );

std::wstring Net_Get( const std::string& url );

#endif // #ifndef NET_UTIL_H_170508100647
