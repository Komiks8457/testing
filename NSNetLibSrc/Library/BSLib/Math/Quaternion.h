#pragma once

class Matrix;

#define QX  0
#define QY  1
#define QZ  2
#define QW  3

//static int g_QNext[3] = { QY, QZ, QX };

class Quaternion
{
public:
	union
	{
		float Quat[4];

		struct
		{
			float x;
			float y;
			float z;
			float w;
		};
	};

public:
	inline Quaternion();
	inline Quaternion(const Quaternion &q);
	inline Quaternion(const float fX,float fY,float fZ,float fW);
	inline Quaternion(const Vector &vAxis,float Angle);
	inline Quaternion(const Vector &vEuler);
	inline Quaternion(const Matrix& m);

	inline void SetRotationMatrix(const Matrix& m);

	inline operator float * (void);

	inline void operator *= (const Quaternion &q);
	inline Quaternion &operator = (const Quaternion &q);
	inline void operator -= (const Quaternion &q);
	inline void operator += (const Quaternion &q);
	inline void operator /= (float fScalar);
	inline void operator *= (float fScalar);
	inline void operator *= (const Vector& v);	

	inline Quaternion operator * (const Quaternion &q);
	inline Quaternion operator - (const Quaternion &q);
	inline Quaternion operator + (const Quaternion &a);
	inline Quaternion operator / (float fScalar);
	inline Quaternion operator * (float fScalar);

	inline void Identity();
	inline void SetEuler(const Vector &v);
	inline void SetAxisAngle(Vector vAxis,float fAngle);
	
	inline float Dot(const Quaternion &q);
	
	inline float GetLengthSqr();
	inline float GetLength();

	inline Quaternion GetNormal();
	inline void Normalize();

	inline void Slerp(Quaternion &q,float fFactor);
	inline void Lerp(Quaternion &q,float fFactor);
};
