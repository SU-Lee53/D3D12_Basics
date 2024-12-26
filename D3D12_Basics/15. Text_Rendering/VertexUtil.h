#pragma once

class VertexUtil
{
public:
	static DWORD CreateBoxMesh(std::vector<BasicVertex>& prefOutVertexList, WORD* refOutIndexList, DWORD dwMaxBufferCount, float fHalfBoxLen);
	void DeleteBoxMesh(std::shared_ptr<BasicVertex> pVertexList);
};

