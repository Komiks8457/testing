// Vector4 Inline ÇÔ¼ö

inline Vector4::Vector4()
{
	//memset(this, 0, sizeof(Vector4));
}
inline Vector4::Vector4(const Vector4 &v)
{
	memcpy(this, &v, sizeof(Vector4));
}

inline Vector4::Vector4(float fX,float fY,float fZ,float fW)
{
	x = fX;
	y = fY;
	z = fZ;
	w = fW;
}

inline float &Vector4::operator [] (int iAxis)
{
	return xyzw[ iAxis];
}

inline bool Vector4::operator < (const Vector4 &v) const
{
	return (x < v.x && y < v.y && z < v.z && w < v.w);
}

inline bool Vector4::operator > (const Vector4 &v) const
{
	return (x > v.x && y > v.y && z > v.z && w > v.w);
}

inline bool Vector4::operator == (const Vector4 &v) const
{
	return (x == v.x && y == v.y && z == v.z && w == v.w);
}

inline bool Vector4::operator != (const Vector4 &v) const
{
	return !(x == v.x && y == v.y && z == v.z && w == v.w);
}

inline Vector4::operator float *()
{
	return &x;
}

inline Vector4::operator const float *() const
{
	return &x;
}

inline Vector4 Vector4::operator = (const Vector4 &v)
{
	memcpy(this, &v, sizeof(Vector4));
	
	return *this;
}

inline void Vector4::operator -= (const Vector4 &v)
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
	w -= v.w;
}

inline void Vector4::operator += (const Vector4 &v)
{
	x += v.x;
	y += v.y;
	z += v.z;
	w += v.w;
}

inline void Vector4::operator /= (float fScalar)
{
	x /= fScalar;
	y /= fScalar;
	z /= fScalar;
	w /= fScalar;
}

inline void Vector4::operator *= (float fScalar)
{
	x *= fScalar;
	y *= fScalar;
	z *= fScalar;
	w *= fScalar;
}

inline Vector4 Vector4::operator - (const Vector4 &v) const
{
	return Vector4(x - v.x, y - v.y, z - v.z, w - v.w);
}

inline Vector4 Vector4::operator + (const Vector4 &v) const
{
	return Vector4(x + v.x, y + v.y, z + v.z, w + v.w);
}

inline Vector4 Vector4::operator - (const Vector4 &v)
{
	return Vector4(x - v.x, y - v.y, z - v.z, w - v.w);
}

inline Vector4 Vector4::operator + (const Vector4 &v)
{
	return Vector4(x + v.x, y + v.y, z + v.z, w + v.w);
}

inline Vector4 Vector4::operator / (float fScalar) const
{
	return Vector4(x / fScalar, y / fScalar, z / fScalar, w / fScalar);
}

inline Vector4 Vector4::operator * (float fScalar) const
{
	return Vector4(x * fScalar, y * fScalar, z * fScalar, w * fScalar);
}

inline Vector4 Vector4::operator / (float fScalar)
{
	return Vector4(x / fScalar, y / fScalar, z / fScalar, w / fScalar);
}

inline Vector4 Vector4::operator * (float fScalar)
{
	return Vector4(x * fScalar, y * fScalar, z * fScalar, w * fScalar);
}

inline void Vector4::SetLength(float fLength)
{
	operator *= (fLength / GetLength());
}

inline float Vector4::GetLengthSqr() const
{
	return Dot(*this);
}

inline float Vector4::GetLengthSqr()
{
	return Dot(*this);
}

inline float Vector4::GetLength() const
{
	return SQRT(x * x + y * y + z * z + w * w);
}

inline float Vector4::GetLength()
{
	return SQRT(x * x + y * y + z * z + w * w);
}

inline float Vector4::GetAngle(const Vector4 &v)
{
	float fDot = Dot(v);
	float fMagnitude = (GetLength() * v.GetLength());
	float value = fDot / fMagnitude;
	CLAMP(value, -1.0f, 1.0f);
	return ACOS(value);
}

inline float Vector4::Dot(const Vector4 &v) const
{
	return x * v.x + y * v.y + z * v.z + w * v.w;
}

inline float Vector4::Dot(const Vector4 &v)
{
	return x * v.x + y * v.y + z * v.z + w * v.w;
}

inline float Vector4::Distance(const Vector4 &v) const
{
	return (v - *this).GetLength();
}

inline float Vector4::Distance(const Vector4 &v)
{
	return (v - *this).GetLength();
}

inline Vector4 Vector4::Lerp(const Vector4 &v, float fFactor) const
{
	return (*this + ((v - *this) * fFactor));
}

inline Vector4 Vector4::Lerp(const Vector4 &v, float fFactor)
{
	return (*this + ((v - *this) * fFactor));
}

inline Vector4 Vector4::GetNormal() const
{
	return *this / GetLength();
}

inline Vector4 Vector4::GetNormal()
{
	return *this / GetLength();
}

inline void Vector4::Normalize()
{
	float fLength = GetLength();
	if (fLength > 0.0f)
		fLength = 1.0f / fLength;
	else 
		fLength = 0.0f;

	x *= fLength;
	y *= fLength;
	z *= fLength;
	w *= fLength;
}

inline Vector4 operator - (const Vector4 &v)
{
	return Vector4(-v.x, -v.y, -v.z, -v.w);
}

inline Vector4 operator - (Vector4 &v)
{
	return Vector4(-v.x, -v.y, -v.z, -v.w);
}

inline Vector4 operator * (float scalar,Vector4 &v)
{
	return Vector4(v.x * scalar, v.y * scalar, v.z * scalar, v.w * scalar);
}

inline BOOL NearlyEquals(const Vector4& a, const Vector4& b, float r) 
{
	Vector4 diff = a - b;
	return (VecDot4(diff, diff) < r*r);
}
