#include "smth.h"


int main(int argc, char* argv[] )
{
	if ( Smth_Init() ) {

		Smth_Login( );

		Smth_RunLoop();

		Smth_Deinit();
	}
	return 0;
}
