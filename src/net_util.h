#ifndef NET_UTIL_H_170508100647
#define NET_UTIL_H_170508100647

#include <string>

bool Net_Init( void );
void Net_Deinit( void );

std::string Net_Get( const std::string& url, const std::string& cookie_file="" );

std::string Net_Login( const std::string& url, const std::string& data, const std::string& cookie_file );

#endif // #ifndef NET_UTIL_H_170508100647
