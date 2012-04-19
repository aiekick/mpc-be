/*
 * $Id: SSF.h 4330 2012-04-10 15:45:58Z XhmikosR $
 *
 * (C) 2003-2006 Gabest
 * (C) 2006-2012 see Authors.txt
 *
 * This file is part of MPC-HC.
 *
 * MPC-HC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-HC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "../SubPic/SubPicProviderImpl.h"
#include "./libssf/SubtitleFile.h"
#include "./libssf/Renderer.h"

#pragma once

namespace ssf
{
	class __declspec(uuid("E0593632-0AB7-47CA-8BE1-E9D2A6A4825E"))
		CRenderer : public CSubPicProviderImpl, public ISubStream
	{
		CString m_fn, m_name;
		CAutoPtr<SubtitleFile> m_file;
		CAutoPtr<Renderer> m_renderer;

	public:
		CRenderer(CCritSec* pLock);
		virtual ~CRenderer();

		bool Open(CString fn, CString name = _T(""));
		bool Open(InputStream& s, CString name);

		void Append(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, LPCWSTR str);

		DECLARE_IUNKNOWN
		STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

		// ISubPicProvider
		STDMETHODIMP_(POSITION) GetStartPosition(REFERENCE_TIME rt, double fps);
		STDMETHODIMP_(POSITION) GetNext(POSITION pos);
		STDMETHODIMP_(REFERENCE_TIME) GetStart(POSITION pos, double fps);
		STDMETHODIMP_(REFERENCE_TIME) GetStop(POSITION pos, double fps);
		STDMETHODIMP_(bool) IsAnimated(POSITION pos);
		STDMETHODIMP Render(SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox);

		// IPersist
		STDMETHODIMP GetClassID(CLSID* pClassID);

		// ISubStream
		STDMETHODIMP_(int) GetStreamCount();
		STDMETHODIMP GetStreamInfo(int i, WCHAR** ppName, LCID* pLCID);
		STDMETHODIMP_(int) GetStream();
		STDMETHODIMP SetStream(int iStream);
		STDMETHODIMP Reload();
	};

}