#pragma once

#include "DXF.h"

class Line : public BaseMesh{

	struct LineVertex {
		XMFLOAT3 position;
	};

public:
	Line(ID3D11Device* device, XMFLOAT3 from, XMFLOAT3 to);
	~Line();

	inline void setLine(XMFLOAT3 from, XMFLOAT3 to) { start = from; end = to; }

	virtual void sendData(ID3D11DeviceContext* deviceContext, D3D_PRIMITIVE_TOPOLOGY top = D3D_PRIMITIVE_TOPOLOGY_LINELIST) override;

protected:
	void initBuffers(ID3D11Device* device);

	XMFLOAT3 start, end;
};

