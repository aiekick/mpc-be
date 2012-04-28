/*
 * $Id$
 *
 * (C) 2003-2006 Gabest
 * (C) 2006-2012 see Authors.txt
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <atlimage.h>
#include "mplayerc.h"


class CPngImage : public CImage
{
public:
	bool LoadFromResource(UINT id);

	CString LoadCurrentPath();
	int FileExists(CString fn);
	HBITMAP LoadExternalImage(CString fn);
	void LoadExternalGradient(CString fn, CDC* dc, CRect r, int ptop, int br, int rc, int gc, int bc);
};
