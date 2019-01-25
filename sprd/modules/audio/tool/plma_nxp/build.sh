#!/bin/bash

# script to build Tfa ref code
## must use cygwin and install msbuild.exe

#Options
## ./build.sh 			default build is climax
## ./build.sh tfa		
## ./build.sh clean		to clean all the build artifacts 
## TODO: ./build.sh installer		to package the TFA and SRV DLL into installer using NSIS 
## ./build.sh test		to test using pybot 
## ./build.sh report		to combine pybot rebot report 
## ./build.sh android	build for android 

OS=`uname -o` 	# Cygwin or GNU/Linux
PRJ_TOP=`pwd`	# top for the project path
#
if [ $OS == "Cygwin" ] ; then 
	## Windows / Cygwin build 
	if which msbuild.exe >/dev/null; then
		for purpose in "Release" "Debug"; do
			for arch in "x64" "Win32"; do
				
				# make lib folder to copy DLL and .h interfaces for release
				mkdir -p ${PRJ_TOP}/lib/${purpose}_${arch};
				mkdir -p ${PRJ_TOP}/lib/inc;
				cp ${PRJ_TOP}/srv/inc/*.h ${PRJ_TOP}/lib/inc/.;
				cp ${PRJ_TOP}/hal/inc/NXP*.h ${PRJ_TOP}/lib/inc/.;
				cp ${PRJ_TOP}/tfa/inc/tfa.h ${PRJ_TOP}/lib/inc/.;
				cp ${PRJ_TOP}/tfa/inc/tfa_*.h ${PRJ_TOP}/lib/inc/.;
				cp ${PRJ_TOP}/app/bld/win/tfa_dll/tfa_matlab.h ${PRJ_TOP}/lib/inc/.;

				if [ $1 == "examples" ]; then 
					cd ${PRJ_TOP}/app/bld/win
					## build reference container example
					msbuild.exe ./exTfa98xxCnt/exTfa98xx.sln /property:Configuration=${purpose} /property:Platform=${arch};
					if [ ! -f ${PRJ_TOP}/app/bld/win/exTfa98xxCnt/${purpose}_${arch}/exTfa98xxCnt.exe ]; then
						echo "Container example with service layer ${purpose}_${arch} build failed. Aborting." 
						exit 1;
					else  
						echo "Example (container with srv layer) ${purpose}_${arch} build completed";
					fi  
				elif [ $1 == "climax" ]; then 
					## build climax
					cd ${PRJ_TOP}/app/bld/win
					msbuild.exe ./climax/climax.sln /property:Configuration=${purpose} /property:Platform=${arch};
					if [ ! -f ${PRJ_TOP}/app/bld/win/climax/${purpose}_${arch}/climax.exe ]; then
						echo "Climax ${purpose}_${arch} build failed. Aborting." 
						exit 1;
					fi
				elif [ $1 == "srv" ]; then 		
					## build service layer DLL
					cd ${PRJ_TOP}/srv/bld/win;
					echo "Welcome to build Tfa API DLL $purpose $arch"
					msbuild.exe ./serviceLayer.sln /property:Configuration=${purpose} /property:Platform=${arch};
					if [ ! -f ${PRJ_TOP}/srv/bld/win/${purpose}_${arch}/Tfa98xx_srv.dll ]; then
						echo "SRV DLL build failed for ${purpose}_${arch}. Aborting." 
						exit 1;
					else  
						echo "SRV DLL build completed for ${purpose}_${arch}.";
						mkdir -p ${PRJ_TOP}/lib/${purpose}_${arch};
						cp ${PRJ_TOP}/srv/bld/win/${purpose}_${arch}/Tfa98xx_srv.dll ${PRJ_TOP}/lib/${purpose}_${arch}/.;
					fi  
				elif [ $1 == "tfa" ]; then 		
					## build TFA device layer DLL
					cd ${PRJ_TOP}/tfa/bld/win
					echo "Welcome to build Tfa API DLL $purpose $arch"
					msbuild.exe ./Tfa98xx.sln /property:Configuration=${purpose} /property:Platform=${arch};
					if [ ! -f ${PRJ_TOP}/tfa/bld/win/${purpose}_${arch}/Tfa98xx_hal.dll ]; then
						echo "HAL DLL build failed for ${purpose}_${arch}. Aborting." 
						exit 1;
					else  
						echo "HAL DLL build completed for ${purpose}_${arch}.";
						mkdir -p ${PRJ_TOP}/lib/${purpose}_${arch};
						cp ${PRJ_TOP}/tfa/bld/win/${purpose}_${arch}/Tfa98xx_hal.dll ${PRJ_TOP}/lib/${purpose}_${arch}/.;
					fi  
					if [ ! -f ${PRJ_TOP}/tfa/bld/win/${purpose}_${arch}/Tfa98xx_2.dll ]; then
						echo "API DLL build failed for ${purpose}_${arch}. Aborting." 
						exit 1;
					else  
						echo "API DLL build completed for ${purpose}_${arch}.";
						# make lib folder to copy DLL for testing
						mkdir -p ${PRJ_TOP}/lib/${purpose}_${arch};
						cp ${PRJ_TOP}/tfa/bld/win/${purpose}_${arch}/Tfa98xx_2.dll ${PRJ_TOP}/lib/${purpose}_${arch}/.;
					fi
                    cd ${PRJ_TOP}/app/bld/win/tfa_dll
					echo "Welcome to build Tfa FULL API DLL $purpose $arch"
					msbuild.exe ./tfa_dll.sln /property:Configuration=${purpose} /property:Platform=${arch};
					if [ ! -f ${PRJ_TOP}/app/bld/win/tfa_dll/${purpose}_${arch}/tfa.dll ]; then
						echo "TFA FULL SINGULAR DLL build failed for ${purpose}_${arch}. Aborting." 
                        echo ${purpose}_${arch}
						exit 1;
					else  
						echo "TFA FULL SINGULAR build completed for ${purpose}_${arch}.";
						mkdir -p ${PRJ_TOP}/lib/${purpose}_${arch};
						cp ${PRJ_TOP}/app/bld/win/tfa_dll/${purpose}_${arch}/tfa.dll ${PRJ_TOP}/lib/${purpose}_${arch}/.;
					fi  
					
					
				elif [ $1 == "all" ]; then 	
					echo "Building only climax by default" 
					${PRJ_TOP}/build.sh examples
					${PRJ_TOP}/build.sh climax
					${PRJ_TOP}/build.sh srv
					${PRJ_TOP}/build.sh tfa
				elif [ $1 == "clean" ]; then 	
					echo "Clean all build artifacts"
					## clean all build artifacts
					cd ${PRJ_TOP}/app/bld/win
					msbuild.exe ./climax/climax.sln /t:clean /property:Configuration=${purpose} /property:Platform=${arch};
					msbuild.exe ./exTfa98xx/exTfa98xx.sln /t:clean /property:Configuration=${purpose} /property:Platform=${arch};
					msbuild.exe ./exTfa98xxCnt/exTfa98xx.sln /t:clean /property:Configuration=${purpose} /property:Platform=${arch};
					cd ${PRJ_TOP}/srv/bld/win;
					msbuild.exe ./serviceLayer.sln /t:clean /property:Configuration=${purpose} /property:Platform=${arch};
					cd ${PRJ_TOP}/tfa/bld/win
					msbuild.exe ./Tfa98xx.sln /t:clean /property:Configuration=${purpose} /property:Platform=${arch};	
					rm -f lib/${purpose}_${arch}/*.dll
					rm -f ${PRJ_TOP}/app/bld/win/exTfa98xxCnt/${purpose}_${arch}/*
					rm -f ${PRJ_TOP}/srv/bld/win/${purpose}_${arch}/*
					rm -f ${PRJ_TOP}/tfa/bld/win/${purpose}_${arch}/*

				elif [ $1 == "installer" ]; then 
					break;
				else
					echo "Building only climax by default (single .dll version)" 
                    ./build.sh climax
				fi
			done
		done		
	else
		echo "Windows build is possible via msbuild.exe."			
		echo "Please install msbuild.exe or check if that is available in the path." 
	fi
	if [ $1 == "installer" ]; then 
		echo "Todo: TFA and SRV layer DLL installer"
		# copy the DLL from Release folders to utl/installer folder
		# download the API doc
		# trigger NSIS script
	fi
	if [ "$1" == "test" ]; then 
                if which pybot.bat >/dev/null; then
                        echo "Pybot (Robot test framework) test."			  
                        if [ -e "$2" ]; then        ## test folder
                                pybot.bat --variable testfolder:$2 --variable pattern:climaxInfo*  ./app/climax/tst/testsuites/acceptance_test/01__test.txt
                                
                        else
                                echo "Pybot testfolder not provided or does not exists."			  
                        fi                
                else
                        echo "Pybot.bat (Robot test framework) not found."			
                fi
	elif [ "$1" == "report" ]; then 
                if which rebot.bat >/dev/null; then
                        rebot.bat --name $2 --output output.xml output*.xml
                else
                        echo "Rebot (Robot test framework) reporting tool not found."			
                fi
        fi;
## android build
elif [ "$1" == "android" ]; then
	# android build specific variables
    if [ "$2" == "marshmallow" ]; then
        ANDROID_ROOT=/mnt/usbdisc/android_marshmallow/
		ANDROID_MODULE_DIR="$ANDROID_ROOT"external/plma_hostsw
	    if [ -n "$WORKSPACE" ]; then
			WORKSPACE=/var/lib/jenkins/jobs/HostSDK_Android_Marshmallow_Build/workspace
	    fi
    else
        ANDROID_ROOT=/mnt/usbdisc/android_lollipop/
		ANDROID_MODULE_DIR="$ANDROID_ROOT"external/plma_hostsw
	    if [ -n "$WORKSPACE" ]; then
		    WORKSPACE=/var/lib/jenkins/jobs/HostSDK_Android_Lollipop_Build/workspace
	    fi
    fi
	if [ -d "$ANDROID_ROOT" ]; then
		# check if link to jenkins workspace folder exists
		if [ ! -h "$ANDROID_MODULE_DIR" ]; then
			# make a softlink to the jenkins workspace 
			ln -s "$WORKSPACE" "$ANDROID_MODULE_DIR"
		fi
		# prepare for building
		cd "$ANDROID_ROOT"
		source build/envsetup.sh
		lunch aosp_arm-eng
		# cleanup output of previous build
		rm -r out/target/product/generic/obj/STATIC_LIBRARIES/libtfa98xx_intermediates
		rm -r out/target/product/generic/obj/EXECUTABLES/climax_hostSW_intermediates
		rm -r out/target/product/generic/system/bin/climax_hostSW
		# do the android module build
		cd "$ANDROID_MODULE_DIR"
		mm
	else
		# android root folder does not exist, no android build possible..
		exit 1;
	fi
else 
	## Sconstruct  
	scons
        ## test and report options
	if [ "$1" == "test" ]; then 
                if which pybot >/dev/null; then
                        echo "Pybot test."			  
                        if [ -e "$2" ]; then        ## test folder
                                pybot --variable testfolder:$2 --variable pattern:climaxInfo  ./app/climax/tst/testsuites/acceptance_test/01__test.txt
                                
                        else
                                echo "Pybot testfolder not provided or does not exists."			  
                        fi                
                else
                        echo "Pybot (Robot test framework) not found."			
                fi
	elif [ "$1" == "report" ]; then 
                if which rebot >/dev/null; then
                        rebot --name $2 --output output.xml output*.xml
                else
                        echo "Rebot (Robot test framework) reporting tool not found."			
                fi
        fi;
        ## doxygen 
        if [ "$1" == "doxygen" ]; then 
		echo "Generate doxygen documentation"
                if which doxygen >/dev/null; then
                        doxygen
                else
                        echo "Install doxygen."			
                fi
	fi
	
fi

exit;
