#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include <sodium.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "go.h"

const char *cmds[] = {
    "cat",
    "import_games",
    "extract_features",
    NULL
};

int main(int argc, char **argv) {
    srand(clock());
    if (sodium_init() == -1) {
        perror("failure to initialize libsodium");
        return -1;
    }
    go_init();

    if (argc < 2) {
        fprintf(stderr, "Usage: kerplunk <cmd> ...\n");
        return -1;
    }

    // initialize lua VM
    lua_State *L = lua_open();
    luaL_openlibs(L);

    // check command name
    bool cmd_found = false;
    for (size_t i = 0; cmds[i]; i++) {
        if (!strcmp(cmds[i], argv[1])) {
            cmd_found = true;
            break;
        }
    }
    
    if (!cmd_found) {
        fprintf(stderr, "unknown command %s\n", argv[1]);
        return -1;
    }

    // run bootstrap script
    char buffer[100];
    snprintf(buffer, sizeof(buffer), "main=require(\"%s\").main\n", argv[1]);
    if (luaL_dostring(L, buffer)) {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        return -1;
    }

    // run script main
    lua_getglobal(L, "main");
    for (int i = 2; i < argc; i++) {
        lua_pushstring(L, argv[i]);
    }
    if (lua_pcall(L, argc-2, 1, 0)) {
        luaL_error(L, "error running command %s main: %s",
                   argv[1], lua_tostring(L, -1));
        return -1;
    }

    int retval = lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_close(L);

    return retval;
}
