
//Vector inlineÇÔ¼öµé
inline Vector::Vector()
{
	//memset(this, 0, sizeof(Vector));
}

inline Vector::Vector(const Vector &v)
{
	//memcpy(this, &v, sizeof(Vector));
	*this = v;
}

inline Vector::Vector(float fX,float fY,float fZ) : x(fX), y(fY), z(fZ)
{
	
}

inline float &Vector::operator [] (int iAxis)
{
	return xyz[ iAxis];
}

inline bool Vector::operator < (const Vector &v) const
{
	return (x < v.x && y < v.y && z < v.z);
}

inline bool Vector::operator > (const Vector &v) const
{
	return (x > v.x && y > v.y && z > v.z);
}

inline bool Vector::operator == (const Vector &v) const
{
	return (x == v.x && y == v.y && z == v.z);
}

inline bool Vector::operator != (const Vector &v) const
{
	return !(x == v.x && y == v.y && z == v.z);
}

inline Vector::operator float * ()
{
	return &x;
}

inline Vector::operator const float *() const
{
	return &x;
}

/*inline Vector Vector::operator = (const Vector &v)
{
	memcpy(this, &v, sizeof(Vector));
	
	return *this;
}*/

inline void Vector::operator -= (const Vector &v)
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
}

inline void Vector::operator += (const Vector &v)
{
	x += v.x;
	y += v.y;
	z += v.z;
}

inline void Vector::operator /= (float fScalar)
{
	x /= fScalar;
	y /= fScalar;
	z /= fScalar;
}

inline void Vector::operator *= (float fScalar)
{
	x *= fScalar;
	y *= fScalar;
	z *= fScalar;
}

inline Vector Vector::operator - (const Vector &v) const
{
	return Vector(x - v.x, y - v.y, z - v.z);
}

inline Vector Vector::operator + (const Vector &v) const
{
	return Vector(x + v.x, y + v.y, z + v.z);
}

inline Vector Vector::operator * (const Vector &v) const
{
	return Vector(x * v.x, y * v.y, z * v.z);
}

// inline Vector Vector::operator - (const Vector &v)
// {
// 	return Vector(x - v.x, y - v.y, z - v.z);
// }
// 
// inline Vector Vector::operator + (const Vector &v)
// {
// 	return Vector(x + v.x, y + v.y, z + v.z);
// }
// 
// inline Vector Vector::operator * (const Vector &v)
// {
// 	return Vector(x * v.x, y * v.y, z * v.z);
// }

inline Vector Vector::operator / (float fScalar) const
{
	return Vector(x / fScalar, y / fScalar, z / fScalar);
}

inline Vector Vector::operator * (float fScalar) const
{
	return Vector(x * fScalar, y * fScalar, z * fScalar);
}

inline Vector Vector::operator / (float fScalar)
{
	return Vector(x / fScalar, y / fScalar, z / fScalar);
}

inline Vector Vector::operator * (float fScalar)
{
	return Vector(x * fScalar, y * fScalar, z * fScalar);
}

/*
inline void Vector::operator *= (const Quaternion& q)
{
	Quaternion temp(-q.x, -q.y, -q.z, q.w);
	temp *= *this;
	temp *= q;

	x = temp.x;
	y = temp.y;
	z = temp.z;
}
*/

// we assume that the quaternion is a unit quaternion.
inline void Vector::operator *= (Quaternion& Q)
{
	float  cj[4];
	float  qv[4];

	// evaluate Q * vector
	qv[QX] = Q.Quat[QW] * xyz[QX]						 + Q.Quat[QY] * xyz[QZ] - Q.Quat[QZ] * xyz[QY];
	qv[QY] = Q.Quat[QW] * xyz[QY] - Q.Quat[QX] * xyz[QZ]						+ Q.Quat[QZ] * xyz[QX];
	qv[QZ] = Q.Quat[QW] * xyz[QZ] + Q.Quat[QX] * xyz[QY] - Q.Quat[QY] * xyz[QX];
	qv[QW] =                      - Q.Quat[QX] * xyz[QX] - Q.Quat[QY] * xyz[QY] - Q.Quat[QZ] * xyz[QZ];

	// make the conjugate of Q.Quat.
	cj[QX] = -Q.Quat[QX]; 
	cj[QY] = -Q.Quat[QY];  
	cj[QZ] = -Q.Quat[QZ]; 
	cj[QW] =  Q.Quat[QW];

	// evaluate vector * conj
	xyz[QX] = qv[QW] * cj[QX] + qv[QX] * cj[QW] + qv[QY] * cj[QZ] - qv[QZ] * cj[QY];
	xyz[QY] = qv[QW] * cj[QY] - qv[QX] * cj[QZ] + qv[QY] * cj[QW] + qv[QZ] * cj[QX];
	xyz[QZ] = qv[QW] * cj[QZ] + qv[QX] * cj[QY] - qv[QY] * cj[QX] + qv[QZ] * cj[QW];
}

inline void Vector_X_Quat(Vector& vResult, Vector& vSrc, Quaternion& Q)
{
	float  cj[4];
	float  qv[4];

	// evaluate Q * vector
	qv[QX] = Q.Quat[QW] * vSrc.xyz[QX]							   + Q.Quat[QY] * vSrc.xyz[QZ] - Q.Quat[QZ] * vSrc.xyz[QY];
	qv[QY] = Q.Quat[QW] * vSrc.xyz[QY] - Q.Quat[QX] * vSrc.xyz[QZ]							   + Q.Quat[QZ] * vSrc.xyz[QX];
	qv[QZ] = Q.Quat[QW] * vSrc.xyz[QZ] + Q.Quat[QX] * vSrc.xyz[QY] - Q.Quat[QY] * vSrc.xyz[QX];
	qv[QW] =						   - Q.Quat[QX] * vSrc.xyz[QX] - Q.Quat[QY] * vSrc.xyz[QY] - Q.Quat[QZ] * vSrc.xyz[QZ];

	// make the conjugate of Q.Quat.
	cj[QX] = -Q.Quat[QX]; 
	cj[QY] = -Q.Quat[QY];  
	cj[QZ] = -Q.Quat[QZ]; 
	cj[QW] =  Q.Quat[QW];

	// evaluate vector * conj
	vResult.xyz[QX] = qv[QW] * cj[QX] + qv[QX] * cj[QW] + qv[QY] * cj[QZ] - qv[QZ] * cj[QY];
	vResult.xyz[QY] = qv[QW] * cj[QY] - qv[QX] * cj[QZ] + qv[QY] * cj[QW] + qv[QZ] * cj[QX];
	vResult.xyz[QZ] = qv[QW] * cj[QZ] + qv[QX] * cj[QY] - qv[QY] * cj[QX] + qv[QZ] * cj[QW];
}

inline void Vector::SetLength(float fLength)
{
	operator *= (fLength / GetLength());
}

inline float Vector::GetLengthSqr() const
{
	return Dot(*this);
}

inline float Vector::GetLengthSqr()
{
	return Dot(*this);
}

inline float Vector::GetLength() const
{
	return SQRT(x * x + y * y + z * z);
}

inline float Vector::GetLength()
{
	return SQRT(x * x + y * y + z * z);
}

inline float Vector::GetAngle(const Vector &v)
{
	float fDot = Dot(v);
	float fMagnitude = (GetLength() * v.GetLength());
	float value = fDot / fMagnitude;
	CLAMP(value, -1.0f, 1.0f);
	return ACOS(value);
}

inline float Vector::Dot(const Vector &v) const
{
	return (x * v.x + y * v.y + z * v.z);
}

inline float Vector::Dot(const Vector &v)
{
	return (x * v.x + y * v.y + z * v.z);
}

inline Vector Vector::Cross(const Vector &v) const
{
	return Vector((y * v.z) - (z * v.y),
				  (z * v.x) - (x * v.z),
				  (x * v.y) - (y * v.x));
}

inline Vector Vector::Cross(const Vector &v)
{
	return Vector((y * v.z) - (z * v.y),
				  (z * v.x) - (x * v.z),
				  (x * v.y) - (y * v.x));
}

inline float Vector::Distance(const Vector &v) const
{
	return (v - *this).GetLength();
}

inline float Vector::Distance(const Vector &v)
{
	return (v - *this).GetLength();
}

inline void Vector::Lerp(const Vector &v, float fFactor)
{
	x += fFactor * (v.x - x);
	y += fFactor * (v.y - y);
	z += fFactor * (v.z - z);
}

inline Vector Vector::GetNormal() const
{
	return *this / GetLength();
}

inline Vector Vector::GetNormal()
{
	return *this / GetLength();
}

inline void Vector::Normalize()
{
	float fLength = GetLength();
	if (fLength > 0.0f)
		fLength = 1.0f / fLength;
	else
		fLength = 0.0f;

	x *= fLength;
	y *= fLength;
	z *= fLength;
}

inline Vector Vector::Min(const Vector &v)
{
	return Vector(MIN(x, v.x),
					MIN(y, v.y),
					MIN(z, v.z));
}

inline Vector Vector::Max(const Vector &v)
{
	return Vector(MAX(x, v.x),
					MAX(y, v.y),
					MAX(z, v.z));
}

inline Vector operator - (const Vector &v)
{
	return Vector(-v.x, -v.y, -v.z);
}

inline Vector operator - (Vector &v)
{
	return Vector(-v.x, -v.y, -v.z);
}

inline Vector operator * (float scalar,Vector &v)
{
	return Vector(v.x * scalar, v.y * scalar, v.z * scalar);
}

inline BOOL NearlyEquals(const Vector& a, const Vector& b, float r) 
{
	Vector diff = a - b;
	return (VecDot(diff, diff) < r*r);
}
