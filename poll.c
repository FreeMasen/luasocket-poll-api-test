#include <poll.h>
#include <errno.h>
#include <stdio.h>

#include "lua.h"
#include "lauxlib.h"

static int global_poll(lua_State *L);

static void dump_stack(lua_State *L)
{
    int i = lua_gettop(L);
    // printf("vv STACK vv\n");
    while (i > 0)
    {
        // printf("%i %s\n", i, lua_typename(L, lua_type(L, i)));
        i -= 1;
    }
    // printf("^^^^^^^^^^^^\n");
}

/* functions in library namespace */
static luaL_Reg func[] = {
    {"poll", global_poll},
    {NULL, NULL}};

int luaopen_poll(lua_State *L)
{
    lua_newtable(L);
    luaL_setfuncs(L, func, 0);
    return 1;
}

static int actual_poll(lua_State *L, struct pollfd *fds, nfds_t nfds, int timeout)
{
    int result = poll(fds, nfds, timeout * 1000);
    // printf("poll returned %i\n", result);
    if (result < 0)
    {
        char *error_msg;
        switch (errno)
        {
        case EFAULT:
            error_msg = "invalid FD provided";
            break;
        case EINTR:
            error_msg = "interrupted";
            break;
        case EINVAL:
            error_msg = "Too many sockets";
            break;
        case ENOMEM:
            error_msg = "No Memory";
            break;
        default:
            error_msg = "Unknown Error";
            break;
        }
        lua_pushstring(L, error_msg);
        return -1;
    }
    return 1;
}

static int getfd(lua_State *L)
{
    int fd = -1;
    lua_pushstring(L, "getfd");
    lua_gettable(L, -2);
    if (!lua_isnil(L, -1))
    {
        lua_pushvalue(L, -2);
        lua_call(L, 1, 1);
        if (lua_isnumber(L, -1))
        {
            double numfd = lua_tonumber(L, -1);
            fd = (numfd >= 0.0) ? (int)numfd : -1;
        }
    }
    lua_pop(L, 1);
    return fd;
}

static int collect_poll_args(lua_State *L, int tab, int rtab, struct pollfd dest[])
{
    // printf("collecting poll args %i -> %i\n", tab, rtab);
    if (lua_isnil(L, tab))
    {
        // printf("no args, returning early\n");
        return 0;
    }
    luaL_checktype(L, tab, LUA_TTABLE);
    int i = 1, n = 0, ret = 0;
    for (;;)
    {
        // printf("(%i) arg %i\n", lua_gettop(L), i);
        int fd;
        lua_pushinteger(L, i);
        // printf("(%i) after lua_pushinteger %i\n", lua_gettop(L), i);
        // dump_stack(L);
        lua_gettable(L, tab);
        // printf("(%i) after lua_gettable %i\n", lua_gettop(L), i);
        // dump_stack(L);
        if (lua_isnil(L, -1))
        {
            // printf("end of list %i\n", i);
            lua_pop(L, 1);
            // printf("(%i) after lua_pop %i\n", lua_gettop(L), i);
            // dump_stack(L);
            break;
        };
        int info = lua_gettop(L);
        // printf("(%i) after lua_gettop %i %i\n", lua_gettop(L), i, info);
        // dump_stack(L);
        lua_getfield(L, info, "sock");
        // printf("(%i) after lua_getfield sock %i\n", lua_gettop(L), i);
        // dump_stack(L);
        fd = getfd(L);
        // printf("(%i) after getfd %i -> %i\n", lua_gettop(L), i, fd);
        // dump_stack(L);
        lua_pushnumber(L, (lua_Number)fd);
        // printf("(%i) after lua_pushnumber %i\n", lua_gettop(L), i);
        // swap fd and sock
        lua_rotate(L, -2, 1);
        // dump_stack(L);
        lua_settable(L, rtab);
        // printf("(%i) after lua_settable %i\n", lua_gettop(L), i);
        // dump_stack(L);
        if (fd != -1)
        {
            short events = POLLERR | POLLHUP;
            lua_getfield(L, info, "read");
            // printf("(%i) after lua_getfield read %i\n", lua_gettop(L), i);
            // dump_stack(L);
            if (lua_toboolean(L, -1))
            {
                // printf("%i/%i is polling for read\n", i, fd);
                events |= POLLIN;
            }
            // printf("(%i) after lua_toboolean %i\n", lua_gettop(L), i);
            // dump_stack(L);
            //  pop boolean property
            lua_pop(L, 1);
            // printf("(%i) after lua_pop %i\n", lua_gettop(L), i);
            // dump_stack(L);
            lua_getfield(L, info, "write");
            // printf("(%i) after lua_getfield write %i\n", lua_gettop(L), i);
            // dump_stack(L);
            if (lua_toboolean(L, -1))
            {
                // printf("%i/%i is polling for write\n", i, fd);
                events |= POLLOUT;
            }
            // printf("(%i) after lua_toboolean %i\n", lua_gettop(L), i);
            // dump_stack(L);
            //  pop boolean prop
            lua_pop(L, 1);
            // printf("(%i) after lua_pop %i\n", lua_gettop(L), i);
            // dump_stack(L);
            struct pollfd pfd = {
                .fd = fd,
                .events = events,
                .revents = 0,
            };
            dest[n] = pfd;
        }
        lua_pop(L, 1);
        n += 1;
        i += 1;
    }
    return n;
}

static int global_poll(lua_State *L)
{
    int top, tab, to;
    struct pollfd fds[4096] = {0};
    top = lua_gettop(L);
    // printf("global_poll %i\n", top);
    if (top > 1)
    {
        to = luaL_optinteger(L, -1, 0);
        // printf("timeout %i\n", to);
        lua_pop(L, 1);
        tab = lua_gettop(L);
        // printf("table %i\n", tab);
    }
    else
    {
        to = 0;
        tab = top;
    }
    lua_newtable(L);
    int rtab = lua_gettop(L);
    // printf("before collect_poll_args %i\n", lua_gettop(L));
    int fd_ct = collect_poll_args(L, tab, rtab, fds);
    // printf("after collect_poll_args %i\n", lua_gettop(L));
    int result = actual_poll(L, fds, fd_ct, to);
    if (result < 0)
    {
        // error string was pushed onto the stack
        lua_pushnil(L);
        // swap str,nil to nil, str
        lua_rotate(L, -2, 1);
        return 2;
    }
    int ready_ct = 0;
    int ret_t = -1;
    for (int i = 0; i < fd_ct; i++)
    {
        struct pollfd *fd = &fds[i];
        if (fd == NULL)
        {
            // printf("fd was nil, exiting readiness check %i\n", i);
            break;
        }
        // printf("(%i) fd %i may be ready %i/%i\n", lua_gettop(L), fd->fd, fd->events, fd->revents);
        if ((fd->revents & (POLLIN | POLLOUT)) > 0)
        {
            // printf("(%i) fd ready: %i\n", lua_gettop(L), fd->fd);
            ready_ct += 1;
            if (ret_t == -1)
            {
                lua_newtable(L);
                ret_t = lua_gettop(L);
            }
            lua_newtable(L);
            int sock_t = lua_gettop(L);
            lua_pushboolean(L, fd->revents & POLLIN);
            lua_setfield(L, sock_t, "read");
            lua_pushboolean(L, fd->revents & POLLOUT);
            lua_setfield(L, sock_t, "write");
            lua_pushinteger(L, fd->fd);
            lua_gettable(L, rtab);
            // dump_stack(L);
            lua_setfield(L, sock_t, "sock");
            lua_pushinteger(L, ready_ct);
            lua_rotate(L, -2, 1);
            lua_settable(L, ret_t);
        }
    }
    // dump_stack(L);
    if (ready_ct == 0)
    {
        lua_pushnil(L);
        lua_pushstring(L, "timeout");
        return 2;
    }
    lua_pushvalue(L, ret_t);
    return 1;
}
