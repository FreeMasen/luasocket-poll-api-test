# luasocket-poll

Test bed for experimenting with a luasocket API based on posix poll


This lua C module exposes the following API

```lua
---@class Poller
---@field sock userdata The luasocket to poll
---@field read boolean If polling for read readiness
---@field write boolean If polling for write readiness
local Poller

--- Poll luasockets
--- @param pollers Poller[]
--- @param timeout number The timeout in seconds to pass to system `poll`
--- @return Poller[]|nil
--- @return nil|string
local function poll(pollers, timeout) end

return {
    poll = poll
}
```

## Example Usage

```lua
local socket = require "socket"
local poll = require "poll"

--- This function will work exactly like `socket.select` but using the `poll.poll` api
--- @param rdrs luasocket[] sockets to poll for read readiness
--- @param wtrs luasocket[] sockets to poll for write readiness
--- @param to number timeout in seconds to pass to system poll
local function select(rdrs,wtrs,to)
    -- to dedupe values in both rdrs and wtrs
    local by_fd = {}
    local poll_arg = {}
    -- collect all readers into the poll_arg list table
    for _,rdr in ipairs(rdrs) do
        local idx = #poll_arg + 1
        by_fd[rdr] = idx
        poll_arg[idx] = {
            sock = rdr,
            read = true,
            write = false,
        }
    end
    -- collect all writers into the poll_arg list table
    for _,wtr in ipairs(wtrs) do
        local idx = by_fd[wtr] or (#poll_arg + 1)
        local rdr = poll_arg[idx]
        if rdr then
            rdr.write = true
        else
            poll_arg[idx] = {
                sock = wtr,
                read = false,
                write = true,
            }
        end
    end
    local ready, err = poll.poll(poll_arg, to)
    if not ready then
        return nil, err
    end
    -- unwrap the return values into the 2 returns provided by select
    local r,w = {}, {}
    for i,rdy in pairs(ready) do
        if rdy.read then
            table.insert(r, rdy.sock)
        end
        if rdy.write then
            table.insert(w, rdy.sock)
        end
    end
    return r, w
end

```
