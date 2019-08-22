local a = "{\"type\":\"at\",\"Code\":\"at\"}"
a = "{\"type\":\"http\",\"Url\":\"http://wy.cmfspay.com/hardware/devicestatus\",\"Mothed\":\"Get\",\"Data\":\"sign=38419a28bf6dbd073e59e28c6061a000&time=1552271732&openid=6000001&sv=0.0.1&sn=40011000000027\"}"
a = "{\"type\":\"at\",\"Code\":\"at+time\"}"
local tmp_res = ""
local res = string.format("%02x",string.byte("<"))
local str_len = string.len(a)
local crc = 0
print("the length of str is "..string.len(a).." hex:"..string.format("%02x",str_len>>8&255)..string.format("%02x",str_len&255))
crc=crc+(str_len>>8&255)+(str_len&255)
res=res..string.format("%02x",str_len>>8&255)..string.format("%02x",str_len&255)
res=res..string.format("%02x",((str_len>>8&255)+(str_len&255))&255)
tmp_res=res
for i = 1, str_len do
    tmp_res=tmp_res..string.format("%02x",string.byte(a,i))
    res=res..string.format("%02x",string.byte(a,i))
    --print(string.format("%02x",string.byte(a,i)))
    if i%100==0 then
       print(tmp_res)
       tmp_res=""
    end
    crc=crc+string.byte(a,i)
end
res=res..string.format("%02x",crc&255)
res=res..string.format("%02x",string.byte(">"))
print("the length of res str is "..string.len(res))
print(res)