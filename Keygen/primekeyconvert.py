import os,sys,struct,zlib,binascii

f = open(sys.argv[1],"r")
o = open("hasp.bin","wb")
kl = f.readlines()
f.close()
key_table = {}
for line in kl:
	line = line.strip()
	if(line[0] != "#"):
		key,req,resp = line.split("\t")
		key_table[key] = { "request": binascii.unhexlify(req), "response": binascii.unhexlify(resp) }

for key in sorted(key_table.keys()):
	o.write(key_table[key]["request"])
	o.write(key_table[key]["response"])
o.close()