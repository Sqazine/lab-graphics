#include "SoftRayTracingScene.h"
#include "App.h"

#undef main
int main(int argc, char** argv)
{
	Scene *s = new SoftRayTracingScene();

    App::Instance().AddScene(s);

    App::Instance().Run();

    return 0;
}