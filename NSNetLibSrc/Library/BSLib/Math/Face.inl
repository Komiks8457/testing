inline CFace::CFace()
{
}

inline CFace::CFace(Vector& p1, Vector& p2, Vector& p3)
{
	SetFace(p1, p2, p3);
}

inline void CFace::SetFace(Vector& p1, Vector& p2, Vector& p3)
{
	v[0] = p1;
	v[1] = p2;
	v[2] = p3;

	RefreshFaceData();
}
/*
inline void CFace::RefreshFaceData()
{
	p.SetPlane(v[0], v[1], v[2]);
	CalcNormal();
}
*/

inline void CFace::RefreshFaceData()
{
	Vector v1(v[2] - v[0]);
	Vector v2(v[1] - v[0]);

	normal = VecCross(v2, v1);
	normal.Normalize();
	
	p.a = normal.x;
	p.b = normal.y;
	p.c = normal.z;

	p.d = -VecDot(normal, v[0]);
}

inline void CFace::CalcNormal()
{
	Vector v1(v[2] - v[0]);
	Vector v2(v[1] - v[0]);

	normal = VecCross(v2, v1);
	normal.Normalize();
}

inline bool CFace::GetIntersectPointInfPlane(Vector& p1, Vector& p2, Vector& vIntersect)
{
	return (p.GetIntersectPoint(p1, p2, vIntersect) == TRUE);
}

inline bool CFace::GetIntersectPoint(Vector& p1, Vector& p2, Vector& vIntersect)
{
	if (p.GetIntersectPoint(p1, p2, vIntersect) == FALSE)
		return false;

	Vector vLine, vLineNormal, vPToV;
	
	vLine = v[1] - v[0];
	vLineNormal = VecCross(normal , vLine);
	vPToV = vIntersect - v[0];
	float dp = VecDot(vLineNormal, vPToV);
	if (dp < 0)
		return false;
		
	vLine = v[2] - v[1];
	vLineNormal = VecCross(normal, vLine);
	vPToV = vIntersect - v[1];
	dp = VecDot(vLineNormal, vPToV);
	if (dp < 0)
		return false;

	vLine = v[0] - v[2];
	vLineNormal = VecCross(normal, vLine);
	vPToV = vIntersect - v[2];
	dp = VecDot(vLineNormal, vPToV);
	if (dp < 0)
		return false;

	return true;
}

inline bool CFace::SolveForY(Vector &vPos)
{
	p.SolveForY(vPos);
	
	Vector vLine, vLineNormal, vPToV;
	
	vLine = v[1] - v[0];
	vLineNormal = VecCross(normal , vLine);
	vPToV = vPos - v[0];
	float dp = VecDot(vLineNormal, vPToV);
	if (dp < 0)
		return false;
		
	vLine = v[2] - v[1];
	vLineNormal = VecCross(normal, vLine);
	vPToV = vPos - v[1];
	dp = VecDot(vLineNormal, vPToV);
	if (dp < 0)
		return false;

	vLine = v[0] - v[2];
	vLineNormal = VecCross(normal, vLine);
	vPToV = vPos - v[2];
	dp = VecDot(vLineNormal, vPToV);
	if (dp < 0)
		return false;

	return true;
}

inline bool CFace::IsIntersect(CFace &face)
{
	Vector vTemp;
	if (GetIntersectPoint(face.v[0], face.v[1], vTemp))
		return true;

	if (GetIntersectPoint(face.v[1], face.v[2], vTemp))
		return true;

	if (GetIntersectPoint(face.v[2], face.v[0], vTemp))
		return true;
	
	return false;
}
/*
inline CFace::CFace()
{
}

inline CFace::CFace(Vector& p1, Vector& p2, Vector& p3)
{
	SetFace(p1, p2, p3);
}

inline void CFace::SetFace(Vector& p1, Vector& p2, Vector& p3)
{
	v[0] = p1;
	v[1] = p2;
	v[2] = p3;

	CalcNormal();
}

inline void CFace::CalcNormal()
{
	Vector v1,v2;

	v1 = v[1] - v[0];
	v2 = v[2] - v[1];

	v1.Normalize();
	v2.Normalize();

	normal = v1.Cross(v2);
}

inline bool CFace::IsIntersect(Vector p1,Vector p2)
{
	Plane p(v[0], v[1], v[2]);

	if (!p.Intersect(p1, p2))
		return false;

	Vector temp = p.GetIntersectPoint(p1, p2);

	Vector vLine,vLineNormal, vPToV;
	float dp;
	
	vLine = v[1] - v[0];
	vLineNormal = normal.Cross(vLine);
	vPToV = temp - v[0];

	dp = vLineNormal.Dot(vPToV);
	if (dp < 0)
		return false;
		
	vLine = v[2] - v[1];
	vLineNormal = normal.Cross(vLine);
	vPToV = temp - v[1];
	
	dp = vLineNormal.Dot(vPToV);
	if (dp < 0)
		return false;

	vLine = v[0] - v[2];
	vLineNormal = normal.Cross(vLine);
	vPToV = temp - v[2];
	dp = vLineNormal.Dot(vPToV);
	if (dp < 0)
		return false;

	return true;
}

inline Vector CFace::GetIntersectPoint(Vector p1, Vector p2)
{
	Plane p(v[0], v[1], v[2]);

	return p.GetIntersectPoint(p1, p2);
}

inline bool CFace::IsIntersect(CFace &face)
{
	if (IsIntersect(face.v[0], face.v[1]))
		return true;

	if (IsIntersect(face.v[1], face.v[2]))
		return true;

	if (IsIntersect(face.v[0], face.v[0]))
		return true;
	
	return false;
}
*/
