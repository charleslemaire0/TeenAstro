// Telescope coordinate conversion
// (C) 2016 Markus L. Noga
// (C) 2019 Charles Lemaire

// Inspired by:
// Toshimi Taki, "A New Concept in Computer-Aided Telescopes", Sky & Telescope Feb 1989, pp. 194-196
// Wikipedia, siderial time article
// Wikipedia, local hour angle article
// Cosine direction vectors


#include "TeenAstroCoordConv.hpp"

using namespace std;

// CoordConv methods
//

// get the transformation to be stored into EEPROM
void CoordConv::getT(float &m11, float &m12, float &m13,float &m21, float &m22, float &m23,float &m31, float &m32, float &m33)
{
	m11=T[0][0];
	m12=T[0][1];
	m13=T[0][2];
	m21=T[1][0];
	m22=T[1][1];
	m23=T[1][2];
	m31=T[2][0];
	m32=T[2][1];
	m33=T[2][2];
}

// set the transformation from EEPROM
void CoordConv::setT(float m11, float m12, float m13,float m21, float m22, float m23,float m31, float m32, float m33)
{
	T[0][0]=m11;
	T[0][1]=m12;
	T[0][2]=m13;
	T[1][0]=m21;
	T[1][1]=m22;
	T[1][2]=m23;
	T[2][0]=m31;
	T[2][1]=m32;
	T[2][2]=m33;
	refs = 0;
  isready = true;
}

void CoordConv::setTinvFromT() {
	invert(Tinv, T);
}

// add a user-provided reference star (all values in degrees, except time in seconds)
void CoordConv::addReferenceDeg(double angle1, double angle2, double axis1, double axis2) {
	addReference(toRad(angle1), toRad(angle2), toRad(axis1), toRad(axis2));
}


// Convert reference angle1/angle2 coordinates to axis axis1/axis2 coordinates (all values in degrees) 
void CoordConv::toAxisDeg(double &axis1, double &axis2,  double angle1,  double angle2) const {
	double axis1Rad=0, axis2Rad=0;
	toAxis(axis1Rad, axis2Rad, toRad(angle1), toRad(angle2));
	axis1=toDeg(axis1Rad);
	axis2=toDeg(axis2Rad);
}

// Convert axis axis1/axis2 coordinates to reference angle1/angle2 coordinates (all values in degrees) 
void CoordConv::toReferenceDeg(double &angle1,  double &angle2, double axis1, double axis2) const {
	double angle1Rad=0, angle2Rad=0;
	toReference(angle1Rad, angle2Rad, toRad(axis1), toRad(axis2));
    angle1=toDeg(angle1Rad);
    angle2=toDeg(angle2Rad);
}



// add reference star (all values in radians). adding more than three has no effect
void CoordConv::addReference(double angle1, double angle2, double axis1, double axis2) {
	if(refs>=3)
		return;

#ifdef DEBUG_COUT
	cout << "adding ref star: angle1 " << angle1 << "r angle2 " << angle2 << "r axis1 " << axis1 << "r axis2 " << axis2 << "r" << endl;
#endif

	toDirCos(dcHDRef[refs], angle2, angle1);
	printV("dcHD", dcHDRef[refs]);
	toDirCos(dcAARef[refs], axis2, axis1 );
	printV("dcAA", dcAARef[refs]);

	refs++;
	if(refs==3)
  {
		buildTransformations();
    isready = true;
    refs = 0;
  }
  else
	isready = false;
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

	buildTransformations();
  isready = true;
  refs=0;
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

// Convert reference angle1/angle2 coordinates to axis axis1/axis2 coordinates (all values in radians) 
void CoordConv::toAxis(double &axis1, double &axis2,  double angle1,  double angle2) const {
	#ifdef DEBUG_COUT
	cout << "angle1 " << angle1 << "r angle2 " << angle2 << "r" << endl;
	#endif
	double dcHD[3], dcAA[3];
	toDirCos(dcHD, angle2, angle1);
	printV("dcHD ", dcHD);
	multiply(dcAA, T, dcHD);
	printV("dcAA ", dcAA);
	normalize(dcAA, dcAA);
	printV("dcAAn", dcAA);
	toAngles(axis2, axis1, dcAA);
	#ifdef DEBUG_COUT
	cout << "axis1 " << axis1 << "r axis2 " << axis2 << "r" << endl;
	#endif
}

// Convert axis axis1/axis2 coordinates to reference angle1/angle2 coordinates (all values in radians)
void CoordConv::toReference(double &angle1,  double &angle2, double axis1, double axis2) const {
	double dcAA[3], dcHD[3];
	toDirCos(dcAA, axis2, axis1);
	printV("dcAA ", dcAA);
	multiply(dcHD, Tinv, dcAA);
	printV("dcHD ", dcHD);
	normalize(dcHD, dcHD);
	printV("dcHDN", dcHD);
	toAngles(angle2, angle1, dcHD);
	#ifdef DEBUG_COUT
	cout << "angle1 " << angle1 << "r angle2 " << angle2 << "r" << endl;
	#endif
}

double CoordConv::polErrorDeg(double lat, Err sel)
{
// 	lat=(40+49/60+51/3600)*pi/180;x=Tinv*[0;0;1]; x_id=[cos(lat);0;sin(lat)];
// pol_err=acos(x'/norm(x)*x_id)*180/pi
// err_az=atan(x(2)/x(1))*180/pi
// err_alt=(atan(x(3)/x(1))-lat)*pi/180

	//y=[cos(lat);0; sin(lat)]; x=Tinv*[0;0;1];x=x/norm(x);
	//Polerr=acos(x'*y)*180/pi
	lat = toRad(lat);
	double x_id [3]= {cos(lat),0,sin(lat)};
	double x[3] = {Tinv[0][2], Tinv[1][2], Tinv[2][2]};
	normalize(x,x);
	switch (sel)
	{
		case EQ_AZ:
		//err_az
			if(x[0]==0)			
			{
				return x[1] > 0 ?  90.0 :  -90.0;
			}
			return toDeg(atan(x[1]/x[0]));
		break;
		case EQ_ALT:
		//err_alt
			if (x[0] == 0)
			{
				return x[2] > 0 ? 90.0 - toDeg(lat) : -90.0 - toDeg(lat);
			}
			return toDeg(atan(x[2]/x[0])-lat);
		break;
		case POL_W:
		//err_pol
			return toDeg(acos(x[0]*x_id[0]+x[1]*x_id[1]+x[2]*x_id[2]));
		break;
		default:
			return 0;
	}
}
