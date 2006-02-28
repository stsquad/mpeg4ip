/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Adnecto d.o.o
 *  Portions created by Adnecto d.o.o. are
 *  Copyright (C) Adnecto d.o.o. 2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Danijel Kopcinovic              danijel.kopcinovic@adnecto.net
 */

#include "mp4common.h"

MP4OhdrAtom::MP4OhdrAtom() 
	: MP4Atom("ohdr")
{
}

MP4OhdrAtom::~MP4OhdrAtom() 
{
}

void MP4OhdrAtom::Read() 
{
  MP4Atom::Read();
}
