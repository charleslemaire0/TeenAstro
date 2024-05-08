// Telescope coordinate conversion
// (C) 2016 Markus L. Noga
// (C) 2019 Charles Lemaire

// Inspired by:
// Toshimi Taki, "A New Concept in Computer-Aided Telescopes", Sky & Telescope Feb 1989, pp. 194-196
// Wikipedia, siderial time article
// Wikipedia, local hour angle article
// Cosine direction vectors


#include "TeenAstroCoordConv.hpp"

#include <arduino.h>

using namespace std;

// CoordConv methods
//

// get the transformation to be stored into EEPROM
void CoordConv::getT(float& m11, float& m12, float& m13, float& m21, float& m22, float& m23, float& m31, float& m32, float& m33)
{
  m11 = T[0][0];
  m12 = T[0][1];
  m13 = T[0][2];
  m21 = T[1][0];
  m22 = T[1][1];
  m23 = T[1][2];
  m31 = T[2][0];
  m32 = T[2][1];
  m33 = T[2][2];
}

// set the transformation from EEPROM
void CoordConv::setT(float m11, float m12, float m13, float m21, float m22, float m23, float m31, float m32, float m33)
{
  T[0][0] = m11;
  T[0][1] = m12;
  T[0][2] = m13;
  T[1][0] = m21;
  T[1][1] = m22;
  T[1][2] = m23;
  T[2][0] = m31;
  T[2][1] = m32;
  T[2][2] = m33;
  refs = 0;
  isready = true;
}

void CoordConv::setTinvFromT() {
  invert(Tinv, T);
}

// add reference star (all values in radians). adding more than three has no effect
void CoordConv::addReference(double angle1, double angle2, double axis1, double axis2) {
  if (refs >= 2)
    return;

#ifdef DEBUG_COUT
  //cout << "adding ref star: angle1 " << angle1 << "r angle2 " << angle2 << "r axis1 " << axis1 << "r axis2 " << axis2 << "r" << endl;
#endif

  toDirCos(dcHDRef[refs], angle1, angle2);
  printV("dcHD", dcHDRef[refs]);
  toDirCos(dcAARef[refs], axis1, axis2);
  printV("dcAA", dcAARef[refs]);
  ax1[refs] = axis1;
  ax2[refs] = axis2;
  refs++;
  if (refs == 2)
  {
    //check angle between the input observation
    anglediff = angle2Vectors(dcHDRef[0], dcHDRef[1]) - angle2Vectors(dcAARef[0], dcAARef[1]);
    calculateThirdReference();
  }
  else
  {
    isready = false;
  }
}

void CoordConv::minimizeAxis1()
{
  double axis3, axis2, direct_axis1;
  LA3::getEulerRxRyRz(Tinv, axis3, axis2, direct_axis1);
#ifdef DEBUG_COUT
  char txt[512];
  sprintf(txt, "%s \t [%f, %f, %f]", "rotations", axis3 * 180. / PI, axis2 * 180. / PI, direct_axis1 * 180. / PI);
  Serial.println(txt);
#endif
  ax1[0] -= direct_axis1;
  ax1[1] -= direct_axis1;
  toDirCos(dcAARef[0], ax1[0], ax2[0]);
  toDirCos(dcAARef[1], ax1[1], ax2[1]);
  anglediff = angle2Vectors(dcHDRef[0], dcHDRef[1]) - angle2Vectors(dcAARef[0], dcAARef[1]);
  refs = 2;
  calculateThirdReference();
}

void CoordConv::minimizeAxis2()
{
  double delta = 0;
  double ax2init[2] = { ax2[0] , ax2[1] };
  #ifdef DEBUG_COUT
  char txt[512];
  Serial.println("minimizeSync");
  #endif
  for (int k = 0; k < 5; k++)
  {
    double denom = 0;
    ax2[0] += 0.8*delta;
    ax2[1] += 0.8*delta;

    toDirCos(dcAARef[0], ax1[0], ax2[0]);
    toDirCos(dcAARef[1], ax1[1], ax2[1]);
    anglediff = angle2Vectors(dcAARef[0], dcAARef[1]) - angle2Vectors(dcHDRef[0], dcHDRef[1]);

    denom += -dcAARef[0][0] * sin(ax2[1]) * cos(-ax1[1]);
    denom += -dcAARef[1][0] * sin(ax2[0]) * cos(-ax1[0]);
    denom += -dcAARef[0][1] * sin(ax2[1]) * sin(-ax1[1]);
    denom += -dcAARef[1][1] * sin(ax2[0]) * sin(-ax1[0]);
    denom += +dcAARef[0][2] * cos(ax2[1]);
    denom += +dcAARef[1][2] * cos(ax2[0]);

    delta = anglediff / denom;
#ifdef DEBUG_COUT
    sprintf(txt, "interartion %d: \t anglediff %f arcmin,delta = %f arcmin]", k, anglediff * 180. / PI * 60, delta * 180. / PI * 60);
    Serial.println(txt);
#endif
 
  }
  refs = 2;
  calculateThirdReference();
}

// Calculate third reference star from two provided ones. Returns false if more or less than two provided 
bool CoordConv::calculateThirdReference() {
  if (refs != 2)
  {
    isready = false;
    return false;
  }

#ifdef DEBUG_COUT
  //cout << "adding artificial 3rd ref star" << endl;
#endif
  crossProduct(dcHDRef[2], dcHDRef[0], dcHDRef[1]);
  normalize(dcHDRef[2], dcHDRef[2]);
  printV("dcHD", dcHDRef[2]);
  crossProduct(dcAARef[2], dcAARef[0], dcAARef[1]);
  normalize(dcAARef[2], dcAARef[2]);
  printV("dcAA", dcAARef[2]);

  buildTransformations();
  isready = true;
  refs = 0;
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

  // now compute the optimal rotation matrix:
  getsvd(Tinv, u, v);
  printV("u", u);
  printV("v", v);

  double du = determinant(u);
  double dv = determinant(v);

  double tmp[3][3];
  double vt[3][3];
  double diag[3][3];
  transpose(vt, v);
  getIdentityMatrix(diag);

  diag[2][2] = du * dv;
  printV("diag", diag);
  multiply(tmp, diag, vt);
  multiply(Tinv, u, tmp);
  printV("Tinv", Tinv);
  transpose(T, Tinv);
  multiply(test, T, Tinv);
  printV("test", test);
}

