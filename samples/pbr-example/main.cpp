#include "Base/App.h"
#include "Scene.h"

#undef main
int main(int argc, const char *argv[])
{
    Scene *s = new PbrScene();
    App::Instance().AddScene(s);
    App::Instance().Run();
    return 0;
}