#include "smth.h"


int main()
{
	if ( Smth_Init() ) {

		Smth_Update();

		Smth_Deinit();
	}
	return 0;
}
