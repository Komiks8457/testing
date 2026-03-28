#pragma once

class CLine
{
public:
	CLine(Vector start, Vector end) { SetLine(start, end); }
	CLine() {}
	
public:
	Vector	m_vLine[2];
	Vector	m_vNearestPosOnSegment;
	Vector	m_vNearestPoint;
	Vector	m_vNearestNormal;

public:
	inline void	SetLine(Vector v1, Vector v2) { m_vLine[0] = v1; m_vLine[1] = v2; }
	inline BOOL	IntersectLineSegments(CLine& TestLine, BOOL bInfiniteLines, float epsilon = _EPSILON);
	inline static void	FindNearestPointOnLineSegment(Vector vA, Vector vLineDir, Vector vPoint, BOOL bInfiniteLine, float epsilon_squared, Vector& vNearestPos, float &parameter);
	inline void	FindNearestPointOfParallelLineSegments(CLine& TestLine, BOOL bInfiniteLines, float epsilon_squared);
	inline void	AdjustNearestPoints(CLine& TestLine, float epsilon_squared, float s, float t);
};

class BBoxAABB3;

class BSphere
{
public:
	Vector m_vOrigin;
	float  m_fRadius;

public:
	inline BSphere();
	inline BSphere(const Vector &vOrigin,float fRadius);

	inline bool Intersect(const BBoxAABB3 &BBox) const;
	inline bool Intersect(const BSphere& sphere) const;

//	inline void operator += (const BSphere &sphere);
};

class BBoxAABB3
{
public:
	Vector m_vMin;
	Vector m_vMax;

public:
	inline BBoxAABB3();
	inline BBoxAABB3(Vector vMin,Vector vMax);

	inline Vector Center();
	inline Vector Size();
	inline void Reset() { m_vMin = Vector(_FLOAT_HUGE, _FLOAT_HUGE, _FLOAT_HUGE); m_vMax = Vector(-_FLOAT_HUGE, -_FLOAT_HUGE, -_FLOAT_HUGE); }
	inline void	SetMinMax(Vector& vMin, Vector& vMax) { m_vMin = vMin; m_vMax = vMax; }

	inline bool Intersect(const BBoxAABB3 &bbox) const;
	inline bool Intersect(const BSphere &sphere) const;

	inline BBoxAABB3 operator + (const BBoxAABB3 &bbox);
	inline void operator += (const BBoxAABB3 &bbox);
	inline void AddPoint(const Vector &vPoint);
	
	inline BOOL	IsIntersect(Vector& p1,Vector& p2);
};
