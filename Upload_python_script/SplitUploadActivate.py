#!/usr/bin/env python3

import argparse
import sys, time
sys.path.append("../modules")
from hertz.tester import hdlc_tester, TestError
from hertz.types import sml, cosem
list1 = list() #the list that we will use to store the splited chunks from the original binaryy filee
FW_name_type = "ITE414L3_HERTZ_PP_FW_NLR" # the string which must be in the  filename to consider it as a correct filename

def InitiateFullImage(port,Numberofblock,Major,Minor):  # function to initiate the FW_upgrade transfer Numberofbocks 
#is the number of 512B blocks we want transfer
    
    sfile = sml.SML_File()
    id = 0xaabb77f1

    msg = sml.SML_PublicOpenREQ()
    msg.id(id,width=8)
    id += 1
    msg.client(b'Gateway')
    msg.server(port.meterID)
    msg.file(b'\x01\x02\x03\x04\x05\x06\x07\x08')
    sfile.append(msg)

    msg = sml.SML_SetProcParameterREQ()
    msg.server(port.meterID)
    msg.id(id,width=8)
    id += 1
    
    path = sml.SML_TreePath()
    path.append(b'\x01\x00\x5E\x31\x02\x01\x20\x00\x00')
    path.append(b'\x00\x02')
    path.append(b'\x00\x03')
    path.append(b'\x00\x04')
    msg.treepath_T(path)
    
    # setup an invalid child value should be a list of 2 items unsigned, value
    # suppling optional, value
    
    tree = sml.SML_Tree()
    tree.name(b'\x01\x00\x5E\x31\x02\x01\x20\x00\x00')
    tree.append(b'\x00\x02', sml.SML_List([sml.SML_Unsigned(1), sml.SML_String(bytes("ITE414L31" + Major + Minor +"00000000000000", 'utf-8'))])) #write FW_name in the init msg
    tree.append(b'\x00\x03', sml.SML_List([sml.SML_Unsigned(1), sml.SML_Unsigned(Numberofblock)])) #write the number of block that we will transfer during the fW_upg
    tree.append(b'\x00\x04', sml.SML_List([sml.SML_Unsigned(1), sml.SML_Unsigned(0)])) 
    msg.tree_T(tree)

    sfile.append(msg)

    msg = sml.SML_PublicCloseREQ()
    msg.id(id,width=8)
    sfile.append(msg)

    data = sfile.encode()

    if not port.write(data,timeout=0.5):
        # no ACK received
        raise TestError("No response from meter.")
        
    print(sfile)
    port.xml_stream.write(sfile.xml())

    data = port.read(timeout=0.5)
    
    if data is None:
        # ACK received for the write, but no SML response
        raise TestError("SML response error.")
    else:
        # ACK received for the write, and SML response
        try:
            sfile.decode(data)
            print(sfile)
            port.xml_stream.write(sfile.xml())
        except sml.DecodeError as ex:
            raise TestError("SML response decode error: %s\n\nSML File:\n%s" % (ex,hexstr(data)))
    
    if type(sfile[1]) == sml.SML_AttentionRES:
        raise TestError("unexpected AttentionRES: %s" % sfile[1].attention_str())

def TransferBlock(port,blockindex,block,):  # generic function to send a block to the meter parameters are the blockindex, and the blockdatas
    sfile = sml.SML_File()
    id = 0xaabb77f1
    msg = sml.SML_PublicOpenREQ()
    msg.id(id,width=8)
    id += 1
    msg.client(b'Gateway')
    msg.server(port.meterID)
    msg.file(b'\x01\x02\x03\x04\x05\x06\x07\x08')
    sfile.append(msg)
    msg = sml.SML_SetProcParameterREQ()
    msg.server(port.meterID)
    msg.id(id,width=8)
    id += 1
    path = sml.SML_TreePath()
    path.append(b'\x01\x00\x5E\x31\x02\x02\x20\x01\x00')
    path.append(b'\x00\x02')
    path.append(b'\x00\x03')
    msg.treepath_T(path)
    tree = sml.SML_Tree()
    tree.name(b'\x01\x00\x5E\x31\x02\x02\x20\x01\x00')
    tree.append(b'\x00\x02', sml.SML_List([sml.SML_Unsigned(1), sml.SML_Unsigned(blockindex)])) #specify the block index
    tree.append(b'\x00\x03', sml.SML_List([sml.SML_Unsigned(1), sml.SML_String(block)]))  # write the data in the SML msg
    msg.tree_T(tree)
    sfile.append(msg)
    msg = sml.SML_PublicCloseREQ()
    msg.id(id,width=8)
    sfile.append(msg)
    data = sfile.encode()
    if not port.write(data,timeout=0.5):
        raise TestError("No response from meter.")
    print(sfile)
    port.xml_stream.write(sfile.xml())
    data = port.read(timeout=0.5)
    if data is None:
        raise TestError("SML response error.")
    else:
        try:
            sfile.decode(data)
            print(sfile)
            port.xml_stream.write(sfile.xml())
        except sml.DecodeError as ex:
            raise TestError("SML response decode error: %s\n\nSML File:\n%s" % (ex,hexstr(data)))
    if type(sfile[1]) == sml.SML_AttentionRES:
        raise TestError("unexpected AttentionRES: %s" % sfile[1].attention_str())

def FirmwareUpgradeActivate(port): # function to activate fW after tranfer 
    sfile = sml.SML_File()
    id = 0xaabb77f1

    msg = sml.SML_PublicOpenREQ()
    msg.id(id,width=8)
    id += 1
    msg.client(b'Gateway')
    msg.server(port.meterID)
    msg.file(b'\x01\x02\x03\x04\x05\x06\x07\x08')
    sfile.append(msg)

    msg = sml.SML_SetProcParameterREQ()
    msg.server(port.meterID)
    msg.id(id,width=8)
    id += 1
    
    path = sml.SML_TreePath()
    path.append(b'\x01\x00\x5E\x31\x02\x03\x00\x01\x00')
    path.append(b'\x00\x02')
    msg.treepath_T(path)
    
    # setup an invalid child value should be a list of 2 items unsigned, value
    # suppling optional, value
    
    tree = sml.SML_Tree()
    tree.name(b'\x01\x00\x5E\x31\x02\x03\x00\x01\x00')
    tree.append(b'\x00\x02', sml.SML_List([sml.SML_Choice(1), sml.SML_Unsigned(0)])) # value
    msg.tree_T(tree)

    sfile.append(msg)

    msg = sml.SML_PublicCloseREQ()
    msg.id(id,width=8)
    sfile.append(msg)

    data = sfile.encode()

    if not port.write(data,timeout=0.5):
        # no ACK received
        raise TestError("No response from meter.")
        
    print(sfile)
    port.xml_stream.write(sfile.xml())

    data = port.read(timeout=0.5)
    
    if data is None:
        # ACK received for the write, but no SML response
        raise TestError("SML response error.")
    else:
        # ACK received for the write, and SML response
        try:
            sfile.decode(data)
            print(sfile)
            port.xml_stream.write(sfile.xml())
        except sml.DecodeError as ex:
            raise TestError("SML response decode error: %s\n\nSML File:\n%s" % (ex,hexstr(data)))


    
def extractFW_versionFromHEAD(Head_block):
    Major = Head_block[29]
    Minor = Head_block[30]
    Majorstr = str(Head_block[29])
    Minorstr = str(Head_block[30])
    Majorstr = Majorstr.zfill(2)
    Minorstr = Minorstr.zfill(2)
    return Majorstr,Minorstr
    
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Split Hertz2firmware & upload", add_help=False)
    parser.add_argument('--filename', type=str, nargs=1, help="the name of the .bin firmware file ")
    parser.add_argument('--chuncksize', type=int, default=512, help="The size of the chunck to split the firmware")
    parser.add_argument('--skipactivation', action='store_true', help="Perform only image transfer without activation")
    
    tester = hdlc_tester(parents=[parser])  # instance an HDLC connection
    
    args = tester.args
     
    
   
    print("*******************************")
    print("Spliting...")
    print("*******************************")
    with open(str(args.filename).strip("['']"),'rb') as f: # open the .bin file
        chunk = f.read(args.chuncksize)
        list1.append(chunk) #fill the list with the first chunck
        while chunk: # while there is something to read in the bin file
            chunk = f.read(args.chuncksize) #reading the next
            list1.append(chunk) #fill the list with every chunk
            #print(type(chunk))
        list1.pop(len(list1)-1)
        MajorMinor = extractFW_versionFromHEAD(list1[0]) # extract the FW version from the firt block which contains the header
        print("*******************************")
        print("FW_Version : "+MajorMinor[0]+"."+MajorMinor[1])
        print("*******************************")

        """for i in range(len(list1)):  #display each chunk (DEBUG)
            print("***************************************************************************************************************\n")
            print("Block"+str(i))
            print("size : "+str(len(list1[i])))
            print(type(list1[i]))
            #print(list1[i].hex())
            print("***************************************************************************************************************\n")"""
        
    
    print("*******************************")
    print("Transfer...")
    print("*******************************")
    tester.jenkins_stream.suite = "FWupgrade Full Transfer"
    if not tester.connect(0x03):
        raise TestError("Unable to connect to the meter.Meter must not be in secured environment.")
    tester.runTest(InitiateFullImage,len(list1),MajorMinor[0],MajorMinor[1])
    tester.connection.retry = 0
    tester.connection.read_timeout = 0.05
    for i in range(len(list1)):  #for each block in the list
        print("***LOADING CHUNCK NUMBER "+str(i)+"***")
        tester.runTest(TransferBlock,i,list1[i]) # transfer the block, arguments are blocknumer,block,communication port through the HDLC_test class
    
    if args.skipactivation == False:
        print("*******************************")
        print("Activate...pls wait")
        print("*******************************")
        time.sleep(5)
        tester.jenkins_stream.suite = "SML FWupgrade Activation"
        tester.runTest(FirmwareUpgradeActivate) # call the firmwareUpgrade routine through the HDLC_test class
    
    tester.disconnect()
    tester.end()
