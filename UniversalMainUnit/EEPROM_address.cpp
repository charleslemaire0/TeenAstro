#include "Global.h"


int getMountAddress(int adress)
{
  return EE_Mounts + MountSize * currentMount + adress;
}

int getMountAddress(int adress, int ndx)
{
  return EE_Mounts + MountSize * ndx + adress;
}

int getSiteAddress(int adress, int ndx)
{
  return EE_sites + SiteSize * ndx + adress;
}
