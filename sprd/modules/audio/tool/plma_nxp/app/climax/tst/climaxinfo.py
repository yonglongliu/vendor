"""
This file works as library for robot Test framework
this file contains robot framework keywords, which can be used to parse climax commands, check climax output 
keywords in this file can work for both Windows operating system and Linux operating sytem
Author: Ji Xia
Date:   March and April of 2014
"""

import sys, inspect, time, threading, subprocess, os, re, filecmp, platform, commands, psutil
from robot.libraries.BuiltIn import BuiltIn
from robot.libraries.OperatingSystem import OperatingSystem
from datetime import datetime
from string import punctuation
import binascii
import difflib

import DatasheetNames88
import DatasheetNames90
import DatasheetNames91
import DatasheetNames95
import DatasheetNames97
from operator import contains


def printLines(string):
    """
    This function print a string per line so that each line is viewable in RTF outputs.
    """
    for line in string.splitlines():
        print "*INFO*", line

        
def tupleWithout(original, element):    
    """
    Removes an element (given as parameter) from a list (given as parameter original)
    """
    lst = list(original)
    while lst.count(element) > 0:
        lst.remove(element)
        
    return tuple(lst)

    
def QuietShouldBeEmpty(line):
    """
    Checks if no messages are printed
    It returns True if line only contains whitespace characters.
    """
    return line.strip() == ""

    
def ValidateDataSheetnames(msg, deviceId):
    """
    Checks if the register names returned climax read register correspond to the datasheetnames. 
    Returns True if the datasheetnames match the names printed by climax.
    """
    printLines("msg: " + msg)
    
    namesWithValues = []
    
    for line in msg.splitlines():
        if line.strip().startswith("[0x"):
            addr = line.split()[1]
            printLines("addr: " + addr)
            if 'Linux' in platform.system():
                #The address line contains the datasheetnames with values, but the first three words contain the adress + values
                namesWithValues = line.split()[4:]
    
    if 'Windows' in platform.system():        
        namesWithValues = msg.splitlines()[-1].split()
        if 'lxDummyInit:' in msg:
            namesWithValues = msg.splitlines()[-2].split()
    
    dsns = []
    for name in namesWithValues:
        dsns.append(name.split(":")[0])
        printLines("name: " + name)
        
    if deviceId == "88":
        #Compare to the datasheetnames produced from the current I2C list (reversed because the I2C list lists the dsns in reverse order) 
        return BuiltIn().should_be_equal(dsns,list(reversed(DatasheetNames88.DATASHEETNAMES[int(addr, 16)])))   
    elif deviceId == "90":
        return BuiltIn().should_be_equal(dsns,list(reversed(DatasheetNames90.DATASHEETNAMES[int(addr, 16)])))
    elif deviceId == "91":
        return BuiltIn().should_be_equal(dsns,list(reversed(DatasheetNames91.DATASHEETNAMES[int(addr, 16)])))
    elif deviceId == "95":
        return BuiltIn().should_be_equal(dsns,list(reversed(DatasheetNames95.DATASHEETNAMES[int(addr, 16)])))
    elif deviceId == "97":
        return BuiltIn().should_be_equal(dsns,list(reversed(DatasheetNames97.DATASHEETNAMES[int(addr, 16)])))
    else: 
        raise Exception("Datasheetname checking is not implemented for " + deviceId)
	return False
                  
                  
def hasDatasheetNames(addr, deviceId):
    """
    Checks the DatasheetNamesXX files for which registers have a name in the given register address
    It returns True if a name has been found on the given address for the given deviceID, otherwise it returns False
    """
    if deviceId == "88":
        if addr in DatasheetNames88.DATASHEETNAMES.keys():
            return len(DatasheetNames88.DATASHEETNAMES[addr])> 0
        else: 
            return False
    elif deviceId == "90":
    	if addr in DatasheetNames90.DATASHEETNAMES.keys():
    	    return len(DatasheetNames90.DATASHEETNAMES[addr]) > 0
    	else:
    	    return False
    elif deviceId == "91":
    	if addr in DatasheetNames91.DATASHEETNAMES.keys():
    	    return len(DatasheetNames91.DATASHEETNAMES[addr]) > 0
    	else:
    	    return False
    elif deviceId == "95":
        if addr in DatasheetNames95.DATASHEETNAMES.keys():
            return len(DatasheetNames95.DATASHEETNAMES[addr]) > 0
        else:
            return False
    elif deviceId == "97":
        if addr in DatasheetNames97.DATASHEETNAMES.keys():
            return len(DatasheetNames97.DATASHEETNAMES[addr])> 0
        else: 
            return False
    else: 
        raise Exception("Datasheetname checking is not implemented for " + deviceId)
	return False

    
def validateCalibrationMessage(string, deviceId, runNumber):
    """
    Validates if the calibration values are returned correctly by climax
    If deviceId equals '88' it checks both left and right calibration values else just one value
    The function checkCalibrationValue is used to check if the values are correct
    Returns an exception if no value is found
    """
    foundCalImpValues = False

    if(int(runNumber) > 1):
        if "DSP already calibrated" not in string:
            raise Exception("If MTPEX is not manually reset, calibrate=once should not calibrate again")

    for line in string.splitlines():     
        if line.endswith("ohm"):
            if deviceId == '88':
                calValL = float(line.split()[-2])                 
                calValR = float(line.split()[-3])
                printLines("Calvalues =" + str(calValL) + " " + str(calValR))
                checkCalibrationValue(calValL)
                checkCalibrationValue(calValR)
                foundCalImpValues = True
            else:
                calVal = float(line.replace("Calibration value is: ", "").replace(" ohm", "").strip())
                checkCalibrationValue(calVal)   
                foundCalImpValues = True
                
    if not foundCalImpValues:
        raise Exception("No calibration values found")
             
        
def validateCalshowMessage(string, deviceId, speakerSelected):
    """
    Validates if the Calibrate (impedance and Tcoefficient) values are set properly.
    It raises an Exception if no value is found. 
    It calls the checkCalibrationValue function to check if the value is valid.
    """
    foundCalImpedanceValues = False
    foundCalTCoefficientValues = False
    
    for line in string.splitlines():
        if line.startswith("Current calibration impedance:"):
            foundCalImpedanceValues = True
            if deviceId == '88' and (speakerSelected != 'L' or speakerSelected != 'R'):
                if speakerSelected != "L" and speakerSelected != "R":
                    calImpedanceLeft = float(line.split()[-1])
                    calImpedanceRight = float(line.split()[-2])
                    printLines("Calvalues = " + str(calImpedanceLeft) + " " + str(calImpedanceRight))
                    checkCalibrationValue(calImpedanceLeft)
                    checkCalibrationValue(calImpedanceRight)
            else:
                calImpedance = float(line.split()[-1])
                printLines("Impedance value = " + str(calImpedance))
                checkCalibrationValue(calImpedance)
                
        if line.startswith("Current calibration tCoef"):
            foundCalTCoefValues = True
            calTCoef = float(line.split()[-1])
            printLines("CalTCoef = " + str(calTCoef))
            if calTCoef <= 0:
                printLines("calTCoef= " + str(calTCoef) + " needs to be > 0.")
                raise Exception("Calibration tCoef <= 0")
            
    if (not foundCalImpedanceValues) or (not foundCalTCoefValues):
        raise Exception("No calibration values found")

        
def checkCalibrationValue(calVal):
    """
    Tests if the calibration impedance value is within an excepted range.
    It raises an exception if it deviates more than 20% from the value 8.0
    """
    maxCalVal = 1.20*8
    minCalVal = 0.80*8
    if int(calVal) > int(maxCalVal):
        printLines("Calibration value of " + str(calVal) + " is greater than " + str(maxCalVal))
        raise Exception("Calibration value is greater than allowed")

    if int(calVal) < int(minCalVal):
        printLines("Calibration value of " + str(calVal) + " is smaller than " + str(minCalVal))
        raise Exception("Calibration value is smaller than allowed")
    
    printLines("Calibration of " + str(calVal) + " is between " + str(minCalVal) + " and " + str(maxCalVal))
    
        
def versionCheck(string): 
    """
    Check if the right version numbers and deviceID numbers are returned by climax --versions
    """
    deviceNumbers = ( "88", "92", "80", "12", "97", "91", "9x" )
    printLines("Full output for --versions: " + string)
    for line in string.splitlines():        #Split all strings into lines
        if "HW  rev" in line:
            #if line[-6:] != getDeviceNumber()
            readNumber = line[-2:]
            if not any(x in readNumber for x in deviceNumbers): #Check if the number is not contained in the list of legitimate numbers
                raise Exception( "Device Number not correct:", readNumber )
            else:
                printLines("Device Number " + readNumber + " is correct")
               
        if "DSP revstring" in line:
            length = len(line)
            if length < 20 :                #For max2 version always should start with 2
                raise Exception("DSP revstring cannot be empty")
            else:
                printLines("DSP revstring length: " + str(length))


def CheckTraceLines(string): 
    """
    Checks the number of trace lines containing an I2C write/read message
    Raises an exception if this number is less then 4
    """
    count = 0
    for line in string.splitlines(): 
        if "I2C W" in line:
            count = count + 1
        if "I2C R" in line:
            count = count + 1
    
    if count < 4:
        raise Exception("Expected 4 lines of trace, but only got", count)
    else:
        printLines("Got " + str(count) + " trace lines")
        

def validateDiagMessage(string, deviceId):
    """
    Validates if "code=0" is contained in the diag return message confirming all tests succeeded
    Raises an exception if "code=0" is not found (thus not all tests succeeded)
    """
    code = -1
    for line in string.splitlines():
        if "code=0" in line:
            code = 0
            
    if code != 0:
        raise Exception("Diag failed")
    else:
        printLines("Found message (code=0)")
 
 
def getAddressesFromIniFile(string):
    """
    Searches through the cnt dump to find the addresses declared in there (0x34/0x36/0x37)
    """
    addresses = []
    for line in string.splitlines():
        if "dev=" in line:
            # add the device name (without spaces) to a list:
            addresses.append((line.split("=")[-1]).strip())
    return addresses 
        
        
def getLiveDataNamePerAddress(string, address):
    """
    Searches through the cnt dump to find the liveData segment names.
    Returns the liveData name or None if no liveData segment was found.
    """
    liveDataName = None
    
    if "slave=" in address:
        address = address.split("=")[-1]
    
    if ("dev="+address) in string:
        # get the segment between dev=<address> and the next '[' character.
        segment = (string.split("dev="+address)[-1]).split("[")[0]
        for line in segment.splitlines():
            if "livedata=" in line:
                liveDataName = line.split("=")[-1]
    else:
        printLines("no device name [" + deviceName + "] found or no livedata section found.")
        
    return liveDataName

    
def getLiveDataSection(string, liveDataName):
    """
    Searches through the cnt dump to find the text section for a given liveData Name.
    Returns a string containing the section with names to track by record.
    """
    section = None
        
    if liveDataName is None:
        printLines("invalid liveDataName parameter given to getLiveDataSection")
        return None
        
    if "["+liveDataName+"]" in string:
        section = string.split("["+liveDataName+"]")[-1] # get all text after [<liveDataName>]
        if "[" in section:
            section = section.split("[")[0] # get the text section up to the next livedata section
        if "container:" in section:
            section = section.split("container:")[0]
    else:
        printLines("No section found in cnt file for livedata with name: " + liveDataName)
    return section

    
dspStateInfoRegisters = ["agcGain", "limitGain", "limitClip", "batteryVoltage", "speakerTemp" ,"icTemp", "boostExcursion", "manualExcursion", "speakerResistance", "shortOnMips"]
          
def hasDspInfoRegisters(string):
    """
    Checks if specific DSP (state info) items are tracked by checking if those item names occur in the cnt dump.
    """
    allSections = ""
    addresses = getAddressesFromIniFile(string)
    for address in addresses:
        liveDataName = getLiveDataNamePerAddress(string, address)
        if liveDataName is not None:
            allSections += (getLiveDataSection(string, liveDataName))
    if any(reg in allSections for reg in dspStateInfoRegisters): 
        return True
    else:
        return False
        

def determineMemtrackItems(string, deviceId, address):
    """
    Determines the number of items --record has to track for each address.
    This is information is obtained from the cnt dump.
    Note: refer to documentation for specifics on the --record function.
    """
    
    dspDrcInfoRegisters = ["GRhighDrc1[0]", "GRhighDrc1[1]", "GRhighDrc2[0]", "GRhighDrc2[1]", "GRmidDrc1[0]", "GRmidDrc1[1]", "GRmidDrc2[0]", "GRmidDrc2[1]", 
                "GRlowDrc1[0]", "GRlowDrc1[1]", "GRlowDrc2[0]", "GRlowDrc2[1]", "GRpostDrc1[0]", "GRpostDrc1[1]", "GRpostDrc2[0]", "GRpostDrc2[1]", "GRblDrc[0]", "GRblDrc[1]"]
    
    memTrackItems = 0
    defaultItemsMax1 = len(dspStateInfoRegisters)

    if "livedata" not in string:
        if "88" in deviceId:
            memTrackItems = 0
        else:
            memTrackItems = defaultItemsMax1
    else:
        section = ""
        liveDataName = getLiveDataNamePerAddress(string, address)
        if liveDataName is not None:
            section = getLiveDataSection(string, liveDataName)

        if hasDspInfoRegisters(string):    
            for line in section.splitlines():
                if "=" in line: 
                    memTrackItems += 1
        else:
            for line in section.splitlines():
                if "=" in line: 
                    memTrackItems += 1
            if not "88" in deviceId:
                # add the default number of dspStateInfoItems:
                memTrackItems += defaultItemsMax1 

    # if device contains DRC info registers (like tfa9891), always add those to memTrackItems:
    if deviceId in ["91", "95"]:
        memTrackItems += len(dspDrcInfoRegisters)
        printLines("memTrackItems = " + str(memTrackItems) + " of which " + str(len(dspDrcInfoRegisters)) + " are DRC info registers.")
                
    return memTrackItems

    
def ignoreTestFunction(string, memTrackItems, address):
    """
    Checks if climax is returns an error message when no livedata is in the ini file.
    After that the rest of the tests for that file with record can be skipped.
    """
    if "0x34" in address and memTrackItems == 0:
        if "Unable to print record data without livedata section." in string:
            return True
        else:
            raise Exception("Record return message is not in expected format")
    
    
def validateRecordFieldnames(string, memTrackItems, address):
    """
    Validates if the number of fieldnames printed is correct for each address.
    The count is an addition of the address specific and general fieldnames.
    """
    count = 0

    if "slave=" in address:
        address = address.split("=")[-1]
    
    if ignoreTestFunction(string, memTrackItems, address):
        return True
    
    # obtain the line that contains all registers that will be printed:
    registersInfo = (string.split(os.linesep+"1: 0x")[0]).split(os.linesep)[-1]
    trackedRegisters = []
    trackedRegisters = registersInfo.split(",")
    for register in trackedRegisters:
        if register > "":
            if "(0x" in register:
                if address in register:
                    count += 1
            else:
                count += 1
                
    if count == memTrackItems:
        printLines("Number of fieldnames printed for device " + address + " is correct!")
    else:
        raise Exception("Number of fieldnames printed for device", address, "is NOT correct!")
    
    
def validateRecordValueCount(string, memTrackItems, address, countValue):
    """
    Validates if the number of values per record line per address is correct and if the number of record lines is correct.
    """
    valuesCount = 0
    linesCount = 0
    values = []
    
    if "slave=" in address:
        address = address.split("=")[-1]
    
    if ignoreTestFunction(string, memTrackItems, address):
        return True
    
    # obtain the first line with values for the given address:
    valueLine = (string.split(": "+address)[-1]).split(os.linesep)[0]
    if 'Linux' in platform.system() and 'lxDummyInit:' in string:
        valueLine = (string.split(": "+address)[-1]).split(os.linesep)[1]
        
    values = valueLine.split(",")
    for value in values:
        if value > "":
            valuesCount += 1
            
    if valuesCount == memTrackItems:
        printLines("Number of values printed for device " + address + " is correct!")
    else:
        raise Exception("Number of values printed for device", address, "is NOT correct!")
        
    for line in string.splitlines():
        if (": "+address) in line:
            linesCount += 1
            
    if str(linesCount) == countValue:
        printLines("Number of record lines is correct!")
    else:
        raise Exception("Number of record lines is correct! Expected: ", countValue, "lines and detected:", linesCount, "lines." )

        
def validateDumpmodelMessage(string, deviceId, model):
    """
    Validates if the excursion or impedance are within the right bounds
    Raises an exception if the number of lines is not correct or if impedance or excursion is out of bounds
    """
    Itemcount = 0
    for line in string.splitlines():
        Itemcount = Itemcount + 1
        if ("," in line) and ("Hz" not in line):
            parts = line.partition(",") 
            try:
                if model == "x":
                    printLines("parts[0]: " + parts[0] + " parts[2]: " + parts[2])
                    if float(parts[2]) < 0.0001:
                       raise Exception(parts[0], "Hz is smaller then 0.0001 mm/V") 
                    if float(parts[2]) > 0.9:
                       raise Exception(parts[0], "Hz is bigger then 0.5 mm/V") 
                else:
                    if float(parts[2]) < 2:
                       raise Exception(parts[0], "Hz is smaller then 2 Ohm") 
                    elif float(parts[2]) > 25:
                       raise Exception(parts[0], "Hz is bigger then 25 Ohm")
            except ValueError:
                printLines("not a float: " + parts[2])
    
    if deviceId != "88":
        Itemcount/=2
    
    if Itemcount < 60 or Itemcount > 70:
        raise Exception("Number of items is not correct: ", Itemcount)


def getWordsFromString(string):
    """
    This function filters punctuations from a string.
    Returns a list containing the words found in a string.
    """
    for c in string:
        if c == '[' or c == ']':
            string= string.replace(c,"")
    
    # used to find/filter punctuations in a string
    r = re.compile(r'[{}]')
    
    formattedString = r.sub(' ',string)
    return formattedString.split()


def countWordsInString(string):
    """
    Returns the number of words found in a string.
    """
    return len(getWordsFromString(string))


def getFirstXNames(string, x):
    """
    Returns a substring that contains the first x words of string
    """
    words = getWordsFromString(string)
    bitFieldNames = ""
    for index, word in enumerate(words):
        if index < x:
            bitFieldNames += (word+" ")
    return bitFieldNames
    
    
def validateBitfieldDumpMessage(message, bitfieldsString):
    """
    Validates if all bitfields are printed by climax <bitfield> command
    Raises an exception if not all expected bitfields are present with their bitfield value
    """
    bitfieldDict = {
    "powerdown" : "PWDN",
    "cf_configured" : "SBSL",
    "reset" : "I2CR",
    "flag_cold_started" : "ACS",
    "src_set_configured" : "MANSCONF",
    "flag_man_operating_state" : "MANOPER"
    }
    
    bitFields = getWordsFromString(bitfieldsString)
    for bitField in bitFields:
        if bitField in bitfieldDict.keys():
            bitField = bitfieldDict[bitField]
        if (bitField+":0" not in message) and (bitField+":1" not in message) and ("Unknown bitfield" not in message):
            raise Exception("Climax does not return", bitfieldDict[bitField], "from bitfields:", bitfieldsString)

            
def validateDumpMessage(string, deviceId):  
    """
    Validates if a number of predefined registers can be found in the register dump
    Raises an exception if one of the expected registers is not printed
    """      
    generalRegisters = ["PWDN", "VDDS", "REV", "CLKS", "AMPS", "AREFS" ]
    registersTfa88 = ["BODE", "AUDFS", "HAPTIC", "MANOPER"]
    registersTfa90 = ["MTPEX", "TEMPS", "RST"]    


    for bitField in generalRegisters:
        if bitField not in string:
            raise Exception("Expected bitfield:", bitField, "not found")

    if deviceId == "88":
        for bitField in registersTfa88:
            if bitField not in string:
                raise Exception("Expected bitfield:", bitField, "not found")

    if deviceId != "88":
        for bitField in registersTfa90:
            if bitField not in string:
                raise Exception("Expected bitfield:", bitField, "not found")

                
def checkDisplaySlaves(string, devAddr, devId):
    """
    Checks if the right slave addresses are printed when slave=-1 is used.
    """
    if devId == "88":
        if not("slave[0]:" in string):
            raise Exception("Not all expected slaves are found");
    else:
        if not ( ("slave[0]:" in string) and ("slave[1]:" in string) ):
            raise Exception("Not all expected slaves are found");
    
    if not(devAddr in string):
        raise Exception("Expected slave device number not printed");
    printLines("Display slaves with option slaves=-1 works.")

    
def checkVolBitfield(string, devId, valOne, valTwo):
    """
    Extracts the values of register VOL for each address.
    Manually adds value 0 for device 88 for code reuse.
    Raises an Exception if the values are not set as expected (to valOne and valTwo).
    """
    values = ["0", "0"]
    for i, line in enumerate(string.splitlines()):
        values[i] = line.split(":")[-1]
    if valOne in values[0] and valTwo in values[1]:
        printLines("Vol bitfield set correctly for one device")
    else:
        raise Exception("Proper bitfield not set for single slave address! Found values: ", values[0], "and", values[1]);
    
    
def validateContainerDump(string, deviceId):
    """
    Checks if the dump of the container file is big enough
    Raises an exception is the number of lines printed is less than 10
    """
    nrOfLines = 0
    
    for line in string.splitlines():
        nrOfLines = nrOfLines + 1
        
    if nrOfLines < 10:
        raise Exception("Container file is to short. Expected at least 10 lines")

        
def validateSplitparms(string, deviceId, pathCnt):
    """
    Validates if the parameters in the .ini file are correctly split up in different files by climax
    Raises an exception if the right folders and files are not created and modified
    """
    from os import listdir
    from os.path import isfile, join
    import os.path, time
    from datetime import datetime
    import shutil
    splitParmsPresentBefore = False
    listFilenames = []
    Matches = 0
    found = 0
    PossibleFileNames = ("patch", "speaker", "vstep", "msg", "config", "preset", "drc", "eq", "volstep", "ini")
    
    pathCnt = pathCnt.replace( pathCnt.split(os.sep)[-1], "" )
    printLines("relative pathCnt: " + pathCnt)
    
    #find the splitparms folder created by splitparms
    dirs = os.listdir(pathCnt)
    for file in dirs:
        printLines("file: " + file)
        if "splitparms" in file:
            pathCnt = pathCnt + file
            printLines("dirname: " + file)
            found = 1
    
    if found != 1:
        raise Exception("Unable to find splitparms folder!")
    
    printLines("path of splitparms: " + pathCnt)
    printLines("string: " + string)
    
    #Get all file names
    for line in string.splitlines():
        list = line.partition("=")  
        if any(s == list[0] for s in PossibleFileNames):
			listFilenames.append(os.path.split(list[2])[-1])

    printLines("Number of files Found: " + str(len(listFilenames)))
    #Remove duplicate    
    listFilenames = set(listFilenames)
    printLines("Number of Unique files: " + str(len(listFilenames)))
    
    #Get all files from the containerfile path
    files = [ f for f in listdir(pathCnt) if isfile(join(pathCnt,f)) ]

    #Compare filenames with files in the directory
    #Count the number of matches, and check the modified date
    for item in listFilenames:
        if len(item) > 1:
            for file in files:
                if file == item:
                    printLines("Match between: " + item + " and " + file)
                    Matches = Matches + 1
                    now = datetime.now()
                    then = datetime.fromtimestamp(os.path.getmtime(join(pathCnt,file)))
                    tdelta = now - then
                    seconds = tdelta.total_seconds()
                    printLines("Current time: " + str(now))
                    printLines("File" + file + " modified: " + str(then))
                    printLines("Difference in seconds: " + str(seconds))
                    
                    if seconds > 5:
                        raise Exception("The file", file, "is not created or modified!")
                        break
    
    #Verify the number of files matching is correct
    if len(listFilenames) != Matches:
        raise Exception("Not all files are found! Found", Matches, "of", len(listFilenames), "files")
    else:
        printLines("All files are found! Found " + str(Matches) + " of " + str(len(listFilenames)) + " files")
        #If the folder is created then we should remove it!
        if ("splitparms" in pathCnt):
            shutil.rmtree(pathCnt, ignore_errors=True)
            printLines("removed: " + pathCnt)


def getFileListFromLocation(string, pathCnt, fileType):
    """
    Checks if the filetype is present in string parameter.
    Then searches for the path that contains files of type fileType.
    Once that is found return a list of strings containing a file path.
    Raises an exception if no fileType is found in the given string parameter.
    """
    name = 0
    result = 0
    pathFile = 0
    pathFileList = []
    
    #Get file name from container
    for line in string.splitlines():
        if fileType in line:
            part = line.partition("=")
            _name = part[-1]
            name = os.path.split(_name)[-1]
			
            #Create new locations
            pathFile = findFilefromPath(pathCnt, name)
            
            if pathFile == 0:
                #result = os.path.join(pathCnt, name)
                raise Exception("Expected file " + name + " not found")
            else:
                fileNameItem = os.path.join(pathFile, name)
            
            pathFileList.append(fileNameItem)
   
    if len(pathFileList) == 0:
       raise Exception("No " + fileType + " file found")

    printLines("returning list of filepaths: ")
    for filename in pathFileList:
        printLines(filename)

    return pathFileList


def getFilesFromLocation(string, pathCnt, fileType):
    """
    Returns first index of list of file paths obtained from the getFileListFromLocation function.
    """
    return getFileListFromLocation(string, pathCnt, fileType)[0] 


def findFilefromPath(path, name):
    """
    Walks through all subdirectories of the path parameter
    Returns the first file that has the same name as the name parameter
    """
    for dirName, subdirList, fileList in os.walk(path, topdown=False):
        print "*INFO*", ('Found directory: %s' % dirName)
        for fname in fileList:
            if (name in fname) and (".svn" not in fname):
				print "*INFO*", ('Found file: %s' % fname)
				return dirName
    return 0


def verifyReturnMessage(string):
    """
    Checks if climax is succesfully executed.
    Raises an exception if climax doesn't return message status 0 (OK) 
    Also raises an exception if the return message contains a DSP error message
    """
    if "last status :0 (Ok)" not in string:
        raise Exception("return message is not :0 (Ok)")
    elif "DSP msg stat:" in string:
        raise Exception("Found a unexpected DSP status msg")
    else:
        printLines("return message is :0 (Ok)")


def getOriginalFileSize(string):
    """
    Tries to read the size of the file by reading the number behind "size:" in the given string
    Raises an exception if no valid number or size property is found in the string.
    """
    fileSize = 0
    for line in string.splitlines():
        if "size:" in line and "CRC:" in line:
            words = re.split(':| ',line)
            for word in words:
                try:
                    fileSize = int(word)
                    break
                except ValueError:
                    pass # No need to print anything here!
    
    if fileSize == 0:
        raise Exception("File size was not found!")
         
    return fileSize    
                

def verifyBin2hdrMessage(string, originalSize):
    """
    Checks if the strings "Robot" "Frame" "Work" strings have been placed in the file header.
    Also checks the amound of words is still the same as before.
    Raises an error if the header hasn't changed or the file size has changed.
    """
    fileSize = 0
    for line in string.splitlines():
        if "customer" in line and "application" in line and "type" in line:
            if "Robot" not in line and "Frame" not in line and "Work" not in line:
                raise Exception("header is not changed!")
        if "size:" in line and "CRC:" in line:
            words = re.split(':| ',line)
            for word in words:
                try:
                    fileSize = int(word)
                    break
                except ValueError:
                    pass # No need to print anything here!
    
    if fileSize == 0: 
        raise Exception("File size was not found!")
    if fileSize != originalSize: 
        raise Exception("File size is changed! old:", originalSize, "new:", fileSize)

        
def getNrOfSteps(string):     
    """
    Returns the number of volume steps out of a vstep file dump string.
    """
    result = 0
    
    for line in string.splitlines():
        if ("VOL=" in line) or ("vstep[" and "att:" in line):
            result = result + 1 
    return result


def verifyVOLregister(string, VOLvalue, CurrentStep):  
    """
    Searches through the vstep file string to find the CurrentStep string.
    Once it finds the Currentstep it gets the value that belongs to the volume step.
    Then it checks if the VOLvalue is equal to the volume step value that was found.
    Raises an exception if no volume value is found or if it doesn't match with the expected value.
    """
    import re
    count = 0   
    #results should be initialised different so it fails when nothing happends
    resultFile = None
    resultReg = None

    #Get VOL value from vstep file MAX2
    for line in string.splitlines():
        if "VOL=" in line:            
            if count == CurrentStep:
                resultFile = float(line.split('VOL=')[-1].split(' ')[0])
            count = count + 1
    
    #Get VOL value from vstep file MAX1
    for line in string.splitlines():
        if ("vstep[" in line) and ("att:" in line):            
            if count == CurrentStep:
                resultFile = float((line.split('att:')[-1]).split(' ')[0])
                printLines(str(resultFile))
            count = count + 1
    
    #Convert from attenuation to VOL level
    if resultFile < 0:
        resultFile = -2*int(resultFile)
    if resultFile > 255:
        resultFile = 255

    #Get VOL value from register dump
    resultReg = None
    for line in VOLvalue.splitlines():
        if ("VOL" in line) or ("VOLSEC" in line):
            resultReg = int(line.split(':')[-1])
    
    if resultFile == None:
        raise Exception("Vstep value could be retrieved from vstep file")
    elif resultReg == None:
        raise Exception("Vstep value could be retrieved from VOL register")
    elif resultFile != resultReg:
        raise Exception("VOL register value: (",resultReg,") does not match expected setting vstep file: (",resultFile,")", "for vStep: ", CurrentStep )
    else:
        printLines("VOL register value: (" + str(resultReg) + ") matches vstep file: (" + str(resultFile) + ")")
    
    
def verifyVstepBitfields(string, cmpValue):
    """ 
    Searches for the SWVSTEP/SWPROFIL value in string and compares it to cmpValue
    Raises an error if cmpValue+1 is not equal to SWVSTEP/SWPROFIL's value
    """
    value = None
    for line in string.splitlines():
        if ("SWVSTEP" in line) or ("SWPROFIL" in line):
            value = int(line.split(':')[-1])
    
    if (value == None): 
        raise Exception("No SWVSTEP/SWPROFIL found")
    elif (value != (cmpValue+1)):
        raise Exception("SWVSTEP/SWPROFIL register has value", value, "instead of expected value", cmpValue+1)
    
    
def getRandomProfile(string, deviceId): 
    """ 
    Reads the number of profiles returned by the climax container file dump
    Returns a random number that is assigned to a profile
    """
    import random
    profileCount = 0
    randomProfile = -1
    powerdownProfiles = []

    # Make sure no powerdown/standby profile is used for testing:
    for line in string.splitlines():
        if "profile" in line:
            profileCount += 1
        if ("profile=" in line) and (("powerdown" in line) or ("standby" in line)):
            powerdownProfiles.append(profileCount-1)
        
    if deviceId is not '88':
        profileCount /= 2
        
    if(len(powerdownProfiles) == profileCount): 
        raise Exception("No profile found in cnt file")
        
    randomProfile = random.randrange(0,profileCount, 1)  

    while(randomProfile in powerdownProfiles):
        randomProfile = random.randrange(0,profileCount, 1)  
        
    return randomProfile


def getProfileForVstepFile(string, deviceId, vstepFile):
    """
    Obtains a profile which contains the given vstep file.
    First the location of the vstep file name is found in the container file dump
    Then it gets the profile name that uses that vstep file.
    Returns the number of the profile that is found.
    """
    indexFound = 0
    profileName = ""
    profileCounter = 0
    profileNumber = -1
    
    if 'Windows' in platform.system():
        # use r prefix to indicate a raw string:
        vstepFileName = re.split(r"\\", vstepFile)[-1] 
    elif 'Linux' in platform.system():
        vstepFileName = re.split("/", vstepFile)[-1]
    
    if "vstep" not in vstepFileName:
        raise Exception("Invalid Vstep file name:", vstepFileName)    

    lines = string.splitlines()
    for index, line in enumerate(lines):
        if vstepFileName in line:
            indexFound = index
    
    if indexFound == 0:
        raise Exception("Selected Vstep file", vstepFileName, "not found in container file")

    for i in range(0, indexFound, 1):
        if ("[" and "]") in lines[i]:
            # assume only one word is contained in brackets:
            profileName = getWordsFromString(lines[i])[0] 
            if countWordsInString(lines[i]) > 1:
                raise Exception("Unexpected profile name found")

    printLines("found profile: " + profileName + " with vstep file: " + vstepFileName)

    for line in lines:
        if "profile=" in line:
            if profileName in line:
                profileNumber = profileCounter
            profileCounter = profileCounter + 1
    
    if deviceId is not '88':
        profileNumber /= 2

    if profileNumber == -1:
        raise Exception("Unexpected layout of container file found")

    return profileNumber


def verifyProfileSettings(string, location, deviceClass, cntFile, profile, currentprof): 
    """
    Checks if the right bitfields are set after a profile switch.
    First retrieves al available profiles, then all corresponding bitfields and values.
    Then it checks if all bitfields of the current profile are set correctly.
    and if the bitfields from the last profile have been restored to default.
    Raises an exception if the current bitfield values do not match the current profile's bitfield values.
    """
    ProfileNames = []
    DsnListProfile = []
    ValueListProfile = []
    DsnListCurrentprof = []
    ValueListCurrentprof = []
    StopLine = 0
    profileLine = 0
    defaultLine = 0
    previousLine = 0

    printLines("string: " + string)

#---Get the profile names and add brackets
    #Get all Profile names
    for line in string.splitlines():
        if "profile" in line:
            name = line.partition("=")
            ProfileNames.append(name[2])
    
    #Add brackets to create correct sections
    for x in range(len(ProfileNames)):
        if len(ProfileNames[x]) > 0:   
            ProfileNames[x] = "["+ProfileNames[x]+"]"    

#---Get the name and value of bitfields from the current and previous profile   
    #Find the line at which the profile starts
    line = string.splitlines()
    for i in range(len(line)):
        if ProfileNames[int(profile)] in line[i]:
            profileLine = i
        if ProfileNames[int(currentprof)] in line[i]:
            previousLine = i
    
    #Find the name and value of the profile settings
    for i in range(profileLine+1, len(line)):
        if "vstep" in line[i] or "default" in line[i] or "[" in line[i] or "." in line[i] or len(line[i]) == 0:
             break
        else:
            printLines("Profileline: " + line[i])
            line[i] = line[i].replace(";", "=")
            parts = line[i].split("=")        
            printLines("   DSN: " + parts[0] + " value: " + parts[1])
            DsnListProfile.append(parts[0])            
            ValueListProfile.append(parts[1])
    
    #Find where the default section starts and get bitfield name and value
    for j in range(previousLine+1, len(line)):
        if "vstep" in line[i] or "default" in line[i] or "[" in line[i] or "." in line[i] or len(line[i]) == 0:
             break
        if defaultLine != 0:
            if len(line[j]) > 0:
                printLines("Previousprofileline: " + line[j])
                line[j] = line[j].replace(";", "=")
                parts = line[j].split("=")        
                printLines("   DSN: " + parts[0] + " value: " + parts[1])
                if parts[0] not in DsnListProfile:      #A bitfield from current default can be re-used in new profile
                    DsnListCurrentprof.append(parts[0])
                    ValueListCurrentprof.append(parts[1])
        if "default" in line[j]:
            defaultLine = j
        if "[" in line[j] or len(line[j]) == 0:
            break

#---Check if the new profile bitfields are set
    for x in range(len(DsnListProfile)): 
        result = climax(location, deviceClass, cntFile, DsnListProfile[x])
        printLines("result: " + result)
        values = []
        if "[left]" and "[right]" in result:
            parts = result.partition("[right]")
            values.append(parts[0].partition(":")[-1]) #retrieve number from line.
            values.append(parts[2].partition(":")[-1]) #retrieve number from line.
        else:
            values.append(result.partition(":")[-1]) #retrieve number from line.
        for value in values:
            if int(value) != int(ValueListProfile[x]):
                raise Exception("Profile Bf(",DsnListProfile[x], ") does not match value! Expected:",ValueListProfile[x]," and got:",value)
            else:
                printLines("Profile Bf(" + DsnListProfile[x] + ") matches expected value")

#---Check if the previous/current profile bitfields are set
    for y in range(len(DsnListCurrentprof)):
        result = climax(location, deviceClass, cntFile, DsnListCurrentprof[y])
        printLines("result: " + result)
        values = []
        if "[left]" and "[right]" in result:
            parts = result.partition("[right]")
            values.append(parts[0].partition(":")[-1]) #retrieve number from line.
            values.append(parts[2].partition(":")[-1]) #retrieve number from line.
        else:
            values.append(result.partition(":")[-1]) #retrieve number from line.
        for value in values:
            if int(value) != int(ValueListCurrentprof[y]):
                raise Exception("Currentprof Bf(",DsnListCurrentprof[y], ") does not match value! Expected:",ValueListCurrentprof[y]," and got:",value)
            else:
                printLines("Currentprof Bf(" + DsnListCurrentprof[y] + ") matches expected value")


def memoryShouldContain(string, memtype):
    """
    Verify if the memory value in the return message is equal to 0x112233 for xmem and ymem.
    Raises an exception if the memory address does not contain the expected value.
    """
    parts = string.partition(":")
    try:
        if memtype == "xmem":
            if int(parts[2], 16) != 0x112233:
                raise Exception("XMEM: (",parts[2],") is NOT equal to written value")
        elif memtype == "ymem":
            if int(parts[2], 16) != 0x112233:
                raise Exception("YMEM: (",parts[2],") is NOT equal to written value")
        else:
            raise Exception("unknown memtype:",memtype)              
    except ValueError:
        printLines("not a hex value: " + parts[2])
    
    
def verifyIni2CntMessage(iniName, iniToCntMsg):
    """
    Verifies if ini2cnt prints the expected info messages.
    Raises an exception if an unexpected message was printed.
    """
    if "GOOD" in iniName:
        if "unknown bitfield:" in iniToCntMsg:
            raise Exception("bitfieldname error message printed by climax for file:", iniName)
        if "Error:" in iniToCntMsg:
            raise Exception("ini2cnt of", iniName, "resulted in errors")
        if "Warning:" in iniToCntMsg:
            raise Exception("ini2cnt of", iniName, "resulted in Warning")
        if ("Created container" not in iniToCntMsg) or ("No container is created" in iniToCntMsg):
            raise Exception("Container file of", iniName, "not created")
    elif "WRONG" in iniName:
        if ("Error:" not in iniToCntMsg) and ("Too many livedata items!" not in iniToCntMsg):
            raise Exception("Expected climax to print an error message for file:", iniName)

            
def verifyCntFileCreated(iniName, cntName):
    """
    Verifies if a container file with a certain name exists.
    Raises an exception if it does or does not exist when expected.
    """
    if "GOOD" in iniName and (not os.path.exists(cntName)):
        raise Exception("Container file was not created: ", cntName)
    elif "WRONG" in iniName and (os.path.exists(cntName)):
        raise Exception("Container file was created for an incorrect ini file: ", iniName) 
            
            
def verifyIni2CntDumpMessage(cntName, cntDump):
    """
    Verifies the dump of the generated container file.
    Raises an exception when the cnt format is incorrect or no status :0 message is printed.
    """
    if "GOOD" in cntName:
        if "last status :0 (Ok)" not in cntDump:
            raise Exception("CntDump does not end with return status :0 (Ok)")
        if ("[system]" and "[tfa]" and "[music]") not in cntDump:
            raise Exception("CntDump is not in expected format")
    
    
def compareIniToCnt(iniName, cntDump):    
    """
    compares a container file dump with a ini file
    this is done by removing some lines from the terminal screen
    then ignoring some lines from the comparisson which contain one of the strings in PossibleFileNames:
    ["patch", "speaker", "vstep", "msg", "config", "preset", "drc", "eq", "volstep", "info", "type"]
    after that it removes all spaces, cased letters and whitelines and compares the strings
    Raises an exception if the ini file string and ini2cnt dump string do not match.
    """
    PossibleFileNames = ("patch", "speaker", "vstep", "msg", "config", "preset", "drc", "eq", "volstep", "info", "type", "filter[")
    keywords = ("container:", "devicename=", "Found Device[", "last status :", "bus=", "CHS12=", "chs12=")
    
    if not cntDump.find("last status :0 (Ok)") == -1:
        #Remove the non-ini parts from the cntDump
        oldCntDump = cntDump
        cntDump = ""
        for line in oldCntDump.splitlines():
            if any( line.strip().startswith(keyword) for keyword in keywords ):
                printLines("Removed line: " + line)
                pass
            else:
                cntDump = cntDump + line + '\n'      
    else:
        printLines("cntDump= '" + cntDump + "'")
        raise Exception("CntDump is not in expected format, did something go wrong while loading the cnt?")       
    
    cntList = cntDump.splitlines()
    #Remove internal redundant whitespace and comments
    for i in range(len(cntList)):
        cntList[i], sep, tail = cntList[i].partition(';')
        cntList[i] = cntList[i].strip()
        if any(s in cntList[i] for s in PossibleFileNames):
            printLines("cntList: " + cntList[i] + " changed to 'ignore'")
            cntList[i] = "ignore"

    cntDump = '\n'.join(cntList)   
    
    #Read the ini
    iniFile = open(iniName, 'r')   
    iniDump = iniFile.read()
        
    #Remove Comments
    iniList = iniDump.splitlines()
    for i in range(len(iniList)):
        iniList[i] = iniList[i].split(";")[0].strip()
        if any(s in iniList[i] for s in PossibleFileNames):
            printLines("iniList: " + iniList[i] + " changed to 'ignore'")
            iniList[i] = "ignore"
        if iniList[i].startswith("bus=") or iniList[i].startswith("CHS12=") or iniList[i].startswith("chs12="):
            printLines("Removed line: " + iniList[i])
            iniList[i] = ""
        
    iniDump = '\n'.join(iniList)
        
    iniDump = formatToCompareString(iniDump)
    cntDump = formatToCompareString(cntDump)

    #Fuzzy compare, ignore case, spacing and whitelines
    BuiltIn().should_be_equal(iniDump, cntDump)

def getAddresses(str):
    """
    This function checks which device address(es) can be accessed
    It returns a string (or list of two strings) containing the climax option to set the device address(es)
    """
    result = []
    for line in str.splitlines():
        if "dev=" in line:
            result.append("--slave=" + line.split("=")[-1])
    print result
    return result

    
def formatToCompareString(string):
    """"
    Returns a lowercase version of the input string with some whitespace removed, to allow fuzzy comparisons
    """ 
    return string.lower().replace(" ", "").replace("\n", "").replace("\r", "").strip()
    
    
def checkSaveReturnMessage(string):
    """
    Checks if the save return message confirms that save succeeded.
    """
    if ("Creating header" not in string) and ("A new Header is created" not in string) and ("Save file" not in string):
        raise Exception("save operation failed!")
    if "Bad_Parameter" in string:
        raise Exception ("save operation failed!", string)
    
    
def getUsedFilePath(string, path, type):
    """
    Returns the path to the file that is used by the container file
    """
    file = None
    for line in string.splitlines():
        if type+"=" in line:
            file = line.split("=")[-1]
            break
    FilePath = "None"
    if file == None:
        printLines("No used " + type + " file found in cnt file!")
    else:
        FilePath = path + type + os.sep + os.path.split(file)[-1]
    return FilePath
    
    
def getValueOfProperty(string, seperator, property):
    """
    Returns a word that can be found behind a property word in the string parameter
    """
    if not seperator in property:
        property += seperator
    return (string.split(property)[-1]).split()[0]
    
    
def compareSpeakerHeaders(originalHeader, newHeader):
    """
    Checks if some properties between original speaker header and new header match.
    """
    propertiesToCheck = ["id", "version", "subversion", "size"]
    propertiesToCheck2 = [ "primary", "secondary" ]
    
    for property in propertiesToCheck:
        if getValueOfProperty(originalHeader, ":", property) != getValueOfProperty(newHeader, ":", property):
            raise Exception("property(", property, "): ", getValueOfProperty(originalHeader, ":", property), " != ", getValueOfProperty(newHeader, ":", property) )
    for property in propertiesToCheck2:
        val = getValueOfProperty(newHeader, "=", property)
        if val == "0ohm" or val == "0":
            raise Exception("property(", property, ") cannot have value 0. val is:", val) 

            
def getBinaryFileAsHex(file):
    """
    Returns the contents of a file as a string of hexadecimal characters.
    """
    printLines("checkSaveBinaryFile: " + file)
    with open(file, 'rb') as f:
        content = f.read()
    hexString = binascii.hexlify(content)
    return hexString

    
def getBoundsForFileType(fileString, fileType):
    """
    This function returns the offsets for each specific file type save is tested on.
    Returns: string1_Lower_Bound, string1_Upper_Bound, string2_Lower_Bound, string2_Upper_Bound
    Note: each offset is calculated from the number of bytes times 2,
    this is because the compare is done on CHARS representing hexadecimal numbers,
    so each byte is written with two chars.
    """
    if fileType == "speaker":
        return 146, 0, 146, 0
    elif fileType == "drc":
        nrOfRegisters = fileString[81]
        vstepOffset = 2964+(8*int(nrOfRegisters))
        return 72, 0, vstepOffset, (vstepOffset+912)
    elif fileType == "vstep":
        return 0, 1890, 48, 1962
    elif fileType == "preset":
        return 72, 0, 72, 0
    elif fileType == "config":
        return 72, 0, 72, 0
    else:
        return 0, 0, 0, 0

        
def compareHexStrings(str1, str2, fileType, acceptanceRatio='0.9'):
    """
    Compares two strings and calculates a diff ratio.
    If this ratio is below standard (default is 0.9 = 90%), an exception is raised.
    """
    str1LBound, str1HBound, str2LBound, str2HBound = getBoundsForFileType(str2, fileType)
    if str1HBound == 0:
        str1HBound = len(str1)
    if str2HBound == 0:
        str2HBound = len(str2)
    ratio = difflib.SequenceMatcher(None, str1[str1LBound:str1HBound], str2[str2LBound:str2HBound]).ratio()
    if ratio > float(acceptanceRatio):
        printLines("ratio is: " + str(ratio) + " Save operation succeeded!")
    else:
        raise Exception("ratio is:", ratio, "Save operation likely failed")
    
    
def getDeviceType(climaxLoc, cntFile):
    """
    """
    devType = None
    result = runClimaxCmd(climaxLoc, "-l "+cntFile , "-b")
    for line in result.splitlines():
        if "type=" in line:
            devType = line.split("=")[-1]
    
    if devType == "9888N1C":
        devType = "0x2c88"
    elif devType == "98881B2":
        devType = "0x2b88"
    elif devType == "98881B3":
        devType = "0x3b88"
    
    return devType
    
    
def getDeviceInterface(deviceClass, climaxLoc, cntFile, deviceId):
    """
    Searches for the hid port that is connected to the device with <deviceId>.
    In case something other than hid is used: do nothing.
    Returns hid(x), scribo or dummy.
    """
    devInterface = deviceClass
    if deviceClass == "hid":
    
        devType = None
        if deviceId == "88":
            devType = getDeviceType(climaxLoc, cntFile)
    
        for i in range(0, 10):
            result = runClimaxCmd(climaxLoc, "-l "+cntFile , "-dhid"+str(i), "-r3")
            
            #translate device id's:
            realId = None # set 88 as default
            if deviceId == "88":
                realId = "88"
            elif deviceId == "91":
                realId = "92"
            elif deviceId == "90":
                realId = "80"
            elif deviceId == "95":
                realId = "12"
            elif deviceId == "97":
                realId = "97"
            elif deviceId == "87":
                realId = "12"
            
            if realId == None:
                raise Exception("selected device ID is not yet supported by RTF.")
            
            #assign usbDevice name if it matches:
            if devType == None:
                if realId in result and not (("i2c slave error" in result) or ("empty read" in result)):
                    devInterface = "hid"+str(i)
            else:
                if devType in result:
                    devInterface = "hid"+str(i)

        if devInterface == "hid":
            raise Exception("proper hid device not found!")
    
    elif "dummy" in devInterface:
        if devInterface.strip() == "dummy":
            devInterface += deviceId
             
    if "dummy" in devInterface and 'Linux' in platform.system():
        setupClimaxServer(climaxLoc, "-ddummy"+deviceId, "--server=@9876")
        devInterface = "127.0.0.1@9876"
   
    return devInterface

    
def setupClimaxServer(location, deviceClass, *args):
    """
    Creates a separate thread that enables a climax server to run with UDP
    """
    #Converting unicode strings
    args = map(str, args)   
    climaxcmd = getClimaxCmd(location, deviceClass, *args)
    class RunCmd(threading.Thread):
        def __init__(self,cmd):
            threading.Thread.__init__(self)
            self.cmd = cmd
            
        def run(self):
            self.p = subprocess.Popen(self.cmd.split(), stdout=subprocess.PIPE)

    tmp = RunCmd(str(climaxcmd))
    tmp.start()

    
def teardownClimax():
    """
    Makes sure no processes of climax (climax server) keep living after RTF ends.
    """
    printLines("Executing RTF teardown.")
    for p in psutil.process_iter():
        try:
            if "climax" in p.name():
                print "*INFO*", "cleaning up climax (server) process:", os.linesep, p.status
                p.kill()
        except psutil.Error:
            printLines("no climax process to clean up.")
    printLines("Teardown succeeded.")

    
def parseClimaxParms(device, *args):
    """
    this function parses climax arguments, which comes from Robot framework
    1. in args, filename is always in format: fullpath+filename. 
    2. for climax commands like: climax --bin2hdr=filename  or climax --ini2cnt=filename, this function processes
        filename only, this function removes fullpath part
    3. for climax --device, if dummy is working , then omit --device from climax command
        if real device is working, then --device should be used in climax command
    """
    parm = list(args)

    arg = ''
    for item in parm:       
          arg = arg + item + ' ' 
    return arg

    
def linuxClimaxCmdDir(location):
    """
    this function returns Linux operating system climax command location
    location should be in the same directory as workspace
    """  
    if location == "None":
        return workSpaceDir()
    else:
        #add absolute path
        #location = os.path.abspath(location) #os.getcwd() + location.replace(".","")
        return location
 
 
def getClimaxCmd(location, device, *args):
    """
    this function creates a string that contains a call to climax with the path and all arguments.
    it treats the path to climax executable differently for windows and linux.
    """
    arg = parseClimaxParms(device, *args)   
    climaxcmd = ""
    
    if 'Windows' in platform.system():
        printLines('climax.exe ' + device + ' ' + arg)
        climaxcmd = os.path.join(winClimaxCmdDir(location), 'climax.exe') + ' ' + device + ' ' + arg
    elif 'Linux' in platform.system():
        printLines(linuxClimaxCmdDir(location) + 'climax ' + device + ' ' + arg)
        climaxcmd = linuxClimaxCmdDir(location) + 'climax ' + device + ' ' + arg
    else:
        raise Exception('this ', platform.system(), ' is not supported! Abort Execution!')
    
    return climaxcmd

    
def runClimaxCmd(location, deviceClass, *args):
    """
    this function runs climax command on Windows and return climax output
    if Dummy is used, for all climax commands, returned message should NOT be empty
    if real device is used, for some climax commands, returned message could be empty
    climax command should be run under the folder of config files. 
    if config files folder is not available, climax command might be run under any dictionary 
    """
    
    #Converting unicode strings
    args = map(str, args)   
    #printLines("Location: " + location + " deviceClass: " + deviceClass + " args: " + args)
    climaxcmd = getClimaxCmd(location, deviceClass, *args)
    printLines("climaxcmd: " + climaxcmd)  
    class RunCmd(threading.Thread):
        def __init__(self,cmd):
            threading.Thread.__init__(self)
            self.cmd = cmd
            
        def run(self):
            self.p = subprocess.Popen(self.cmd.split(), stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            self.message = self.p.communicate()[0]
            self.p.wait()


    tmp = RunCmd(str(climaxcmd))
    tmp.start()
    tmp.join(30)
    if tmp.is_alive():
        tmp.p.terminate()
        tmp.join()
        raise Exception('climax crashes or timeout occurred with command:\n', str(climaxcmd))

    return tmp.message    

    
def winClimaxCmdDir(location):
    """
    this function returns climax command location on Windows operating system 
    location should be: workspace\app\bld\win\climax\Release_x64\climax.exe
    """ 
    if location == "None":
        if '32' in BuiltIn().get_variable_value('${bit}'):
    #     if '32' in BuiltIn().replace_variables('${bit}'):
            WinVer = 'Release_Win32'
        else:
            WinVer = 'Release_x64'
        
        wsPath = []
        for root, dirs, files in os.walk(workSpaceDir()):
            for file in files:
                if file.endswith('climax.exe'):
                    wsPath.append(root)
        for ws in wsPath:
            mypath = re.search(r'.*app[\\]*bld[\\]*win[\\]*climax[\\]*' + WinVer + '.*', ws)
            if mypath:
                return ws    
        else:
            raise Exception('can not find climax command under subfolders of ' + workSpaceDir())
            exit(0)
    else:
        return location

        
def workSpaceDir():
    """
    workspace folder is usually the folder above /app/climax/* folders
    for Linux Jenkins, usually workspace folder is : /var/lib/jenkins/jobs/robot_branch_sandbox/workspace
    for Windows Jenkins, usually workspace folder is : C:\Jenkins\workspace
    if the running folder does not contain /app/climax/* folders information, return current running folder   
    """
    climaxFolder = re.split(r'app[\/,\\,\w]*', BuiltIn().replace_variables('${SUITE SOURCE}'))
    if os.path.isdir(climaxFolder[0].strip()):
        return climaxFolder[0].strip()
    else:
        return os.getcwd()

        
def climax(location, deviceClass, *args):
    """
    this function runs the climax command on Linux or Windows, and returns an output message
    from robot framework, parameter:device passes deviceType + deviceID
    """     
    args =  tupleWithout(args, "None")
    
    if os.path.exists(location + os.sep + "climax.exe") or os.path.exists(location + os.sep + "climax") :
    #    verifyconfigFilesDir()
        runningOS = platform.system()
        if deviceClass == False:
            raise Exception("deviceClass not correct!") 
                
        if 'dummy' in deviceClass:
            if '--start' in args:
                deviceClass = '-d' + deviceClass
            else:
                deviceClass = '-d' + deviceClass + ',warm'  # device is dummy, always use 'warm' to do climax test
        elif 'Windows' in runningOS:  # device is real device
                if 'hid' in deviceClass:
                    deviceClass = '-d' + deviceClass
                else:
                    deviceClass = ''
        elif 'Linux' in runningOS:
                deviceClass = '-d' + deviceClass
        else:
            raise Exception(str(deviceClass) + '   and \n' + str(args) + '\nthis combination could not be recognized')
            exit(0)  
        #os.chdir(workSpaceDir())    
     
        message = runClimaxCmd(location, deviceClass, *args)    
    
        if '--start' in args:
            time.sleep(0.4)
            
        # anaylize returned message, according to climax command, return useful message  
        parm = list(args)
        if parm[0] == "-r3":
            mydict = convertMessageToDict(message)
            return mydict['0x03'][0]
        else:
            return message
        
    else: 
        raise Exception("Could not find a file named climax in the specified folder:", location + os.sep)

        
def deleteLines(message, arg):
    """
    this function filters lines containing arg from message, and returns filtered message
    this function is mainly for filtering 'I2c error' from returned message, since this error is not climax'error. 
    """

    if message:
        message = message.split('\n')
    else:
        return message
    
    result = ''
    for line in message:
        if arg.lower() not in line.lower():
            result = result + line + '\n'

    return result
          
          
def isWindowsOperatingSystem():
	"""
	This function returns True if Operating System is Windows, else it will return false
	"""
	if 'Windows' in platform.system():
		return True
	else:
		return False

        
def getOperatingSystem():
    """
    This function returns the running Operating System
    """
    return platform.system()

    
def shouldNotContainAny(message, *args):
    """
    this function verifies text message contains either of arguments' value
    """
    for item in args:
        if re.search(str(item).replace(' ', ''), str(message).replace(' ', ''), re.IGNORECASE) != None:
            raise Exception(message + '\n contains either of ' + str(args))
    return True

    
def shouldContainAll(message, *args):
    """
    this function verifies text message contains all of arguments' values
    """
    for item in args:
        should_contain_ignore_spaces(message, item)
        
    return True

    
def shouldContainIgnoreSpaces(msg, item):
    """
    this function checks if msg contains substring item.
    spaces are removed from compare strings
    """
    BuiltIn().should_contain(msg.replace(' ', ''), item.replace(' ', ''))
    
	