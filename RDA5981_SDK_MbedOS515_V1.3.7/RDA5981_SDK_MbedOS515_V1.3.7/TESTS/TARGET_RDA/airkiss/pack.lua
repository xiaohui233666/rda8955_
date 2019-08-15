a = "{\"type\":\"at\",\"Code\":\"at\"}"
local res = string.format("%02x",string.byte("<"))
local str_len = string.len(a)
local crc = 0
print("the length of str is "..string.len(a).." hex:"..string.format("%02x",str_len>>8&255)..string.format("%02x",str_len&255))
crc=crc+(str_len>>8&255)+(str_len&255)
res=res..string.format("%02x",str_len>>8&255)..string.format("%02x",str_len&255)
res=res..string.format("%02x",((str_len>>8&255)+(str_len&255))&255)
for i = 1, str_len do
    res=res..string.format("%02x",string.byte(a,i))
    print(string.format("%02x",string.byte(a,i)))
    crc=crc+string.byte(a,i)
end
res=res..string.format("%02x",crc&255)
res=res..string.format("%02x",string.byte(">"))
print("the length of res str is "..string.len(res))
print(res)