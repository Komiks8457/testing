// Matrix inline함수
inline Matrix::Matrix(BOOL id)
{
	if (id == TRUE)
		Identity();
}

inline Matrix::Matrix(Quaternion& q)
{ 
	SetRotationQuaternion(q); 
}

inline Matrix::Matrix(float *pfData)
{
	memcpy(m, pfData, sizeof(float) * 16);
}

inline Matrix::Matrix(float f11,float f12,float f13,float f14,
					float f21,float f22,float f23,float f24,
					float f31,float f32,float f33,float f34,
					float f41,float f42,float f43,float f44)
{
	_11 = f11;_12 = f12;_13 = f13;_14 = f14;
	_21 = f21;_22 = f22;_23 = f23;_24 = f24;
	_31 = f31;_32 = f32;_33 = f33;_34 = f34;
	_41 = f41;_42 = f42;_43 = f43;_44 = f44;
}

inline Matrix::Matrix(const Matrix &m)
{
	memcpy(this, &m, sizeof(Matrix));
}

inline Matrix &Matrix::operator = (const Matrix &m)
{
	memcpy(this, &m, sizeof(Matrix));
	return *this;
}

inline void Matrix::operator *= (const Matrix &m)
{
	*this = Matrix( ((_11 * m._11) + (_12 * m._21) + (_13 * m._31) + (_14 * m._41)),
						((_11 * m._12) + (_12 * m._22) + (_13 * m._32) + (_14 * m._42)),
						((_11 * m._13) + (_12 * m._23) + (_13 * m._33) + (_14 * m._43)),
						((_11 * m._14) + (_12 * m._24) + (_13 * m._34) + (_14 * m._44)),

						((_21 * m._11) + (_22 * m._21) + (_23 * m._31) + (_24 * m._41)),
						((_21 * m._12) + (_22 * m._22) + (_23 * m._32) + (_24 * m._42)),
						((_21 * m._13) + (_22 * m._23) + (_23 * m._33) + (_24 * m._43)),
						((_21 * m._14) + (_22 * m._24) + (_23 * m._34) + (_24 * m._44)),

						((_31 * m._11) + (_32 * m._21) + (_33 * m._31) + (_34 * m._41)),
						((_31 * m._12) + (_32 * m._22) + (_33 * m._32) + (_34 * m._42)),
						((_31 * m._13) + (_32 * m._23) + (_33 * m._33) + (_34 * m._43)),
						((_31 * m._14) + (_32 * m._24) + (_33 * m._34) + (_34 * m._44)),

						((_41 * m._11) + (_42 * m._21) + (_43 * m._31) + (_44 * m._41)),
						((_41 * m._12) + (_42 * m._22) + (_43 * m._32) + (_44 * m._42)),
						((_41 * m._13) + (_42 * m._23) + (_43 * m._33) + (_44 * m._43)),
						((_41 * m._14) + (_42 * m._24) + (_43 * m._34) + (_44 * m._44)));
}

inline float Matrix::operator () (int iRow,int iCol) const
{
	return m[ iRow][ iCol];
}

inline float Matrix::operator () (int iRow,int iCol)
{
	return m[ iRow][ iCol];
}

inline Matrix::operator const float * () const
{
	return &_11;
}

inline Matrix::operator float * ()
{
	return &_11;
}

inline Matrix Matrix::operator * (const Matrix &m)
{
	return Matrix( ((_11 * m._11) + (_12 * m._21) + (_13 * m._31) + (_14 * m._41)),
						((_11 * m._12) + (_12 * m._22) + (_13 * m._32) + (_14 * m._42)),
						((_11 * m._13) + (_12 * m._23) + (_13 * m._33) + (_14 * m._43)),
						((_11 * m._14) + (_12 * m._24) + (_13 * m._34) + (_14 * m._44)),

						((_21 * m._11) + (_22 * m._21) + (_23 * m._31) + (_24 * m._41)),
						((_21 * m._12) + (_22 * m._22) + (_23 * m._32) + (_24 * m._42)),
						((_21 * m._13) + (_22 * m._23) + (_23 * m._33) + (_24 * m._43)),
						((_21 * m._14) + (_22 * m._24) + (_23 * m._34) + (_24 * m._44)),

						((_31 * m._11) + (_32 * m._21) + (_33 * m._31) + (_34 * m._41)),
						((_31 * m._12) + (_32 * m._22) + (_33 * m._32) + (_34 * m._42)),
						((_31 * m._13) + (_32 * m._23) + (_33 * m._33) + (_34 * m._43)),
						((_31 * m._14) + (_32 * m._24) + (_33 * m._34) + (_34 * m._44)),

						((_41 * m._11) + (_42 * m._21) + (_43 * m._31) + (_44 * m._41)),
						((_41 * m._12) + (_42 * m._22) + (_43 * m._32) + (_44 * m._42)),
						((_41 * m._13) + (_42 * m._23) + (_43 * m._33) + (_44 * m._43)),
						((_41 * m._14) + (_42 * m._24) + (_43 * m._34) + (_44 * m._44)));
}

inline void Matrix::Identity()
{
	::memset(m, 0, sizeof(Matrix));
	_11 = _22 = _33 = _44 = 1.0f;
}

inline void Matrix::Zero()
{
	::memset(m, 0, sizeof(Matrix));
}

inline void Matrix::SetTranslation(const Vector* v)
{
	::memcpy(m[3], v, sizeof(Vector));
}

inline void Matrix::ZeroTranslation()
{
	::memset(m[3], 0, sizeof(Vector));
}

inline Vector Matrix::GetTranslation()
{
	return Vector(m[3][0], m[3][1], m[3][2]);
}

inline void Matrix::SetRotationEuler(const Vector &v)
{
	float a,b,c,d,e,f;

	a = COS(v.x);
	b = SIN(v.x);
	c = COS(v.y);
	d = SIN(v.y);
	e = COS(v.z);
	f = SIN(v.z);

	float ad = a * d;
	float bd = b * d;

	m[0][0] =   c * e;
	m[0][1] =  -c * f;
	m[0][2] =  -d;
	m[1][0] = -bd * e + a * f;
	m[1][1] =  bd * f + a * e;
	m[1][2] =  -b * c;
	m[2][0] =  ad * e + b * f;
	m[2][1] = -ad * f + b * e;
	m[2][2] =   a * c;

	m[0][3] =  m[1][3] = m[2][3] = m[3][0] = m[3][1] = m[3][2] = 0;
	m[3][3] =  1;
}

inline void Matrix::SetRotationAxisAngle(const Vector &v,float fAngle)
{
	Vector vNormal(Vector(v).GetNormal());

	float c = COS(-fAngle);
	float s = SIN(-fAngle);
	float t = 1 - c;

    m[0][0] = (t * vNormal.x * vNormal.x) + c;
    m[1][1] = (t * vNormal.y * vNormal.y) + c;
    m[2][2] = (t * vNormal.z * vNormal.z) + c;

    m[1][0] = (t * vNormal.x * vNormal.y) + s * vNormal.z;
    m[2][0] = (t * vNormal.x * vNormal.z) - s * vNormal.y;
    m[0][1] = (t * vNormal.x * vNormal.y) - s * vNormal.z;
    
    m[2][1] = (t * vNormal.y * vNormal.z) + s * vNormal.x;
    m[0][2] = (t * vNormal.x * vNormal.z) + s * vNormal.y;
    m[1][2] = (t * vNormal.y * vNormal.z) - s * vNormal.x;
}

inline void Matrix::SetRotationQuaternion(const Quaternion &q)
{
	float f2X = (q.x * 2.0f);
	float f2Y = (q.y * 2.0f);
	float f2Z = (q.z * 2.0f);
	float fWX = (q.w * f2X);
	float fWY = (q.w * f2Y);
	float fWZ = (q.w * f2Z);
	float fXX = (q.x * f2X);
	float fXY = (q.x * f2Y);
	float fXZ = (q.x * f2Z);
	float fYY = (q.y * f2Y);
	float fYZ = (q.y * f2Z);
	float fZZ = (q.z * f2Z);
	
	_11 = (1.0f - fYY - fZZ);
	_12 = (fXY + fWZ);
	_13 = (fXZ - fWY);

	_21 = (fXY - fWZ);
	_22 = (1.0f - fXX - fZZ);
	_23 = (fYZ + fWX);

	_31 = (fXZ + fWY);
	_32 = (fYZ - fWX);
	_33 = (1.0f - fXX - fYY);

	_14 = _24 = _34 = _41 = _42 = _43 = 0.0f;
	_44 = 1.0f;
	
	/*
	double xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz;

	double x = q.x;
	double y = q.y;
	double z = q.z;
	double w = q.w;

    const double s = 2.0;
    
    xs = x * s;		ys = y * s;	zs = z * s;
    wx = w * xs;	wy = w * ys;	wz = w * zs;
    xx = x * xs;	xy = x * ys;	xz = x * zs;
    yy = y * ys;	yz = y * zs;	zz = z * zs;
    
    m[0][0] = float(1.0 - yy - zz);
    m[0][1] = float(xy + wz);
    m[0][2] = float(xz - wy);

    m[1][0] = float(xy - wz);
    m[1][1] = float(1.0 - xx - zz);
    m[1][2] = float(yz + wx);

    m[2][0] = float(xz + wy);
    m[2][1] = float(yz - wx);
    m[2][2] = float(1.0 - xx - yy);
	*/
}

inline void Matrix::SetRotationX(float fAngle)
{
	float s = SIN(fAngle);
	float c = COS(fAngle);

	Identity();

	m[ 1][ 1] = c;
	m[ 1][ 2] = s;
	m[ 2][ 1] = -s;
	m[ 2][ 2] = c;
}

inline void Matrix::SetRotationY(float fAngle)
{
	float s = SIN(fAngle);
	float c = COS(fAngle);

	Identity();

	m[ 0][ 0] = c;
	m[ 0][ 2] = s;
	m[ 2][ 0] = -s;
	m[ 2][ 2] = c;
}

inline void Matrix::SetRotationZ(float fAngle)
{
	float s = SIN(fAngle);
	float c = COS(fAngle);

	Identity();

	m[ 0][ 0] = c;
	m[ 0][ 1] = s;
	m[ 1][ 0] = -s;
	m[ 1][ 1] = c;
}

inline void Matrix::SetRotationTarget(const Vector &vOrigin,const Vector &vTarget)
{
	Vector vZAxis(vTarget - vOrigin);
	vZAxis.Normalize();

	Vector vTempYAxis(0.0f, 1.0f, 0.0f);
	Vector vXAxis(vTempYAxis.Cross(vZAxis));
	vXAxis.Normalize();

	Vector vYAxis(vZAxis.Cross(vXAxis));

	*this = Matrix(vXAxis.x, vXAxis.y, vXAxis.z, -vXAxis.Dot(vOrigin),
					 vYAxis.x, vYAxis.y, vYAxis.z, -vYAxis.Dot(vOrigin),
					 vZAxis.x, vZAxis.y, vZAxis.z, -vZAxis.Dot(vOrigin),
					 0.0f, 0.0f, 0.0f, 1.0f);
}

inline void Matrix::SetScale(const Vector &v)
{
	Identity();

	_11 = v.x;
	_22 = v.y;
	_33 = v.z;
}

inline void Matrix::SetScale(float fScale)
{
	Identity();

	_11 = _22 = _33 = fScale;
}

inline Vector Matrix::GetScale()
{
	return Vector(_11, _22, _33);
}

inline void Matrix::Transpose()
{
	Matrix temp(*this);

    _11 = temp._11;    _12 = temp._21;    _13 = temp._31;    _14 = temp._41;
    _21 = temp._12;    _22 = temp._22;    _23 = temp._32;    _24 = temp._42;
    _31 = temp._13;    _32 = temp._23;    _33 = temp._33;    _34 = temp._43;
    _41 = temp._14;    _42 = temp._24;    _43 = temp._34;    _44 = temp._44;
}

inline float Matrix::Determinant()
{
    return ((_11 * ((_22 * _33) - (_23 * _32))) -
            (_12 * ((_21 * _33) - (_23 * _31))) +
            (_13 * ((_21 * _32) - (_22 * _31))));
}

inline Matrix Matrix::GetInverse()
{
	Matrix r;

	if( fabs(m[3][3] - 1.0f) > .001f)
		return r;
	if( fabs(m[0][3]) > .001f || fabs(m[1][3]) > .001f || fabs(m[2][3]) > .001f )
		return r;

	float fDetInv = 1.0f / ( m[0][0] * ( m[1][1] * m[2][2] - m[1][2] * m[2][1] ) -
							 m[0][1] * ( m[1][0] * m[2][2] - m[1][2] * m[2][0] ) +
							 m[0][2] * ( m[1][0] * m[2][1] - m[1][1] * m[2][0] ) );

	r.m[0][0] =  fDetInv * ( m[1][1] * m[2][2] - m[1][2] * m[2][1] );
	r.m[0][1] = -fDetInv * ( m[0][1] * m[2][2] - m[0][2] * m[2][1] );
	r.m[0][2] =  fDetInv * ( m[0][1] * m[1][2] - m[0][2] * m[1][1] );
	r.m[0][3] = 0.0f;

	r.m[1][0] = -fDetInv * ( m[1][0] * m[2][2] - m[1][2] * m[2][0] );
	r.m[1][1] =  fDetInv * ( m[0][0] * m[2][2] - m[0][2] * m[2][0] );
	r.m[1][2] = -fDetInv * ( m[0][0] * m[1][2] - m[0][2] * m[1][0] );
	r.m[1][3] = 0.0f;

	r.m[2][0] =  fDetInv * ( m[1][0] * m[2][1] - m[1][1] * m[2][0] );
	r.m[2][1] = -fDetInv * ( m[0][0] * m[2][1] - m[0][1] * m[2][0] );
	r.m[2][2] =  fDetInv * ( m[0][0] * m[1][1] - m[0][1] * m[1][0] );
	r.m[2][3] = 0.0f;

	r.m[3][0] = -( m[3][0] * r.m[0][0] + m[3][1] * r.m[1][0] + m[3][2] * r.m[2][0] );
	r.m[3][1] = -( m[3][0] * r.m[0][1] + m[3][1] * r.m[1][1] + m[3][2] * r.m[2][1] );
	r.m[3][2] = -( m[3][0] * r.m[0][2] + m[3][1] * r.m[1][2] + m[3][2] * r.m[2][2] );
	r.m[3][3] = 1.0f;

	return r;
}
/*
inline Matrix Matrix::GetInverse()
{
	Matrix mR;
	float fDetInverse = (1.0f / Determinant());

    mR._11 = (fDetInverse * ((_22 * _33) - (_23 * _32)));
    mR._12 = (-fDetInverse * ((_12 * _33) - (_13 * _32)));
    mR._13 = (fDetInverse * ((_12 * _23) - (_13 * _22)));
    mR._14 = 0.0f;

    mR._21 = (-fDetInverse * ((_21 * _33) - (_23 * _31)));
    mR._22 = (fDetInverse * ((_11 * _33) - (_13 * _31)));
    mR._23 = (-fDetInverse * ((_11 * _23) - (_13 * _21)));
    mR._24 = 0.0f;

    mR._31 = (fDetInverse * ((_21 * _32) - (_22 * _31)));
    mR._32 = (-fDetInverse * ((_11 * _32) - (_12 * _31)));
    mR._33 = (fDetInverse * ((_11 * _22) - (_12 * _21)));
    mR._34 = 0.0f;

    mR._41 = -((_41 * mR._11) + (_42 * mR._21) + (_43 * mR._31));
    mR._42 = -((_41 * mR._12) + (_42 * mR._22) + (_43 * mR._32));
    mR._43 = -((_41 * mR._13) + (_42 * mR._23) + (_43 * mR._33));
    mR._44 = 1.0f;

	return mR;
}
*/

inline void Matrix::Inverse()
{
	*this = GetInverse();
}

inline void Matrix::SetViewMatrix(Vector &vFrom,Vector &vAt,Vector &vUp)
{
	Vector vView = vAt - vFrom;
	vView.Normalize();

	float fDotProduct = vUp.Dot(vView);

	Vector vViewUp = vUp - fDotProduct * vView;

	vViewUp.Normalize();

	Vector vRight;

	vRight = vViewUp.Cross(vView);

	m[ 0][ 0] = vRight.x;
	m[ 1][ 0] = vRight.y;
	m[ 2][ 0] = vRight.z;

	m[ 0][ 1] = vViewUp.x;
	m[ 1][ 1] = vViewUp.y;
	m[ 2][ 1] = vViewUp.z;

	m[ 0][ 2] = vView.x;
	m[ 1][ 2] = vView.y;
	m[ 2][ 2] = vView.z;

	m[ 3][ 0] = - vFrom.Dot(vRight);
	m[ 3][ 1] = - vFrom.Dot(vViewUp);
	m[ 3][ 2] = - vFrom.Dot(vView);
	m[ 3][ 3] = 1.0f;
}

inline void Matrix::SetProjectionMatrix(float fov,float aspect,float nearplane,float farplane)
{
	fov /= 2;

	float w = aspect * (COS(fov) / SIN(fov));
	float h = 1.0f * (COS(fov) / SIN(fov));
	float q = farplane / (farplane - nearplane);

	Zero();

	m[ 0][ 0] = w;
	m[ 1][ 1] = h;
	m[ 2][ 2] = q;
	m[ 2][ 3] = 1.0f;
	m[ 3][ 2] = -q * nearplane;
}

inline void Matrix::SetProjectionMatrix_Orthogonal(float w,float h,float nearplane,float farplane)
{
	Identity();

	m[ 0][ 0] = 2.0f / w;
	m[ 1][ 1] = 2.0f / h;
	m[ 2][ 2] = 1.0f / (farplane - nearplane);

    /*
     * The projection matrix scales (xmin,ymin,zmin)-(xmax,ymax,zmax) to
     * (-1,-1,0)-(1,1,1).  The window scale will then scale this to the
     * buffer.
     * 
     * P = [     2 / xdiff             0                   0           0 ]
     *     [       0                 2 / ydiff             0           0 ]
     *     [       0                   0                 1 / zdiff     0 ]
     *     [ 1 - 2*xmax / xdiff  1 - 2*ymax / ydiff  1 - zmax / zdiff) 1 ]
     */
}

inline void Matrix::SetMirrorMatrix(const Plane &plane)
{
	/*
	float fX = plane.n.x;
	float fY = plane.n.y;
	float fZ = plane.n.z;
	float fD = plane.d;

	*this = Matrix((0.0f - (2.0f * fX * fX)), (0.0f - (2.0f * fX * fY)), (0.0f - (2.0f * fX * fZ)), 0.0f,
					 (0.0f - (2.0f * fY * fX)), (0.0f - (2.0f * fY * fY)), (0.0f - (2.0f * fY * fZ)), 0.0f,
					 (0.0f - (2.0f * fZ * fX)), (0.0f - (2.0f * fZ * fY)), (0.0f - (2.0f * fZ * fZ)), 0.0f,
					 (0.0f - (2.0f * fD * fX)), (0.0f - (2.0f * fD * fY)), (0.0f - (2.0f * fD * fZ)), 1.0f);
					 */
}

inline Vector Matrix::GetVM_View()
{
	return Vector(_13, _23, _33);
}

inline Vector Matrix::GetVM_Right()
{
	return Vector(_11, _21, _31);
}

inline Vector Matrix::GetVM_Up()
{
	return Vector(_12, _22, _32);
}

inline Vector Matrix::MultiplyMat3(Vector& v) // by kjg
{
	Vector rV;

	rV.x = v.x * m[ 0][ 0] + v.y * m[ 1][ 0] + v.z * m[ 2][ 0];
	rV.y = v.x * m[ 0][ 1] + v.y * m[ 1][ 1] + v.z * m[ 2][ 1];
	rV.z = v.x * m[ 0][ 2] + v.y * m[ 1][ 2] + v.z * m[ 2][ 2];

	return rV;
}

inline Vector operator *(Vector &v,Matrix &m)
{
	Vector rV;

	rV.x = v.x * m.m[ 0][ 0] + v.y * m.m[ 1][ 0] + v.z * m.m[ 2][ 0] + m.m[ 3][ 0];
	rV.y = v.x * m.m[ 0][ 1] + v.y * m.m[ 1][ 1] + v.z * m.m[ 2][ 1] + m.m[ 3][ 1];
	rV.z = v.x * m.m[ 0][ 2] + v.y * m.m[ 1][ 2] + v.z * m.m[ 2][ 2] + m.m[ 3][ 2];

	return rV;
}

inline void operator *=(Vector &v,Matrix &m)
{
	Vector rV;

	rV.x = v.x * m.m[ 0][ 0] + v.y * m.m[ 1][ 0] + v.z * m.m[ 2][ 0] + m.m[ 3][ 0];
	rV.y = v.x * m.m[ 0][ 1] + v.y * m.m[ 1][ 1] + v.z * m.m[ 2][ 1] + m.m[ 3][ 1];
	rV.z = v.x * m.m[ 0][ 2] + v.y * m.m[ 1][ 2] + v.z * m.m[ 2][ 2] + m.m[ 3][ 2];
	
	v = rV;
}

inline Vector WorldToScreen(Vector src,float width,float height,float nearZ,float farZ,Matrix &ProjectionMatrix,Matrix &ViewMatrix)
{
	Vector result;

	result = src * ViewMatrix;
	
	float w = ProjectionMatrix.m[ 0][ 0];
	float h = ProjectionMatrix.m[ 1][ 1];
//	float q = ProjectionMatrix.m[ 2][ 2];

	if (ProjectionMatrix.m[ 2][ 3] != 0)
	{
		result.x *= w;
		result.y *= h;

		result.x /= result.z;
		result.y /= result.z;
	}
	else
	{
		result.x *= w;
		result.y *= h;
	}

	result.x *= width / 2;

	result.y = -result.y;
	result.y *= height / 2;

	result.x += width / 2;
	result.y += height / 2;


	return result;
}

inline Vector ScreenToWorld(Vector src,float width,float height,float nearZ,float farZ,Matrix &ProjectionMatrix,Matrix &ViewMatrix)
{
	Vector result;
	
	src.x -= width / 2;
	src.y -= height / 2;
	src.x /= width / 2;
	src.y /= height / 2;

	src.y = -src.y;

	Matrix invView = ViewMatrix.GetInverse();

	float w = ProjectionMatrix.m[ 0][ 0];
	float h = ProjectionMatrix.m[ 1][ 1];
	float q = ProjectionMatrix.m[ 2][ 2];

	if (ProjectionMatrix.m[ 2][ 3] != 0) // Perspective 이면
	{
		float z = q * (nearZ + (farZ - nearZ) * src.z) - q * nearZ;

		src.x *= z;
		src.y *= z;

		src.x /= w;
		src.y /= h;

		src.z = z;

		result = src * invView;
	}
	else
	{
		src.x /= w;
		src.y /= h;

//		float z = q * (nearZ + (farZ - nearZ) * src.z) - q * nearZ;
		src.z = nearZ + (farZ - nearZ) * src.z;
//		src.z = z;

		result = src * invView;
	}
	
	return result;
}
