// Telescope coordinate conversion
// (C) 2016 Markus L. Noga
// (C) 2019 Charles Lemaire

// Inspired by:
// Toshimi Taki, "A New Concept in Computer-Aided Telescopes", Sky & Telescope Feb 1989, pp. 194-196
// Wikipedia, siderial time article
// Wikipedia, local hour angle article
// Cosine direction vectors

#include "TeenAstroLA3.hpp"

using namespace std;

// LA3 methods
//

// Multiply two 3-vectors
void LA3::crossProduct(double (&out)[3], const double (&a)[3], const double (&b)[3]) {
	out[0]=a[1]*b[2] - a[2]*b[1];
	out[1]=a[2]*b[0] - a[0]*b[2];
	out[2]=a[0]*b[1] - a[1]*b[0];
}

// Calculate the 2-norm of a 3-vector, i.e. its Euclidean length
double LA3::norm(const double (&in)[3]) {
	return sqrt(in[0]*in[0] + in[1]*in[1] + in[2]*in[2]);
}

// Normalize a 3-vector to unit length 
void LA3::normalize(double (&out)[3], const double (&in)[3]) {
	double normInv=1.0/norm(in);
	out[0]=in[0]*normInv;
	out[1]=in[1]*normInv;
	out[2]=in[2]*normInv;
}

// Calculate cosine direction vector from two given polar angles (in radians)
void LA3::toDirCos(double (&dc)[3], double ang1, double ang2) {
	dc[0]= cos(ang1)*cos(-ang2);
	dc[1]= cos(ang1)*sin(-ang2);
	dc[2]= sin(ang1);
}

// Calculate polar angles (in radians) from given cosine direction vector
void LA3::toAngles(double &ang1, double &ang2, const double (&dc)[3]) {
	ang1= asin( dc[2]);
	ang2=-atan2(dc[1], dc[0]);
  // 	if(ang2<0)
		//ang2+=2*M_PI;
}

// Copy a 3x3 matrix 
void LA3::copy(double (&out)[3][3], const double (&in)[3][3]) {
	for(unsigned char i=0; i<3; i++)
		for(unsigned char j=0; j<3; j++)
			out[i][j]=in[i][j];
}

// Transpose a 3x3 matrix 
void LA3::transpose(double (&out)[3][3], const double (&in)[3][3]) {
	for(unsigned char i=0; i<3; i++)
		for(unsigned char j=0; j<3; j++)
			out[i][j]=in[j][i];
}

// Calculate the inverse of a given invertable 3x3 matrix
void LA3::invert(double (&mInv)[3][3], const double (&m)[3][3]) {
	double invDet=1.0/determinant(m);

	mInv[0][0]= (m[1][1]*m[2][2] - m[2][1]*m[1][2]) * invDet;
	mInv[0][1]= (m[0][2]*m[2][1] - m[0][1]*m[2][2]) * invDet;
	mInv[0][2]= (m[0][1]*m[1][2] - m[0][2]*m[1][1]) * invDet;
	mInv[1][0]= (m[1][2]*m[2][0] - m[1][0]*m[2][2]) * invDet;
	mInv[1][1]= (m[0][0]*m[2][2] - m[0][2]*m[2][0]) * invDet;
	mInv[1][2]= (m[1][0]*m[0][2] - m[0][0]*m[1][2]) * invDet;
	mInv[2][0]= (m[1][0]*m[2][1] - m[2][0]*m[1][1]) * invDet;
	mInv[2][1]= (m[2][0]*m[0][1] - m[0][0]*m[2][1]) * invDet;
	mInv[2][2]= (m[0][0]*m[1][1] - m[1][0]*m[0][1]) * invDet;
}

// Calculate determinant of a 3x3 matrix
double LA3::determinant(const double (&m)[3][3]) {
	return m[0][0] * ( m[1][1]*m[2][2] - m[2][1]*m[1][2]) -
	       m[0][1] * ( m[1][0]*m[2][2] - m[1][2]*m[2][0]) +
	       m[0][2] * ( m[1][0]*m[2][1] - m[1][1]*m[2][0]);
}

// Multiply two 3x3 matrices
void LA3::multiply(double(&out)[3][3], const double(&a)[3][3], const double(&b)[3][3]) {
	for (unsigned char i = 0; i < 3; i++)
		for (unsigned char j = 0; j < 3; j++) {
			double res = 0;
			for (unsigned char k = 0; k < 3; k++)
				res += a[i][k] * b[k][j];
			out[i][j] = res;
		}
}

// Multiply two 3x3 matrices
void LA3::scalar(double(&a)[3][3], const double s) {
	for (unsigned char i = 0; i < 3; i++)
	{
		for (unsigned char j = 0; j < 3; j++)
		{
			a[i][j] *= s;
		}
	}
}

// Multiply two 3x3 matrices
void LA3::sum(double(&out)[3][3], const double(&a)[3][3], const double(&b)[3][3]) {
  for (unsigned char i = 0; i < 3; i++)
  {
    for (unsigned char j = 0; j < 3; j++)
		{
      out[i][j] = a[i][j] + b[i][j];
    }
  }
}


// Multiply 3x3 matrix with 3-vector
void LA3::multiply(double (&out)[3], const double (&m)[3][3], const double (&v)[3]) {
	for(unsigned char i=0; i<3; i++) {
		double res=0;
		for(unsigned char j=0; j<3; j++)
			res+=m[i][j] * v[j];
		out[i]=res;
	}
}


// get getIdentityMatrix
void LA3::getIdentityMatrix(double(&out)[3][3])
{
	out[0][0] = 1;
	out[1][0] = 0;
	out[2][0] = 0;
	//column 1
	out[0][1] = 0;
	out[1][1] = 1;
	out[2][1] = 0;
	//column 2
	out[0][2] = 0;
	out[1][2] = 0;
	out[2][2] = 1;
}

void LA3::GetTrace(const double(&m)[3][3], double& trace)
{
	trace = m[0][0] + m[1][1] + m[2][2];
	return;
}

void LA3::GetEigenValueFromSymetricMatrix(const double(&m)[3][3], double& eig1, double& eig2, double& eig3)
{
	double p1 = pow(m[0][1],2.)  + pow(m[0][2],2.) + pow(m[1][2],2.);
	if (p1 == 0)
	{
		eig1 = m[0][0];
		eig2 = m[1][1];
		eig3 = m[2][2];
	}
	else 
	{
		double q = 0;
		GetTrace(m, q);
		q /= 3;
		double p2 = pow(m[0][0] - q, 2.) + pow(m[1][1] - q, 2.) + pow(m[2][2] - q, 2) + 2 * p1;
		double p = sqrt(p2 / 6);
		double I[3][3];
		getIdentityMatrix(I);
		double B[3][3];
		for (unsigned char i = 0; i < 3; i++)
		{
			for (unsigned char j = 0; j < 3; j++)
			{
				B[i][j] = 1. / p * (m[i][j] - q * I[i][j]);
			}
		}
		double r = determinant(B)/2.;
		// In exact arithmetic for a symmetric matrix - 1 <= r <= 1
		// but computation error can leave it slightly outside this range.
		double phi = 0;
    if (r <= -1)
    {
      phi = M_PI / 3.;
    }
    else if (r >= 1.)
    {
      phi = 0.;
    }
    else
    {
      phi = acos(r) / 3.;
    }
		// the eigenvalues satisfy eig3 <= eig2 <= eig1
		eig1 = q + 2. * p * cos(phi);
		eig3 = q + 2. * p * cos(phi + (2. * M_PI / 3.));
		eig2 = 3. * q - eig1 - eig3; // since trace(A) = eig1 + eig2 + eig3
	}
}


// get a Matrix after a single rotation
void LA3::getSingleRotationMatrix(double(&out)[3][3], const LA3::SingleRotation sr)
{
	double cosA = cos(sr.angle);
	double sinA = sin(sr.angle);
	switch (sr.axis)
	{
	case LA3::ROTAXISX:
	{
		//column 0
		out[0][0] = 1;
		out[1][0] = 0;
		out[2][0] = 0;
		//column 1
		out[0][1] = 0;
		out[1][1] = cosA;
		out[2][1] = sinA;
		//column 2
		out[0][2] = 0;
		out[1][2] = -sinA;
		out[2][2] = cosA;
	}
	break;
	case LA3::ROTAXISY:
	{
		//column 0
		out[0][0] = cosA;
		out[1][0] = 0;
		out[2][0] = -sinA;
		//column 1
		out[0][1] = 0;
		out[1][1] = 1;
		out[2][1] = 0;
		//column 2
		out[0][2] = sinA;
		out[1][2] = 0;
		out[2][2] = cosA;
	}
	break;
	case LA3::ROTAXISZ:
	{
		//column 0
		out[0][0] = cosA;
		out[1][0] = sinA;
		out[2][0] = 0;
		//column 1
		out[0][1] = -sinA;
		out[1][1] = cosA;
		out[2][1] = 0;
		//column 2
		out[0][2] = 0;
		out[1][2] = 0;
		out[2][2] = 1;
	}
	break;
	}
}

// get a Matrix after a sequence of rotations
void LA3::getMultipleRotationMatrix(double(&out)[3][3], const LA3::SingleRotation* sr, int n)
{
	double next_rotm[3][3];
	double curr_rotm[3][3];
	double prod_rotm[3][3];

	getIdentityMatrix(curr_rotm);

	for (int k = 0; k < n; k++)
	{
		getSingleRotationMatrix(next_rotm, sr[k]);
		multiply(prod_rotm, curr_rotm, next_rotm);
		copy(curr_rotm, prod_rotm);
	}
	copy(out, prod_rotm);
}

// Calculate Euler Angle RX0RYRX1 (in radians) from Matrix()
void LA3::getEulerRx0RyRx1(const double(&r)[3][3], double& thetaX0, double& thetaY, double& thetaX1)
{
	if (r[0][0] < 1)
	{
		if (r[0][0] > -1)
		{
			thetaY = acos(r[0][0]);
			thetaX0 = atan2(r[1][0], -r[2][0]);
			thetaX1 = atan2(r[0][1], r[0][2]);
		}
		else // r00 = -1
		{
			// Not a unique solution : thetaX1 - thetaX0 = atan2(-r12 , r11)
			thetaY = M_PI;
			thetaX0 = -atan2(-r[1][2], r[1][1]);
			thetaX1 = 0;
		}
	}
	else // r00 = +1
	{
		// Not a unique solution : thetaX1 + thetaX0 = atan2(-r12 , r11)
		thetaY = 0;
		thetaX0 = atan2(-r[1][2], r[1][1]);
		thetaX1 = 0;
	}
}

// Calculate Euler Angle (in radians) from Matrix()
void LA3::getEulerRzRxRy(const double(&r)[3][3], double& thetaZ, double& thetaX, double& thetaY)
{
	if (r[2][1] < +1)
	{
		if (r[2][1] > -1)
		{
			thetaX = asin(r[2][1]);
			thetaZ = atan2(-r[0][1], r[1][1]);
			thetaY = atan2(-r[2][0], r[2][2]);
		}
		else // r21 = -1
		{
			// Not a unique solution : thetaY - thetaZ = atan2(r02 , r00 )
			thetaX = -M_PI / 2;
			thetaZ = -atan2(r[0][2], r[0][0]);
			thetaY = 0;
		}
	}
	else // r21 = +1
	{
		// Not a unique solution : thetaY + thetaZ = atan2(r02 , r00 )
		thetaX = +M_PI / 2;
		thetaZ = atan2(r[0][2], r[0][0]);
		thetaY = 0;
	}
}

// Calculate Euler Angle (in radians) from Matrix()
void LA3::getEulerRzRyRx(const double(&r)[3][3], double& thetaZ, double& thetaY, double& thetaX)
{
	if (r[2][0] < +1)
	{
		if (r[2][0] > -1)
		{
			thetaY = asin(-r[2][0]);
			thetaZ = atan2(r[1][0], r[0][0]);
			thetaX = atan2(r[2][1], r[2][2]);
		}
		else // r21 = -1
		{
			// Not a unique solution : thetaX - thetaZ = atan2(-r12 , r11 )
			thetaY = +M_PI / 2;
			thetaZ = -atan2(r[1][2], r[1][1]);
			thetaX = 0;
		}
	}
	else // r21 = +1
	{
		// Not a unique solution : thetaX + thetaZ = atan2(-r12 , r11 )
		thetaY = -M_PI / 2;
		thetaZ = atan2(-r[1][2], r[1][1]);
		thetaX = 0;
	}
}

void LA3::getEulerRxRyRz(const double(&r)[3][3], double& thetaX, double& thetaY, double& thetaZ)
{
  if (r[0][2] < +1)
  {
    if (r[0][2] > -1)
    {
      thetaY = asin(r[0][2]);
      thetaX = atan2(-r[1][2], r[2][2]);
      thetaZ = atan2(-r[0][1], r[0][0]);
    }
    else // r02 = -1
    {
      // Not a unique solution : thetaZ - thetaX = atan2 ( r10 , r11 )
      thetaY = -M_PI / 2;
      thetaX = 0;
      thetaZ = atan2(r[1][0], r[1][1]);
    }
  }
  else // r02 = +1
  {
    // Not a unique solution : thetaZ + thetaX = atan2 ( r10 , r11 )
    thetaY = +M_PI / 2;
    thetaX = 0;
    thetaZ = atan2(r[1][0], r[1][1]);
  }
}

// -----------------------------------------------------------------------------------------------------------------------------
// Coordinate conversion
//Topocentric Apparent convertions
// returns the amount of refraction (in arcminutes) at the given true altitude (degrees), pressure (millibars), and temperature (celsius)
void LA3::Topocentric2Apparent(double& Alt, RefrOpt Opt)
{
	double TPC = (Opt.Pressure / 101.) * (283. / (273. + Opt.Temperature));
	double Beta = Alt + 0.0031375594238031 / (Alt + 0.08918632477691);
	if (Beta > 0 && Opt.use)
	{
		Alt += TPC * 2.96705972839036e-4 / tan(Beta);
	}
}

void LA3::Apparent2Topocentric(double& Alt, RefrOpt Opt)
{
	double TPC = (Opt.Pressure / 101.) * (283. / (273. + Opt.Temperature));
	double Beta = Alt + 0.00222675333864084 / (Alt + 0.0767944870877505);
	if (Beta > 0 && Opt.use)
	{
		Alt -= TPC * 2.908882086657216e-4 / tan(Beta);
	}
}


void LA3::printV(const char *label, const double (&v)[3]) {
#ifdef DEBUG_COUT
	cout << label << " =[ " << v[0] << "\t" << v[1] << "\t" << v[2] << " ]" << endl;   
#endif
}

void LA3::printV(const char *label, const double (&m)[3][3]) {
#ifdef DEBUG_COUT
	cout << label << "\t=[ " << m[0][0] << "\t" << m[0][1] << "\t" << m[0][2] << " ]" << endl   
	              << "\t [ " << m[1][0] << "\t" << m[1][1] << "\t" << m[1][2] << " ]" << endl 
	              << "\t [ " << m[2][0] << "\t" << m[2][1] << "\t" << m[2][2] << " ]" << endl << endl; 
#endif
}
