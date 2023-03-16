#include "TeenAstroCoord.hpp"

//
//Coord_EQ::Coord_EQ(double FrE, double Dec, double Ha)
//{
//  m_Eulers[0] = { LA3::RotAxis::ROTAXISX, FrE };
//  m_Eulers[1] = { LA3::RotAxis::ROTAXISY, Dec };
//  m_Eulers[2] = { LA3::RotAxis::ROTAXISZ, Ha };
//};
//Coord_HO Coord_EQ::To_Coord_HO(double Lat)
//{
//  double frh, alt, az;
//  double tmp[3][3];
//  LA3::SingleRotation rots[4] = {
//    m_Eulers[0],
//    m_Eulers[1],
//    m_Eulers[2],
//    {LA3::RotAxis::ROTAXISY, (M_PI_2 - Lat) }
//  };
//  LA3::getMultipleRotationMatrix(tmp, rots, 4);
//  LA3::getEulerRxRyRz(tmp, frh, alt, az);
//  return Coord_HO(frh, alt, az);
//};
//Coord_IN Coord_EQ::To_Coord_IN(double Lat, double missaligment[3][3])
//{
//  return To_Coord_HO(double Lat).To_Coord_IN(missaligment);
//};
//double Coord_EQ::FrE()
//{
//  return m_Eulers[0].angle;
//};
//double Coord_EQ::Dec()
//{
//  return m_Eulers[1].angle;
//};
//double Coord_EQ::Ha()
//{
//  return m_Eulers[2].angle;
//};
//
//
//
//Coord_HO::Coord_HO(double FrH, double Alt, double Az)
//{
//  m_Eulers[0] = { LA3::RotAxis::ROTAXISX, FrH };
//  m_Eulers[1] = { LA3::RotAxis::ROTAXISY, Alt };
//  m_Eulers[2] = { LA3::RotAxis::ROTAXISZ, Az };
//};
//Coord_EQ Coord_HO::To_Coord_EQ(double Lat)
//{
//  double fre, dec, ha;
//  double tmp[3][3];
//  LA3::SingleRotation rots[4] = {
//    m_Eulers[0],
//    m_Eulers[1],
//    m_Eulers[2],
//    {LA3::RotAxis::ROTAXISY, -(M_PI_2 - Lat) }
//  };
//  LA3::getMultipleRotationMatrix(tmp, rots, 4);
//  LA3::getEulerRxRyRz(tmp, fre, dec, ha);
//  return Coord_EQ(fre, dec, ha);
//};
//Coord_IN Coord_HO::To_Coord_IN(double missaligment[3][3])
//{
//  double axis3, axis2, axis1;
//  double tmp1[3][3];
//  double tmp2[3][3];
//  LA3::SingleRotation rots[3] = {
//    m_Eulers[0],
//    m_Eulers[1],
//    m_Eulers[2]
//  };
//  LA3::getMultipleRotationMatrix(tmp1, rots, 3);
//  LA3::multiply(tmp2, tmp1, missaligment);
//  LA3::getEulerRxRyRz(tmp2, axis3, axis2, axis1);
//}
//double Coord_HO::FrH()
//{
//  return m_Eulers[0].angle;
//};
//double Coord_HO::Alt()
//{
//  return m_Eulers[1].angle;
//};
//double Coord_HO::Az()
//{
//  return m_Eulers[2].angle;
//};
//
//
//
//Coord_IN::Coord_IN(double Axis3, double Axis2, double Axis1)
//{
//  m_Eulers[0] = { LA3::RotAxis::ROTAXISX, Axis3 };
//  m_Eulers[1] = { LA3::RotAxis::ROTAXISY, Axis2 };
//  m_Eulers[2] = { LA3::RotAxis::ROTAXISZ, Axis1 };
//};
//Coord_HO Coord_IN::To_Coord_HO(double invmissaligment[3][3])
//{
//  double frh, alt, az;
//  double tmp1[3][3];
//  double tmp2[3][3];
//  LA3::SingleRotation rots[3] = {
//    m_Eulers[0],
//    m_Eulers[1],
//    m_Eulers[2]
//  };
//  LA3::getMultipleRotationMatrix(tmp1, rots, 3);
//  LA3::multiply(tmp2, tmp1, invmissaligment);
//  LA3::getEulerRxRyRz(tmp2, frh, alt, az);
//};
//Coord_EQ Coord_IN::To_Coord_EQ(double invmissaligment[3][3], double Lat)
//{
//  return To_Coord_HO(invmissaligment).To_Coord_EQ(Lat);
//};
//double Coord_IN::Axis3()
//{
//  return m_Eulers[0].angle;
//};
//double Coord_IN::Axis2()
//{
//  return m_Eulers[1].angle;
//};
//double Coord_IN::Axis1()
//{
//  return m_Eulers[2].angle;
//};
