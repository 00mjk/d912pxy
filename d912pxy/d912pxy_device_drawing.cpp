/*
MIT License

Copyright(c) 2018-2019 megai2

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/
#include "stdafx.h"

#define API_OVERHEAD_TRACK_LOCAL_ID_DEFINE PXY_METRICS_API_OVERHEAD_DEVICE_DRAWING

//scene terminators

HRESULT WINAPI d912pxy_device::BeginScene(void)
{
	LOG_DBG_DTDM(__FUNCTION__);
	return D3D_OK;
}

HRESULT WINAPI d912pxy_device::EndScene(void)
{
	LOG_DBG_DTDM(__FUNCTION__);

	d912pxy_s(iframe)->EndSceneReset();

	return D3D_OK;
}

//drawers

HRESULT WINAPI d912pxy_device::DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount)
{ 
	LOG_DBG_DTDM(__FUNCTION__);

	if (1)
	{
		LOG_DBG_DTDM("DP NON INDEXED SKIPPING");
		return D3D_OK;
	}
}

HRESULT WINAPI d912pxy_device::DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
{ 
	LOG_DBG_DTDM("DrawIndexed PT %u BV %u MV %u NV %u SI %u PC %u", PrimitiveType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);

	API_OVERHEAD_TRACK_START(0)

#ifdef _DEBUG
	if (PrimitiveType == D3DPT_TRIANGLEFAN)
	{
		LOG_DBG_DTDM("DP TRIFAN skipping");
		return D3D_OK;
	}
#endif

	d912pxy_s(iframe)->CommitBatch(PrimitiveType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);

#ifdef PER_BATCH_FLUSH_DEBUG
	replayer->Finish();

	iframe->End();
	mGPUque->Flush(0);

	iframe->Start();
#endif

	API_OVERHEAD_TRACK_END(0)
		
	return D3D_OK;
}

HRESULT __stdcall d912pxy_device::DrawIndexedPrimitive_PS(IDirect3DDevice9 * self, D3DPRIMITIVETYPE PrimitiveType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
{
	d912pxy_device* _self = (d912pxy_device*)self;

	API_OVERHEAD_TRACK_START(0)

#ifdef _DEBUG
	if (PrimitiveType == D3DPT_TRIANGLEFAN)
	{
		LOG_DBG_DTDM("DP TRIFAN skipping");
		return D3D_OK;
	}
#endif

	d912pxy_s(iframe)->CommitBatch(PrimitiveType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);

#ifdef PER_BATCH_FLUSH_DEBUG
	replayer->Finish();

	iframe->End();
	mGPUque->Flush(0);

	iframe->Start();
#endif

	for (int i = 0; i != 32; ++i)
		if (_self->stageFormatsTrack[i] == D3DFMT_D24X8)
		{
			if (d912pxy_s(psoCache)->GetPShader())
				_self->TrackShaderCodeBugs(PXY_INNER_SHDR_BUG_PCF_SAMPLER, i + 1, d912pxy_s(psoCache)->GetPShader()->GetID());
		}

	UINT srgbState = d912pxy_s(textureState)->GetTexStage(30);
	if (srgbState)
		for (int i = 0; i != 32; ++i)
		{
			if (srgbState & 1)
			{
				if (d912pxy_s(textureState)->GetTexStage(i) != _self->mNullTextureSRV)
				{
					if (d912pxy_s(psoCache)->GetPShader())
						_self->TrackShaderCodeBugs(PXY_INNER_SHDR_BUG_SRGB_READ, 1, d912pxy_s(psoCache)->GetPShader()->GetID());
					break;
				}
			}
			srgbState = srgbState >> 1;
		}

	if (d912pxy_s(psoCache)->GetDX9RsValue(D3DRS_SRGBWRITEENABLE))
		if (d912pxy_s(psoCache)->GetPShader())
			_self->TrackShaderCodeBugs(PXY_INNER_SHDR_BUG_SRGB_WRITE, 1, d912pxy_s(psoCache)->GetPShader()->GetID());

	if (d912pxy_s(psoCache)->GetDX9RsValue(D3DRS_ALPHATESTENABLE))
		if (d912pxy_s(psoCache)->GetPShader())
			_self->TrackShaderCodeBugs(PXY_INNER_SHDR_BUG_ALPHA_TEST, 1, d912pxy_s(psoCache)->GetPShader()->GetID());

	if (d912pxy_s(psoCache)->GetDX9RsValue(D3DRS_CLIPPLANEENABLE))
	{
		UINT32 cp = d912pxy_s(psoCache)->GetDX9RsValue(D3DRS_CLIPPLANEENABLE);
		if (cp & 1)
		{
			if (d912pxy_s(psoCache)->GetVShader())
				_self->TrackShaderCodeBugs(PXY_INNER_SHDR_BUG_CLIPPLANE0, 1, d912pxy_s(psoCache)->GetVShader()->GetID());
		}
	}

	API_OVERHEAD_TRACK_END(0)

	return D3D_OK;
}

//megai2: you should know, that there is no apps, that can't storage their data in vertex buffers 
HRESULT WINAPI d912pxy_device::DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
	API_OVERHEAD_TRACK_START(0)

	LOG_DBG_DTDM2("DPUP %u %u %016llX %u", PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);

	void* dstPtr;
	mDrawUPVbuf->Lock(mDrawUPStreamPtr, 0, &dstPtr, 0);
	memcpy(dstPtr, pVertexStreamZeroData, VertexStreamZeroStride * 3 * PrimitiveCount);
	mDrawUPVbuf->Unlock();

	d912pxy_vstream* oi = d912pxy_s(iframe)->GetIBuf();
	d912pxy_device_streamsrc oss = d912pxy_s(iframe)->GetStreamSource(0);
	d912pxy_device_streamsrc ossi = d912pxy_s(iframe)->GetStreamSource(1);
	
	d912pxy_s(iframe)->SetIBuf(d912pxy_vstream_from_index(mDrawUPIbuf));
	d912pxy_s(iframe)->SetVBuf((d912pxy_vstream*)mDrawUPVbuf, 0, mDrawUPStreamPtr, VertexStreamZeroStride);
	d912pxy_s(iframe)->SetStreamFreq(0, 1);
	d912pxy_s(iframe)->SetStreamFreq(1, 0);

	mDrawUPStreamPtr += PrimitiveCount * 3 * VertexStreamZeroStride;

	DrawIndexedPrimitive(PrimitiveType, 0, 0, 0, 0, PrimitiveCount);

	d912pxy_s(iframe)->SetIBuf(oi);
	d912pxy_s(iframe)->SetVBuf(oss.buffer, 0, oss.offset, oss.stride);
	d912pxy_s(iframe)->SetStreamFreq(0, oss.divider);
	d912pxy_s(iframe)->SetStreamFreq(1, ossi.divider);

	API_OVERHEAD_TRACK_END(0)

	return D3D_OK;
}

#undef API_OVERHEAD_TRACK_LOCAL_ID_DEFINE 