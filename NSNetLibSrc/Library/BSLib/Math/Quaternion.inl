inline Quaternion::Quaternion()
{
	x = y = z = 0;
	w = 1.0f;
}

inline Quaternion::Quaternion(const Quaternion &q)
{
	memcpy(this, &q, sizeof(Quaternion));
}

inline Quaternion::Quaternion(const float fX,float fY,float fZ,float fW)
{
	x = fX;
	y = fY;
	z = fZ;
	w = fW;
}


inline Quaternion::Quaternion(const Matrix& m)
{
	SetRotationMatrix(m);
}

inline void Quaternion::SetRotationMatrix(const Matrix& m)
{
	/*
	float diag, s;
	int i, j, k;
	
	diag = m.m[0][0] + m.m[1][1] + m.m[2][2];

	if (diag < -0.999f)
	{
		i = QX;
		if (m.m[QY][QY] > m.m[QX][QX])
			i = QY;
		if (m.m[QZ][QZ] > m.m[i][i])
			i = QZ;
		
		j = g_QNext[i];
		k = g_QNext[j];

		s = sqrtf(m.m[i][i] - (m.m[j][j] + m.m[k][k]) + 1.0f);

		Quat[i] = s * 0.5f;
		s = 0.5f / s;
		Quat[QW] = (m.m[k][j] - m.m[j][k]) * s;
		Quat[j] = (m.m[j][i] + m.m[i][j]) * s;
		Quat[k] = (m.m[k][i] + m.m[i][k]) * s;
		return;
	}

	s = sqrtf(diag + 1.0f);
	
	w = s * 0.5f;
	s = 0.5f / s;

	x = (m.m[2][1] - m.m[1][2]) * s;
	y = (m.m[0][2] - m.m[2][0]) * s;
	z = (m.m[1][0] - m.m[0][1]) * s;
	*/
	/*
	float   tr, s;

    tr = m._11 + m._22 + m._33;

    // check the diagonal
    if (tr > 0.0)
    {
        s = (float)sqrt (tr + 1.0f);
        w = s * 0.5f;
        s = 0.5f / s;
        x = (m._32 - m._23) * s;
        y = (m._13 - m._31) * s;
        z = (m._21 - m._12) * s;
    }
    else
    {
        if (m._22 > m._11 && m._33 <= m._22)
        {
            s = (float)sqrt ((m._22 - (m._33 + m._11)) + 1.0f);

            x = s * 0.5f;

            if (s != 0.0)
                s = 0.5f / s;

            z = (m._13 - m._31) * s;
            y = (m._32 + m._23) * s;
            w = (m._12 + m._21) * s;
        }
        else if ((m._22 <= m._11  &&  m._33 > m._11)  ||  (m._33 > m._22))
        {
            s = (float)sqrt ((m._33 - (m._11 + m._22)) + 1.0f);

            y = s * 0.5f;

            if (s != 0.0)
                s = 0.5f / s;

            z = (m._21 - m._12) * s;
            s = (m._13 + m._31) * s;
            x = (m._23 + m._32) * s;
        }
        else
        {
            s = (float)sqrt ((m._11 - (m._22 + m._33)) + 1.0f);

            w = s * 0.5f;

            if (s != 0.0)
                s = 0.5f / s;

            z = (m._32 - m._23) * s;
            x = (m._21 + m._12) * s;
            y = (m._31 + m._13) * s;
        }
    }
	*/
	float fTrace = m.m[0][0]+m.m[1][1]+m.m[2][2];
    float fRoot;

    if ( fTrace > 0.0 )
    {
        // |w| > 1/2, may as well choose w > 1/2
        fRoot = SQRT(float(fTrace + 1.0));  // 2w
        w = float(0.5*fRoot);
        fRoot = float(0.5/fRoot);  // 1/(4w)
        x = (m.m[2][1]-m.m[1][2])*fRoot;
        y = (m.m[0][2]-m.m[2][0])*fRoot;
        z = (m.m[1][0]-m.m[0][1])*fRoot;
    }
    else
    {
        // |w| <= 1/2
        static int s_iNext[3] = { 1, 2, 0 };
        int i = 0;
        if ( m.m[1][1] > m.m[0][0] )
            i = 1;
        if ( m.m[2][2] > m.m[i][i] )
            i = 2;
        int j = s_iNext[i];
        int k = s_iNext[j];

        fRoot = SQRT(float(m.m[i][i]-m.m[j][j]-m.m[k][k] + 1.0));
        float* apkQuat[3] = { &x, &y, &z };
        *apkQuat[i] = float(0.5*fRoot);
        fRoot = float(0.5/fRoot);
        w = (m.m[k][j]-m.m[j][k])*fRoot;
        *apkQuat[j] = (m.m[j][i]+m.m[i][j])*fRoot;
        *apkQuat[k] = (m.m[k][i]+m.m[i][k])*fRoot;
    }
}

inline Quaternion::Quaternion(const Vector &vAxis,float Angle)
{
	SetAxisAngle(vAxis, Angle);
}

inline Quaternion::Quaternion(const Vector &vEuler)
{
	SetEuler(vEuler);
}

inline Quaternion::operator float * (void)
{
	return &x;
}

inline void Quaternion::operator *= (const Quaternion &q)
{
	Quaternion tempQ;

	tempQ.x = ((w * q.x) + (x * q.w) + (y * q.z) - (z * q.y));
	tempQ.y = ((w * q.y) - (x * q.z) + (y * q.w) + (z * q.x));
	tempQ.z = ((w * q.z) + (x * q.y) - (y * q.x) + (z * q.w));
	tempQ.w = ((w * q.w) - (x * q.x) - (y * q.y) - (z * q.z));

	*this = tempQ;
}

inline Quaternion &Quaternion::operator = (const Quaternion &q)
{
	memcpy(this, &q, sizeof(Quaternion));

	return *this;
}

inline void Quaternion::operator -= (const Quaternion &q)
{
	x -= q.x;
	y -= q.y;
	z -= q.z;
	w -= q.w;
}

inline void Quaternion::operator += (const Quaternion &q)
{
	x += q.x;
	y += q.y;
	z += q.z;
	w += q.w;
}

inline void Quaternion::operator /= (float fScalar)
{
	x /= fScalar;
	y /= fScalar;
	z /= fScalar;
	w /= fScalar;
}

inline void Quaternion::operator *= (float fScalar)
{
	x *= fScalar;
	y *= fScalar;
	z *= fScalar;
	w *= fScalar;
}

inline Quaternion Quaternion::operator * (const Quaternion &q)
{
	return Quaternion(((w * q.x) + (x * q.w) + (y * q.z) - (z * q.y)),
					   ((w * q.y) - (x * q.z) + (y * q.w) + (z * q.x)),
					   ((w * q.z) + (x * q.y) - (y * q.x) + (z * q.w)),
					   ((w * q.w) - (x * q.x) - (y * q.y) - (z * q.z)));					   ;
}


inline void Quaternion::operator *= (const Vector& v)
{
	float qx, qy, qz, qw;
	qx = x;
	qy = y;
	qz = z;
	qw = w;

	x = qw * v.x            + qy * v.z - qz * v.y;
	y = qw * v.y - qx * v.z            + qz * v.x;
	z = qw * v.z + qx * v.y - qy * v.x;
	w =          - qx * v.x - qy * v.y - qz * v.z;
}

inline Quaternion Quaternion::operator - (const Quaternion &q)
{
	return Quaternion(x - q.x,
					   y - q.y,
					   z - q.z,
					   w - q.w);
}

inline Quaternion Quaternion::operator + (const Quaternion &q)
{
	return Quaternion(x + q.x,
					   y + q.y,
					   z + q.z,
					   w + q.w);
}

inline Quaternion Quaternion::operator / (float fScalar)
{
	return Quaternion(x / fScalar,
					   y / fScalar,
					   z / fScalar,
					   w / fScalar);
}
inline Quaternion Quaternion::operator * (float fScalar)
{
	return Quaternion(x * fScalar,
					   y * fScalar,
					   z * fScalar,
					   w * fScalar);
}

inline void Quaternion::Identity()
{
	x = y = z = 0;
	w = 1.0f;
}

inline void Quaternion::SetEuler(const Vector &v)
{
    float fSINx = SIN(v.x * 0.5f);
    float fSINy = SIN(v.y * 0.5f);
    float fSINz = SIN(v.z * 0.5f);
    float fCOSx = COS(v.x * 0.5f);
    float fCOSy = COS(v.y * 0.5f);
    float fCOSz = COS(v.z * 0.5f);
	float fSINyCOSz = (fSINy * fCOSz);
	float fSINySINz = (fSINy * fSINz);
	float fCOSyCOSz = (fCOSy * fCOSz);
	float fCOSySINz = (fCOSy * fSINz);

	x = ((fSINx * fCOSyCOSz) - (fCOSx * fSINySINz));
	y = ((fSINx * fCOSySINz) + (fCOSx * fSINyCOSz));
	z = ((fCOSx * fCOSySINz) - (fSINx * fSINyCOSz));
	w = ((fCOSx * fCOSyCOSz) + (fSINx * fSINySINz));
}

inline void Quaternion::SetAxisAngle(Vector vAxis,float fAngle)
{
	vAxis.Normalize();
	vAxis = vAxis * SIN(fAngle * 0.5f);

	x = vAxis.x;
	y = vAxis.y;
	z = vAxis.z;
	w = COS(fAngle * 0.5f);
}

inline float Quaternion::Dot(const Quaternion &q)
{
	return x * q.x + y * q.y + z * q.z + w * q.w;
}

inline float Quaternion::GetLengthSqr()
{
	return Dot(*this);
}

inline float Quaternion::GetLength()
{
	return SQRT(x * x + y * y + z * z + w * w);
}

inline Quaternion Quaternion::GetNormal()
{
	float fLength = GetLength();

	return Quaternion(x / fLength,
					   y / fLength,
					   z / fLength,
					   w / fLength);
}

inline void Quaternion::Normalize()
{
	float fLength = GetLength();

	x /= fLength;
	y /= fLength;
	z /= fLength;
	w /= fLength;
}

inline void Quaternion::Slerp(Quaternion &q, float d)
{
	float norm = x * q.x + y * q.y + z * q.z + w * q.w;

	BOOL bFlip = FALSE;

	if (norm < 0.0f)
	{
		norm = -norm;
		bFlip = TRUE;
	}

	float inv_d;
	if (1.0f - norm < 0.000001f)
		inv_d = 1.0f - d;
	else
	{
		float theta = acos(norm);

		float s = 1.0f / sin(theta);

		inv_d = sin((1.0f - d) * theta) * s;
		d = sin(d * theta) * s;
	}

	if (bFlip)
		d = -d;
	
	x = inv_d * x + d * q.x;
	y = inv_d * y + d * q.y;
	z = inv_d * z + d * q.z;
	w = inv_d * w + d * q.w;
}

inline void Quaternion::Lerp(Quaternion &q, float fFactor)
{
	Quaternion qR;
	qR.x = (x + ((q.x - x) * fFactor));
	qR.y = (y + ((q.y - y) * fFactor));
	qR.z = (z + ((q.z - z) * fFactor));
	qR.w = (w + ((q.w - w) * fFactor));
	
	*this = qR;
}
