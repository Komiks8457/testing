// Vector2 InlineÇÔ¼öµé
inline Vector2::Vector2()
{
	//memset(this, 0, sizeof(Vector2));
}

inline Vector2::Vector2(const Vector2 &v)
{
	memcpy(this, &v, sizeof(Vector2));
}

inline Vector2::Vector2(float x,float y)
{
	xy[0] = x;
	xy[1] = y;
}

inline void Vector2::Zero()
{
	x = y = 0.0f;
}

inline float &Vector2::operator [] (int iAxis)
{
	return xy[iAxis];
}

inline bool Vector2::operator < (const Vector2 &v) const
{
	return (x < v.x && y < v.y);
}

inline bool Vector2::operator > (const Vector2 &v) const
{
	return (x > v.x && y > v.y);
}

inline bool Vector2::operator == (const Vector2 &v) const
{
	return (x == v.x && y == v.y);
}

inline bool Vector2::operator != (const Vector2 &v) const
{
	return !(x == v.x && y == v.y);
}

inline Vector2::operator float *()
{
	return &x;
}

inline Vector2::operator const float *() const
{
	return &x;
}

inline Vector2 Vector2::operator = (const Vector2 &v)
{
	memcpy(this, &v, sizeof(Vector2));
	return *this;
}

inline void Vector2::operator -= (const Vector2 &v)
{
	x -= v.x;
	y -= v.y;
}

inline void Vector2::operator += (const Vector2 &v)
{
	x += v.x;
	y += v.y;
}

inline void Vector2::operator /= (float fScalar)
{
	x /= fScalar;
	y /= fScalar;
}

inline void Vector2::operator *= (float fScalar)
{
	x /= fScalar;
	y /= fScalar;
}

inline Vector2 Vector2::operator - (const Vector2 &v) const
{
	return Vector2(x - v.x, y - v.y);	
}

inline Vector2 Vector2::operator + (const Vector2 &v) const
{
	return Vector2(x + v.x, y + v.y);
}

inline Vector2 Vector2::operator - (const Vector2 &v)
{
	return Vector2(x - v.x, y - v.y);
}

inline Vector2 Vector2::operator + (const Vector2 &v)
{
	return Vector2(x + v.x, y + v.y);
}

inline Vector2 Vector2::operator / (float fScalar) const
{
	return Vector2(x / fScalar, y / fScalar);
}

inline Vector2 Vector2::operator * (float fScalar) const
{
	return Vector2(x * fScalar, y * fScalar);
}

inline Vector2 Vector2::operator / (float fScalar)
{
	return Vector2(x / fScalar, y / fScalar);
}

inline Vector2 Vector2::operator * (float fScalar)
{
	return Vector2(x * fScalar, y * fScalar);
}

inline void Vector2::SetLength(float fLength)
{
	Normalize();
	x *= fLength;
	y *= fLength;
}

inline float Vector2::GetLengthSqr() const
{
	return Dot(*this);
}

inline float Vector2::GetLengthSqr()
{
	return Dot(*this);
}

inline float Vector2::GetLength() const
{
	return SQRT(x * x + y * y);
}

inline float Vector2::GetLength()
{
	return SQRT(x * x + y * y);
}

inline float Vector2::GetAngle(const Vector2 &v)
{
	float fDot = Dot(v);
	float fMagnitude = (GetLength() * v.GetLength());
	float value = fDot / fMagnitude;
	CLAMP(value, -1.0f, 1.0f);
	return ACOS(value);
}

inline float Vector2::Distance(const Vector2 &v) const
{
	return (v - *this).GetLength();
}

inline float Vector2::Distance(const Vector2 &v)
{
	return (v - *this).GetLength();
}

inline float Vector2::Dot(const Vector2 &v) const
{
	return x * v.x + y * v.y;
}

inline float Vector2::Dot(const Vector2 &v)
{
	return x * v.x + y * v.y;
}

inline Vector2 Vector2::Lerp(const Vector2 &v,float fFactor) const
{
	return (*this + ((v - *this) * fFactor));
}

inline Vector2 Vector2::Lerp(const Vector2 &v,float fFactor)
{
	return (*this + ((v - *this) * fFactor));
}

inline Vector2 Vector2::GetNormal() const
{
	return *this / GetLength();
}
inline Vector2 Vector2::GetNormal()
{
	return *this / GetLength();
}

inline void Vector2::Normalize()
{
	float fLength = GetLength();
	if (fLength > 0.0f)
		fLength = 1.0f / fLength;
	else
		fLength = 0.0f;

	x *= fLength;
	y *= fLength;
}

inline Vector2 operator - (const Vector2 &v)
{
	return Vector2(-v.x, -v.y);
}

inline Vector2 operator - (Vector2 &v)
{
	return Vector2(-v.x, -v.y);
}

inline Vector2 operator * (float scalar,Vector2 &v)
{
	return Vector2(v.x * scalar, v.y * scalar);
}

inline BOOL NearlyEquals(const Vector2& a, const Vector2& b, float r) 
{
  Vector2 diff = a - b;

  return (VecDot2(diff, diff) < r*r);
}
