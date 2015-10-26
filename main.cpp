#include <iostream>
#include <vector>
#include <string>
#include "Runner.h"

int main(int argc, char *argv[])
{
	Runner* runner = new Runner();
	if(runner->syntaxCheck(argc, argv) == false)
		runner->printHelp();
	else
		runner->run();//switch action inside run();
	delete runner;

	return 0;
}