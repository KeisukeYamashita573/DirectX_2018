#include "Application.h"

using namespace std;

int main(void) {
	Application& ap = Application::Instance();

	ap.Initialize();
	ap.Run();
	ap.Terminate();

	return 0;
}