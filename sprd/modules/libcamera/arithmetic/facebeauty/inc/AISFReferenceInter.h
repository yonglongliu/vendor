/*----------------------------------------------------------------------------------------------
*
* This file is ArcSoft's property. It contains ArcSoft's trade secret, proprietary and
* confidential information.
*
* The information and code contained in this file is only for authorized ArcSoft employees
* to design, create, modify, or review.
*
* DO NOT DISTRIBUTE, DO NOT DUPLICATE OR TRANSMIT IN ANY FORM WITHOUT PROPER AUTHORIZATION.
*
* If you are not an intended recipient of this file, you must not copy, distribute, modify,
* or take any action in reliance on it.
*
* If you have received this file in error, please immediately notify ArcSoft and
* permanently delete the original and any copy of any file and any printout thereof.
*
*-------------------------------------------------------------------------------------------------*/
/*
* AISFReferenceInter.h
*
* Reference:
*
* Description:
*
* Create by xhliu 2014-07-22
*
*/
#ifndef __AISFREFERENCEINTER_H_20141028__
#define __AISFREFERENCEINTER_H_20141028__

#include "amcomdef.h"
#include "AISFCommonDef.h"


START_NAMESPACE_AISF
class AISFReferenceInter
{
public:
	AISFReferenceInter()
	{

	}
	virtual ~AISFReferenceInter()
	{

	}
	//Add Reference,return the refer count after add
	virtual MInt32 AddRef() =0;
	//Release referrence,return the refer count after release
	virtual MInt32 Release() =0;

};

END_NAMESPACE_AISF

#endif
