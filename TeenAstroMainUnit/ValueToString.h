#pragma once
bool dateToYYYYMMDD(int *y1, int *m1, int *d1, char *date);
bool hmsToDouble(double *f, char *hms, bool hP);
bool hmsToHms(int *h1, int *m1, int *m2, int *s1, char*hms, bool hP);
bool doubleToHms(char *reply, double *f, bool highPrecision);
bool dmsToDouble(double *f, char *dms, bool sign_present, bool hP);
bool doubleToDms(char *reply, const double *f, bool fullRange, bool signPresent, bool hP);
