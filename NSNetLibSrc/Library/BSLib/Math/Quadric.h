#pragma once

//----------------------------------------------------------------------------//
// Quadric.h                                                                  //
// Copyright (C) 2001, 2002 Bruno 'Beosil' Heidelberger                       //
//----------------------------------------------------------------------------//

class CQuadric
{
// member variables
protected:
	double m_aa, m_ab, m_ac, m_ad;
	double       m_bb, m_bc, m_bd;
	double             m_cc, m_cd;
	double                   m_dd;
	double m_area;

// constructors/destructor
public:
	inline CQuadric();
	inline CQuadric(double nx, double ny, double nz, double d, double area);
	inline CQuadric(double x1, double y1, double z1, double x2, double y2, double z2, double nx, double ny, double nz);
	inline CQuadric(const CQuadric& q);
	inline ~CQuadric();

// member functions
public:
	inline void Add(const CQuadric& q);
	inline double Evaluate(double x, double y, double z);
	inline double GetArea();
	inline void Scale(double factor);
	inline void Set(double nx, double ny, double nz, double d, double area);
	inline void Set(double x1, double y1, double z1, double x2, double y2, double z2, double nx, double ny, double nz);
	inline void Set(const CQuadric& q);

};
