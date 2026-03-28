#pragma once

//--------- 삼각면, 유한한 면
class CFace
{
public:
	inline CFace();
	inline CFace(Vector& p1,Vector& p2,Vector& p3);

public:
	Vector	v[3];
	Vector	normal;
	Plane	p;

public:
	inline void SetFace(Vector& p1, Vector& p2, Vector& p3);
	inline bool GetIntersectPoint(Vector& p1,Vector& p2, Vector& vIntersect);			// 유한 평면 체크
	inline bool GetIntersectPointInfPlane(Vector& p1, Vector& p2, Vector& vIntersect);	// 무한 평면 체크

	inline bool	SolveForY(Vector &vPos);		// Face 위의 한점의 x,z를 입력받아 y를 계산한다.

	inline void RefreshFaceData();

	//------ 드뎌 삼각형끼리의 충돌이 필요하게 되었다.
	inline bool IsIntersect(CFace &face);
	
	void operator = (const CFace &face)
	{
		memcpy( this, &face, sizeof(CFace));
	}

protected:
	inline void CalcNormal();							// normal구하는 내부함수
};

