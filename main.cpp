#include "../nlohmann/json.hpp"
#include "StrategyManager.h"

using namespace std;

int main(int argc, char *argv[]) {
	StrategyManager manager;
    if (argc == 3)
        manager.run_feed (argv[1], atoi (argv[2]));
    else if  (argc == 2)
        manager.run_feed (argv[1], -1);
	manager.run();
	return 0;
}