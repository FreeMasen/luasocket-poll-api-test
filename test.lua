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
print("first select", ret, err)
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
for i, info in ipairs(writable) do
  print(string.rep("*", 5) .. tostring(i) .. string.rep("*", 5))
  for k, v in pairs(info) do
    print("  " .. k .. " = " .. tostring(v))
  end
  print(string.rep("*", 15))
end
assert(#writable == 2, "sockets not writable")
assert(writable[1].sock == sock, "unexpected socket for writable[1]")
assert(not writable[1].read, "sock1 was readable")
assert(writable[1].write, "sock1 was not writable")
assert(writable[2].sock == sock2, "unexpected socket for writable[2]")
assert(not writable[2].read, "sock2 was readable")
assert(writable[2].write, "sock2 was not writable")
-- make sock ready to read
sock2:sendto("some data", sock:getsockname())

local readable = assert(poll.poll({{
  sock = sock,
  read = true,
  write = false,
}, {
  sock = sock2,
  read = true,
  write = false,
}}))

assert(#readable == 1, "sockets not readable")
assert(readable[1].sock == sock, "unexpected socket for writable[1]")
assert(readable[1].read, "sock1 was not readable")
assert(not readable[1].write, "sock1 was writable")
