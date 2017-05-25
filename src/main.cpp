#include "smth.h"


int main(int argc, char* argv[] )
{
	if ( Smth_Init() ) {

		if ( argc == 3 ) {
			Smth_Login( argv[1], argv[2] );
		}

		Smth_RunLoop();

		Smth_Deinit();
	}
	return 0;
}
