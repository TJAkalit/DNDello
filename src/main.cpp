#include "panel.hpp"

#include <iostream>
#include <string>
#include "lua.hpp"
#include <memory>
#include <functional>

extern "C" {
# include "lua.h"
# include "lauxlib.h"
# include "lualib.h"
}

#include "LuaBridge/LuaBridge.h"

void view_by_lua(dndello::server::Session& _session, std::string param)
{
    lua_State* L;
    L = luaL_newstate();
    luaL_openlibs(L);
    lua_pcall(L, 0, 0, 0);

    std::string script {};
    script.append("uri=\"")
        .append(_session.getTarget())
        .append("\"");

    // TODO: Remove this later
    if (luaL_dostring(L, script.c_str()) != LUA_OK)
    {
      std::cout << lua_tostring(L, -1) << std::endl;  
      _session.response.result(500);
      return;
    };

    if (luaL_dofile(L, param.c_str()) != LUA_OK)
    {
      std::cout << lua_tostring(L, -1) << std::endl;  
      _session.response.result(500);
      return;
    };
    // luabridge::LuaRef lua_body = luabridge::getGlobal(L, "body");

    if (!luabridge::getGlobal(L, "body").isNil()&&
         luabridge::getGlobal(L, "body").isString())
    {
        _session.response.body() = std::move(
            luabridge::getGlobal(L, "body")
                .cast<std::string>()
        );
    }
    else
    {
        _session.response.result(500);
    };
    
    std::cout << "marker" << std::endl;
    lua_close(L);
};

int main(int argc, char **argv)
{
    std::shared_ptr<dndello::server::Panel> 
    panel(new dndello::server::Panel(1));

    std::shared_ptr<dndello::server::Server> 
    serv1 = panel->make_server(std::string("127.0.0.1"), 8080);

    serv1->add_route(
        std::string("/index.html"), 
        dndello::server::static_file_view, std::string("index.html")
    );
    serv1->add_route(
        std::string("/page.html"), 
        view_by_lua, std::string("page.lua")
    );
    serv1->sync_run();

    return 0;
};