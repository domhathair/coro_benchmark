local overal = 0
local times = 1000
local process = "./.pio/build/linux/program"
local main = "./src/main.c"

local function change(i)
    local f = io.open(main, "r")
    if not f then error("Failed to open file " .. main) end
    local content = f:read("*a")
    f:close()

    local pattern = "#define%s+USE_MCO[^\n\r]*"
    local replacement = "#define USE_MCO " .. i

    local new_content, n = content:gsub(pattern, replacement, 1)

    if n ~= 1 then
        error("Pattern not found")
    end

    local fw = io.open(main, "w")
    if not fw then error("Failed to open file " .. main) end
    fw:write(new_content)
    fw:close()

    local devnull = package.config:sub(1, 1) == "\\" and "NUL" or "/dev/null"

    if not os.execute("pio run 1> " .. devnull) then
        error("Failed to execute process")
    end
end

local function test()
    overal = 0

    for _ = 1, times do
        local p = assert(io.popen(process, "r"))

        local count = tonumber(p:read("*a"))
        overal = overal + count

        local ok, _, code = p:close()
        if not ok then
            error(tostring(code))
        end
    end
end

local function out(fmt)
    print("Using " .. fmt .. ":\t" .. overal / times .. " switches/ms")
end

change(0)
test()
out("libco")
change(1)
test()
out("minicoro")
