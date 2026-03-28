//////////////////////////////////////////////////////////
// CLine Inline functions
// 
// Copyright (C) Graham Rhodes, 2001. 
// All rights reserved worldwide. 
// 
// revised by overdrv 2002.
//////////////////////////////////////////////////////////

inline BOOL CLine::IntersectLineSegments(CLine& TestLine, BOOL bInfiniteLines, float epsilon)
{
	float temp = 0.f;
	float epsilon_squared = epsilon * epsilon;

	// Compute parameters from Equations (1) and (2) in the text
	Vector vLineDirA = m_vLine[1] - m_vLine[0];
	Vector vLineDirB = TestLine.m_vLine[1] - TestLine.m_vLine[0];
	
	// From Equation (15)
	float L11 =  (vLineDirA.x * vLineDirA.x) + (vLineDirA.y * vLineDirA.y) + (vLineDirA.z * vLineDirA.z);
	float L22 =  (vLineDirB.x * vLineDirB.x) + (vLineDirB.y * vLineDirB.y) + (vLineDirB.z * vLineDirB.z);

	// Line/Segment A is degenerate ---- Special Case #1
	if (L11 < epsilon_squared)
	{
		m_vNearestPosOnSegment = m_vLine[0];
		FindNearestPointOnLineSegment(TestLine.m_vLine[0], vLineDirB, m_vLine[0], bInfiniteLines, epsilon, TestLine.m_vNearestPosOnSegment, temp);
	}
	// Line/Segment B is degenerate ---- Special Case #1
	else if (L22 < epsilon_squared)
	{
		TestLine.m_vNearestPosOnSegment = TestLine.m_vLine[0];
		FindNearestPointOnLineSegment(m_vLine[0], vLineDirA, TestLine.m_vLine[0], bInfiniteLines, epsilon, m_vNearestPosOnSegment, temp);
	}
	// Neither line/segment is degenerate
	else
	{
		// Compute more parameters from Equation (3) in the text.
		float ABx = TestLine.m_vLine[0].x - m_vLine[0].x;
		float ABy = TestLine.m_vLine[0].y - m_vLine[0].y;
		float ABz = TestLine.m_vLine[0].z - m_vLine[0].z;

		// and from Equation (15).
		float L12 = -(vLineDirA.x * vLineDirB.x) - (vLineDirA.y * vLineDirB.y) - (vLineDirA.z * vLineDirB.z);

		float DetL = L11 * L22 - L12 * L12;
		// Lines/Segments A and B are parallel ---- special case #2.
		if (FABS(DetL) < epsilon)
			FindNearestPointOfParallelLineSegments(TestLine, bInfiniteLines, epsilon);
		// The general case
		else
		{
			// from Equation (15)
			float ra = vLineDirA.x * ABx + vLineDirA.y * ABy + vLineDirA.z * ABz;
			float rb = -vLineDirB.x * ABx - vLineDirB.y * ABy - vLineDirB.z * ABz;
			float t  = (L11 * rb - ra * L12)/DetL; // Equation (12)

#ifdef USE_CRAMERS_RULE
			float s = (L22 * ra - rb * L12)/DetL;
#else
			float s = (ra - L12 * t) / L11;             // Equation (13)
#endif // USE_CRAMERS_RULE

#ifdef CHECK_ANSWERS
			float check_ra = s * L11 + t * L12;
			float check_rb = s * L12 + t * L22;
			assert(FABS(check_ra - ra) < epsilon);
			assert(FABS(check_rb - rb) < epsilon);
#endif // CHECK_ANSWERS

			// if we are dealing with infinite lines or if parameters s and t both
			// lie in the range [0,1] then just compute the points using Equations
			// (1) and (2) from the text.
			m_vNearestPosOnSegment.x = (m_vLine[0].x + s * vLineDirA.x);
			m_vNearestPosOnSegment.y = (m_vLine[0].y + s * vLineDirA.y);
			m_vNearestPosOnSegment.z = (m_vLine[0].z + s * vLineDirA.z);

			TestLine.m_vNearestPosOnSegment.x = (TestLine.m_vLine[0].x + t * vLineDirB.x);
			TestLine.m_vNearestPosOnSegment.y = (TestLine.m_vLine[0].y + t * vLineDirB.y);
			TestLine.m_vNearestPosOnSegment.z = (TestLine.m_vLine[0].z + t * vLineDirB.z);

			// otherwise, at least one of s and t is outside of [0,1] and we have to
			// handle this case.
			if (FALSE == bInfiniteLines && (OUT_OF_RANGE(s) || OUT_OF_RANGE(t)))
				AdjustNearestPoints(TestLine, epsilon, s, t);
		}
	}

	m_vNearestPoint.x = 0.5f * (m_vNearestPosOnSegment.x + TestLine.m_vNearestPosOnSegment.x);
	m_vNearestPoint.y = 0.5f * (m_vNearestPosOnSegment.y + TestLine.m_vNearestPosOnSegment.y);
	m_vNearestPoint.z = 0.5f * (m_vNearestPosOnSegment.z + TestLine.m_vNearestPosOnSegment.z);

	m_vNearestNormal.x = TestLine.m_vNearestPosOnSegment.x - m_vNearestPosOnSegment.x;
	m_vNearestNormal.y = TestLine.m_vNearestPosOnSegment.y - m_vNearestPosOnSegment.y;
	m_vNearestNormal.z = TestLine.m_vNearestPosOnSegment.z - m_vNearestPosOnSegment.z;

	// optional check to indicate if the lines truly intersect
	BOOL bIntersected = (FABS(m_vNearestNormal.x) + FABS(m_vNearestNormal.y) + FABS(m_vNearestNormal.z)) < epsilon ? TRUE : FALSE;
	return bIntersected;
}

inline void CLine::FindNearestPointOnLineSegment(Vector vA, Vector vLineDir, Vector vPoint, BOOL bInfiniteLine, float epsilon_squared, Vector& vNearestPos, float& parameter)
{
	// Line/Segment is degenerate --- special case #1
	float D = vLineDir.x * vLineDir.x + vLineDir.y * vLineDir.y + vLineDir.z * vLineDir.z;
	if (D < epsilon_squared)
	{
		vNearestPos = vA;
		return;
	}

	vPoint = vPoint - vA;
	
	// parameter is computed from Equation (20).
	parameter = (vLineDir.x * vPoint.x + vLineDir.y * vPoint.y + vLineDir.z * vPoint.z) / D;

	if (FALSE == bInfiniteLine) 
		parameter = MAX(0.0f, MIN(1.0f, parameter));

	vNearestPos.x = vA.x + parameter * vLineDir.x;
	vNearestPos.y = vA.y + parameter * vLineDir.y;
	vNearestPos.z = vA.z + parameter * vLineDir.z;
}

inline void CLine::FindNearestPointOfParallelLineSegments(CLine& TestLine, BOOL  bInfiniteLines, float epsilon_squared)
{
	float s[2], temp;

	Vector vLineDirA = m_vLine[1] - m_vLine[0];
	Vector vLineDirB = TestLine.m_vLine[1] - TestLine.m_vLine[0];
	
	FindNearestPointOnLineSegment(TestLine.m_vLine[0], vLineDirA, TestLine.m_vLine[0], TRUE, epsilon_squared, m_vNearestPosOnSegment, s[0]);
	if (bInfiniteLines == TRUE)
		TestLine.m_vNearestPosOnSegment = TestLine.m_vLine[0];
	else
	{
		Vector vDummy;
		FindNearestPointOnLineSegment(TestLine.m_vLine[1], vLineDirA, TestLine.m_vLine[1], TRUE, epsilon_squared, vDummy, s[1]);
		if (s[0] < 0.f && s[1] < 0.f)
		{
			m_vNearestPosOnSegment = m_vLine[0];

			if (s[0] < s[1])
				TestLine.m_vNearestPosOnSegment = TestLine.m_vLine[1];
			else
				TestLine.m_vNearestPosOnSegment = TestLine.m_vLine[0];
		}
		else if (s[0] > 1.f && s[1] > 1.f)
		{
			m_vNearestPosOnSegment = m_vLine[1];

			if (s[0] < s[1])
				TestLine.m_vNearestPosOnSegment = TestLine.m_vLine[0];
			else
				TestLine.m_vNearestPosOnSegment = TestLine.m_vLine[1];
		}
		else
		{
			temp = 0.5f * (MAX(0.0f, MIN(1.0f, s[0])) + MAX(0.0f, MIN(1.0f, s[1])));
			m_vNearestPosOnSegment.x = (m_vLine[0].x + temp * vLineDirA.x);
			m_vNearestPosOnSegment.y = (m_vLine[0].y + temp * vLineDirA.y);
			m_vNearestPosOnSegment.z = (m_vLine[0].z + temp * vLineDirA.z);

			FindNearestPointOnLineSegment(TestLine.m_vLine[0], vLineDirB, m_vNearestPosOnSegment, TRUE, epsilon_squared, TestLine.m_vNearestPosOnSegment, temp);
		}
	}
}

inline void CLine::AdjustNearestPoints(CLine& TestLine, float epsilon_squared, float s, float t)
{
	Vector vLineDirA = m_vLine[1] - m_vLine[0];
	Vector vLineDirB = TestLine.m_vLine[1] - TestLine.m_vLine[0];

	// handle the case where both parameter s and t are out of range
	if (OUT_OF_RANGE(s) && OUT_OF_RANGE(t))
	{
		s = MAX(0.0f, MIN(1.0f, s));
		m_vNearestPosOnSegment.x = (m_vLine[0].x + s * vLineDirA.x);
		m_vNearestPosOnSegment.y = (m_vLine[0].y + s * vLineDirA.y);
		m_vNearestPosOnSegment.z = (m_vLine[0].z + s * vLineDirA.z);

		FindNearestPointOnLineSegment(TestLine.m_vLine[0], vLineDirB, m_vNearestPosOnSegment, TRUE, epsilon_squared, TestLine.m_vNearestPosOnSegment, t);
		
		if (OUT_OF_RANGE(t))
		{
			t = MAX(0.0f, MIN(1.0f, t));
			TestLine.m_vNearestPosOnSegment.x = (TestLine.m_vLine[0].x + t * vLineDirB.x);
			TestLine.m_vNearestPosOnSegment.y = (TestLine.m_vLine[0].y + t * vLineDirB.y);
			TestLine.m_vNearestPosOnSegment.z = (TestLine.m_vLine[0].z + t * vLineDirB.z);
			
			FindNearestPointOnLineSegment(m_vLine[0], vLineDirA, TestLine.m_vNearestPosOnSegment, FALSE, epsilon_squared, m_vNearestPosOnSegment, s);
			FindNearestPointOnLineSegment(TestLine.m_vLine[0], vLineDirB, m_vNearestPosOnSegment, FALSE, epsilon_squared, TestLine.m_vNearestPosOnSegment, t);
		}
	}
	// otherwise, handle the case where the parameter for only one segment is
	// out of range
	else if (OUT_OF_RANGE(s))
	{
		s = MAX(0.0f, MIN(1.0f, s));
		m_vNearestPosOnSegment.x = (m_vLine[0].x + s * vLineDirA.x);
		m_vNearestPosOnSegment.y = (m_vLine[0].y + s * vLineDirA.y);
		m_vNearestPosOnSegment.z = (m_vLine[0].z + s * vLineDirA.z);
		
		FindNearestPointOnLineSegment(TestLine.m_vLine[0], vLineDirB, m_vNearestPosOnSegment, FALSE, epsilon_squared, TestLine.m_vNearestPosOnSegment, t);
	}
	else if (OUT_OF_RANGE(t))
	{
		t = MAX(0.0f, MIN(1.0f, t));
		TestLine.m_vNearestPosOnSegment.x = (TestLine.m_vLine[0].x + t * vLineDirB.x);
		TestLine.m_vNearestPosOnSegment.y = (TestLine.m_vLine[0].y + t * vLineDirB.y);
		TestLine.m_vNearestPosOnSegment.z = (TestLine.m_vLine[0].z + t * vLineDirB.z);
		
		FindNearestPointOnLineSegment(m_vLine[0], vLineDirA, TestLine.m_vNearestPosOnSegment, FALSE, epsilon_squared, m_vNearestPosOnSegment, s);
	}
	else
	{
		_ASSERT(FALSE);
	}
}

//////////////////////////////////////////////////////////
// BSphere inline함수
//////////////////////////////////////////////////////////
inline BSphere::BSphere() : m_vOrigin(0.0f, 0.0f, 0.0f), m_fRadius(0.0f)
{
}

inline BSphere::BSphere(const Vector &vOrigin,float fRadius) : m_vOrigin(vOrigin), m_fRadius(fRadius)
{
}

inline bool BSphere::Intersect(const BBoxAABB3 &BBox) const
{
	return BBox.Intersect(*this);
}

inline bool BSphere::Intersect(const BSphere &sphere) const
{
	Vector vDistance(m_vOrigin - sphere.m_vOrigin);
	float fDistance = vDistance.GetLengthSqr();

	float fRadius = (m_fRadius + sphere.m_fRadius);
	
	if (fDistance <= (fRadius * fRadius)) 
		return true;

	return false;
}

//////////////////////////////////////////////////////////
// BBoxAABB Inline함수
//////////////////////////////////////////////////////////
inline BBoxAABB3::BBoxAABB3()
	:m_vMin(_FLOAT_HUGE, _FLOAT_HUGE, _FLOAT_HUGE), m_vMax(-_FLOAT_HUGE, -_FLOAT_HUGE, -_FLOAT_HUGE)
{
}

inline BBoxAABB3::BBoxAABB3(Vector vMin,Vector vMax)
	: m_vMin(vMin), m_vMax(vMax)
{
}

inline Vector BBoxAABB3::Center()
{
	return (m_vMax + m_vMin) / 2;
}
inline Vector BBoxAABB3::Size()
{
	return (m_vMax - m_vMin);
}

inline BBoxAABB3 BBoxAABB3::operator + (const BBoxAABB3 &bbox)
{
	BBoxAABB3 Result;
	if (m_vMin.x > bbox.m_vMin.x)
		Result.m_vMin.x = bbox.m_vMin.x;
	if (m_vMin.y > bbox.m_vMin.y)
		Result.m_vMin.y = bbox.m_vMin.y;
	if (m_vMin.z > bbox.m_vMin.z)
		Result.m_vMin.z = bbox.m_vMin.z;

	if (m_vMax.x < bbox.m_vMax.x)
		Result.m_vMax.x = bbox.m_vMax.x;
	if (m_vMax.y < bbox.m_vMax.y)
		Result.m_vMax.y = bbox.m_vMax.y;
	if (m_vMax.z < bbox.m_vMax.z)
		Result.m_vMax.z = bbox.m_vMax.z;

	return Result;
}

inline void BBoxAABB3::operator += (const BBoxAABB3 &bbox)
{
	if (m_vMin.x > bbox.m_vMin.x)
		m_vMin.x = bbox.m_vMin.x;
	if (m_vMin.y > bbox.m_vMin.y)
		m_vMin.y = bbox.m_vMin.y;
	if (m_vMin.z > bbox.m_vMin.z)
		m_vMin.z = bbox.m_vMin.z;

	if (m_vMax.x < bbox.m_vMax.x)
		m_vMax.x = bbox.m_vMax.x;
	if (m_vMax.y < bbox.m_vMax.y)
		m_vMax.y = bbox.m_vMax.y;
	if (m_vMax.z < bbox.m_vMax.z)
		m_vMax.z = bbox.m_vMax.z;
}

inline void BBoxAABB3::AddPoint(const Vector &vPoint)
{
	if (m_vMin.x > vPoint.x) 
		m_vMin.x = vPoint.x;
	if (m_vMin.y > vPoint.y) 
		m_vMin.y = vPoint.y;
	if (m_vMin.z > vPoint.z) 
		m_vMin.z = vPoint.z;
	
	if (m_vMax.x < vPoint.x) 
		m_vMax.x = vPoint.x;
	if (m_vMax.y < vPoint.y) 
		m_vMax.y = vPoint.y;
	if (m_vMax.z < vPoint.z) 
		m_vMax.z = vPoint.z;
}

inline bool BBoxAABB3::Intersect(const BBoxAABB3 &bbox) const
{
	/*
	if (m_vMin > bbox.m_vMax) 
		return false;
	if (m_vMax < bbox.m_vMin) 
		return false;
*/
	if (m_vMin.x > bbox.m_vMax.x || m_vMin.y > bbox.m_vMax.y || m_vMin.z > bbox.m_vMax.z)
		return false;
	
	if (m_vMax.x < bbox.m_vMin.x || m_vMax.y < bbox.m_vMin.y || m_vMax.z < bbox.m_vMin.z)
		return false;

	return true;
}

inline bool BBoxAABB3::Intersect(const BSphere &sphere) const
{
	float fDistance = 0.0f;
	for (int iAxis = 0; iAxis < 3; iAxis ++)
	{
		if (sphere.m_vOrigin[iAxis] < m_vMin[iAxis])
		{
			float fValue = (sphere.m_vOrigin[iAxis] - m_vMin[iAxis]);
			fDistance += (fValue * fValue);
		}
		else if (sphere.m_vOrigin[iAxis] < m_vMax[iAxis])
		{
			float fValue = (sphere.m_vOrigin[iAxis] - m_vMax[iAxis]);
			fDistance += (fValue * fValue);
		}
	}

	if (fDistance <= (sphere.m_fRadius * sphere.m_fRadius)) 
		return true;
	
	return false;
}
inline BOOL	BBoxAABB3::IsIntersect(Vector& p1,Vector& p2)
{
	if(m_vMin.x > p1.x && m_vMin.x > p2.x)
		return FALSE;
	if(m_vMin.y > p1.y && m_vMin.y > p2.y)
		return FALSE;
	if(m_vMin.z > p1.z && m_vMin.z > p2.z)
		return FALSE;

	if(m_vMax.x < p1.x && m_vMax.x < p2.x)
		return FALSE;
	if(m_vMax.y < p1.y && m_vMax.y < p2.y)
		return FALSE;
	if(m_vMax.z < p1.z && m_vMax.z < p2.z)
		return FALSE;
	
	// 두점이 바운드 박스안에 있을때 처리 되게 할라믄.. 처리를 한점이라도 박스 안에 있으면 TRUE를 리턴하자.
	if( (m_vMin.x <= p1.x && p1.x <= m_vMax.x) &&
		(m_vMin.y <= p1.y && p1.y <= m_vMax.y) && 
		(m_vMin.z <= p1.z && p1.z <= m_vMax.z) )
		return TRUE;
	if( (m_vMin.x <= p2.x && p2.x <= m_vMax.x) &&
		(m_vMin.y <= p2.y && p2.y <= m_vMax.y) && 
		(m_vMin.z <= p2.z && p2.z <= m_vMax.z) )
		return TRUE;


	// 여기 부턴 바운딩 박스 출돌이닷.
	Vector	v1(m_vMin.x,m_vMax.y,m_vMax.z);
	Vector& v2 = m_vMax;
	Vector	v3(m_vMax.x,m_vMax.y,m_vMin.z);
	Vector	v4(m_vMin.x,m_vMax.y,m_vMin.z);

	Vector	v5(m_vMin.x,m_vMin.y,m_vMax.z);
	Vector	v6(m_vMax.x,m_vMin.y,m_vMax.z);
	Vector	v7(m_vMax.x,m_vMin.y,m_vMin.z);
	Vector&	v8 = m_vMin;
	
	Vector vIntersect;
	// Top
	CFace face;
	face.SetFace(v1,v2,v3);
	if(face.GetIntersectPoint(p1, p2, vIntersect))
		return TRUE;
	face.SetFace(v1,v3,v4);
	if(face.GetIntersectPoint(p1, p2, vIntersect))
		return TRUE;
	// Right
	face.SetFace(v3,v2,v6);
	if(face.GetIntersectPoint(p1, p2, vIntersect))
		return TRUE;
	face.SetFace(v3,v6,v7);
	if(face.GetIntersectPoint(p1, p2, vIntersect))
		return TRUE;
	// Front
	face.SetFace(v4,v3,v7);
	if(face.GetIntersectPoint(p1, p2, vIntersect))
		return TRUE;
	face.SetFace(v4,v7,v8);
	if(face.GetIntersectPoint(p1, p2, vIntersect))
		return TRUE;
	// Left
	face.SetFace(v1,v4,v8);
	if(face.GetIntersectPoint(p1, p2, vIntersect))
		return TRUE;
	face.SetFace(v1,v8,v5);
	if(face.GetIntersectPoint(p1, p2, vIntersect))
		return TRUE;
	// Back
	face.SetFace(v2,v1,v5);
	if(face.GetIntersectPoint(p1, p2, vIntersect))
		return TRUE;
	face.SetFace(v2,v5,v6);
	if(face.GetIntersectPoint(p1, p2, vIntersect))
		return TRUE;
	// Bottom
	face.SetFace(v8,v7,v6);
	if(face.GetIntersectPoint(p1, p2, vIntersect))
		return TRUE;
	face.SetFace(v8,v6,v5);
	if(face.GetIntersectPoint(p1, p2, vIntersect))
		return TRUE;
	return FALSE;
}
