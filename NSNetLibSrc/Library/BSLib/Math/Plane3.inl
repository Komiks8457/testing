inline Plane::Plane() : a(0.0), b(0.0), c(0.0), d(0.0)
{
}

inline Plane::Plane(float fa, float fb, float fc, float fd) : a(fa), b(fb), c(fc), d(fd)
{
}

inline Plane::Plane(Vector& p1,Vector& p2,Vector& p3) : a(0.0), b(0.0), c(0.0), d(0.0)
{
	SetPlane(p1, p2, p3);
}

inline void Plane::SetPlane(Vector& p1, Vector& p2, Vector& p3)
{
	Vector v1(p3 - p1);
	Vector v2(p2 - p1);
		
	Vector n(VecCross(v2, v1));

	n.Normalize();

	a = n.x;
	b = n.y;
	c = n.z;

	d = -VecDot(n, p1);
}

inline Plane::Plane(Vector n,float dis) : a(n.x), b(n.y), c(n.z), d(dis)
{	
}

inline void Plane::Normalize()
{
	float m = (float)sqrt(a * a + b * b + c * c);
	a /= m;
	b /= m;
	c /= m;	
}

inline Vector Plane::GetNormal()
{
	return Vector(a, b, c);
}

inline float Plane::GetDistance(Vector& v)
{
	return v.x * a + v.y * b + v.z * c + d;
}

inline float Plane::GetDistFromOrg()
{
	return d;
}

inline BOOL Plane::IsIntersect(Vector& p1,Vector& p2)
{
	float d1,d2;

	d1 = GetDistance(p1);
	d2 = GetDistance(p2);

	// d1 == d2 == 0 <-- 이게 어떤 경우냐면... y = 1 인 평면에 수평인 Vector로 충돌 체크 하는 경우다...

	return ((d1 * d2) < 0.0f);
}

inline BOOL Plane::GetIntersectPoint(Vector& p1,Vector& p2, Vector& v)
{
	float d1,d2;

	d1 = GetDistance(p1);
	d2 = GetDistance(p2);

	// d1 == d2 == 0 <-- 이게 어떤 경우냐면... y = 1 인 평면에 수평인 Vector로 충돌 체크 하는 경우다...
	if ((d1 * d2) < 0.0f)
	{
		float alpha = (float)fabs(d1) / (float)(fabs(d1) + fabs(d2));

		Vector dir = p2 - p1;

		v = p1 + dir * alpha;

		return TRUE;
	}

	return FALSE;
}

inline float Plane::SolveForX(float Y, float Z)const
{
	//Ax + By + Cz + D = 0
	// Ax = -(By + Cz + D)
	// x = -(By + Cz + D)/A

	if (a)
		return (-(b*Y + c*Z + d) / a);

	return 0.0f;
}

inline float Plane::SolveForY(float X, float Z)const
{
	//Ax + By + Cz + D = 0
	// By = -(Ax + Cz + D)
	// y = -(Ax + Cz + D)/B

	if (b)
		return (-(a*X + c*Z + d) / b);
	
	return 0.0f;
}

//----------------------------------------------------------------------------------------
//	Given X and Y, Solve for Z on the plane 
//-------------------------------------------------------------------------------------://
inline float Plane::SolveForZ(float X, float Y)const
{
	//Ax + By + Cz + D = 0
	// Cz = -(Ax + By + D)
	// z = -(Ax + By + D)/C

	if (c)
		return (-(a*X + b*Y + d) / c);
	
	return 0.0f;
}

// Plane 위의 한점의 x,z를 입력받아 y를 계산한다.
inline void	 Plane::SolveForY(Vector &vPos)		
{
	if (b)
		vPos.y = (-(a*vPos.x + c*vPos.z + d) / b);
	else
		vPos.y = 0.0f;
}