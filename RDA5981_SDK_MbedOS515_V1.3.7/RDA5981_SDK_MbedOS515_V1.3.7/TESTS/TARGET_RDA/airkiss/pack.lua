function request(method, url, data, ctype, headers)
    local response_header, response_code, response_body = {}
    local _, idx, offset, ssl, auth, https, host, port, path
    if type(headers) == "string" then
        local tmp = {}
        for k, v in string.gmatch(headers, "(.-):%s*(.-)\r\n") do tmp[k] = v end
        headers = tmp
    elseif type(headers) ~= "table" then
        headers = {
            ['User-Agent'] = 'Mozilla/4.0',
            ['Accept'] = '*/*',
            ['Accept-Language'] = 'zh-CN,zh,cn',
			['Content-Type'] = 'application/x-www-form-urlencoded',
            ['Content-Length'] = '0',
            ['Connection'] = 'Keep-alive',
            ["Keep-Alive"] = 'timeout=20',
        }
    end
	--headers['Content-Length'] = #data or 0
    _, idx, auth = url:find("(.-:.-)@", (offset or 0) + 1)
    offset = idx or offset
    if url:match("^[^/]+:(%d+)", (offset or 0) + 1) then
        _, offset, host, port = url:find("^([^/]+):(%d+)", (offset or 0) + 1)
    elseif url:find("(.-)/", (offset or 0) + 1) then
        _, offset, host = url:find("(.-)/", (offset or 0) + 1)
        offset = offset - 1
    else
        offset, host = #url, url:sub((offset or 0) + 1, -1)
    end
    if not host then return '105', 'ERR_NAME_NOT_RESOLVED' end
    if not headers.Host then headers["Host"] = host end
    port = port or 80
    path = url:sub(offset + 1, -1)
    path = path == "" and "/" or path
    ctype = ctype or 2
    --headers['Content-Type'] = Content_type[ctype]

    local str = ""
    for k, v in pairs(headers) do str = str .. k .. ": " .. v .. "\r\n" end
        str = method .. ' ' .. path .. ' HTTP/1.1\r\n' .. str .. '\r\n' .. (data and data .. "\r\n" or "")
	return str
end

local a = "{\"type\":\"at\",\"Code\":\"at\"}"
a = "{\"type\":\"http\",\"Url\":\"http://wy.cmfspay.com/hardware/devicestatus\",\"Mothed\":\"Get\",\"Data\":\"sign=38419a28bf6dbd073e59e28c6061a000&time=1552271732&openid=6000001&sv=0.0.1&sn=40011000000027\"}"
a = "{\"type\":\"at\",\"Code\":\"AT+TIME\"}"
a = "{\"type\":\"at\",\"Code\":\"AT+WSMAC=?\"}"
a = "{\"type\":\"at\",\"Code\":\"AT+WSCONN=?\"}"
a = "{\"type\":\"at\",\"Code\":\"AT+WSCONN=0,\\\"xiaohui\\\",\\\"qwertyui\\\"\"}"

--a = "{\"type\":\"tcp0+start\",\"Ip\":\"88.99.213.221\",\"Port\":\"80\"}"
local header = "GET //www.lua.org HTTP/1.1\r\nContent-Length: 0\r\nConnection: Keep-alive\r\nAccept: */*\r\nAccept-Language: zh-CN,zh,cn\r\nHost: http:\r\nUser-Agent: Mozilla/4.0\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n"
a = "{\"type\":\"tcp0+send\",\"buf\":\""..header.."\"}"
a = "{\"type\":\"tcp0+stop\",\"buf\":\""..header.."\"}"
--a = "{\"type\":\"at\",\"Code\":\"AT+WSCONN=0,\\\"xiaohui\\\",\\\"qwertyui\\\"\"}"
local tmp_res = ""
local res = string.format("%02x",string.byte("<"))
local str_len = string.len(a)
local crc = 0
print("the length of str is "..string.len(a).." hex:"..string.format("%02x",(str_len>>8)&255)..string.format("%02x",str_len&255))
crc=crc+(str_len>>8&255)+(str_len&255)
res=res..string.format("%02x",str_len>>8&255)..string.format("%02x",str_len&255)
res=res..string.format("%02x",((str_len>>8&255)+(str_len&255))&255)
tmp_res=res
local i = 0
for i = 1, str_len do
    tmp_res=tmp_res..string.format("%02x",string.byte(a,i))
    res=res..string.format("%02x",string.byte(a,i))
    --print(string.format("%02x",string.byte(a,i)))
    if i%40==0 then
       print(tmp_res)
       tmp_res=""
    end
    crc=crc+string.byte(a,i)
end
print(tmp_res)


res=res..string.format("%02x",crc&255)
res=res..string.format("%02x",string.byte(">"))
print(string.format("%02x",crc&255)..string.format("%02x",string.byte(">")))
print(a)
print("the length of res str is "..string.len(res))
print(res)



print(request("GET", "http://www.lua.org", 10000,"", nil, nil))


