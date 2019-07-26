// Telescope coordinate conversion
// (C) 2016 Markus L. Noga

// Inspired by:
// Toshimi Taki, "A New Concept in Computer-Aided Telescopes", Sky & Telescope Feb 1989, pp. 194-196
// Wikipedia, siderial time article
// Wikipedia, local hour angle article
// Cosine direction vectors

#include "CoordConv.hpp"

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
   	if(ang2<0)
		ang2+=2*M_PI;
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
void LA3::multiply(double (&out)[3][3], const double (&a)[3][3], const double (&b)[3][3]) {
	for(unsigned char i=0; i<3; i++)
		for(unsigned char j=0; j<3; j++) {
			double res=0;
			for(unsigned char k=0; k<3; k++)
				res+=a[i][k] * b[k][j];
			out[i][j]=res;
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


// HourAngleConv members
//

// conversion factor from local time in seconds, to siderial time in radians
const double HourAngleConv::toSiderial=1.002737909350795*2*M_PI/(24.0*60.0*60.0);  


// CoordConv methods
//

// add a user-provided reference star (all values in degrees, except time in seconds)
void CoordConv::addReferenceDeg(double ra, double decl, double az, double alt, unsigned long t) {
	if(refs==0)
		setT0(t);
	addReference(toHourAngle(toRad(ra),t), toRad(decl), toRad(az), toRad(alt));
}


// Convert equatorial right ascension/dec coordinates to horizontal az/alt coordinates (all values in degrees, except time in seconds) 
void CoordConv::toHorizontalDeg(double &az, double &alt,  double ra,  double decl, unsigned long t) const {
	double azRad=0, altRad=0;
	toHorizontal(azRad, altRad, toHourAngle(toRad(ra),t), toRad(decl));
	az =toDeg(azRad);
	alt=toDeg(altRad);
}

// Convert horizontal az/alt coordinates to  equatorial right ascension/dec coordinates (all values in degrees, except time in seconds) 
void CoordConv::toEquatorialDeg(double &ra,  double &decl, double az, double alt, unsigned long t) const {
	double haRad=0, declRad=0;
	toEquatorial(haRad, declRad, toRad(az), toRad(alt));
    ra=toDeg(toRightAsc(haRad, t));
    decl=toDeg(declRad);
}



// add reference star (all values in radians). adding more than three has no effect
void CoordConv::addReference(double ha, double decl, double az, double alt) {
	if(refs>=3)
		return;

#ifdef DEBUG_COUT
	cout << "adding ref star: ha " << ha << "r decl " << decl << "r az " << az << "r alt " << alt << "r" << endl;
#endif

	toDirCos(dcHDRef[refs], decl, ha);
	printV("dcHD", dcHDRef[refs]);
	toDirCos(dcAARef[refs], alt, az );
	printV("dcAA", dcAARef[refs]);

	refs++;
	if(refs==3)
		buildTransformations();
}

// Calculate third reference star from two provided ones. Returns false if more or less than two provided 
bool CoordConv::calculateThirdReference() {
	if(refs!=2)
		return false;

#ifdef DEBUG_COUT
	cout << "adding artificial 3rd ref star" << endl;
#endif
	crossProduct(dcHDRef[2], dcHDRef[0], dcHDRef[1]);
	normalize   (dcHDRef[2], dcHDRef[2]);
	printV("dcHD", dcHDRef[2]);
	crossProduct(dcAARef[2], dcAARef[0], dcAARef[1]);
	normalize   (dcAARef[2], dcAARef[2]);
	printV("dcAA", dcAARef[2]);

	refs=3;
	buildTransformations();

	return true;
}

// Build coordinate system transformation matrix
void CoordConv::buildTransformations() {
	double dcAARefT[3][3], dcHDRefT[3][3], inv[3][3], test[3][3];

	printV("dcAA ", dcAARef);
	transpose(dcAARefT, dcAARef);
	printV("dcAAt", dcAARefT);

	printV("dcHD ", dcHDRef);
	transpose(dcHDRefT, dcHDRef);
	printV("dcHDt", dcHDRefT);
	invert(inv, dcHDRefT);
	printV("inv", inv);
	multiply(test, dcHDRefT, inv);
	printV("test", test);
	multiply(T, dcAARefT, inv);
	printV("T", T);
	invert(Tinv, T);
	printV("Tinv", Tinv);
	multiply(test, T, Tinv);
	printV("test", test);
}

// Convert equatorial hour angle/dec coordinates to horizontal az/alt coordinates (all values in radians) 
void CoordConv::toHorizontal(double &az, double &alt,  double ha,  double decl) const {
	#ifdef DEBUG_COUT
	cout << "ha " << ha << "r decl " << decl << "r" << endl;
	#endif
	double dcHD[3], dcAA[3];
	toDirCos(dcHD, decl, ha);
	printV("dcHD ", dcHD);
	multiply(dcAA, T, dcHD);
	printV("dcAA ", dcAA);
	normalize(dcAA, dcAA);
	printV("dcAAn", dcAA);
	toAngles(alt, az, dcAA);
	#ifdef DEBUG_COUT
	cout << "az " << az << "r alt " << alt << "r" << endl;
	#endif
}

// Convert horizontal az/alt coordinates to equatorial hour angle/dec coordinates (all values in radians)
void CoordConv::toEquatorial(double &ha,  double &decl, double az, double alt) const {
	double dcAA[3], dcHD[3];
	toDirCos(dcAA, alt, az);
	printV("dcAA ", dcAA);
	multiply(dcHD, Tinv, dcAA);
	printV("dcHD ", dcHD);
	normalize(dcHD, dcHD);
	printV("dcHDN", dcHD);
	toAngles(decl, ha, dcHD);
	#ifdef DEBUG_COUT
	cout << "ha " << ha << "r decl " << decl << "r" << endl;
	#endif
}
