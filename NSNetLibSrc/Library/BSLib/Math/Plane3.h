#pragma once

//--------------- 무한 평면
class Plane
{
public:
	inline Plane();
	inline Plane(float fa, float fb, float fc, float fd);
	inline Plane(Vector& p1, Vector& p2, Vector& p3);
	inline Plane(Vector n,float dis);
	
public:
	float a,b,c,d;

	inline operator float* () { return (float*) &a; }
    inline operator const float* () const { return (const float*) &a; }

	// unary operators
    Plane operator + () const { return *this; }
    Plane operator - () const { return Plane(-a, -b, -c, -d); }

	inline void SetPlane(Vector& p1, Vector& p2, Vector& p3);

	BOOL operator == (const Plane& p) const
	{
		return (a == p.a && b == p.b && c == p.c && d == p.d);
	}

	BOOL operator != (const Plane& p) const
	{
		return (a != p.a || b != p.b || c != p.c || d != p.d);
	}
	
	inline float PlaneDotPlane(const Plane* plane)
	{
		return (a*plane->a + b*plane->b + c*plane->c + d*plane->d);
	}

	inline float PlaneDotVec(const Vector* pV)
	{
		return (a*pV->x + b*pV->y + c*pV->z + d);
	}

	inline float PlaneDotNormal(const Vector* pV)
	{
		return (a*pV->x + b*pV->y + c*pV->z);
	}

	inline void	  Normalize();
	inline Vector GetNormal();						// Plane 방향얻기
	inline float  GetDistance(Vector& v);			// 점에서 Plane까지의 수선의 길이를 구함
	inline float  GetDistFromOrg();
	inline float  SolveForX(float Y, float Z) const;
	inline float  SolveForY(float X, float Z) const;
	inline float  SolveForZ(float X, float Y) const;
	
	inline void	  SolveForY(Vector &vPos);		// Plane 위의 한점의 x,z를 입력받아 y를 계산한다.

	inline BOOL   IsIntersect(Vector& p1,Vector& p2);					// 두점이 Plane을 통과하는가
	inline BOOL	  GetIntersectPoint(Vector& p1,Vector& p2, Vector& v);	// 두선분과 Plane이 만나는 지점 구하기
};
