local socket = require "socket"
local poll = require "poll"

local sock = assert(socket.udp())
assert(sock:setsockname("0.0.0.0", 0))
local sock2 = assert(socket.udp())
assert(sock2:setsockname("0.0.0.0", 0))

local ret, err = poll.poll({{
  sock = sock,
  read = true,
  write = false,
}, {
  sock = sock2,
  read = true,
  write = false,
}}, 1)
-- nothing should be ready
assert(ret == nil, string.format("expected error found %s", ret))
assert(err == "timeout", string.format("expected 'timeout' found '%s'", err))
print(string.rep("=", 50))
-- check for write ready
local writable = assert(poll.poll({{
  sock = sock,
  read = false,
  write = true,
}, {
  sock = sock2,
  read = false,
  write = true,
}}, 1))
assert(#writable == 2, "sockets not writable")
-- nothing should be ready
-- assert(ret == nil, string.format("expected error found %s", ret))
-- make sock ready to read
sock2:sendto("some data", sock:getsockname())
