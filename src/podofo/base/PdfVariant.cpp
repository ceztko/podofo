/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *   Copyright (C) 2020 by Francesco Pretto                                *
 *   ceztko@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#include "PdfVariant.h"

#include "PdfArray.h"
#include "PdfData.h"
#include "PdfDictionary.h"
#include "PdfOutputDevice.h"
#include "PdfParserObject.h"
#include "PdfDefinesPrivate.h"

#include <sstream>

#include <string.h>

using namespace PoDoFo;
using namespace std;

PdfVariant PdfVariant::NullValue;

PdfVariant::PdfVariant(EPdfDataType type)
    : m_Data{ }, m_eDataType(type) { }

PdfVariant::PdfVariant()
    : PdfVariant(EPdfDataType::Null) { }

PdfVariant::PdfVariant( bool b )
    : PdfVariant(EPdfDataType::Bool)
{
    m_Data.bBoolValue = b;
}

PdfVariant::PdfVariant( int64_t l )
    : PdfVariant(EPdfDataType::Number)
{
    m_Data.nNumber = l;
}

PdfVariant::PdfVariant( double d )
    : PdfVariant(EPdfDataType::Real)
{
    m_Data.dNumber = d;    
}

PdfVariant::PdfVariant( const PdfString & rsString )
    : PdfVariant(EPdfDataType::String)
{
    m_Data.pData = new PdfString( rsString );
}

PdfVariant::PdfVariant( const PdfName & rName )
    : PdfVariant(EPdfDataType::Name)
{
    m_Data.pData = new PdfName( rName );
}

PdfVariant::PdfVariant( const PdfReference & rRef )
    : PdfVariant(EPdfDataType::Reference)
{
    m_Data.pData = new PdfReference( rRef );
}

PdfVariant::PdfVariant( const PdfArray & rArray )
    : PdfVariant(EPdfDataType::Array)
{
    m_Data.pData = new PdfArray( rArray );
}

PdfVariant::PdfVariant( const PdfDictionary & rObj )
    : PdfVariant(EPdfDataType::Dictionary)
{
    m_Data.pData = new PdfDictionary( rObj );
}

PdfVariant::PdfVariant( const PdfData & rData )
    : PdfVariant(EPdfDataType::RawData)
{
    m_Data.pData = new PdfData( rData );
}

PdfVariant::PdfVariant( const PdfVariant & rhs )
    : PdfVariant(EPdfDataType::Unknown)
{
    this->operator=(rhs);
}

PdfVariant::~PdfVariant()
{
    Clear();
}

void PdfVariant::Clear()
{
    switch( m_eDataType ) 
    {
        case EPdfDataType::Array:
        case EPdfDataType::Reference:
        case EPdfDataType::Dictionary:
        case EPdfDataType::Name:
        case EPdfDataType::String:
        case EPdfDataType::RawData:
        {
            delete m_Data.pData;
            break;
        }
            
        case EPdfDataType::Bool:
        case EPdfDataType::Null:
        case EPdfDataType::Number:
        case EPdfDataType::Real:
        case EPdfDataType::Unknown:
        default:
            break;
            
    }

    m_Data = { };
}

void PdfVariant::Write( PdfOutputDevice& pDevice, EPdfWriteMode eWriteMode,
    const PdfEncrypt* pEncrypt) const
{
    switch( m_eDataType ) 
    {
        case EPdfDataType::Bool:
        {
            if( (eWriteMode & EPdfWriteMode::Compact) == EPdfWriteMode::Compact ) 
                pDevice.Write( " ", 1 ); // Write space before true or false

            if( m_Data.bBoolValue )
                pDevice.Write( "true", 4 );
            else
                pDevice.Write( "false", 5 );
            break;
        }
        case EPdfDataType::Number:
        {
            if( (eWriteMode & EPdfWriteMode::Compact) == EPdfWriteMode::Compact ) 
                pDevice.Write( " ", 1 ); // Write space before numbers

            pDevice.Print( "%" PDF_FORMAT_INT64, m_Data.nNumber );
            break;
        }
        case EPdfDataType::Real:
        {
            if( (eWriteMode & EPdfWriteMode::Compact) == EPdfWriteMode::Compact ) 
                pDevice.Write( " ", 1 ); // Write space before numbers

            // Use ostringstream, so that locale does not matter
            // NOTE: Don't use printf() formatting! It may write the number
            // way that is incompatible in PDF
            std::ostringstream oss;
            PdfLocaleImbue(oss);
            oss << std::fixed << m_Data.dNumber;
            std::string copy = oss.str();
            size_t len = copy.size();

            if( (eWriteMode & EPdfWriteMode::Compact) == EPdfWriteMode::Compact && 
                copy.find('.') != string::npos )
            {
                const char *str = copy.c_str();
                while( str[len - 1] == '0' )
                    --len;
                if( str[len - 1] == '.' )
                    --len;
                if( len == 0 )
                {
                    pDevice.Write( "0", 1 );
                    break;
                }
            }

            pDevice.Write( copy.c_str(), len );
            break;
        }
        case EPdfDataType::String:
        case EPdfDataType::Name:
        case EPdfDataType::Array:
        case EPdfDataType::Dictionary:
        case EPdfDataType::Reference:
        case EPdfDataType::RawData:
            if (!m_Data.pData)
                PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

            m_Data.pData->Write(pDevice, eWriteMode, pEncrypt);
            break;
        case EPdfDataType::Null:
        {
            if( (eWriteMode & EPdfWriteMode::Compact) == EPdfWriteMode::Compact ) 
            {
                pDevice.Write( " ", 1 ); // Write space before null
            }

            pDevice.Print( "null" );
            break;
        }
        case EPdfDataType::Unknown:
        default:
        {
            PODOFO_RAISE_ERROR( EPdfError::InvalidDataType );
            break;
        }
    };
}

void PdfVariant::ToString( std::string & rsData, EPdfWriteMode eWriteMode ) const
{
    ostringstream   out;
    // We don't need to this stream with the safe PDF locale because
    // PdfOutputDevice will do so for us.
    PdfOutputDevice device( &out );

    this->Write(device, eWriteMode, nullptr);
    
    rsData = out.str();
}

const PdfVariant & PdfVariant::operator=( const PdfVariant & rhs )
{
    Clear();

    m_eDataType = rhs.m_eDataType;
    
    switch( m_eDataType ) 
    {
        case EPdfDataType::Array:
        {
            m_Data.pData = new PdfArray(*static_cast<PdfArray*>(rhs.m_Data.pData));
            break;
        }
        case EPdfDataType::Reference:
        {
            m_Data.pData = new PdfReference(*static_cast<PdfReference*>(rhs.m_Data.pData));
            break;
        }
        case EPdfDataType::Dictionary:
        {
            m_Data.pData = new PdfDictionary(*static_cast<PdfDictionary*>(rhs.m_Data.pData));
            break;
        }
        case EPdfDataType::Name:
        {
            m_Data.pData = new PdfName(*static_cast<PdfName*>(rhs.m_Data.pData));
            break;
        }
        case EPdfDataType::String:
        {
            m_Data.pData = new PdfString(*static_cast<PdfString*>(rhs.m_Data.pData));
            break;
        }
            
        case EPdfDataType::RawData: 
        {
            m_Data.pData = new PdfData(*static_cast<PdfData*>(rhs.m_Data.pData));
            break;
        }
        case EPdfDataType::Bool:
        case EPdfDataType::Null:
        case EPdfDataType::Number:
        case EPdfDataType::Real:
            m_Data = rhs.m_Data;
            break;
            
        case EPdfDataType::Unknown:
        default:
            break;
    };

    return *this;
}

const char * PdfVariant::GetDataTypeString() const
{
    switch (m_eDataType)
    {
        case EPdfDataType::Bool:
            return "Bool";
        case EPdfDataType::Number:
            return "Number";
        case EPdfDataType::Real:
            return "Real";
        case EPdfDataType::String:
            return "String";
        case EPdfDataType::Name:
            return "Name";
        case EPdfDataType::Array:
            return "Array";
        case EPdfDataType::Dictionary:
            return "Dictionary";
        case EPdfDataType::Null:
            return "Null";
        case EPdfDataType::Reference: return
            "Reference";
        case EPdfDataType::RawData:
            return "RawData";
        case EPdfDataType::Unknown:
            return "Unknown";
        default:
            return "INVALID_TYPE_ENUM";
    }
}

bool PdfVariant::operator==(const PdfVariant& rhs) const
{
    if (this == &rhs)
        return true;

    switch (m_eDataType)
    {
    case EPdfDataType::Bool:
    {
        bool value;
        if (rhs.TryGetBool(value))
            return m_Data.bBoolValue == value;
        else
            return false;
    }
    case EPdfDataType::Number:
    {
        int64_t value;
        if (rhs.TryGetNumber(value))
            return m_Data.nNumber == value;
        else
            return false;
    }
    case EPdfDataType::Real:
    {
        // NOTE: Real type equality semantics is strict
        double value;
        if (rhs.TryGetRealStrict(value))
            return m_Data.dNumber == value;
        else
            return false;
    }
    case EPdfDataType::Reference:
    {
        PdfReference value;
        if (rhs.TryGetReference(value))
            return *(PdfReference*)m_Data.pData == value;
        else
            return false;
    }
    case EPdfDataType::String:
    {
        const PdfString* value;
        if (rhs.TryGetString(value))
            return *(PdfString*)m_Data.pData == *value;
        else
            return false;
    }
    case EPdfDataType::Name:
    {
        const PdfName* value;
        if (rhs.TryGetName(value))
            return *(PdfName*)m_Data.pData == *value;
        else
            return false;
    }
    case EPdfDataType::Array:
    {
        const PdfArray* value;
        if (rhs.TryGetArray(value))
            return *(PdfArray*)m_Data.pData == *value;
        else
            return false;
    }
    case EPdfDataType::Dictionary:
    {
        const PdfDictionary* value;
        if (rhs.TryGetDictionary(value))
            return *(PdfDictionary*)m_Data.pData == *value;
        else
            return false;
    }
    case EPdfDataType::RawData:
        PODOFO_RAISE_ERROR_INFO(EPdfError::NotImplemented, "Equality not yet implemented for RawData");
    case EPdfDataType::Null:
        return m_eDataType == EPdfDataType::Null;
    case EPdfDataType::Unknown:
        return false;
    default:
        PODOFO_RAISE_ERROR(EPdfError::NotImplemented);
    }
}

bool PdfVariant::operator!=(const PdfVariant& rhs) const
{
    if (this != &rhs)
        return true;

    switch (m_eDataType)
    {
    case EPdfDataType::Bool:
    {
        bool value;
        if (rhs.TryGetBool(value))
            return m_Data.bBoolValue != value;
        else
            return true;
    }
    case EPdfDataType::Number:
    {
        int64_t value;
        if (rhs.TryGetNumber(value))
            return m_Data.nNumber != value;
        else
            return true;
    }
    case EPdfDataType::Real:
    {
        // NOTE: Real type equality semantics is strict
        double value;
        if (rhs.TryGetRealStrict(value))
            return m_Data.dNumber != value;
        else
            return true;
    }
    case EPdfDataType::Reference:
    {
        PdfReference value;
        if (rhs.TryGetReference(value))
            return *(PdfReference*)m_Data.pData != value;
        else
            return true;
    }
    case EPdfDataType::String:
    {
        const PdfString* value;
        if (rhs.TryGetString(value))
            return *(PdfString*)m_Data.pData != *value;
        else
            return true;
    }
    case EPdfDataType::Name:
    {
        const PdfName* value;
        if (rhs.TryGetName(value))
            return *(PdfName*)m_Data.pData != *value;
        else
            return true;
    }
    case EPdfDataType::Array:
    {
        const PdfArray* value;
        if (rhs.TryGetArray(value))
            return *(PdfArray*)m_Data.pData != *value;
        else
            return true;
    }
    case EPdfDataType::Dictionary:
    {
        const PdfDictionary* value;
        if (rhs.TryGetDictionary(value))
            return *(PdfDictionary*)m_Data.pData != *value;
        else
            return true;
    }
    case EPdfDataType::RawData:
        PODOFO_RAISE_ERROR_INFO(EPdfError::NotImplemented, "Disequality not yet implemented for RawData");
    case EPdfDataType::Null:
        return m_eDataType != EPdfDataType::Null;
    case EPdfDataType::Unknown:
        return true;
    default:
        PODOFO_RAISE_ERROR(EPdfError::NotImplemented);
    }
}

bool PdfVariant::GetBool() const
{
    bool ret;
    if (!TryGetBool(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return ret;
}

bool PdfVariant::TryGetBool(bool& value) const
{
    if (m_eDataType != EPdfDataType::Bool)
    {
        value = false;
        return false;
    }

    value = m_Data.bBoolValue;
    return true;
}

int64_t PdfVariant::GetNumberLenient() const
{
    int64_t ret;
    if (!TryGetNumberLenient(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return ret;
}

bool PdfVariant::TryGetNumberLenient(int64_t& value) const
{
    if (!(m_eDataType == EPdfDataType::Number
        || m_eDataType == EPdfDataType::Real))
    {
        value = 0;
        return false;
    }

    if (m_eDataType == EPdfDataType::Real)
        value = static_cast<int64_t>(std::round(m_Data.dNumber));
    else
        value = m_Data.nNumber;

    return true;
}

int64_t PdfVariant::GetNumber() const
{
    int64_t ret;
    if (!TryGetNumber(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return m_Data.nNumber;
}

bool PdfVariant::TryGetNumber(int64_t& value) const
{
    if (m_eDataType != EPdfDataType::Number)
    {
        value = 0;
        return false;
    }

    value = m_Data.nNumber;
    return true;
}

double PdfVariant::GetReal() const
{
    double ret;
    if (!TryGetReal(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return ret;
}

bool PdfVariant::TryGetReal(double& value) const
{
    if (!(m_eDataType == EPdfDataType::Real
        || m_eDataType == EPdfDataType::Number))
    {
        value = 0;
        return false;
    }

    if (m_eDataType == EPdfDataType::Number)
        value = static_cast<double>(m_Data.nNumber);
    else
        value = m_Data.dNumber;

    return true;
}

double PdfVariant::GetRealStrict() const
{
    double ret;
    if (!TryGetRealStrict(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return m_Data.dNumber;
}

bool PdfVariant::TryGetRealStrict(double& value) const
{
    if (m_eDataType != EPdfDataType::Real)
    {
        value = 0;
        return false;
    }

    value = m_Data.dNumber;
    return true;
}

const PdfString & PdfVariant::GetString() const
{
    const PdfString* ret;
    if (!TryGetString(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return *ret;
}

bool PdfVariant::TryGetString(const PdfString*& str) const
{
    if (m_eDataType != EPdfDataType::String)
    {
        str = nullptr;
        return false;
    }

    str = (PdfString*)m_Data.pData;
    return true;
}

const PdfName & PdfVariant::GetName() const
{
    const PdfName* ret;
    if (!TryGetName(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return *ret;
}

bool PdfVariant::TryGetName(const PdfName*& name) const
{
    if (m_eDataType != EPdfDataType::Name)
    {
        name = nullptr;
        return false;
    }

    name = (PdfName*)m_Data.pData;
    return true;
}

PdfReference PdfVariant::GetReference() const
{
    PdfReference ret;
    if (!TryGetReference(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return ret;
}

bool PdfVariant::TryGetReference(PdfReference& ref) const
{
    if (m_eDataType != EPdfDataType::Reference)
    {
        ref = PdfReference();
        return false;
    }

    ref = *(PdfReference*)m_Data.pData;
    return true;
}

const PdfData& PdfVariant::GetRawData() const
{
    const PdfData* ret;
    if (!TryGetRawData(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return *ret;
}

PdfData& PdfVariant::GetRawData()
{
    PdfData* ret;
    if (!TryGetRawData(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return *ret;
}

bool PdfVariant::TryGetRawData(const PdfData*& data) const
{
    if (m_eDataType != EPdfDataType::RawData)
    {
        data = nullptr;
        return false;
    }

    data = (PdfData*)m_Data.pData;
    return true;
}



bool PdfVariant::TryGetRawData(PdfData*& data)
{
    if (m_eDataType != EPdfDataType::RawData)
    {
        data = nullptr;
        return false;
    }

    data = (PdfData*)m_Data.pData;
    return true;
}

const PdfArray & PdfVariant::GetArray() const
{
    PdfArray* ret;
    if (!tryGetArray(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return *ret;
}

PdfArray & PdfVariant::GetArray()
{
    PdfArray* ret;
    if (!tryGetArray(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return *ret;
}

bool PdfVariant::TryGetArray(const PdfArray*& arr) const
{
    return tryGetArray(const_cast<PdfArray*&>(arr));
}

bool PdfVariant::TryGetArray(PdfArray*& arr)
{
    return tryGetArray(arr);
}

const PdfDictionary & PdfVariant::GetDictionary() const
{
    PdfDictionary* ret;
    if (!tryGetDictionary(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return *ret;
}

PdfDictionary & PdfVariant::GetDictionary()
{
    PdfDictionary* ret;
    if (!tryGetDictionary(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return *ret;
}

bool PdfVariant::TryGetDictionary(const PdfDictionary*& dict) const
{
    return tryGetDictionary(const_cast<PdfDictionary*&>(dict));
}

bool PdfVariant::TryGetDictionary(PdfDictionary*& dict)
{
    return tryGetDictionary(dict);
}

bool PdfVariant::tryGetDictionary(PdfDictionary*& dict) const
{
    if (m_eDataType != EPdfDataType::Dictionary)
    {
        dict = nullptr;
        return false;
    }

    dict = (PdfDictionary*)m_Data.pData;
    return true;
}

bool PdfVariant::tryGetArray(PdfArray*& arr) const
{
    if (m_eDataType != EPdfDataType::Array)
    {
        arr = nullptr;
        return false;
    }

    arr = (PdfArray*)m_Data.pData;
    return true;
}

void PdfVariant::SetBool(bool b)
{
    if (m_eDataType != EPdfDataType::Bool)
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    m_Data.bBoolValue = b;
}

void PdfVariant::SetNumber(int64_t l)
{
    if (!(m_eDataType == EPdfDataType::Number
        || m_eDataType == EPdfDataType::Real))
    {
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);
    }

    if (m_eDataType == EPdfDataType::Real)
        m_Data.dNumber = static_cast<double>(l);
    else
        m_Data.nNumber = l;
}

void PdfVariant::SetReal(double d)
{
    if (!(m_eDataType == EPdfDataType::Real
        || m_eDataType == EPdfDataType::Number))
    {
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);
    }

    if (m_eDataType == EPdfDataType::Number)
        m_Data.nNumber = static_cast<int64_t>(std::round(d));
    else
        m_Data.dNumber = d;
}

void PdfVariant::SetName(const PdfName &name)
{
    if (m_eDataType != EPdfDataType::Name)
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    *((PdfName*)m_Data.pData) = name;
}

void PdfVariant::SetString(const PdfString &str)
{
    if (m_eDataType != EPdfDataType::String)
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    *((PdfString*)m_Data.pData) = str;
}

void PdfVariant::SetReference(const PdfReference &ref)
{
    if (m_eDataType != EPdfDataType::Reference)
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    *((PdfReference*)m_Data.pData) = ref;
}

bool PdfVariant::IsBool() const
{
    return m_eDataType == EPdfDataType::Bool;
}

bool PdfVariant::IsNumber() const
{
    return m_eDataType == EPdfDataType::Number;
}

bool PdfVariant::IsRealStrict() const
{
    return m_eDataType == EPdfDataType::Real;
}

bool PdfVariant::IsNumberOrReal() const
{
    return m_eDataType == EPdfDataType::Number || m_eDataType == EPdfDataType::Real;
}

bool PdfVariant::IsString() const
{
    return m_eDataType == EPdfDataType::String;
}

bool PdfVariant::IsName() const
{
    return m_eDataType == EPdfDataType::Name;
}

bool PdfVariant::IsArray() const
{
    return m_eDataType == EPdfDataType::Array;
}

bool PdfVariant::IsDictionary() const
{
    return m_eDataType == EPdfDataType::Dictionary;
}

bool PdfVariant::IsRawData() const
{
    return m_eDataType == EPdfDataType::RawData;
}

bool PdfVariant::IsNull() const
{
    return m_eDataType == EPdfDataType::Null;
}

bool PdfVariant::IsReference() const
{
    return m_eDataType == EPdfDataType::Reference;
}
