import os,sys,struct,binascii
GAME_PATH="./"
'''
	PIU Prime Key Generator
'''

if(len(sys.argv) < 2):
	print("Usage: %s dirlist (i for info)" % sys.argv[0])
	exit(1)
def makerequest(key,dirname):
	leafkey = 0
	for i in range(0,64):
		if(key[i] != 0x00):
			leafkey = 1
			break
	if(leafkey):
		#We don't have root.
		buff2 = bytearray(64)
		seed = 0x1D
		while(seed != 0x5D):
			tmpbyte = seed & 0xFF
			tmpbyte = seed ^ 0xA5
			buff2[seed-0x1D] = key[((((tmpbyte >> 0x1F) >> 0x1A) + (seed ^ 0xA5)) & 0x3F) - ((seed >> 0x1F) >> 0x1A)];
			seed += 1
		key = buff2

	for i in range(0,64):
		rsize = len(dirname)
		tmp1 = key[(i ^ 9)]
		tmp2 = dirname[i % rsize] & 0xFF
		key[i ^ 9] = (tmp1-tmp2) & 0xFF

	return key

#Determines if a response can be constructed yet for this path.
def request_state(path):
	parent_path,target_dir = os.path.split(path)
	if(parent_path in key_table):
		return True, parent_path
	else:
		return False, ""


#We could also load from a keytable at this point for new stuff.
key_table = {}
root_req = binascii.unhexlify("d5d3b1aeacb1d3d5d5d3b1aeacb1d3d5d5d3b1aeacb1d3d5d5d3b1aeacb1d3d5d5d3b1aeacb1d3d5d5d3b1aeacb1d3d5d5d3b1aeacb1d3d5d5d3b1aeacb1d3d5")
root_resp = binascii.unhexlify("c1b27c56678cc2260974dec73ba6dcd6be0c0b8039b5acd00494de2bd2da04cddb1616555de086dccba612321b7203497c1421c8cf0f5fcd40472e7cf72c05a6")

if( not os.path.exists("prime_keys.txt")):
	pass
else:

	print "Reading keys"
	f = open("prime_keys.txt","r")
	kl = f.readlines()
	f.close()

	for line in kl:
		line = line.strip()
		if(line[0] != "#"):
			key,req,resp = line.split("\t")
			key_table[key] = {"request":binascii.unhexlify(req),"response":binascii.unhexlify(resp)}
	print "Loaded keys"
	if( not os.path.exists("temp_keys.txt")):
	    pass
	else:
	    print "Reading keys from temp_keys.txt"
	    f = open("temp_keys.txt","r")
	    kl = f.readlines()
	    f.close()

	    for line in kl:
		    line = line.strip()
		    if(line[0] != "#"):
		        try:
			        key,req,resp = line.split("\t")
			        if len(binascii.unhexlify(resp)) == 64 and len(binascii.unhexlify(req)):
			            key_table[key] = {"request":binascii.unhexlify(req),"response":binascii.unhexlify(resp)}
			        else:
			            print "Invalid entry for %s" %key
		        except Exception,e:
		            print "Error reading input line. Skipping"
	    print "Loaded keys from temp_keys.txt"


append = 0
r = open(sys.argv[1],"r")
print "Reading input dirlist"
lines = r.readlines()
r.close()
ready_convert = {}
pending_convert = []

key_table["-+ROOT+-"] = {"request":root_req,"response":root_resp}
print "Generating Request"
for line in lines:
	key_path = ""
	line = line.strip()
	key_path = "-+ROOT+-"+"/"+line
	if(not key_path in key_table):
		state,parent_key = request_state(key_path)
		if(state):
			cur_dir = key_path.split("/")[-1]
			ready_convert[key_path] = {"request":makerequest(key_table[parent_key]["response"],bytearray(cur_dir)),"response":""}

		else:
			pending_convert.append(key_path)
print "Generated"

print("%d keys ready to be processed" % len(ready_convert))
print("%d keys pending responses to process" % len(pending_convert))
print("%d keys in store" % len(key_table))
if(len(sys.argv) > 2):
	if(sys.argv[2] == "i"):
		exit(0)
if(len(ready_convert)):
	#Generate new request.bin file
	reqf = open("request.bin","wb")
	req_count = 0
	for req in ready_convert.keys():
		reqf.write(ready_convert[req]["request"])
		req_count+=1
	reqf.close()

	#Let prime run the keys
	print("Processing...")
	os.system("LD_PRELOAD=./haspdump.so ./primegen %s" % GAME_PATH)
	respf = open("response.bin","rb")
	#reqf = open("request.bin","rb")
	of = open("temp_keys.txt","a")
	for req in ready_convert.keys():#reqf.read(64)
		ready_convert[req]["response"] = respf.read(64)
		key_table[req] = ready_convert[req]
		if len(key_table[req]["response"]) == 64:
		    of.write("%s\t%s\t%s\n" % (req,binascii.hexlify(key_table[req]['request']),binascii.hexlify(key_table[req]["response"])))
		del ready_convert[req]
	respf.close()
	#reqf.close()
	os.remove("response.bin")


	while(len(pending_convert)):
		print("\n%d keys remaining..." % len(pending_convert))
		for entry in pending_convert:
			key_path = entry
			state,parent_key = request_state(key_path)
			if(state):
				cur_dir = key_path.split("/")[-1]
				ready_convert[key_path] = {"request":makerequest(key_table[parent_key]["response"],bytearray(cur_dir)),"response":""}
				pending_convert.remove(entry)


		print("%d keys ready to be processed" % len(ready_convert))
		print("%d keys pending responses to process" % len(pending_convert))
		print("%d keys in store" % len(key_table))
		if(not len(ready_convert)):
			break
		#Generate new request.bin file
		reqf = open("request.bin","wb")
		req_count = 0
		for req in ready_convert.keys():
			reqf.write(ready_convert[req]["request"])
			req_count+=1
		reqf.close()

		#Let prime run the keys
		print("Processing...")
		respf = open("response.bin","wb")
		respf.close()
		os.system("LD_PRELOAD=./haspdump.so ./primegen %s" % GAME_PATH)
		respf = open("response.bin","rb")
		#reqf = open("request.bin","rb")
		of = open("temp_keys.txt","a")
		for req in ready_convert.keys():#reqf.read(64)
			ready_convert[req]["response"] = respf.read(64)
			key_table[req] = ready_convert[req]
			if len(key_table[req]["response"]) == 64:
			    of.write("%s\t%s\t%s\n" % (req,binascii.hexlify(key_table[req]['request']),binascii.hexlify(key_table[req]["response"])))
			del ready_convert[req]
		respf.close()
		#reqf.close()
		os.remove("response.bin")

	f = open("prime_keys.txt","w")
	f.write("# Pump it Up Prime HASP Rainbow Table\n")

	for key in sorted(key_table.keys()):

		f.write("%s\t%s\t%s\n" % (key,binascii.hexlify(key_table[key]['request']),binascii.hexlify(key_table[key]['response'])))

	f.close()
	print("\nFINISHED!!!")
