#include "jolt_character_test_app.h"
#include <iostream>

int main()
{
    JoltCharacterTestApp app;
    
    if (!app.Initialize()) {
        std::cerr << "应用程序初始化失败" << std::endl;
        return -1;
    }

    app.Run();
    app.Shutdown();
    
    return 0;
}
