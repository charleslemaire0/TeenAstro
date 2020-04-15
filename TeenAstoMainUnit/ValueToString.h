#pragma once
boolean dateToYYYYMMDD(int *y1, int *m1, int *d1, char *date);
boolean hmsToDouble(double *f, char *hms, bool hP);
boolean hmsToHms(int *h1, int *m1, int *m2, int *s1, char*hms, bool hP);
boolean doubleToHms(char *reply, double *f, bool highPrecision);
boolean dmsToDouble(double *f, char *dms, boolean sign_present, bool hP);
boolean doubleToDms(char *reply, const double *f, bool fullRange, bool signPresent, bool hP);
