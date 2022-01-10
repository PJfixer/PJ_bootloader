import serial
import time
import crcmod 

class REM():
	"""docstring for REM"""
	def __init__(self,REM_ID,port,baudrate):
		self.rem_id = REM_ID
		self.rem_port = serial.Serial(port,baudrate,timeout=0.00001)
		self.rem_crc = crcmod.mkCrcFun(0x11021, 0x0000, False, 0x0000)
		self.rem_recv_a_start = 0
		self.rem_recv_a_end = 0
		self.received_frame = bytearray()
		self.received_data = bytearray()
		self.raw_data = bytearray()
		self.last_received_ack = -1
		self.rem_max_retry = 10




	def Rem_packFrame(self,frame_data,slaveID,frametype):
		frame = bytearray.fromhex("AA")
		frame.append(slaveID)
		frame.append(frametype)
		frame.append(len(frame_data))
		frame.extend(frame_data)
		crc = self.rem_crc(frame_data)
		frame.append(crc & 0xff)
		frame.append(((crc >> 8) & 0xff))
		frame.append(0xBB)
		return frame

	def rem_check_CRC(self,recv_data,recv_crc):
		calc_crc = self.rem_crc(recv_data)
		if(calc_crc == recv_crc):
			return 0
		else:
			return -1

	def set_max_retry(self,retry):
		self.rem_max_retry = retry


	def rem_receive(self):

			
			self.raw_data += self.rem_port.read()
			self.rem_port.flush()
			self.received_frame  = self.raw_data
			
			#print(self.received_frame)
			#print(len(self.received_frame))
			if(0xAA in self.received_frame and 0xBB in self.received_frame): #frame complete
				if(self.received_frame[0] == 0xAA and self.received_frame[(self.received_frame[3]+6)] == 0xBB): #frame complete
					print(self.received_frame.hex())
					
					self.raw_data = bytearray()
					end_frame_idx = self.received_frame.find(0xBB)
					#first check CRC
					self.received_data =  self.received_frame[4:(4+self.received_frame[3])]
					crc_l = self.received_frame[(end_frame_idx-2)]
					crc_h = self.received_frame[(end_frame_idx-1)]
					msg_crc = ( crc_h << 8) | crc_l
					
					if(self.rem_check_CRC(self.received_data,msg_crc) == 0): #if crc is good then check what kind of frame this is 
						if(self.received_frame[2] == 0xF1):
							print("FRAME TYPE DATA REQUEST")
							print(self.received_frame.hex())
							return 0
						elif(self.received_frame[2] == 0xF2):
							print("FRAME TYPE SET")
							print(self.received_frame.hex())
							return 1
						elif(self.received_frame[2] == 0xF3):
							self.last_received_ack = self.received_frame[4]
							print("FRAME SIMPLE ACK")
							print(self.received_frame.hex())
							return 2
						elif(self.received_frame[2] == 0xF4):
							print("FRAME TYPE ACK+DATA RESPONSE")
							self.last_received_ack = self.received_frame[4]
							print(self.received_frame.hex())
							print(len(self.received_frame))
							self.received_data = self.received_data[1:len(self.received_data)]
							print(self.received_data.hex())
							print(len(self.received_data))
							return 3
						
					else:
						print("CRC fucked up")
						return -1
			return -2 # if no REM frame was received 

			
		

	def Rem_send_set_wait_ack(self,set_command,slaveID,ms_timeout):
		count = 0
		tx_result = 0
		ack_status = -1
		self.last_received_ack = -1
		frame = self.Rem_packFrame(set_command,slaveID,0xF2)

		timing = (int(time.time() * 1000000))
		print(frame.hex())
		self.rem_port.write(frame)
	

		while( count < self.rem_max_retry):

			tx_result = self.rem_receive()
			print(tx_result)
			if(tx_result !=2 and (  (int(time.time() * 1000000))-timing)  > ms_timeout): #if we hit wait ack time resend the frame 
				ack_status = -1
				self.rem_port.write(frame)
				count = count + 1
				timing = (int(time.time() * 1000000))
				print("re-try")
			else:
				if(self.last_received_ack == 0x40):
					ack_status = 1
					print("round trip delay :"+str((int(time.time() * 1000000))-timing)+"us")
					break
				elif(self.last_received_ack == 0x80):
					ack_status = 0
					print("round trip delay :"+str((int(time.time() * 1000000))-timing)+"us")
					break
		   
		return count,ack_status


	def Rem_send_req_wait_ack(self,req_command,slaveID,ms_timeout):
		count = 0
		tx_result = 0
		ack_status = -1
		self.last_received_ack = -1
		frame = self.Rem_packFrame(req_command,slaveID,0xF1)

		timing = (int(time.time() * 1000000))

		self.rem_port.write(frame)
	

		while( count < self.rem_max_retry):

			tx_result = self.rem_receive()
			
			if(tx_result !=3 and (  (int(time.time() * 1000000))-timing)  > ms_timeout): #if we hit wait ack time resend the frame 
				ack_status = -1
				self.rem_port.write(frame)
				count = count + 1
				timing = (int(time.time() * 1000000))
				print("re-try")
			else:
				#print(tx_result)
				if(self.last_received_ack == 0x40):
					ack_status = 1
					print("round trip delay :"+str((int(time.time() * 1000000))-timing)+"us")
					break
				elif(self.last_received_ack == 0x80):
					ack_status = 0
					print("round trip delay :"+str((int(time.time() * 1000000))-timing)+"us")
					break
		   
		return count,ack_status


		
	def Rem_get_object(self,i_object,slaveID,ms_timeout):
		status = 0
		response = bytearray()
		command = bytearray()
		command.append(i_object)
		
		result = self.Rem_send_req_wait_ack(command,slaveID,ms_timeout)
		if(result[0] > 1):
			print("total retry : "+str((result[0])))
		if(result[1] == 1):
			response = self.received_data
			status = 1
			print("Received_ack")
		elif(result[1] == 0):
			print("received_nack")
			status = 0
		else:
			print("slave no answer")
			status = 0
		return status,response


	def Rem_set_object(self,i_object,i_datatoset,slaveID,ms_timeout):
		status = 0
		data = bytearray()
		data.append(i_object)
		data.append(i_datatoset)
		print(data)
		result = self.Rem_send_set_wait_ack(data,slaveID,ms_timeout)
		if(result[0] > 1):
			print("retry : "+str((result[0])))
		if(result[1] == 1):
			
			status = 1
			print("Received_ack")
		elif(result[1] == 0):
			print("received_nack")
			status = 0
		else:
			print("slave no answer")
			status = 0
		return status