#include "panel.hpp"

#include <iostream>
#include <string>
#include "lua.hpp"
#include <memory>

extern "C" {
# include "lua.h"
# include "lauxlib.h"
# include "lualib.h"
}

void test()
{
    lua_State* L;
    L = luaL_newstate();
    lua_close(L);
};

int main(int argc, char **argv)
{
    std::shared_ptr<dndello::server::Panel> panel(new dndello::server::Panel(1));

    std::shared_ptr<dndello::server::Server> serv1 = panel->make_server(std::string("127.0.0.1"), 8080);
    serv1->sync_run();



    return 0;
};