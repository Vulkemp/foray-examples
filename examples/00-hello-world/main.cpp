#include "app.hpp"

// this a basic example on the usage of the foray framework

int main(int argv, char** args)
{
	// let cmake define our working directory
    foray::osi::OverrideCurrentWorkingDirectory(CWD_OVERRIDE);
    example::App app;
	return app.Run();
}
