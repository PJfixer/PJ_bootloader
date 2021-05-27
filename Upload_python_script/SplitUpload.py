#!/usr/bin/env python3

import argparse
import sys, time
import serial
import struct
list1 = list() #the list that we will use to store the splited chunks from the original binaryy filee

def read_port():
	data = bytes()
	
	while serial.in_waiting:  # Or: while ser.inWaiting():
		data = serial.read(serial.in_waiting)
	return data
	
def push_chunks(chunk):
	frame = bytearray(b'#') #packet start
	frame.extend(chunk)
	frame += b'!'
	serial.write(frame)


    
if __name__ == '__main__':
	parser = argparse.ArgumentParser(description="Split Firmware & upload", add_help=True)
	parser.add_argument('--filename', type=str, nargs=1, help="the name of the .bin firmware file ")
	parser.add_argument('--port', type=str, help="serial port  to upload firmware ")
	parser.add_argument('--chuncksize', type=int, default=4, help="The size of the chunck to split the firmware")
	parser.add_argument('--mcu_page_size',type=int, default=0x400, help="the mcu page size to calculate how many page erase before flash the binary")
	parser.add_argument('--erase_only',type=int, default=-1, help="if this argument is invoked the script will only connect bottload and erase x pages of app firmware")
	args = parser.parse_args()
    
     #opening serial port 
	serial = serial.Serial(args.port, 115200)


	
	
	if(args.erase_only != -1):
		print("*******************************")
		print("Perform erase only")
		print("*******************************")
		flash_command = bytearray("#ERASE_MEM", 'utf-8') 
		flash_command.extend(struct.pack('<L',int(args.erase_only)))
		flash_command += b'!'
		serial.write(flash_command)
		print(flash_command)
		time.sleep(0.1)
		response = read_port()
		if(response.rstrip().decode('utf-8') == "Flash: Erased!"):
			print("app FW succesfully erased ! \n")
		
		else:
			print("error erasing app FW ! \n")
	
	
	else: #normal firmware flash
	
		print("*******************************")
		print("Spliting...")
		print("*******************************")
		with open(str(args.filename).strip("['']"),'rb') as f: # open the .bin file
			chunk = f.read(args.chuncksize)
			list1.append(chunk) #fill the list with the first chunck
			while chunk: # while there is something to read in the bin file
				chunk = f.read(args.chuncksize) #reading the next
				list1.append(chunk) #fill the list with every chunk
			list1.pop(len(list1)-1) #remove last chunk (end of file)
			if(len(list1[len(list1)-1]) < int(args.chuncksize)): # check the size of last chunks and padd it if not equal to chunksize
				print("last chunks is only "+str(len(list1[len(list1)-1]))+" bytes long --> padding to 256")
				diff =   int(args.chuncksize) - len(list1[len(list1)-1])
				print(diff)
				list1[len(list1)-1] +=  bytes(b'\xff' * diff)
				print("last chunks is now "+str(len(list1[len(list1)-1]))+ " bytes long" )
				#print(list1[len(list1)-1])
				
			print(".bin splitted in " +  str(len(list1)) +  " chunks \n \n") 
			numberPage_toErase = round( ((len(list1) * args.chuncksize) / args.mcu_page_size) + 0.5)
			print("according the firmware size and mcu page size, "+str(numberPage_toErase)+" pages wille be erased before flashing")
			time.sleep(5)
	
    
		
		print("*******************************")
		print("Initiating Flash process")
		print("*******************************")
		flash_command = bytearray("#FLASH_START", 'utf-8') 
		flash_command.extend(struct.pack('<L',int(numberPage_toErase)))
		flash_command += b'!'
		serial.write(flash_command)
		
		time.sleep(0.1)
		response = read_port()
		if(response.rstrip().decode('utf-8') == "Flash: Unlocked!"):
			print("set Flash mode : SUCCESS \n")
			time.sleep(0.5)
			print("start flashing APP FW \n")
			push_index = 0
			for chunks in list1:
				
				push_chunks(chunks)
				time.sleep(0.5)
				response = read_port()
				
				if(response.rstrip().decode('utf-8') == "Flash: OK"): #if the block is succesfully loaded
					push_index += 1
					print("send block : "+str(push_index)+"/"+str(len(list1)),end='\r')
					
			
			
				else:
					print("ERROR FLASHING CHUNKS ! ABORTING FLASH ")
					print(response)
					serial.write(b'#FLASH_ABORT!')
					
					break
					
			time.sleep(1)
			serial.write(b'#FLASH_FINISH!')
			time.sleep(0.1)
			response = read_port()
			if(response.rstrip().decode('utf-8') == "Flash: Success! Rebooting !"):
				print("\n firmware update complete succesfully")
			else:
				print("set FLASH_FINISH & REBOOT : FAILED")
				print(response)
			
			
		
		else:
			print("set Flash mode : FAILED  \n \n")
			print(response)

	
	
