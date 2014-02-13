/*
 * (C) 2006-2014 see Authors.txt
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

#include "stdafx.h"
#include <moreuuids.h>
#include "../../../DSUtil/DSUtil.h"
#include "DXVADecoderH264.h"
#include "MPCVideoDec.h"
#include "DXVAAllocator.h"
#include "FfmpegContext.h"
#include <ffmpeg/libavcodec/avcodec.h>

CDXVADecoderH264::CDXVADecoderH264(CMPCVideoDecFilter* pFilter, IAMVideoAccelerator*  pAMVideoAccelerator, DXVAMode nMode, int nPicEntryNumber)
	: CDXVADecoder(pFilter, pAMVideoAccelerator, nMode, nPicEntryNumber)
{
	m_bUseLongSlice = (GetDXVA1Config()->bConfigBitstreamRaw != 2);
	Init();
}

CDXVADecoderH264::CDXVADecoderH264(CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, DXVAMode nMode, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config)
	: CDXVADecoder(pFilter, pDirectXVideoDec, nMode, nPicEntryNumber, pDXVA2Config)
{
	m_bUseLongSlice = (GetDXVA2Config()->ConfigBitstreamRaw != 2);
	Init();
}

CDXVADecoderH264::~CDXVADecoderH264()
{
	DbgLog((LOG_TRACE, 3, L"CDXVADecoderH264::Destroy()"));
}

void CDXVADecoderH264::Init()
{
	DbgLog((LOG_TRACE, 3, L"CDXVADecoderH264::Init()"));

	memset(m_DXVA_Context, 0, sizeof(m_DXVA_Context));

	Reserved16Bits		= 3;
	if (m_pFilter->GetPCIVendor() == PCIV_Intel && m_guidDecoder == DXVA_Intel_H264_ClearVideo) {
		Reserved16Bits	= 0x534c;
	} else if (IsATIUVD(m_pFilter->GetPCIVendor(), m_pFilter->GetPCIDevice())) {
		Reserved16Bits	= 0;
	}

	switch (GetMode()) {
		case H264_VLD :
			AllocExecuteParams (4);
			break;
		default :
			ASSERT(FALSE);
	}

	DXVA_Context* ctx = &((DXVA_Context*)m_DXVA_Context)[0];
	FFH264SetDxvaParams(m_pFilter->GetAVCtx(), ctx->SliceLong, m_DXVA_Context);

	Flush();
}

void CDXVADecoderH264::InitDXVAContent()
{
	memset(m_DXVA_Context, 0, sizeof(m_DXVA_Context));
	if (Reserved16Bits == 0x534c) {
		m_DXVA_Context[1].workaround = m_DXVA_Context[0].workaround = FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO;
	} else if (Reserved16Bits == 0) {
		m_DXVA_Context[1].workaround = m_DXVA_Context[0].workaround = FF_DXVA2_WORKAROUND_SCALING_LIST_ZIGZAG;	
	}
}


void CDXVADecoderH264::Flush()
{
	StatusReportFeedbackNumber = 1;

	__super::Flush();
}

void CDXVADecoderH264::CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize)
{
	DXVA_Context *ctx_pic			= &m_DXVA_Context[m_nFieldNum];
	DXVA_Slice_H264_Short *slice	= NULL;
	BYTE* current					= pDXVABuffer;

	for (unsigned i = 0; i < ctx_pic->slice_count; i++) {
		static const BYTE start_code[]		= { 0, 0, 1 };
		static const UINT start_code_size	= sizeof(start_code);

		slice = m_bUseLongSlice ? (DXVA_Slice_H264_Short*)&ctx_pic->SliceLong[i] : &ctx_pic->SliceShort[i]; 

		UINT position	= slice->BSNALunitDataLocation;
		UINT size		= slice->SliceBytesInBuffer;

		slice->BSNALunitDataLocation	= current - pDXVABuffer;
		slice->SliceBytesInBuffer		= start_code_size + size;

		if (m_bUseLongSlice) {
			DXVA_Slice_H264_Long *slice_long = (DXVA_Slice_H264_Long*)slice;
			FF264UpdateRefFrameSliceLong(&ctx_pic->DXVAPicParams, slice_long, m_pFilter->GetAVCtx()); // TODO ...
			if (i < ctx_pic->slice_count - 1) {
				slice_long->NumMbsForSlice = slice_long[1].first_mb_in_slice - slice_long[0].first_mb_in_slice;
			} else {
				//slice_long->NumMbsForSlice = mb_count - slice_long->first_mb_in_slice; // TODO ...
			}
		}

		memcpy(current, start_code, start_code_size);
		current += start_code_size;

		memcpy(current, &ctx_pic->bitstream[position], size);
		current += size;
	}

	nSize = current - pDXVABuffer;
	if (slice && nSize % 128) {
		UINT padding = 128 - (nSize % 128);
		memset(current, 0, padding);
		nSize += padding;

		slice->SliceBytesInBuffer += padding;
	}
}

HRESULT CDXVADecoderH264::DecodeFrame(BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	HRESULT						hr					= S_FALSE;
	UINT						SecondFieldOffset	= 0;
	int							got_picture			= 0;

	InitDXVAContent();
	
	CHECK_HR_FALSE (FFH264DecodeFrame(m_pFilter->GetAVCtx(), m_pFilter->GetFrame(), pDataIn, nSize, rtStart, 
					&SecondFieldOffset, &got_picture));

	if (m_nSurfaceIndex == -1 || !m_DXVA_Context[0].slice_count) {
		return S_FALSE;
	}

	for (UINT i = 0; i < _countof(m_DXVA_Context); i++) {
		if (!m_DXVA_Context[i].slice_count) {
			continue;
		}

		m_nFieldNum = i;

		m_DXVA_Context[i].DXVAPicParams.Reserved16Bits				= Reserved16Bits;
		m_DXVA_Context[i].DXVAPicParams.StatusReportFeedbackNumber	= StatusReportFeedbackNumber++;

		// Begin frame
		CHECK_HR_FALSE (BeginFrame(m_nSurfaceIndex, m_pSampleToDeliver));
		// Add picture parameters
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_PictureParametersBufferType, sizeof(DXVA_PicParams_H264), &m_DXVA_Context[i].DXVAPicParams));
		// Add quantization matrix
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_InverseQuantizationMatrixBufferType, sizeof(DXVA_Qmatrix_H264), &m_DXVA_Context[i].DXVAScalingMatrix));
		// Add bitstream
		CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_BitStreamDateBufferType));
		// Add slice control
		if (m_bUseLongSlice) {
			CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_SliceControlBufferType, sizeof(DXVA_Slice_H264_Long) * m_DXVA_Context[i].slice_count, m_DXVA_Context[i].SliceLong));
		} else {
			CHECK_HR_FRAME (AddExecuteBuffer(DXVA2_SliceControlBufferType, sizeof(DXVA_Slice_H264_Short) * m_DXVA_Context[i].slice_count, m_DXVA_Context[i].SliceShort));
		}
		// Decode frame
		CHECK_HR_FRAME (Execute());
		CHECK_HR_FALSE (EndFrame(m_nSurfaceIndex));
	}

	if (got_picture) {
		AddToStore(m_nSurfaceIndex, m_pSampleToDeliver, rtStart, rtStop);
		hr = DisplayNextFrame();
	}

	return hr;
}
