#pragma once

class Quaternion;

class Vector
{
public:
	union
	{
		float xyz[3];
		struct { float x, y, z;};
	};

public:
	inline Vector();
	inline Vector(const Vector &v);
	inline Vector(float fX,float fY,float fZ);

	inline float &operator [] (int iAxis);
	inline bool operator < (const Vector &v) const;
	inline bool operator > (const Vector &v) const;
	inline bool operator == (const Vector &v) const;
	inline bool operator != (const Vector &v) const;

	inline operator float * ();
	inline operator const float *() const;

//	inline Vector operator = (const Vector &v);
	inline void operator -= (const Vector &v);
	inline void operator += (const Vector &v);
	inline void operator /= (float fScalar);
	inline void operator *= (float fScalar);
	inline void operator *= (Quaternion& q);

	inline Vector operator - (const Vector &v) const;
	inline Vector operator + (const Vector &v) const;
	inline Vector operator * (const Vector &v) const;
// 	inline Vector operator - (const Vector &v);
// 	inline Vector operator + (const Vector &v);
// 	inline Vector operator * (const Vector &v);
	inline Vector operator / (float fScalar) const;
	inline Vector operator * (float fScalar) const;
	inline Vector operator / (float fScalar);
	inline Vector operator * (float fScalar);

	inline void SetLength(float fLength);
	inline float GetLengthSqr() const;
	inline float GetLengthSqr();
	inline float GetLength() const;
	inline float GetLength();

	inline float GetAngle(const Vector &v);
	inline float Dot(const Vector &v) const;
	inline float Dot(const Vector &v);
	inline Vector Cross(const Vector &v) const;
	inline Vector Cross(const Vector &v);
	inline float Distance(const Vector &v) const;
	inline float Distance(const Vector &v);

	inline void	Lerp(const Vector &v, float fFactor);
	inline Vector GetNormal() const;
	inline Vector GetNormal();
	inline void Normalize();

	inline void Zero() { x = y = z = 0.0f; }

	inline Vector Min(const Vector &v);
	inline Vector Max(const Vector &v);
};

inline Vector operator - (const Vector &v);
inline Vector operator - (Vector &v);
inline Vector operator * (float scalar,Vector &v);
inline void   Vector_X_Quat(Vector& vResult, Vector& vSrc, Quaternion& Q);
inline BOOL	  NearlyEquals(const Vector& a, const Vector& b, float r);
