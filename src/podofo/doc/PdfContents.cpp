/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
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

#include "PdfContents.h"

#include "base/PdfDefinesPrivate.h"
#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfName.h"
#include "base/PdfOutputDevice.h"

#include "PdfDocument.h"
#include "PdfPage.h"

#include <iostream>

using namespace PoDoFo;

PdfContents::PdfContents(PdfPage &parent, PdfObject &obj)
    : m_parent(&parent), m_object(&obj)
{
}

PdfContents::PdfContents(PdfPage &parent) 
    : m_parent(&parent), m_object(parent.GetObject()->GetDocument()->GetObjects().CreateObject(PdfArray()))
{
    parent.GetObject()->GetDictionary().AddKey("Contents", m_object->GetIndirectReference());
}

PdfObject * PdfContents::GetContents() const
{
    return m_object;
}

PdfStream & PdfContents::GetStreamForAppending(EPdfStreamAppendFlags flags)
{
    PdfArray *arr;
    if (m_object->IsArray())
    {
        arr = &m_object->GetArray();
    }
    else if (m_object->IsDictionary())
    {
        // Create a /Contents array and put the current stream into it
        auto newObjArray = m_parent->GetObject()->GetDocument()->GetObjects().CreateObject(PdfArray());
        m_parent->GetObject()->GetDictionary().AddKey("Contents", newObjArray->GetIndirectReference());
        arr = &newObjArray->GetArray();
        arr->push_back(m_object->GetIndirectReference());
        m_object = newObjArray;
    }
    else
    {
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);
    }

    if ((flags & EPdfStreamAppendFlags::NoSaveRestorePrior) == EPdfStreamAppendFlags::None)
    {
        // Record all content and readd into a new stream that
        // substitue all the previous streams
        PdfMemoryOutputStream memstream;
        for (int i = 0; i < arr->GetSize(); i++)
        {
            const PdfStream* stream;
            if (arr->FindAt(i).TryGetStream(stream))
                stream->GetFilteredCopy(&memstream);
        }

        if (memstream.GetLength() != 0)
        {
            PdfObject* newobj = m_object->GetDocument()->GetObjects().CreateObject();
            auto &stream = newobj->GetOrCreateStream();
            stream.BeginAppend();
            stream.Append("q\n");
            stream.Append(memstream.GetBuffer(), memstream.GetLength());
            // TODO: Avoid adding unuseful \n prior Q
            stream.Append("\nQ");
            stream.EndAppend();
            arr->clear();
            arr->push_back(newobj->GetIndirectReference());
        }
    }

    // Create a new stream, add it to the array, return it
    PdfObject * newStm = m_object->GetDocument()->GetObjects().CreateObject();
    if ((flags & EPdfStreamAppendFlags::Prepend) == EPdfStreamAppendFlags::Prepend)
        arr->insert(arr->begin(), newStm->GetIndirectReference());
    else
        arr->push_back(newStm->GetIndirectReference());
    return newStm->GetOrCreateStream();
};
