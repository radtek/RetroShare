
/*
 * libretroshare/src/serialiser: rstlvgenericmap.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2014 by Robert Fernie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include "rstlvbase.h"
#include "rstlvtypes.h"
#include "rstlvgenericmap.h"
#include "rsbaseserial.h"
#include "util/rsprint.h"
#include <ostream>
#include <sstream>
#include <iomanip>
#include <iostream>

#define TLV_DEBUG 1

/* generic print */
template<class T>
std::ostream & RsTlvParamRef<T>::print(std::ostream &out, uint16_t indent)
{
	printIndent(out, indent);
	out << "Type: " << mParamType << "Param: " << mParam;
	return out;
}


template<>
uint32_t RsTlvParamRef<uint32_t>::TlvSize()
{
	return GetTlvUInt32Size();
}

template<>
void RsTlvParamRef<uint32_t>::TlvClear()
{
	mParam = 0;
}

template<>
bool RsTlvParamRef<uint32_t>::SetTlv(void *data, uint32_t size, uint32_t *offset)
{
        return SetTlvUInt32(data, size, offset, mParamType, mParam);
}

template<>
bool RsTlvParamRef<uint32_t>::GetTlv(void *data, uint32_t size, uint32_t *offset)
{
	return GetTlvUInt32(data, size, offset, mParamType, &mParam);
}

#if 0
template<>
std::ostream & RsTlvParamRef<uint32_t>::print(std::ostream &out, uint16_t indent)
{
	printIndent(out, indent);
	out << mParam;
	return out;
}
#endif

/***** std::string ****/
template<>
uint32_t RsTlvParamRef<std::string>::TlvSize()
{
	return GetTlvStringSize(mParam);
}

template<>
void RsTlvParamRef<std::string>::TlvClear()
{
	mParam.clear();
}

template<>
bool RsTlvParamRef<std::string>::SetTlv(void *data, uint32_t size, uint32_t *offset)
{
        return SetTlvString(data, size, offset, mParamType, mParam);
}

template<>
bool RsTlvParamRef<std::string>::GetTlv(void *data, uint32_t size, uint32_t *offset)
{
	return GetTlvString(data, size, offset, mParamType, mParam);
}

#if 0
template<>
std::ostream & RsTlvParamRef<std::string>::print(std::ostream &out, uint16_t indent)
{
	printIndent(out, indent);
	out << "Type: " << mParamType << "Param: " << mParam;
	return out;
}
#endif

// declare likely combinations.
template class RsTlvParamRef<uint32_t>;
template class RsTlvParamRef<std::string>;

/*********************************** RsTlvGenericPairRef ***********************************/

template<class K, class V>
void RsTlvGenericPairRef<K, V>::TlvClear()
{
	RsTlvParamRef<K> key(mKeyType, mKey);
	RsTlvParamRef<V> value(mValueType, mValue);
	key.TlvClear();
	value.TlvClear();
}

template<class K, class V>
uint32_t RsTlvGenericPairRef<K, V>::TlvSize()
{
	uint32_t s = TLV_HEADER_SIZE; /* header */
	RsTlvParamRef<K> key(mKeyType, mKey);
	RsTlvParamRef<V> value(mValueType, mValue);

	s += key.TlvSize();
	s += value.TlvSize();
	return s;

}

template<class K, class V>
bool  RsTlvGenericPairRef<K, V>::SetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;

	/* start at data[offset] */
	ok &= SetTlvBase(data, tlvend, offset, mPairType, tlvsize);
	
	RsTlvParamRef<K> key(mKeyType, mKey);
	RsTlvParamRef<V> value(mValueType, mValue);
	ok &= key.SetTlv(data, tlvend, offset);
	ok &= value.SetTlv(data, tlvend, offset);

	return ok;
}

template<class K, class V>
bool  RsTlvGenericPairRef<K, V>::GetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
{
	if (size < *offset + TLV_HEADER_SIZE)
		return false;	
	
	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != mPairType) /* check type */
		return false;

	bool ok = true;

	/* ready to load */
	TlvClear();

	RsTlvParamRef<K> key(mKeyType, mKey);
	RsTlvParamRef<V> value(mValueType, mValue);
	ok &= key.GetTlv(data, tlvend, offset);
	ok &= value.GetTlv(data, tlvend, offset);

	/***************************************************************************
	 * NB: extra components could be added (for future expansion of the type).
	 *            or be present (if this code is reading an extended version).
	 *
	 * We must chew up the extra characters to conform with TLV specifications
	 ***************************************************************************/
	if (*offset != tlvend)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvGenericPairRef::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}
	return ok;
	
}

template<class K, class V>
std::ostream &RsTlvGenericPairRef<K, V>::print(std::ostream &out, uint16_t indent)
{ 
	printBase(out, "RsTlvGenericPairRef", indent);
	uint16_t int_Indent = indent + 2;

	RsTlvParamRef<K> key(mKeyType, mKey);
	RsTlvParamRef<V> value(mValueType, mValue);

	printIndent(out, int_Indent);
	out << "Key:";
	key.print(out, 0);
	out << std::endl;

	printIndent(out, int_Indent);
	out << "Value:";
	value.print(out, 0);
	out << std::endl;
	
	printEnd(out, "RsTlvGenericPairRef", indent);
	return out;
}

// declare likely combinations.
//template<> class RsTlvGenericPairRef<uint32_t, uint32_t>;
//template<> class RsTlvGenericPairRef<uint32_t, std::string>;
//template<> class RsTlvGenericPairRef<std::string, uint32_t>;
//template<> class RsTlvGenericPairRef<std::string, std::string>;

/************************************ RsTlvGenericMapRef ***********************************/

template<class K, class V>
void RsTlvGenericMapRef<K, V>::TlvClear()
{
	mRefMap.clear(); //empty list
}

template<class K, class V>
uint32_t RsTlvGenericMapRef<K, V>::TlvSize()
{
	uint32_t s = TLV_HEADER_SIZE; /* header */

	typename std::map<K, V>::iterator it;
	for(it = mRefMap.begin(); it != mRefMap.end(); ++it)
	{
		RsTlvGenericPairRef<const K, V> pair(mPairType, mKeyType, mValueType, it->first, it->second);
		s += pair.TlvSize();
	}

	return s;
}

template<class K, class V>
bool  RsTlvGenericMapRef<K, V>::SetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;

	/* start at data[offset] */
	ok &= SetTlvBase(data, tlvend, offset, mMapType, tlvsize);
	

	typename std::map<K, V>::iterator it;
	for(it = mRefMap.begin(); it != mRefMap.end(); ++it)
	{
		RsTlvGenericPairRef<const K, V> pair(mPairType, mKeyType, mValueType, it->first, it->second);
		ok &= pair.SetTlv(data, size, offset);
	}

	return ok;
}

template<class K, class V>
bool  RsTlvGenericMapRef<K, V>::GetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
{
	if (size < *offset + TLV_HEADER_SIZE)
		return false;	

	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != mMapType) /* check type */
		return false;

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;

        /* while there is TLV  */
        while((*offset) + 2 < tlvend)
        {
                /* get the next type */
                uint16_t tlvsubtype = GetTlvType( &(((uint8_t *) data)[*offset]) );

		if (tlvsubtype == mPairType)
		{
			K k;
			V v;
			RsTlvGenericPairRef<const K, V> pair(mPairType, mKeyType, mValueType, k, v);
			ok &= pair.GetTlv(data, size, offset);
			if (ok)
			{
				mRefMap[k] = v;
			}
		}
		else
		{
			ok &= SkipUnknownTlv(data, tlvend, offset);
			ok = false;
		}

                if (!ok)
			break;
	}
		
	/***************************************************************************
	 * NB: extra components could be added (for future expansion of the type).
	 *            or be present (if this code is reading an extended version).
	 *
	 * We must chew up the extra characters to conform with TLV specifications
	 ***************************************************************************/
	if (*offset != tlvend)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvGenericMapRef::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	return ok;
}

template<class K, class V>
std::ostream &RsTlvGenericMapRef<K, V>::print(std::ostream &out, uint16_t indent)
{
	printBase(out, "RsTlvGenericMapRef", indent);
	uint16_t int_Indent = indent + 2;
	
	printIndent(out, int_Indent);
	out << "MapType:" << mMapType << std::endl;

	typename std::map<K, V>::iterator it;
	for(it = mRefMap.begin(); it != mRefMap.end() ; ++it)
	{
		RsTlvGenericPairRef<const K, V> pair(mPairType, mKeyType, mValueType, it->first, it->second);
		pair.print(out, int_Indent);
	}

	printEnd(out, "RsTlvGenericMapRef", indent);
	return out;
}

// declare likely combinations.
template class RsTlvGenericMapRef<uint32_t, uint32_t>;
template class RsTlvGenericMapRef<uint32_t, std::string>;
template class RsTlvGenericMapRef<std::string, uint32_t>;
template class RsTlvGenericMapRef<std::string, std::string>;




