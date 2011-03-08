//
// itkBioFormats.cxx
//

/*
OME Bio-Formats ITK plugin for calling Bio-Formats from the Insight Toolkit.
Copyright (c) 2008-@year@, UW-Madison LOCI.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the UW-Madison LOCI nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY UW-MADISON LOCI ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL UW-MADISON LOCI BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
IMPORTANT NOTE: Although this software is distributed according to a
"BSD-style" license, it requires the OME Bio-Formats Java library to do
anything useful, which is licensed under the GPL v2 or later.
As such, if you wish to distribute this software with Bio-Formats itself,
your combined work must be distributed under the terms of the GPL.
*/

/*
Adapted from the Slicer3 project: http://www.slicer.org/
http://viewvc.slicer.org/viewcvs.cgi/trunk/Libs/MGHImageIO/

See slicer-license.txt for Slicer3's licensing information.

For more information about the ITK Plugin IO mechanism, see:
http://www.itk.org/Wiki/Plugin_IO_mechanisms
*/

// Special thanks to Alex Gouaillard, Sebastien Barre,
#include <fstream>

#include "itkBioFormatsJNIImageIO.h"
#include "itkIOCommon.h"
#include "itkExceptionObject.h"
#include "itkByteSwapper.h"
#include "itkMetaDataObject.h"

#include <vnl/vnl_matrix.h>
#include <vnl/vnl_vector.h>
#include <vnl/vnl_cross.h>

#include <cmath>

#include <stdio.h>
#include <stdlib.h>

#if defined (_WIN32)
#define PATHSEP string(";")
#define SLASH '\\'
#else
#define PATHSEP string(":")
#define SLASH '/'
#endif

namespace itk {

	BioFormatsJNIImageIO::BioFormatsJNIImageIO() {
      cout << "init is " << init << endl;
      if(jvm == NULL)
        cout << "the jvm is null" << endl;

      if(env == NULL)
        cout << "env is null" << endl;

      if(BFITKBridge == NULL)
        cout << "BFITKBridge is null" << endl;

      if(init == false) {
		  // NB: This program requires loci_tools.jar in the same directory.
		  string classpath = "-Djava.class.path";
		  const int numJars = 1;

      const char name[] = "ITK_AUTOLOAD_PATH";
      const char* namePtr;
      namePtr = name;
      char* path;
      path = getenv(name);
      std::string dir("");
      if( path != NULL) {
        dir.assign(path);
      }

      if( dir.at(dir.length() - 1) != SLASH ) {
        dir.append(1,SLASH);
      }

		  string jars[numJars] = {
			(dir + "loci_tools.jar")
		  };
		  for (int i=0; i<numJars; i++) {
			classpath += i == 0 ? "=" : PATHSEP;
			classpath += jars[i];
		  }
		  cout << "Classpath = " << classpath << endl;

		  // get the default initialization arguments and set the class path
		  JavaVMInitArgs vm_args;
		  //JNI_GetDefaultJavaVMInitArgs(&vm_args);
		  const int numOptions = 4;
		  JavaVMOption options[numOptions];
		  options[0].optionString = (char*) classpath.c_str();
      options[1].optionString = "-Xcheck:jni";
      options[2].optionString = "-Xmx256m";
      options[3].optionString = "-Djava.awt.headless=true";
		  //options[1].optionString = "-verbose:jni";
		  vm_args.version = JNI_VERSION_1_6; // VM version 1.4
		  vm_args.options = options;
		  vm_args.nOptions = numOptions;

		  // load and initialize a Java VM, return a JNI interface pointer in env
		  if (JNI_CreateJavaVM(&jvm, (void**) &env, &vm_args)) {
			cout << "Failed to create the JVM" << endl;
			exit(1);
		  }
      init = true;
      }

      // construct BFITK bridge object
 	    jclass itkBridgeClass = env->FindClass("loci/formats/BioFormatsItkBridge");
	    cout << "Got itkBridge class: " << itkBridgeClass << endl;
      if (env->ExceptionOccurred()) {
        env->ExceptionDescribe();
      }

   	  jmethodID mid = env->GetMethodID(itkBridgeClass,
  	    "<init>", "()V");
  	  cout << "Got constructor method: " << mid << endl;

      BFITKBridge = env->NewObject(itkBridgeClass, mid); 

	} // end constructor

	BioFormatsJNIImageIO::~BioFormatsJNIImageIO() {
	  //jvm->DestroyJavaVM();
	} // end destructor

	bool BioFormatsJNIImageIO::CanReadFile(const char* FileNameToRead) {
	  itkDebugMacro("BioFormatsJNIImageIO::CanReadFile: FileNameToRead = " << FileNameToRead);
	  std::string filename(FileNameToRead);

	  if (filename == "") {
		itkDebugMacro("A file name must be specified.");
		return false;
	  }

	  jclass itkBridgeClass = env->GetObjectClass(BFITKBridge);
	  cout << "Got itkBridge class: " << itkBridgeClass << endl;
    if (env->ExceptionOccurred()) {
      env->ExceptionDescribe();
    }

	  jmethodID mid = env->GetStaticMethodID(itkBridgeClass,
	    "canReadFile", "(Ljava/lang/String;)Z");
	  cout << "Got canReadFile method: " << mid << endl;

	  jclass stringClass = env->FindClass("java/lang/String");
	  cout << "Got String class: " << stringClass << endl;

	  jobjectArray args = env->NewObjectArray(1, stringClass, 0);
	  jstring arg = env->NewStringUTF(FileNameToRead);
    cout << "File name: " << arg << endl;
    env->SetObjectArrayElement(args, 0, arg);

	  bool isType = 0;
	  try {
		// call Bio-Formats to check file type
		isType = env->CallStaticBooleanMethod(itkBridgeClass, mid, arg);
    cout << "istype: " << isType << endl;
		itkDebugMacro("isType = " << isType);
	  }
	  catch (std::exception& e) {
		itkDebugMacro("A C++ error occurred: " << e.what());
	  }
	  return isType;
	} // end CanReadFile function

	void BioFormatsJNIImageIO::ReadImageInformation() {
	    itkDebugMacro(
	      "BioFormatsImageIO::ReadImageInformation: m_FileName = " << m_FileName);

	  jclass itkBridgeClass = env->GetObjectClass(BFITKBridge);
		cout << "Got itkBridge class: " << itkBridgeClass << endl;

		jmethodID mid = env->GetStaticMethodID(itkBridgeClass,
		"readImageInfo", "(Ljava/lang/String;)[I");
		cout << "Got readImageInfo method: " << mid << endl;

		jclass stringClass = env->FindClass("java/lang/String");
		cout << "Got String class: " << stringClass << endl;

		jobjectArray args = env->NewObjectArray(1, stringClass, 0);
		jstring arg = env->NewStringUTF(m_FileName.c_str());
		env->SetObjectArrayElement(args, 0, arg);

		jint imageInfo [13];
		jintArray imageInfoArr = (jintArray)env->CallStaticObjectMethod(itkBridgeClass, mid, arg);
		env->GetIntArrayRegion(imageInfoArr, 0, 13, imageInfo);
    cout << "Image info: " << imageInfo << endl;
	    try {
	      int seriesCount = imageInfo[1];
	      itkDebugMacro("Series count = " << seriesCount);

	      // set ITK byte order
	      bool little = false;
	      if(imageInfo[0] == 1)
	    	  little = true;

	      if (little) SetByteOrderToLittleEndian(); // m_ByteOrder
	      else SetByteOrderToBigEndian(); // m_ByteOrder

	      // set ITK component type
	      int pixelType = imageInfo[2];
	      int bpp = imageInfo[3];
	      itkDebugMacro("Bytes per pixel = " << bpp);
	      int iotype = imageInfo[4];
	      IOComponentType itkComponentType;
	      if (iotype == 0)
	        itkComponentType = UCHAR;
	      else if (iotype == 1)
	        itkComponentType = CHAR;
	      else if (iotype == 2)
	        itkComponentType = USHORT;
	      else if (iotype == 3)
	        itkComponentType = SHORT;
	      else if (iotype == 4)
	        itkComponentType = UINT;
	      else if (iotype == 5)
	        itkComponentType = INT;
	      else if (iotype == 6)
	        itkComponentType = FLOAT;
	      else if (iotype == 7)
	        itkComponentType = DOUBLE;
	      else
	        itkComponentType = UNKNOWNCOMPONENTTYPE;
	      SetComponentType(itkComponentType); // m_ComponentType
	      if (itkComponentType == UNKNOWNCOMPONENTTYPE)
	      {
	        itkDebugMacro("Unknown pixel type: " << pixelType);
	      }

	      // get pixel resolution and dimensional extents
	      int sizeX = imageInfo[5];
	      int sizeY = imageInfo[6];
	      int sizeZ = imageInfo[7];
	      int sizeC = imageInfo[9];
	      int sizeT = imageInfo[8];
	      int effSizeC = imageInfo[10];
	      int rgbChannelCount = imageInfo[11];
	      int imageCount = imageInfo[12];

	      itkDebugMacro("Dimensional extents:" << std::endl
	        << "\tSizeX = " << sizeX << std::endl
	        << "\tSizeY = " << sizeY << std::endl
	        << "\tSizeZ = " << sizeZ << std::endl
	        << "\tSizeC = " << sizeC << std::endl
	        << "\tSizeT = " << sizeT << std::endl
	        << "\tRGB Channel Count = " << rgbChannelCount << std::endl
	        << "\tEffective SizeC = " << effSizeC << std::endl
	        << "\tImage Count = " << imageCount);

	      // NB: Always return 5D, to be unambiguous.
	      int numDims = 5;
	      /*
	      int numDims = 2; // X and Y
	      if (sizeZ > 1) numDims++; // multiple focal planes
	      if (sizeT > 1) numDims++; // multiple time points
	      if (effSizeC > 1) numDims++; // multiple independent channels
	      */

	      SetNumberOfDimensions(numDims);
	      m_Dimensions[0] = sizeX;
	      m_Dimensions[1] = sizeY;
	      /*
	      int dim = 2;
	      if (sizeZ > 1) m_Dimensions[dim++] = sizeZ;
	      if (sizeT > 1) m_Dimensions[dim++] = sizeT;
	      if (effSizeC > 1) m_Dimensions[dim++] = effSizeC;
	      */
	      m_Dimensions[2] = sizeZ;
	      m_Dimensions[3] = sizeT;
	      m_Dimensions[4] = effSizeC;

	      // set ITK pixel type
	      IOPixelType itkPixelType;
	      if (rgbChannelCount == 1)
	        itkPixelType = SCALAR;
	      else if (rgbChannelCount == 3)
	        itkPixelType = RGB;
	      else
	        itkPixelType = VECTOR;
	      SetPixelType(itkPixelType); // m_PixelType
	      SetNumberOfComponents(rgbChannelCount); // m_NumberOfComponents

	      // get physical resolution
	      double physX = 1, physY = 1, physZ = 1, timeIncrement = 1;
	      // CTR - avoid invalid memory access error on some systems (OS X 10.5)
	      //MetadataRetrieve retrieve = MetadataTools::asRetrieve(omeMeta);
	      //physX = retrieve.getPixelsPhysicalSizeX(0).doubleValue();
	      //physY = retrieve.getPixelsPhysicalSizeY(0).doubleValue();
	      //physZ = retrieve.getPixelsPhysicalSizeZ(0).doubleValue();
	      //timeIncrement = retrieve.getPixelsTimeIncrement(0).doubleValue();
	      m_Spacing[0] = physX;
	      m_Spacing[1] = physY;
	      // TODO: verify m_Spacing.length > 2
	      if (imageCount > 1) m_Spacing[2] = physZ;
	      m_Spacing[3] = timeIncrement;

	      itkDebugMacro("Physical resolution = " << physX << " x " << physY
	        << " x " << physZ << " x " << timeIncrement);
	    }
	    catch (std::exception& e) {
	      itkDebugMacro("A C++ error occurred: " << e.what());
	    }

      cout << "Done setting image info" << endl;
	} // end ReadImageInformation function

	void BioFormatsJNIImageIO::Read(void* pData) {
		/*
		   itkDebugMacro("BioFormatsImageIO::Read");

		    try {
		      int pixelType = reader->getPixelType();
		      int bpp = FormatTools::getBytesPerPixel(pixelType);
		      int rgbChannelCount = reader->getRGBChannelCount();

		      itkDebugMacro("Pixel type:" << std::endl
		        << "Pixel type code = " << pixelType << std::endl
		        << "Bytes per pixel = " << bpp << std::endl
		        << "RGB channel count = " << rgbChannelCount);

		      // check IO region to identify the planar extents desired
		      ImageIORegion region = GetIORegion();
		      int regionDim = region.GetImageDimension();
		      int xStart = 0, xCount = 1;
		      int yStart = 0, yCount = 1;
		      int zStart = 0, zCount = 1;
		      int tStart = 0, tCount = 1;
		      int cStart = 0, cCount = 1;

		      //int sizeZ = reader->getSizeZ();
		      //int sizeT = reader->getSizeT();
		      //int effSizeC = reader->getEffectiveSizeC();

		      int xIndex = 0, yIndex = 1, zIndex = 2, tIndex = 3, cIndex = 4;
		      //  Currently unnecessary, as images are assumed to be 5D
		      //if (sizeZ == 1) {
		      //  zIndex = -1;
		      //  tIndex--;
		      //  cIndex--;
		      //}
		      //if (sizeT == 1) {
		      //  tIndex = -1;
		      //  cIndex--;
		      //}
		      //if (effSizeC == 1) {
		      //  cIndex = -1;
		      //}

		      for (int dim = 0; dim < regionDim; dim++) {
		        int index = region.GetIndex(dim);
		        int size = region.GetSize(dim);
		        if (dim == xIndex) {
		          xStart = index;
		          xCount = size;
		        }
		        else if (dim == yIndex) {
		          yStart = index;
		          yCount = size;
		        }
		        else if (dim == zIndex) {
		          zStart = index;
		          zCount = size;
		        }
		        else if (dim == tIndex) {
		          tStart = index;
		          tCount = size;
		        }
		        else if (dim == cIndex) {
		          cStart = index;
		          cCount = size;
		        }
		      }
		      int bytesPerPlane = xCount * yCount * bpp * rgbChannelCount;
		      bool isInterleaved = reader->isInterleaved();

		      itkDebugMacro("Region extents:" << std::endl
		        << "\tRegion dimension = " << regionDim << std::endl
		        << "\tX: start = " << xStart << ", count = " << xCount << std::endl
		        << "\tY: start = " << yStart << ", count = " << yCount << std::endl
		        << "\tZ: start = " << zStart << ", count = " << zCount << std::endl
		        << "\tT: start = " << tStart << ", count = " << tCount << std::endl
		        << "\tC: start = " << cStart << ", count = " << cCount << std::endl
		        << "\tBytes per plane = " << bytesPerPlane << std::endl
		        << "\tIsInterleaved = " << isInterleaved);


		      int imageCount = reader->getImageCount();

		      // allocate temporary array
		      bool canDoDirect = (rgbChannelCount == 1 || isInterleaved);
		      jbyte* tmpData = NULL;
		      if (!canDoDirect) tmpData = new jbyte[bytesPerPlane];

		      jbyte* jData = (jbyte*) pData;
		      ByteArray buf(bytesPerPlane); // pre-allocate buffer
		      for (int c=cStart; c<cStart+cCount; c++) {
		        for (int t=tStart; t<tStart+tCount; t++) {
		          for (int z=zStart; z<zStart+zCount; z++) {
		            int no = reader->getIndex(z, c, t);
		            itkDebugMacro("Reading image plane " << no + 1
		              << " (Z=" << z << ", T=" << t << ", C=" << c << ")"
		              << " of " << imageCount << " available planes)");
		            reader->openBytes(no, buf, xStart, yStart, xCount, yCount);

		            JNIEnv* env = jace::helper::attach();
		            jbyteArray jArray = static_cast<jbyteArray>(buf.getJavaJniArray());
		            if (canDoDirect) {
		              env->GetByteArrayRegion(jArray, 0, bytesPerPlane, jData);
		            }
		            else {
		              // need to reorganize byte array after copy
		              env->GetByteArrayRegion(jArray, 0, bytesPerPlane, tmpData);

		              // reorganize elements
		              int pos = 0;
		              for (int x=0; x<xCount; x++) {
		                for (int y=0; y<yCount; y++) {
		                  for (int i=0; i<rgbChannelCount; i++) {
		                    for (int b=0; b<bpp; b++) {
		                      int index = yCount * (xCount * (rgbChannelCount * b + i) + x) + y;
		                      jData[pos++] = tmpData[index];
		                    }
		                  }
		                }
		              }
		            }
		            jData += bytesPerPlane;
		          }
		        }
		      }

		      // delete temporary array
		      if (tmpData != NULL) {
		        delete tmpData;
		        tmpData = NULL;
		      }

		      ((IFormatHandler*)reader)->close();
		    }
		    catch (Exception& e) {
		      itkDebugMacro("A Java error occurred: " << DebugTools::getStackTrace(e));
		    }
		    catch (JNIException& jniException) {
		      itkDebugMacro(
		        "A JNI error occurred: " << jniException.what());
		    }
		    catch (std::exception& e) {
		      itkDebugMacro("A C++ error occurred: " << e.what());
		    }
		    itkDebugMacro("Done.");
		    */
	} // end Read function

	bool BioFormatsJNIImageIO::CanWriteFile(const char* name) {
		// TODO
		return false;
	} // end CanWriteFile function

    void BioFormatsJNIImageIO::WriteImageInformation() {
  	  itkDebugMacro("BioFormatsImageIO::WriteImageInformation");
	  // NB: Nothing to do.
    } // end WriteImageInformation function

    void BioFormatsJNIImageIO::Write(const void* buffer) {
	  itkDebugMacro("BioFormatsImageIO::Write");
	  // CTR TODO - implement Write function
    } // end Write function

} // end namespace itk
