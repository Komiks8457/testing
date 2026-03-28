#pragma once

class Matrix
{
public:
	union
	{
		float d[ 16];
		float m[ 4][ 4];

		struct
		{
			float _11, _12, _13, _14;
			float _21, _22, _23, _24;
			float _31, _32, _33, _34;
			float _41, _42, _43, _44;
		};
	
		struct
		{
			Vector4 rx;
			Vector4 ry;
			Vector4 rz;

			union
			{
				struct
				{
					Vector t;
					float w;
				};

				struct Pos
				{
					float x,y,z,w;
				};
			};
		};
	};

public:
	inline Matrix(Quaternion& q);
	inline Matrix(BOOL id = TRUE);
	inline Matrix(float *pfData);
	inline Matrix(float f11,float f12,float f13,float f14,
					 float f21,float f22,float f23,float f24,
					 float f31,float f32,float f33,float f34,
					 float f41,float f42,float f43,float f44);
	inline Matrix(const Matrix &m);

	inline Matrix &operator = (const Matrix &m);
	inline void operator *= (const Matrix &m);

	inline float operator () (int iRow,int iCol) const;
	inline float operator () (int iRow,int iCol);
	inline operator const float * () const;
	inline operator float * ();

	inline Matrix operator * (const Matrix &m);

	inline void Identity();
	inline void Zero();
	inline void SetRotationEuler(const Vector &v);
	inline void SetRotationAxisAngle(const Vector &v,float fAngle);
	inline void SetRotationQuaternion(const Quaternion &q);
	inline void SetRotationX(float fAngle);
	inline void SetRotationY(float fAngle);
	inline void SetRotationZ(float fAngle);
	inline void SetRotationTarget(const Vector &vOrigin,const Vector &vTarget);	// Direction

	inline void SetScale(const Vector &v);
	inline void SetScale(float fScale);
	inline Vector GetScale();
	
	inline void Transpose();
	inline float Determinant();
	inline Matrix GetInverse();
	inline void Inverse();

	inline void SetViewMatrix(Vector &vFrom,Vector &vAt,Vector &vUp);
	inline void SetProjectionMatrix(float fov,float aspect,float nearplane,float farplane);
	inline void SetProjectionMatrix_Orthogonal(float w,float h,float nearplane,float farplane);

	inline void SetMirrorMatrix(const Plane &plane);

	inline void		SetTranslation(const Vector* v);
	inline void		ZeroTranslation();
	inline Vector	GetTranslation();

	inline Vector GetVM_View();
	inline Vector GetVM_Right();
	inline Vector GetVM_Up();
	
	inline Vector MultiplyMat3(Vector& v);	// by kjg 회전만 시켜 주는 함수
};

inline Vector operator *(Vector &v,Matrix &m);

inline void operator *=(Vector &v,Matrix &m);

inline Vector WorldToScreen(Vector src,float width,float height,float nearZ,float farZ,Matrix &ProjectionMatrix,Matrix &ViewMatrix);
inline Vector ScreenToWorld(Vector src,float width,float height,float nearZ,float farZ,Matrix &ProjectionMatrix,Matrix &ViewMatrix);
