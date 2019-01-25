"""
This file works as robot framework external library
this files contains several keywords that could be used by robot scripts
Author: Ji Xia
Date:  March and April of 2014
Note: Older version
"""


import os
import glob
from robot.libraries.BuiltIn import BuiltIn
import fnmatch
import re
import commands
import subprocess
import ast
import platform
from scipy.weave.build_tools import old_argv


DEVICELIST = ['88', '90', '91', '95', '97']


def getDeviceID(path):
    """
    Get the device ID from the used container file path
    If no ID is found, return 88 as default
    """
    deviceID = None
    for item in DEVICELIST:
        if item in os.path.normpath(path):
            deviceID = item
    if (deviceID == None) or (deviceID == ""):
        deviceID = '88'
    return deviceID
        
        
def createNewDirectory(path, dirName):
    """
    Creates a directory in <path> with name <dirName> if it not already exists
    """
    if not os.path.exists(path+dirName):
        os.makedirs(path+dirName)
        # give the proper use permissions:
        os.chmod(path+dirName, 0766)
        
        
def cleanupDirectory(path):
    """
    Removes file with name is equal to fileName parameter or with .old appended.
    Also restores a .backup file as a file with name equal to fileName parameter.
    """
    if os.path.exists(path):
        try:
            for file in os.listdir(path):
                print "*INFO*", "remove file:", path+file
                os.remove(path+file)
            os.rmdir(path)
        except:
            print "*INFO*", "Cleanup Directory failed!"
        
        
def getRelativePath(path, offset):
    """
    Get a new absolute path by combining an absolute path with the relative path offset
    Note: does not support backslashes in the path when used in Linux.
    """
    path = os.path.normpath(os.path.abspath(path+offset).replace('/', os.sep))
    if offset[-1] == '/' or offset[-1] == os.sep:
        path += os.sep
    return path

    
def getFilesWithExtension(path, fileType):
    """
    Get a list of files in a given path with file extension <fileType>
    """
    fileList = []
    for file in os.listdir(path):
        if file.endswith("."+fileType):
            fileList.append(path+os.sep+file)
    return fileList
    
    
def getRecordTestFiles(path, deviceType):
    """
    Get a list of files in a given path with file extension <fileType> and for the right device
    """
    fileList = []
    if deviceType == '88':
        for file in os.listdir(path):
            if file.endswith(".cnt") and "max2" in file:
                fileList.append(path+os.sep+file)
    elif deviceType == '91':
        for file in os.listdir(path):
            if file.endswith(".cnt") and "max1" in file:
                fileList.append(path+os.sep+file)
    return fileList
    
    
def getPlmaConfigPath(cntPath):
    """
    This returns the path up to /plma_config/
    """
    configPart = "plma_config"+os.sep
    result = cntPath.split(configPart)[0] + configPart
    
    if len(result) < 5:
        raise Exception("path not found from:", cntPath) 
    
    return result

    
def deviceFolders(folder=os.getcwd()):
    """
    returns a list of device folders. This is just the base folder with the device ID appended to it.
    """
    result = []
    for device in DEVICELIST:
        result.append(folder + os.sep + device)
    
    return result
    

def replaceFilenameExtension(file, destExt):
    """
    Replaces the original file extension with the new file extention
    """
    return os.path.splitext(file)[0] + "." + destExt

    
def backupAndRemoveFile(fileName):
    """
    Tries to convert file with name <fileName> to a backup file and replaces the old one if it already exists
    """
    if os.path.exists(fileName):
        backupFileName = fileName + ".backup"
        try:
            os.remove(backupFileName)
        except:
            pass
        os.rename(fileName, backupFileName)
    else:
        raise Exception("filename not found:", fileName)
    
    
def backupCreateAndRemoveFile(fileName):
    """
    Tries to create a backup file and replaces the old one if it already exists
    """
    import shutil
    if os.path.exists(fileName):
        backupFileName = fileName + ".backup"
        try:
            os.remove(backupFileName)
        except:
            pass
        shutil.copy(fileName, backupFileName)
    else:
        raise Exception("filename not found:", fileName)

        
def cleanupAndRemoveFile(fileName):
    """
    Removes file with name is equal to fileName parameter or with .old appended.
    Also restores a .backup file as a file with name equal to fileName parameter.
    """
    if os.path.exists(fileName) or os.path.exists(fileName+".backup") :
        try:
            os.remove(fileName)
            os.remove(fileName + ".old")
        except:
            pass   
        os.rename(fileName + ".backup", fileName)
    else:
        raise Exception("filename not found:", fileName)

        