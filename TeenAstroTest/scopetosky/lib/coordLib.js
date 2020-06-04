// copyright Mel Bartels, 2011-2014
// see coordLib unitTests.htm for unit tests
// turn on jslint's Tolerate ++ and --

'use strict';

//MLB.coordLib = {};

// IE does not support string.trim(), so add it here if necessary
(function () {
	if (typeof (String.prototype.trim) !== 'function') {
		String.prototype.trim = function () {
			return this.replace(/^\s+|\s+$/g, '');
		};
	}
}());

// bring radian within range of 0 to one revolution (360 degrees or 2 * Math.PI)
exports.validRev = function (rad) {
	var uom = MLB.sharedLib.uom,
	    validatedRad = rad % uom.oneRev;

	if (validatedRad < 0) {
		validatedRad += uom.oneRev;
	}
	return validatedRad;
};

// bring radian within range of -half revolution to half revolution (-180 to 180 degrees or -Math.PI to Math.PI)
exports.validHalfRev = function (rad) {
	var validRev = exports.validRev,
	    validatedRad = validRev(rad),
	    uom = MLB.sharedLib.uom;

	if (validatedRad > uom.halfRev) {
		validatedRad -= uom.oneRev;
	}
	return validatedRad;
};

// bring Declination within range of -90 to +90 degrees
exports.validDec = function (Dec) {
	var halfRevDec,
	    correctedDec,
		uom = MLB.sharedLib.uom,
		validHalfRev = exports.validHalfRev;

	halfRevDec = validHalfRev(Dec);

	if (halfRevDec > uom.qtrRev) {
		// > 90 degrees: reverse scale
		correctedDec = uom.halfRev - halfRevDec;
	} else if (halfRevDec >= -uom.qtrRev) {
		// between -90 and 90 degrees: don't change
		correctedDec = halfRevDec;
	} else {
		// < -90 degrees: reverse negative scale
		correctedDec = -uom.halfRev - halfRevDec;
	}
	return correctedDec;
};

exports.reverseRev = function (value) {
	var uom = MLB.sharedLib.uom;

	return uom.oneRev - value;
};

exports.calcDecIsFlipped = function (Dec) {
	var uom = MLB.sharedLib.uom;

	return Dec > uom.qtrRev || Dec < -uom.qtrRev;
};

exports.flipRa = function (Ra) {
	var uom = MLB.sharedLib.uom,
	    validRev = exports.validRev;

	return validRev(Ra + uom.halfRev);
};

exports.flipDec = function (Dec) {
	var halfRevDec,
	    flippedDec,
		uom = MLB.sharedLib.uom,
		validHalfRev = exports.validHalfRev;

	halfRevDec = validHalfRev(Dec);

	if (halfRevDec >= 0) {
		flippedDec = uom.halfRev - halfRevDec;
	} else {
		flippedDec = -uom.halfRev - halfRevDec;
	}

	return flippedDec;
};

exports.convertHMSMToRad = function (hours, minutes, seconds, milliSeconds) {
	var uom = MLB.sharedLib.uom;

	return (hours + minutes / 60 + seconds / 3600 + milliSeconds / 3600000) * uom.hrToRad;
};

exports.convertDMSMToRad = function (degrees, minutes, seconds, milliSeconds) {
	var absDegrees,
	    absMinutes,
		absSeconds,
		absMilliSeconds,
		uom = MLB.sharedLib.uom;

	if (degrees < 0 || minutes < 0 || seconds < 0 || milliSeconds < 0) {
		absDegrees = -Math.abs(degrees);
		absMinutes = -Math.abs(minutes);
		absSeconds = -Math.abs(seconds);
		absMilliSeconds = -Math.abs(milliSeconds);
	} else {
		absDegrees = Math.abs(degrees);
		absMinutes = Math.abs(minutes);
		absSeconds = Math.abs(seconds);
		absMilliSeconds = Math.abs(milliSeconds);
	}

	return (absDegrees + absMinutes / 60 + absSeconds / 3600 + absMilliSeconds / 3600000) * uom.degToRad;
};

exports.convertRadToHMSM = function (radians) {
	var sign,
	    remainder,
		factor,
		hours,
		minutes,
		seconds,
		milliSeconds,
		int = MLB.sharedLib.int,
		uom = MLB.sharedLib.uom;

	if (radians >= 0) {
		sign = uom.plus;
		remainder = radians;
	} else {
		sign = uom.minus;
		remainder = -radians;
	}

	factor = uom.hrToRad;
	hours = int(remainder / factor);
	remainder -= hours * factor;

	factor /= 60;
	minutes = int(remainder / factor);
	remainder -= minutes * factor;

	factor /= 60;
	seconds = int(remainder / factor);
	remainder -= seconds * factor;

	factor /= 1000;
	milliSeconds = Math.round(remainder / factor);

	if (milliSeconds >= 1000) {
		milliSeconds = 0;
		seconds += 1;
		if (seconds >= 60) {
			seconds = 0;
			minutes++;
			if (minutes >= 60) {
				minutes = 0;
				hours++;
				if (hours >= 24) {
					hours = 0;
				}
			}
		}
	}

	return {
		sign: sign,
		hours: hours,
		minutes: minutes,
		seconds: seconds,
		milliSeconds: milliSeconds
	};
};

exports.convertRadToHMS = function (radians) {
	var sign,
	    remainder,
		factor,
		hours,
		minutes,
		seconds,
		int = MLB.sharedLib.int,
		uom = MLB.sharedLib.uom;

	if (radians >= 0) {
		sign = uom.plus;
		remainder = radians;
	} else {
		sign = uom.minus;
		remainder = -radians;
	}

	factor = uom.hrToRad;
	hours = int(remainder / factor);
	remainder -= hours * factor;

	factor /= 60;
	minutes = int(remainder / factor);
	remainder -= minutes * factor;

	factor /= 60;
	seconds = Math.round(remainder / factor);

	if (seconds >= 60) {
		seconds = 0;
		minutes++;
		if (minutes >= 60) {
			minutes = 0;
			hours++;
			if (hours >= 24) {
				hours = 0;
			}
		}
	}

	return {
		sign: sign,
		hours: hours,
		minutes: minutes,
		seconds: seconds
	};
};

exports.convertRadToDMSM = function (radians) {
	var sign,
	    remainder,
		factor,
		degrees,
		minutes,
		seconds,
		milliSeconds,
		int = MLB.sharedLib.int,
		uom = MLB.sharedLib.uom;

	if (radians >= 0) {
		sign = uom.plus;
		remainder = radians;
	} else {
		sign = uom.minus;
		remainder = -radians;
	}

	factor = uom.degToRad;
	degrees = int(remainder / factor);
	remainder -= degrees * factor;

	factor /= 60;
	minutes = int(remainder / factor);
	remainder -= minutes * factor;

	factor /= 60;
	seconds = int(remainder / factor);
	remainder -= seconds * factor;

	factor /= 1000;
	milliSeconds = Math.round(remainder / factor);

	if (milliSeconds >= 1000) {
		milliSeconds = 0;
		seconds += 1;
		if (seconds >= 60) {
			seconds = 0;
			minutes++;
			if (minutes >= 60) {
				minutes = 0;
				degrees++;
			}
		}
	}

	return {
		sign: sign,
		degrees: degrees,
		minutes: minutes,
		seconds: seconds,
		milliSeconds: milliSeconds
	};
};

exports.convertRadToDMS = function (radians) {
	var sign,
	    remainder,
		factor,
		degrees,
		minutes,
		seconds,
		int = MLB.sharedLib.int,
		uom = MLB.sharedLib.uom;

	if (radians >= 0) {
		sign = uom.plus;
		remainder = radians;
	} else {
		sign = uom.minus;
		remainder = -radians;
	}

	factor = uom.degToRad;
	degrees = int(remainder / factor);
	remainder -= degrees * factor;

	factor /= 60;
	minutes = int(remainder / factor);
	remainder -= minutes * factor;

	factor /= 60;
	seconds = Math.round(remainder / factor);
	remainder -= seconds * factor;

	if (seconds >= 60) {
		seconds = 0;
		minutes++;
		if (minutes >= 60) {
			minutes = 0;
			degrees++;
		}
	}

	return {
		sign: sign,
		degrees: degrees,
		minutes: minutes,
		seconds: seconds
	};
};

exports.convertHMSMToString = function (hmsm, limitToHundredsArcsec) {
	var string = '',
	    int = MLB.sharedLib.int,
		uom = MLB.sharedLib.uom;

	if (hmsm.sign === uom.minus) {
		string += uom.minus;
	}
	string += ('  ' + hmsm.hours).slice(-2);
	string += ':';
	string += ('00' + hmsm.minutes).slice(-2);
	string += ':';
	string += ('00' + hmsm.seconds).slice(-2);
	if (hmsm.milliSeconds !== undefined) {
		string += '.';
		if (limitToHundredsArcsec) {
			string += ('00' + int(hmsm.milliSeconds / 10 + 0.5)).slice(-2);
		} else {
			string += ('000' + hmsm.milliSeconds).slice(-3);
		}
	}

	return string;
};

exports.convertDMSMToString = function (dmsm, limitToTenthsArcsec) {
	var string = '',
	    int = MLB.sharedLib.int,
		uom = MLB.sharedLib.uom;

	if (dmsm.sign === uom.minus) {
		string += uom.minus;
	}
	string += ('   ' + dmsm.degrees).slice(-3);
	string += uom.deg;
	string += ('00' + dmsm.minutes).slice(-2);
	string += uom.min;
	string += ('00' + dmsm.seconds).slice(-2);
	if (dmsm.milliSeconds !== undefined) {
		string += '.';
		if (limitToTenthsArcsec) {
			string += int(dmsm.milliSeconds / 100 + 0.5);
		} else {
			string += ('000' + dmsm.milliSeconds).slice(-3);
		}
	}
	string += uom.sec;

	return string;
};

exports.convertRadiansToHMSMString = function (radians, limitToHundredsArcsec) {
	var convertRadToHMSM = exports.convertRadToHMSM,
	    convertHMSMToString = exports.convertHMSMToString;

	return convertHMSMToString(convertRadToHMSM(radians), limitToHundredsArcsec);
};

exports.convertRadiansToHMSString = function (radians) {
	var convertRadToHMS = exports.convertRadToHMS,
	    convertHMSMToString = exports.convertHMSMToString;

	return convertHMSMToString(convertRadToHMS(radians));
};

exports.convertRadiansToDMSMString = function (radians, limitToTenthsArcsec) {
	var convertRadToDMSM = exports.convertRadToDMSM,
	    convertDMSMToString = exports.convertDMSMToString;

	return convertDMSMToString(convertRadToDMSM(radians), limitToTenthsArcsec);
};

exports.convertRadiansToDMSString = function (radians) {
	var convertRadToDMS = exports.convertRadToDMS,
	    convertDMSMToString = exports.convertDMSMToString;

	return convertDMSMToString(convertRadToDMS(radians));
};

exports.dateFromString = function (string) {
	return new Date(Date.parse(string));
};

exports.getMonth = function (monthNumber) {
	return ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'][monthNumber];
};

exports.getMDY = function (date) {
	var getMonth = exports.getMonth;

	return getMonth(date.getMonth()) + ' ' + date.getDate() + ', ' + date.getFullYear();
};

exports.dateToString = function (date) {
	var convertHMSMToString = exports.convertHMSMToString,
	    getMDY = exports.getMDY,
	    hmsm = {
			hours: date.getHours(),
			minutes: date.getMinutes(),
			seconds: date.getSeconds()
		};

	return getMDY(date) + ' ' + convertHMSMToString(hmsm);
};

exports.dateTodayString = function () {
	var getMDY = exports.getMDY;

	return getMDY(new Date());
};

exports.isLeapYear = function (year) {
	return year % 4 === 0 && (year % 100 !== 0 || year % 400 === 0);
};

// from Meeus
exports.calcDayOfTheYear = function (year, month, day, hours, minutes, seconds, milliseconds) {
	var int = MLB.sharedLib.int,
	    isLeapYear = exports.isLeapYear,
	    k = isLeapYear(year) ? 1 : 2;

	return int(275 * month / 9) - k * int((month + 9) / 12) + day - 30 + hours / 24 + minutes / 1440 + seconds / 86400 + milliseconds / 86400000;
};

// JD = Julian Day; Julian year is calculated from the Julian Date at Jan 1, 2000 (JD=2451545), noon; JDE = Julian Emphemeris Day
exports.calcJulianYear = function (JD) {
	var uom = MLB.sharedLib.uom;

	return (JD - uom.JD2000) / 365.25 + 2000;
};

exports.calcJDFromJulianYear = function (JulianYear) {
	var uom = MLB.sharedLib.uom;

	return (JulianYear - 2000) * 365.25 + uom.JD2000;
};

// from Meeus
// timezone is minus when west of Greenwich;
// Meeus says that factor of 30.6001 can be 30.6
exports.calcJD = function (year, month, day, hours, minutes, seconds, milliseconds, timeZoneOffsetHrs) {
	var a, b, JD, fracJD, int = MLB.sharedLib.int;

	if (month < 3) {
		year--;
		month += 12;
	}
	a = int(year / 100);
	b = 2 - a + int(a / 4);
	JD = int(365.25 * (year + 4716)) + int(30.6001 * (month + 1)) + day + b - 1524.5;

	fracJD = (hours + minutes / 60 + seconds / 3600 + milliseconds / 3600000 - timeZoneOffsetHrs) / 24;

	return JD + fracJD;
};

// timezone is minus when west of Greenwich;
// from Meeus (note that he states that the factor must be 30.6001 and not 30.6, contrary to discussion elsewhere)
exports.calcDateFromJD = function (JD, timeZoneOffsetHrs) {
	var tzJD,
	    z,
		f,
		a,
		i,
		b,
		c,
		d,
		e,
		day,
		month,
		year,
		hrs,
		mins,
		secs,
		millisecs,
		int = MLB.sharedLib.int;

	tzJD = JD + timeZoneOffsetHrs / 24;
	z = int(tzJD + 0.5);
	f = tzJD + 0.5 - z;
	if (z < 2299161) {
		a = z;
	} else {
		i = int((z - 1867216.25) / 36524.25);
		a = z + 1 + i - int(i / 4);
	}
	b = a + 1524;
	c = int((b - 122.1) / 365.25);
	d = int(365.25 * c);
	e = int((b - d) / 30.6001);
	day = int(b - d - int(30.6001 * e) + f);
	if (e < 14) {
		month = e - 1;
	} else {
		month = e - 13;
	}
	if (month > 2) {
		year = c - 4716;
	} else {
		year = c - 4715;
	}

	// round to nearest millisecond by adding in a half of a millisecond before extracting hrs, mins, et al
	f += 0.5 / 86400000;
	hrs = int(f * 24);
	f -= hrs / 24;
	mins = int(f * 1440);
	f -= mins / 1440;
	secs = int(f * 86400);
	f -= secs / 86400;
	millisecs = int(f * 86400000);

	return {
		year: year,
		month: month,
		day: day,
		hours: hrs,
		minutes: mins,
		seconds: secs,
		milliseconds: millisecs
	};
};

/* 
from http://en.wikipedia.org/wiki/Sidereal_time
from http://www2.arnes.si/~gljsentvid10/sidereal.htm, accuracy is 0.1 sec over a century
longitude is minus when west of Greenwich, so longitude is added to the Greenwich local sidereal time to arrive at local sidereal time
agrees (to the accuracy of the system clock) w/: scope.exe, ScopeII/java, CdC, http://tycho.usno.navy.mil/sidereal.html, http://www.jgiesen.de/astro/astroJS/siderealClock/
*/

exports.calcSidTFromJD = function (JD, longitudeDeg) {
	var daysSinceJD2000,
	    GMSTHrs,
	    uom = MLB.sharedLib.uom,
		validRev = exports.validRev;

	daysSinceJD2000 = JD - uom.JD2000;
	GMSTHrs = (18.697374558 + 24.06570982441908 * daysSinceJD2000) % 24;
	return validRev(GMSTHrs * uom.hrToRad + longitudeDeg * uom.degToRad);
};

exports.calcSidT = function (year, month, day, hour, minute, second, millisecond, timeZoneOffsetHrs, longitudeDeg) {
	var calcJD = exports.calcJD,
	    calcSidTFromJD = exports.calcSidTFromJD,
	    JD = calcJD(year, month, day, hour, minute, second, millisecond, timeZoneOffsetHrs);

	return calcSidTFromJD(JD, longitudeDeg);
};

// coordinate coorrections: proper motion, precession, nutation, annual aberration, refraction

exports.calcProperMotion = function (properMotionRA, properMotionDec, deltaJulianYear) {
	return {
		deltaRA: properMotionRA * deltaJulianYear,
		deltaDec: properMotionDec * deltaJulianYear
	};
};

/*
from http://www.seds.org/~spider/spider/ScholarX/coordpCh.html
The star with the largest observed proper motion is 9.7 mag Barnard's Star in Ophiuchus with 10.27 "/y (arc seconds per year).
According to F. Schmeidler, only about 500 stars are known to have proper motions of more than 1 "/y.

from http://www.seds.org/~spider/spider/ScholarX/coordpCh.html#precession
low precision but quick processing routine for precession
high precision routine from Meeus

Precession of the Earth's polar axis is caused by the gravitational pull of the Sun and the Moon on the equatorial
bulge of the flattened rotating Earth. It makes the polar axis precess around the pole of the ecliptic,
with a period of 25,725 years (the so-called Platonic year).
The effect is large enough for changing the equatorial coordinate system significantly in comparatively short times
(therefore, Hipparchus was able to discover it around 130 B.C.).
Sun and moon together give rise to the lunisolar precession p0, while the other planets contribute the
significantly smaller planetary precession p1, which sum up to the general precession p
numerical values for these quantities are (from Schmeidler t is the time in tropical years from 2000.0):
p0 =  50.3878" + 0.000049" * t
p1 = - 0.1055" + 0.000189" * t
p  =  50.2910" + 0.000222" * t

These values give the annual increase of ecliptical longitude for all stars.
The effect on equatorial coordinates is formally more complicated, and approximately given by
RA  = m + n * sin RA * tan Dec
Dec = n * cos RA
(my note: p0,p1,p is per year)
where the constants m and n are the precession components given by
m = + 46.124" + 0.000279" * t
  =   3.0749 s + 0.0000186 s * t
n = + 20.043" - 0.000085" * t
  =   1.3362s - 0.0000056 s * t

Quick vs rigorous calculations:
for 10 yr period:
RA=0deg, discrepancy=-0.017451678342158 arcsec; Dec=0deg, discrepancy=-0.00524010120891173 arcsec
RA=0deg, discrepancy=-4.37426810253779 arcsec; Dec=87deg, discrepancy=-0.00523773447528501 arcsec
*/

// calculates the precession or change in coordinates; 
// to get the precessed coordinate, remember to add the precession change to the original coordinates
exports.calcPrecessionQuick = function (RA, Dec, deltaYr) {
	var m,
	    n,
		deltaRA,
		deltaDec,
		uom = MLB.sharedLib.uom;

	m = 46.124 + 0.000279 * deltaYr / 100;
	n = 20.043 - 0.0085 * deltaYr / 100;
	deltaRA = deltaYr * (m + n * Math.sin(RA) * Math.tan(Dec)) * uom.arcsecToRad;
	deltaDec = deltaYr * n * Math.cos(RA) * uom.arcsecToRad;

	return {
		deltaRA: deltaRA,
		deltaDec: deltaDec
	};
};

exports.calcPrecessionRigorous = function (RA, Dec, startJulianYear, deltaJulianYear) {
	var t1,
	    t2,
		t1Sqr,
		t2Sqr,
		t2Cube,
		eta,
		zeta,
		theta,
		sinTheta,
		cosTheta,
		unflippedDec,
		unflippedRa,
		sinDec,
		cosDec,
		RaPlusEta,
		cosRaPlusEta,
		a,
		b,
		c,
		precessedRA,
		deltaRA,
		acosParm,
		precessedDec,
		deltaDec,
		withinRange = MLB.sharedLib.withinRange,
		uom = MLB.sharedLib.uom,
		validHalfRev = exports.validHalfRev,
		validDec = exports.validDec,
		calcDecIsFlipped = exports.calcDecIsFlipped,
		flipRa = exports.flipRa,
		flipDec = exports.flipDec;

	// from Meeus: t1 and t2 are in Julian centuries where: t1 is difference between starting date and 2000, and t2 is the difference between ending and starting date
	t1 = (startJulianYear - 2000) / 100;
	t2 = deltaJulianYear / 100;
	t1Sqr = t1 * t1;
	t2Sqr = t2 * t2;
	t2Cube = t2Sqr * t2;

	if (t1 === 0) {
		eta = (2306.2181 * t2 + 0.30188 * t2Sqr + 0.017998 * t2Cube) * uom.arcsecToRad;
		zeta = (2306.2181 * t2 + 1.09468 * t2Sqr + 0.018203 * t2Cube) * uom.arcsecToRad;
		theta = (2004.3109 * t2 - 0.42665 * t2Sqr + 0.041883 * t2Cube) * uom.arcsecToRad;
	} else {
		eta = ((2306.2181 + 1.39656 * t1 - 0.000139 * t1Sqr) * t2 + (0.30188 - 0.000344 * t1) * t2Sqr + 0.017998 * t2Cube) * uom.arcsecToRad;
		zeta = ((2306.2181 + 1.39656 * t1 - 0.000139 * t1Sqr) * t2 + (1.09468 + 0.000066 * t1) * t2Sqr + 0.018203 * t2Cube) * uom.arcsecToRad;
		theta = ((2004.3109 - 0.8533 * t1 - 0.000217 * t1Sqr) * t2 - (0.42665 + 0.000217 * t1) * t2Sqr + 0.041883 * t2Cube) * uom.arcsecToRad;
	}
	sinTheta = Math.sin(theta);
	cosTheta = Math.cos(theta);

	// if necessary, bring Dec within range of +-90deg and rotate RA by 12 hrs
	if (calcDecIsFlipped(Dec)) {
		unflippedDec = flipDec(Dec);
		unflippedRa = flipRa(RA);
	} else {
		unflippedDec = Dec;
		unflippedRa = RA;
	}

	sinDec = Math.sin(unflippedDec);
	cosDec = Math.cos(unflippedDec);
	RaPlusEta = RA + eta;
	cosRaPlusEta = Math.cos(RaPlusEta);
	a = cosDec * Math.sin(RaPlusEta);
	b = cosTheta * cosDec * cosRaPlusEta - sinTheta * sinDec;
	c = sinTheta * cosDec * cosRaPlusEta + cosTheta * sinDec;

	// RA
	precessedRA = Math.atan2(a, b) + zeta;
	deltaRA = validHalfRev(precessedRA - unflippedRa);

	// Dec
	// use alternative formula if very close to pole
	if (withinRange(Math.abs(unflippedDec), uom.qtrRev, uom.arcminToRad)) {
		acosParm = Math.sqrt(a * a + b * b);
		precessedDec = Math.acos(acosParm);
	} else {
		precessedDec = Math.asin(c);
	}
	deltaDec = validDec(precessedDec - unflippedDec);

	return {
		deltaRA: deltaRA,
		deltaDec: deltaDec
	};
};

// following methods that calculate nutation and annual aberration are accurate to ~ 0.1 arcseconds

exports.celestialParms = function (JulianYearsSinceJD2000) {
	var t,
	    degToRad,
		arcsecToRad,
		SunMeanLon,
		SunMeanAnomaly,
		SunEquationOfTheCenter,
		SunTrueLon,
		MoonMeanLon,
		MoonMeanAnomaly,
		MoonAscendingNodeLon,
		EarthPerihelionLon,
		eEarthOrbit,
		MeanObliquityEcliptic,
		nutationLon,
		nutationObliquity,
		eclipticObliquity,
		uom = MLB.sharedLib.uom,
		validRev = exports.validRev;

	// Julian centuries since JD2000
	t = JulianYearsSinceJD2000 / 100;
	degToRad = uom.degToRad;
	arcsecToRad = uom.arcsecToRad;

	// Lon = longitude
	SunMeanLon = validRev((280.46646 + 36000.76983 * t) * degToRad);
	MoonMeanLon = validRev((218.3165 + 481267.8813 * t) * degToRad);
	// e = eccentricity
	eEarthOrbit = 0.016708634 - 0.000042037 * t - 0.0000001267 * t * t;
	EarthPerihelionLon = validRev((102.93735 + 1.71946 * t + 0.00046 * t * t) * degToRad);
	SunMeanAnomaly = validRev((357.52772 + 35999.050340 * t - 0.0001603 * t * t - t * t * t / 300000) * degToRad);
	MoonMeanAnomaly = validRev((134.96298 + 477198.867398 * t - 0.0086972 * t * t + t * t * t / 56250) * degToRad);

	MoonAscendingNodeLon = validRev((125.04452 - 1934.136261 * t + 0.0020708 * t * t + t * t * t / 450000) * degToRad);
	MeanObliquityEcliptic = (23.43929 - 46.815 / 3600 * t - 0.00059 / 3600 * t * t + 0.001813 / 3600 * t * t * t) * degToRad;
	SunEquationOfTheCenter = (1.914602 - 0.004817 * t - 0.000014 * t * t) * Math.sin(SunMeanAnomaly) + (0.019993 - 0.000101 * t) * Math.sin(2 * SunMeanAnomaly) + 0.000289 * Math.sin(3 * SunMeanAnomaly);
	SunEquationOfTheCenter *= degToRad;
	SunTrueLon = validRev(SunMeanLon + SunEquationOfTheCenter);

	nutationLon = -17.2 * Math.sin(MoonAscendingNodeLon) - 1.32 * Math.sin(2 * SunMeanLon) - 0.23 * Math.sin(2 * MoonMeanLon) + 0.21 * Math.sin(2 * MoonAscendingNodeLon);
	nutationObliquity = 9.2 * Math.cos(MoonAscendingNodeLon) + 0.57 * Math.cos(2 * SunMeanLon) + 0.1 * Math.cos(2 * MoonMeanLon) - 0.09 * Math.cos(2 * MoonAscendingNodeLon);

	eclipticObliquity = MeanObliquityEcliptic + nutationObliquity * arcsecToRad;

	return {
		eEarthOrbit: eEarthOrbit,
		EarthPerihelionLon: EarthPerihelionLon,
		SunTrueLon: SunTrueLon,
		nutationLon: nutationLon,
		nutationObliquity: nutationObliquity,
		eclipticObliquity: eclipticObliquity
	};
};

exports.calcNutation = function (RA, Dec, eclipticObliquity, nutationLon, nutationObliquity) {
	var cosRA,
	    sinRA,
		tanDec,
		cosEclipticObliquity,
		sinEclipticObliquity,
		deltaRA,
		deltaDec,
		uom = MLB.sharedLib.uom;

	cosRA = Math.cos(RA);
	sinRA = Math.sin(RA);
	tanDec = Math.tan(Dec);
	cosEclipticObliquity = Math.cos(eclipticObliquity);
	sinEclipticObliquity = Math.sin(eclipticObliquity);

	deltaRA = (cosEclipticObliquity + sinEclipticObliquity * sinRA * tanDec) * nutationLon - (cosRA * tanDec) * nutationObliquity;
	deltaRA *= uom.arcsecToRad;

	deltaDec = sinEclipticObliquity * cosRA * nutationLon + sinRA * nutationObliquity;
	deltaDec *= uom.arcsecToRad;

	return {
		deltaRA: deltaRA,
		deltaDec: deltaDec
	};
};

// from Astronomical Algorithms, 2nd edition (published 2000), Meeus
exports.calcAnnualAberration = function (Ra, Dec, SunTrueLon, eclipticObliquity, eEarthOrbit, EarthPerihelionLon) {
	var k = 20.49552,
	    cosRa,
		sinRa,
		cosDec,
		sinDec,
		cosSunTrueLon,
		sinSunTrueLon,
		cosEclipticObliquity,
		tanEclipticObliquity,
		cosEarthPerihelionLon,
		sinEarthPerihelionLon,
		deltaRA,
		deltaDec,
		uom = MLB.sharedLib.uom;

	cosRa = Math.cos(Ra);
	sinRa = Math.sin(Ra);
	cosDec = Math.cos(Dec);
	sinDec = Math.sin(Dec);
	cosSunTrueLon = Math.cos(SunTrueLon);
	sinSunTrueLon = Math.sin(SunTrueLon);
	cosEclipticObliquity = Math.cos(eclipticObliquity);
	tanEclipticObliquity = Math.tan(eclipticObliquity);
	cosEarthPerihelionLon = Math.cos(EarthPerihelionLon);
	sinEarthPerihelionLon = Math.sin(EarthPerihelionLon);

	deltaRA = -k * ((cosRa * cosSunTrueLon * cosEclipticObliquity + sinRa * sinSunTrueLon) / cosDec) + eEarthOrbit * k * ((cosRa * cosEarthPerihelionLon * cosEclipticObliquity + sinRa * sinEarthPerihelionLon) / cosDec);
	deltaRA *= uom.arcsecToRad;

	deltaDec = -k * (cosSunTrueLon * cosEclipticObliquity * (tanEclipticObliquity * cosDec - sinRa * sinDec) + cosRa * sinDec * sinSunTrueLon) + eEarthOrbit * k * (cosEarthPerihelionLon * cosEclipticObliquity * (tanEclipticObliquity * cosDec - sinRa * sinDec) + cosRa * sinDec * sinEarthPerihelionLon);
	deltaDec *= uom.arcsecToRad;

	return {
		deltaRA: deltaRA,
		deltaDec: deltaDec
	};
};

exports.calcProperMotionPrecessionNutationAnnualAberration = function (RA, Dec, coordJD, JD, properMotionRA, properMotionDec) {
	var coordJDYear,
	    JDYear,
		deltaJulianYear,
		JulianYearsSinceJD2000,
		properMotion,
		afterProperMotionRA,
		afterProperMotionDec,
		precession,
		cp,
		nutation,
		annualAberration,
		deltaRA,
		deltaDec,
		calcJulianYear = exports.calcJulianYear,
		calcProperMotion = exports.calcProperMotion,
		calcPrecessionRigorous = exports.calcPrecessionRigorous,
		celestialParms = exports.celestialParms,
		calcNutation = exports.calcNutation,
		calcAnnualAberration = exports.calcAnnualAberration;

	coordJDYear = calcJulianYear(coordJD);
	JDYear = calcJulianYear(JD);
	deltaJulianYear = JDYear - coordJDYear;
	JulianYearsSinceJD2000 = JDYear - 2000;

	properMotion = calcProperMotion(properMotionRA, properMotionDec, deltaJulianYear);
	afterProperMotionRA = RA + properMotion.deltaRA;
	afterProperMotionDec = Dec + properMotion.deltaDec;
	precession = calcPrecessionRigorous(afterProperMotionRA, afterProperMotionDec, coordJDYear, deltaJulianYear);
    cp = celestialParms(JulianYearsSinceJD2000);
	nutation = calcNutation(afterProperMotionRA, afterProperMotionDec, cp.eclipticObliquity, cp.nutationLon, cp.nutationObliquity);
	annualAberration = calcAnnualAberration(afterProperMotionRA, afterProperMotionDec, cp.SunTrueLon, cp.eclipticObliquity, cp.eEarthOrbit, cp.EarthPerihelionLon);

	deltaRA = properMotion.deltaRA + precession.deltaRA + nutation.deltaRA + annualAberration.deltaRA;
	deltaDec = properMotion.deltaDec + precession.deltaDec + nutation.deltaDec + annualAberration.deltaDec;

	return {
		properMotion: properMotion,
		precession: precession,
		nutation: nutation,
		annualAberration: annualAberration,
		total: {
			deltaRA: deltaRA,
			deltaDec: deltaDec
		}
	};
};

exports.Position = function (RA, Dec, az, alt, SidT, HA) {
	this.RA = RA;
	this.Dec = Dec;
	this.az = az;
	this.alt = alt;
	this.SidT = SidT;
	this.HA = HA;
};

exports.calcAngularSepUsingEquat = function (a, z) {
	var deltaHA;

	a.HA = a.SidT - a.RA;
	z.HA = z.SidT - z.RA;
	deltaHA = a.HA - z.HA;

	return Math.acos(Math.sin(a.Dec) * Math.sin(z.Dec) + Math.cos(a.Dec) * Math.cos(z.Dec) * Math.cos(deltaHA));
};

exports.calcAngularSepUsingAltaz = function (a, z) {
	// cos of angle same as cos of -angle, so doesn't matter if deltaAZ is positive or negative
	var deltaAz = a.az - z.az;

	return Math.acos(Math.sin(a.alt) * Math.sin(z.alt) + Math.cos(a.alt) * Math.cos(z.alt) * Math.cos(deltaAz));
};

exports.angSepDiff = function (a, z) {
	var calcAngularSepUsingEquat = exports.calcAngularSepUsingEquat,
	    calcAngularSepUsingAltaz = exports.calcAngularSepUsingAltaz;

	return Math.abs(Math.abs(calcAngularSepUsingEquat(a, z)) - Math.abs(calcAngularSepUsingAltaz(a, z)));
};

// following formulae from http://en.wikipedia.org/wiki/Air_mass_%28astronomy%29, the 2nd formula is more recent

exports.calcAirMass = function (elevationDegrees) {
	var zenithDistanceDeg,
	    zenithDistanceRad,
		uom = MLB.sharedLib.uom;

	zenithDistanceDeg = 90 - elevationDegrees;
	zenithDistanceRad = zenithDistanceDeg * uom.degToRad;
	return 1 / (Math.cos(zenithDistanceRad) + 0.50572 * Math.pow(96.07995 - zenithDistanceDeg, -1.6364));
};

exports.calcAirMass2 = function (apparentElevationDegrees) {
	var uom = MLB.sharedLib.uom;

	return 1 / Math.sin((apparentElevationDegrees + 244 / (165 + 47 * Math.pow(apparentElevationDegrees, 1.1))) * uom.degToRad);
};

/*
refraction makes an object appear higher in the sky than it really is when close to the horizon;
if you are looking at the horizon, then you will be seeing an object that is actually -34.5' below horizon;
an object that is otherwise on the horizon appears to be 34.5 arcminutes above the horizon;
translate apparent->true as 0 -> -34.5 and true->apparent as -34.5 -> 0;
causes tracking rate to slow down the closer the apparent is to the horizon;
if translating equatorial into altazimuth, then add refraction from true to get corrected altitude to point to;
if translating corrected or apparent altazimuth into equatorial, the subtract apparent refraction from altitude, then transform into equatorial;

use interpolation so that refraction can be added, then subtracted yielding the exact original value;

interpolate:
1. find points that the angle fits between
   ex: angle of 10 has end point of r[10,0] and beginning point of r[9,0]
2. get position between end points
   position = (a-bp)/(ep-bp)
   ex: a=1, bp=2, ep=0
	   position = (1-2)/(0-2) = .5
3. apparent->true refraction = amount of refraction at beginning point + position (amount of refract at end point - amount of refract at beg point)
   r = br + p*(er-br), r = br + (a-bp)/(ep-bp)*(er-br), r = br + (a-bp)*(er-br)/(ep-bp)
   ex: br=18, er=34.5
	   r = 18 + .5*(34.5-18) = 26.25 arcmin
4. corrected angle = angle - refraction
   ex: c = a-r = c = 60 arcmin - 26.25 arcmin = 33.75 arcmin

to reverse (true->apparent): have corrected angle of c, find altitude of a;
   ex: c = 60 arcmin - 26.25 arcmin = 33.75 arcmin, solve for a:
1. c = a - r, a = c + r, a = (c+br)(ep-bp) + (a-bp)*(er-br)/(ep-bp),
   a(ep-bp) = c*ep - c*bp + br*ep - br*bp + a*er - a*br - bp*er + bp*br,
   a*ep - a*bp - a*er + a*br = c*ep - c*bp + br*ep - br*bp - bp*er + bp*br,
   a*(ep-bp-er+br) = bp(-c-br-er+br) + ep(c+br),
   a = (bp(-c-er) + ep(c+br)) / (ep-bp-er+br),
   ex: using example from above, convert all units to armin...
	   c=33.75 arcmin
	   br=18
	   er=34.5
	   bp=120
	   ep=0
   a = (120(-33.75-34.5) + 0) / (0-120-34.5+18),
   a = 120*-68.25 / -136.5,
   a = 60 armin

(if refract added to angle, eg, corrected angle = angle + refraction, ie, c=a+r, then use the following to back out the correction:
 to reverse: have corrected angle of ca, find altitude of a
	ex: ca = 1deg + 26.25arcmin = 86.25arcmin, solve for a
 1. ca = a + r, a = ca - r, a = ca - br - (a-bp)*(er-br)/(ep-bp),
	a*(ep-bp)+(a-bp)*(er-br) = (ca-br)*(ep-bp),
	a*ep-a*bp+a*er-a*br-bp*er+bp*br = ca*ep-ca*bp-br*ep+br*bp,
	a*(ep-bp+er-br) = ca*ep-ca*bp-br*ep+br*bp+bp*er-bp*br,
	a*(ep-bp+er-br) = ca*(ep-bp)-br*ep+bp*er,
	a = (ca*(ep-bp)-br*ep+bp*er) / (ep-bp+er-br),
	ex: convert all units to armin...
		a = 86.25-18-(34.5-18)(60-120)/(0-120) = 86.25-18-8.25 = 60 (from 1st line)
		a = (86.25*(0-120)-18*0+120*34.5)/(0-120+34.5-18) = (-10350+4140)/(-103.5) = 60)
*/

exports.refractTable = {
	// interpolation table of refraction in arcminutes per elevation in degrees
	r: [
		[90, 0],
		[60, 0.55],
		[30, 1.7],
		[20, 2.6],
		[15, 3.5],
		[10, 5.2],
		[8, 6.4],
		[6, 8.3],
		[4, 11.5],
		[2, 18],
		[0, 34.5],
		// to allow for true->apparent interpolation when apparent->true results in negative elevation
		[-1, 42.75]
	]
};

exports.setRefractionWorkVars = function (elevationDegrees) {
	var ix,
	    refractTable = exports.refractTable,
	    r = refractTable.r;

	for (ix = 0; ix < r.length - 1; ix++) {
		if (elevationDegrees > r[ix][0]) {
			break;
		}
	}
	return {
		bp: r[ix - 1][0],
		ep: r[ix][0],
		br: r[ix - 1][1],
		er: r[ix][1]
	};
};

// this function calcs refraction to add given a true elevation; apparent elevation = true elevation + refraction
exports.calcRefractionFromTrue = function (trueElevationDegrees) {
	var wv,
	    refractionArcminutes,
		uom = MLB.sharedLib.uom,
		setRefractionWorkVars = exports.setRefractionWorkVars;

	wv = setRefractionWorkVars(trueElevationDegrees);
	refractionArcminutes = wv.br + (trueElevationDegrees - wv.bp) * (wv.er - wv.br) / (wv.ep - wv.bp);
	return refractionArcminutes * uom.arcminToRad;
};

// this function calcs refraction to remove given an apparent elevation; true elevation = apparent elevation - refraction
exports.calcRefractionFromApparent = function (apparentElevationDegrees) {
	var wv,
	    apparentElevationArcminutes,
		bp,
		ep,
		er,
		br,
		trueElevationArcminutes,
		refractionArcminutes,
		uom = MLB.sharedLib.uom,
		setRefractionWorkVars = exports.setRefractionWorkVars;

	wv = setRefractionWorkVars(apparentElevationDegrees);
	// convert deg to arcmin
	apparentElevationArcminutes = apparentElevationDegrees * 60;
	bp = wv.bp * 60;
	ep = wv.ep * 60;
	er = wv.er;
	br = wv.br;
	trueElevationArcminutes = (apparentElevationArcminutes * (ep - bp) - br * ep + bp * er) / (ep - bp + er - br);
	refractionArcminutes = apparentElevationDegrees * 60 - trueElevationArcminutes;
	return refractionArcminutes * uom.arcminToRad;
};

/*
variants:
radians, eg, 1.23456

HMS, eg, 05 14 45
HMS, eg, 05:14:45
HMSM, eg, 05:06:17.045

degree, eg, 123.456

DMS, eg, -08 11 48
DMS, eg, -08:11:48

GoogleSky RA 00h42m44.30s 
GoogleSky Dec +41°16'10.0"

LX200 signed long deg, eg, "-10^05:02#" (^ stands in for the LX200 degree symbol)
LX200 signed short deg, eg, "-10^05.5#" (^ stands in for the LX200 degree symbol)
LX200 long deg, eg, "010^05:02#" (^ stands in for the LX200 degree symbol)
LX200 short deg, eg, "010^05.5#" (^ stands in for the LX200 degree symbol)
LX200 long hr, eg, "05:06:17#" 
LX200 short hr, eg, "05:06#" 

DMS can also be in form of 1[d] 2[m] 3[s] 
where [d] is optional: d, deg, degree, degrees, °, ^
where [m] is optional: m, min, mins, minute, minutes, '
where [s] is optional: s, sec, secs, second, seconds, "
eg, 1d 2m 3s, or 1 deg 2 min 3 sec, or +1°2'3.0"
therefore delimiter list: -> :dms°^'"<-

HMS can also be in form of 1[h] 2[m] 3[s] 
where [h] is optional: h, hr, hrs, hour, hours
where [m] is optional: m, min, mins, minute, minutes
where [s] is optional: s, sec, secs, second, seconds
eg, 1h 2m 3s, or 1 hr 2 min 3 sec
therefore delimiter list: -> :hms<-

rules:
decode to unit of measurement type;
if multiple unit of measurements encountered, then values are added, eg, 2 deg 3 deg results in 5 deg;
if no type: if hours and degrees not filled then number goes to degrees, else it goes to minutes, if minutes already filled then it goes to seconds, if seconds already filled then it's an error,  eg, '1hr 2 3' is 1h 2m 3s, '1 2 3' is 1d 1m 1s, '1h 1d 2 3' is 1h 2m 3s;
hint for hours can be set, in which case undefined numbers are set to hours/minutes/seconds;
values can be in any order, eg, 3 sec 4 min 5 hrs;
not every value need be specified, eg, 5 hrs 3 sec;
if hours or degrees are negative, then minutes and seconds will be made negative also;

coordinate parser design: three levels of parsing:
    lowest level: parse characters into numbers and non-numbers, trimming and removing empty strings
	middle level: create an array of values: radians, degrees, hours, minutes, seconds by inspecting the non-numbers
	highest level: add up the values and return the result in radians
*/

exports.parseCoordinateAddWIP = function (wip, workToAdd) {
	// trim is not implemented in IE --- see the inline code at the top of the file
	workToAdd = workToAdd.trim();
	// throw away empty strings, expected delimiters and orphaned '+'
	if (workToAdd.length > 0 && workToAdd !== ':' && workToAdd !== '#' && workToAdd !== '+') {
		wip.push(workToAdd);
	}
};

exports.parseCoordinateNumberStrategy = function (isNum, isNumMatch, wip, work, c) {
	var parseCoordinateAddWIP = exports.parseCoordinateAddWIP;

	if (isNum[0] !== isNumMatch) {
		parseCoordinateAddWIP(wip, work[0]);
		work[0] = '';
		isNum[0] = !isNum[0];
	}
	work[0] += c;
};

// core lowest level function;
// create tokens, separating out numbers: numbers separated from other numbers by non-numeric characters (ie, white space, words);
// use arrays to box parameters, ie, pass by reference
exports.parseCoordinateIntoStringArray = function (string) {
	var wip = [],
	    wip2 = [],
		stringLength,
		ix,
		wipLength,
		isNumMatch,
		isNum = [false],
		work = [''],
		parseCoordinateAddWIP = exports.parseCoordinateAddWIP,
		parseCoordinateNumberStrategy = exports.parseCoordinateNumberStrategy;

	stringLength = string.length;
	for (ix = 0; ix < stringLength; ix++) {
		isNumMatch = (string[ix].match(/\d|\.|\,|\+|\-/) !== null);
		parseCoordinateNumberStrategy(isNum, isNumMatch, wip, work, string[ix]);
	}
	// don't forget to add in last work in progress
	parseCoordinateAddWIP(wip, work[0]);

	// glue together '-' that is separated from its number, 
	// ie, starting string "- 5", final array is ["-", "5"], need to consolidate into a single value, ["-5"]
	wipLength = wip.length;
	for (ix = 0; ix < wipLength; ix++) {
		// if not last token, if matches '-', if next token is a number
		if (ix < wipLength - 1 && wip[ix] === '-' && !isNaN(parseFloat(wip[ix + 1]))) {
			wip2.push(wip[ix] + wip[ix + 1]);
			ix++;
		} else {
			wip2.push(wip[ix]);
		}
	}

	return wip2;
};

exports.ParseCoordinatesTypes = {
	radians: 0,
	hours: 1,
	degrees: 2,
	minutes: 3,
	seconds: 4
};

exports.ParseCoordinatesLits = [
	['radians', 'radian', 'rads', 'rad', 'r'],
	['hours', 'hour', 'hrs', 'hr', 'h'],
	['degrees', 'degree', 'degs', 'deg', 'd', '°', '^'],
	['minutes', 'minute', 'mins', 'min', 'm', '\''],
	['seconds', 'second', 'secs', 'sec', 's', '"']
];

exports.parseCoordinatesAddError = function (errors, result, nextResult) {
	var errorResult = result;
	if (nextResult !== null) {
		errorResult += ' ' + nextResult;
	}
	errors.push('I do not know what to do with \'' + errorResult + '\'');
};

exports.parseCoordinatesTwoNumbersStrategy = function (result, values, errors, useHours) {
	var valueSet = false,
	    ParseCoordinatesTypes = exports.ParseCoordinatesTypes,
		parseCoordinatesAddError = exports.parseCoordinatesAddError;

	if (useHours === true && values[ParseCoordinatesTypes.hours] === undefined) {
		values[ParseCoordinatesTypes.hours] = result;
		valueSet = true;
	} else if (values[ParseCoordinatesTypes.hours] === undefined && values[ParseCoordinatesTypes.degrees] === undefined) {
		values[ParseCoordinatesTypes.degrees] = result;
		valueSet = true;
	} else if (values[ParseCoordinatesTypes.minutes] === undefined) {
		values[ParseCoordinatesTypes.minutes] = result;
		valueSet = true;
	} else if (values[ParseCoordinatesTypes.seconds] === undefined) {
		values[ParseCoordinatesTypes.seconds] = result;
		valueSet = true;
	}
	if (!valueSet) {
		parseCoordinatesAddError(errors, result, null);
	}
};

exports.parseCoordinatesOneNumberStrategy = function (result, nextResult, values, errors) {
	var valueSet = false,
	    tIx = 0,
		lIx,
		ParseCoordinatesTypes = exports.ParseCoordinatesTypes,
		ParseCoordinatesLits = exports.ParseCoordinatesLits,
		parseCoordinatesAddError = exports.parseCoordinatesAddError;

	while (tIx <= ParseCoordinatesTypes.seconds && !valueSet) {
		lIx = 0;
		while (lIx < ParseCoordinatesLits[tIx].length && !valueSet) {
			if (nextResult === ParseCoordinatesLits[tIx][lIx]) {
				if (values[tIx] === undefined) {
					values[tIx] = result;
				} else {
					values[tIx] += result;
				}
				valueSet = true;
			}
			lIx++;
		}
		tIx++;
	}
	if (!valueSet) {
		parseCoordinatesAddError(errors, result, nextResult);
	}
};

exports.parseCoordinateGetResultIsNum = function (token) {
	var result, isNum;

	result = parseFloat(token);
	isNum = !isNaN(result);
	if (!isNum) {
		result = token;
	}
	return {
		result: result,
		isNum: isNum
	};
};

// core mid level function
exports.parseCoordinate = function (string, useHours) {
	var values = [],
		results,
	    result,
		resultIsNum,
		nextResult,
		nextResultIsNum,
		errors = [],
		sIx = 0,
		parseCoordinateIntoStringArray = exports.parseCoordinateIntoStringArray,
		parseCoordinatesTwoNumbersStrategy = exports.parseCoordinatesTwoNumbersStrategy,
		parseCoordinatesOneNumberStrategy = exports.parseCoordinatesOneNumberStrategy,
		parseCoordinateGetResultIsNum = exports.parseCoordinateGetResultIsNum,
		stringArray = parseCoordinateIntoStringArray(string),
		stringArrayLength = stringArray.length;

	while (sIx < stringArrayLength) {
		// set result, resultIsNum
		results = parseCoordinateGetResultIsNum(stringArray[sIx]);
		result = results.result;
		resultIsNum = results.isNum;
		// set nextResult, nextResultIsNum
		if (sIx < stringArrayLength - 1) {
			results = parseCoordinateGetResultIsNum(stringArray[sIx + 1]);
			nextResult = results.result;
			nextResultIsNum = results.isNum;
		} else {
			nextResult = null;
			nextResultIsNum = null;
		}
		// act on resultIsNum, nextResult, nextResultIsNum
		if (resultIsNum) {
			if (nextResult === null || nextResultIsNum) {
				parseCoordinatesTwoNumbersStrategy(result, values, errors, useHours);
			} else {
				parseCoordinatesOneNumberStrategy(result, nextResult, values, errors);
			}
		}
		sIx++;
	}
	return {
		values: values,
		errors: errors
	};
};

exports.ParseCoordinatesHoursConversion = [
	[1],
	[MLB.sharedLib.uom.hrToRad],
	[MLB.sharedLib.uom.degToRad],
	[MLB.sharedLib.uom.minToRad],
	[MLB.sharedLib.uom.secToRad]
];

exports.ParseCoordinatesDegreesConversion = [
	[1],
	[MLB.sharedLib.uom.hrToRad],
	[MLB.sharedLib.uom.degToRad],
	[MLB.sharedLib.uom.arcminToRad],
	[MLB.sharedLib.uom.arcsecToRad]
];

// if hours or degrees are negative, then the minutes and seconds should be also

exports.isNegNumber = function (num) {
	if (num === 0 && (1 / num) === -Infinity) {
		return true;
	}
	return num < 0;
};

exports.parseCoordinateSetNegativeValues = function (values) {
	var ParseCoordinatesTypes = exports.ParseCoordinatesTypes,
	    isNegNumber = exports.isNegNumber;

	if (isNegNumber(values[ParseCoordinatesTypes.hours]) || isNegNumber(values[ParseCoordinatesTypes.degrees])) {
		if (values[ParseCoordinatesTypes.minutes] > 0) {
			values[ParseCoordinatesTypes.minutes] = -values[ParseCoordinatesTypes.minutes];
		}
		if (values[ParseCoordinatesTypes.seconds] > 0) {
			values[ParseCoordinatesTypes.seconds] = -values[ParseCoordinatesTypes.seconds];
		}
	}
};

// highest level function
exports.parseCoordinateGetValueInRadians = function (string, useHours) {
	var radians = 0,
		hoursFound,
		conversion,
		valuesLength,
		ix,
		value,
		parseCoordinate = exports.parseCoordinate,
	    result = parseCoordinate(string, useHours),
		ParseCoordinatesTypes = exports.ParseCoordinatesTypes,
		ParseCoordinatesHoursConversion = exports.ParseCoordinatesHoursConversion,
		ParseCoordinatesDegreesConversion = exports.ParseCoordinatesDegreesConversion,
		parseCoordinateSetNegativeValues = exports.parseCoordinateSetNegativeValues;

	parseCoordinateSetNegativeValues(result.values);

	hoursFound = result.values[ParseCoordinatesTypes.hours] !== undefined;
	if (hoursFound || useHours === true) {
		conversion = ParseCoordinatesHoursConversion;
	} else {
		conversion = ParseCoordinatesDegreesConversion;
	}
	valuesLength = result.values.length;
	for (ix = 0; ix < valuesLength; ix++) {
		value = result.values[ix];
		if (value !== undefined) {
			radians += value * conversion[ix];
		}
	}

	return {
		radians: radians,
		errors: result.errors
	};
};

/*
Coordinate Transform Notes:

Hour angle increases to the west, Right Ascension increases to the east; formula is HA=SidT-RA;
for example, sidereal time of 10 and RA = 5: object will be 5 hrs to the west and hour angle therefore is: HA = SidT-RA, HA=10-5=5;

Hour angle *offset* has same sign convention as hour angle proper: given that scope thinks that it is pointed to meridian, HAOffset = SidT - actual RA; incorporating HA, complete formula is: HA = SidT-HAOffset-RA, HAOffset=SidT-HA-RA;
for example, scope thinks it is pointed to meridian where meridian = SidT of 10hr, but scope is actually tipped to the east by 1hr and pointing to RA of 11hr: HAOffset=10-0-11=-1hr

Defined alignments for ConvertMatrix (altaz and equat) (90 deg apart):
1st point: scope's celestial pole (faces north in northern hemisphere, faces south in southern hemisphere)
2nd point: intersection of celestial equator and meridian (faces south in northern hemisphere, faces north in southern hemisphere)

********************************************************************************

Coordinate Rules (consequences of the defined alignments, to be applied to all coordinate conversions):
    alt increases from horizon to zenith;
    az always increases clockwise (position your body along the pole with head pointed upward, sweep arms from left to right clockwise) 
		eg, northern hemisphere: east=90, west=270, southern hemisphere: east=270, west=90;
    az of 0 always points towards Earth's closest pole, az of 180 points to Earth's equator;
    tracking motion causes az to reverse direction in southern hemisphere as compared to northern hemisphere;

********************************************************************************

Fabrication Errors:

(follows Taki's convention)

z1: offset of elevation to perpendicular of horizon, ie, one side of rocker box higher than the other; +z1 means that the scope's az axis leans to the right when facing az=0 (north horizon in the northern hemisphere), 
	eg, when z1=0 scope's zenith points to site zenith (RA=0) and when z1=1 scope's zenith points 1 degree east of site zenith when facing north (resulting in RA=1.22 or HA = -1.22, computed to site az=90,alt=89)
	functionally, setting z1 after standard initializing results in same coordinate results as setting z1 before initializing and using the adjusted RA, Dec,
		eg, setPositionDeg(0, 35, 0, 90, 0, 0, cmws.one); setPositionDeg(0, -55, 180, 0, 0, 0, cmws.two); xform.setFabErrorsDeg(1, 0, 0); results in same pointing as
		xform.setFabErrorsDeg(1, 0, 0); setPositionDeg(1.2207, 34.9939, 0, 90, 0, 0, cmws.one); setPositionDeg(0, -55, 180, 0, 0, 0, cmws.two);	

z2: optical axis pointing error in same plane, ie, tube horiz.: optical axis error left to right (horiz); add z2 to setting circles to get true pointing azimuth

z3: correction to zero setting of elevation, ie, vertical offset error (same as altitude offset); add z3 to setting circles to get true pointing altitude

********************************************************************************

Meridian Flip:

if scope is on east side of pier facing west, then flip is defined as 'off' such that no coordinate translation need be done;
if scope is on west side of pier facing east, then flip is active;

if mount is flipped across meridian, then flipped ra differs from original setting circle ra by 12 hrs altitude reading is mirrored across the pole, that is, an alt of 80 is actually 100
(mirrored across 90) as read from the original setting circle orientation;
ie,
northern hemisphere (az increases as scope tracks, az=0 when on meridian):
not flipped:
    1 hr west of meridian (ra < sidT), coord are ra:(sidT-1hr), dec:45, alt:45, az:15
    1 hr east of meridian (ra > sidT), coord are ra:(sidT+1hr), dec:45, alt:45, az:345 (should be flipped)
same coord flipped (scope assumed to have moved the flip, but aimed back at original equat coord):
    1 hr west of meridian (ra < sidT), coord are ra:(sidT-1hr), dec:45, alt:135, az:195 (should be un-flipped)
    1 hr east of meridian (ra > sidT), coord are ra:(sidT+1hr), dec:45, alt:135, az:165
southern hemisphere (az decreases as scope tracks, az=0 when on meridian):
not flipped:
    1 hr west of meridian (ra < sidT), coord are ra:(sidT-1hr), dec:-45, alt:45, az:345
    1 hr east of meridian (ra > sidT), coord are ra:(sidT+1hr), dec:-45, alt:45, az:15 (should be flipped)
same coord flipped (scope assumed to have moved the flip, but aimed back at original equat coord):
    1 hr west of meridian (ra < sidT), coord are ra:(sidT-1hr), dec:-45, alt:135, az:165 (should be un-flipped)
    1 hr east of meridian (ra > sidT), coord are ra:(sidT+1hr), dec:-45, alt:135, az:195

z3 or altitude offset error:
if scope aimed at 70 but setting circles say 60, then z3 = 10;

for meridian flip, use true, not indicated altitude;
northern hemisphere:
    if setting circles = 85 and Z3 = 10 then scope aimed at 95 and is flipped;
southern hemisphere:
    if setting circles = -85 and Z3 = 10 then scope aimed at -75 and is NOT flipped;
    if setting circles = -85 and Z3 = -10 then scope aimed at -95 and is flipped;
	
********************************************************************************

Swinging through the pole:

Some equatorial mounts can take advantage of swinging through the pole, or must swing through the pole, to reach objects just under the pole.
These mounts include fork, German, siderostat and uranostat mounts. Fork mounts can be single or two armed, and may not be able to rotate 360 deg in the primary axis. These mounts must swing through the pole to reach objects underneath the pole.

Swinging through the pole is the equivalent of a meridian flip.

For example, an object 10 degrees above the pole and on the meridian (RA=0. SidT=0, Dec=80) has a scope or motor position of az=180, alt=80.
Moving to an object 10 degrees below the pole (RA=12, SidT=0, Dec=80) results in a scope or motor position of az=0, alt=80 and a movement of az=180, alt=0 deg.
But the shortest distance is actually a move through the pole of 20 deg in alt, resulting in a scope or motor position of az=180, alt=100 and a movement of az=0, alt=20 deg.
This is equivalent to taking a meridian flip on the position below the pole, ie, the flipped position of (az=0, alt=80) becomes az=180, alt=100, the desired swung through the pole position.
A meridian flip is commonly triggered when the scope moves from 'on east pointing west' to 'on west pointing east' and visa versa.
Adding the option to swing through the pole means triggering the meridian flip when the scope would move past azimuth limits of a fork mount that cannot rotate 360 deg in azimuth or possibly when the distance to move is less than 180 deg.
The distance to move being calculated from: either the initial non-flipped position to the final flipped position or the initial flipped position to the final non-flipped position.
*/

exports.AxisTuple = function () {
	this.pri = 0;
	this.sec = 0;
};

exports.setPositionDeg = function (RAdeg, DecDeg, azDeg, altDeg, sidTdeg, HAdeg, position) {
	var uom = MLB.sharedLib.uom;

	position.RA = RAdeg * uom.degToRad;
	position.Dec = DecDeg * uom.degToRad;
	position.az = azDeg * uom.degToRad;
	position.alt = altDeg * uom.degToRad;
	position.SidT = sidTdeg * uom.degToRad;
	position.HA = HAdeg * uom.degToRad;
};

// in order: RA, Dec, azimuth, altitude, sidereal time; all in degrees
exports.setPositionDegFromString = function (string, position) {
	var setPositionDeg = exports.setPositionDeg,
	    stringArray = string.match(/(\+|-)?((\d+(\.\d+)?)|(\.\d+))/g);

	if (stringArray === null) {
		position.RA = undefined;
		position.Dec = undefined;
		position.az = undefined;
		position.alt = undefined;
		position.SidT = undefined;
		position.HA = undefined;
	} else if (stringArray.length === 5) {
		setPositionDeg(stringArray[0], stringArray[1], stringArray[2], stringArray[3], stringArray[4], 0, position);
	} else if (stringArray.length === 6) {
		setPositionDeg(stringArray[0], stringArray[1], stringArray[2], stringArray[3], stringArray[4], stringArray[5], position);
	}
};

exports.copyPosition = function (from, to) {
	to.RA = from.RA;
	to.Dec = from.Dec;
	to.az = from.az;
	to.alt = from.alt;
	to.SidT = from.SidT;
	to.HA = from.HA;
};

exports.clearPosition = function (position) {
	position.RA = undefined;
	position.Dec = undefined;
	position.az = undefined;
	position.alt = undefined;
	position.SidT = undefined;
	position.HA = undefined;
};

exports.positionDegToString = function (position) {
	var uom = MLB.sharedLib.uom;

	return 'RA: ' + position.RA / uom.degToRad + ' Dec: ' + position.Dec / uom.degToRad + ' Az: ' + position.az / uom.degToRad + ' Alt: ' + position.alt / uom.degToRad + ' SidT: ' + position.SidT / uom.degToRad;
};

exports.isValidPosition = function (position) {
	return (position !== undefined) && !isNaN(position.RA) && !isNaN(position.Dec) && !isNaN(position.az) && !isNaN(position.alt) && !isNaN(position.SidT);
};

exports.FabErrors = function () {
	this.z1 = 0;
	this.z2 = 0;
	this.z3 = 0;
};

exports.setFabErrorsDeg = function (z1deg, z2deg, z3deg, fabErrors) {
	var uom = MLB.sharedLib.uom;

	fabErrors.z1 = z1deg * uom.degToRad;
	fabErrors.z2 = z2deg * uom.degToRad;
	fabErrors.z3 = z3deg * uom.degToRad;

	return fabErrors;
};

exports.MeridianFlipStates = {
	onEastSidePointingWest: 'onEastSidePointingWest',
	onWestSidePointingEast: 'onWestSidePointingEast'
};

exports.MeridianFlip = function () {
	var MeridianFlipStates = exports.MeridianFlipStates;

	this.canFlip = false;
	this.state = MeridianFlipStates.onEastSidePointingWest;
	this.flippedState = MeridianFlipStates.onWestSidePointingEast;
	this.swungThroughPole = false;
};

exports.setFlipState = function (isFlipped, meridianFlip) {
	var MeridianFlipStates = exports.MeridianFlipStates;

	if (isFlipped) {
		if (meridianFlip.flippedState === MeridianFlipStates.onEastSidePointingWest) {
			meridianFlip.state = MeridianFlipStates.onEastSidePointingWest;
		} else {
			meridianFlip.state = MeridianFlipStates.onWestSidePointingEast;
		}
	} else {
		if (meridianFlip.flippedState === MeridianFlipStates.onEastSidePointingWest) {
			meridianFlip.state = MeridianFlipStates.onWestSidePointingEast;
		} else {
			meridianFlip.state = MeridianFlipStates.onEastSidePointingWest;
		}
	}
};

exports.isFlipped = function (meridianFlip) {
	if (meridianFlip === undefined || meridianFlip === null) {
		return false;
	}
	return meridianFlip.canFlip && meridianFlip.state === meridianFlip.flippedState;
};

exports.flipAltazAcrossPole = function (position) {
	var uom = MLB.sharedLib.uom,
	    validRev = exports.validRev;

	position.alt = uom.halfRev - position.alt;
	position.az = validRev(position.az + uom.halfRev);
};

exports.translateAltazAcrossPoleBasedOnMeridianFlip = function (position, meridianFlip) {
	var isFlipped = exports.isFlipped,
	    flipAltazAcrossPole = exports.flipAltazAcrossPole;

	if (isFlipped(meridianFlip)) {
		flipAltazAcrossPole(position);
	}
};

exports.calcMeridianFlipFromCurrentAltaz = function (position, fabErrors) {
	var uom = MLB.sharedLib.uom,
	    trueAlt = position.alt + fabErrors.z3;

	return (trueAlt > uom.qtrRev || trueAlt < -uom.qtrRev);
};

// convert coordinates to/from equatorial to alt-azimuth using popular trigometry algorithm

exports.convertPriAxis = function (lat, toSec, fromPri, fromSec) {
	var cosToPri,
	    toPri,
	    uom = MLB.sharedLib.uom,
		validRev = exports.validRev,
		reverseRev = exports.reverseRev;

	// avoid dividing by zero (Math.cos(lat)) when lat = 90 or -90
	if (lat === uom.qtrRev) {
		return validRev(fromPri + uom.halfRev);
	}

	cosToPri = (Math.sin(fromSec) - Math.sin(lat) * Math.sin(toSec)) / (Math.cos(lat) * Math.cos(toSec));
	if (cosToPri < -1) {
		cosToPri = -1;
	} else if (cosToPri > 1) {
		cosToPri = 1;
	}
	toPri = Math.acos(cosToPri);
	// heading east or west of 0 pt?
	if (Math.sin(fromPri) > 0) {
		toPri = reverseRev(toPri);
	}
	return toPri;
};

exports.convertSecAxis = function (lat, fromPri, fromSec) {
	var sinToSec = Math.sin(fromSec) * Math.sin(lat) + Math.cos(fromSec) * Math.cos(lat) * Math.cos(fromPri);

	return Math.asin(sinToSec);
};

/*
set latitude, RA, Dec, SidT (all in radians) before calling;
if southern hemisphere, then flip Dec bef. calc, then flip az after: necessary to conform to coordinate scheme for southern hemisphere
*/
exports.getAltazTrig = function (position, meridianFlip, latitude) {
	var absLatitude,
	    dec,
		holdRa,
		uom = MLB.sharedLib.uom,
		validRev = exports.validRev,
		reverseRev = exports.reverseRev,
		translateAltazAcrossPoleBasedOnMeridianFlip = exports.translateAltazAcrossPoleBasedOnMeridianFlip,
		convertPriAxis = exports.convertPriAxis,
		convertSecAxis = exports.convertSecAxis;

	absLatitude = Math.abs(latitude);
	dec = position.Dec;
	if (latitude < 0) {
		dec = -dec;
	}
	// bring Dec to within -90 to +90 deg, otherwise trig formulae may fail
	holdRa = position.RA;
	if (dec > uom.qtrRev || dec < -uom.qtrRev) {
		dec = uom.halfRev - dec;
		position.RA = validRev(position.RA + uom.halfRev);
	}

	position.HA = position.SidT - position.RA;
	// depends on HA
	position.alt = convertSecAxis(absLatitude, position.HA, dec);
	// depends on HA and Alt calculations
	position.az = convertPriAxis(absLatitude, position.alt, position.HA, dec);
	if (latitude < 0) {
		position.az = reverseRev(position.az);
	}

	position.RA = holdRa;

	// if flipped, then restore 'true' or actual altaz coordinates since input equat and the subsequent coordinate translation is not aware of meridian flip and always results in not flipped coordinate values
	translateAltazAcrossPoleBasedOnMeridianFlip(position, meridianFlip);
};

/*
set latitude, position alt, az, and SidT (all in radians) before calling;
if southern hemisphere, then flip az bef. calc, followed by flip dec after: necessary to conform to coordinate scheme for southern hemisphere
*/
exports.getEquatTrig = function (position, meridianFlip, latitude) {
	var holdAz,
	    holdAlt,
		absLatitude,
		az,
		uom = MLB.sharedLib.uom,
		validRev = exports.validRev,
		reverseRev = exports.reverseRev,
		translateAltazAcrossPoleBasedOnMeridianFlip = exports.translateAltazAcrossPoleBasedOnMeridianFlip,
		convertPriAxis = exports.convertPriAxis,
		convertSecAxis = exports.convertSecAxis;

	// return to equivalent not flipped altaz values for purposes of coordinate translation
	translateAltazAcrossPoleBasedOnMeridianFlip(position, meridianFlip);

	// bring altitude to within -90 to +90 deg, otherwise trig formulae may fail
	holdAz = position.az;
	holdAlt = position.alt;
	if (position.alt > uom.qtrRev || position.alt < -uom.qtrRev) {
		position.alt = uom.halfRev - position.alt;
		position.az = validRev(position.az + uom.halfRev);
	}

	absLatitude = Math.abs(latitude);
	az = position.az;
	if (latitude < 0) {
		az = reverseRev(az);
	}
	position.Dec = convertSecAxis(absLatitude, az, position.alt);
	// depends on Dec
	position.HA = convertPriAxis(absLatitude, position.Dec, az, position.alt);
	// depends on Dec and HA
	position.RA = validRev(position.SidT - position.HA);
	if (latitude < 0) {
		position.Dec = -position.Dec;
	}

	position.az = holdAz;
	position.alt = holdAlt;
};

// convert coordinates to/from equatorial to alt-azimuth using Taki's matrix algorithm

exports.create2DArray = function (dimen) {
	var a = [],
	    ix;

	for (ix = 0; ix < dimen; ix++) {
		a[ix] = [];
	}
	return a;
};

exports.zeroArrays = function (cmws) {
	var i,
	    j;

	for (i = 0; i < cmws.dimen; i++) {
		for (j = 0; j < cmws.dimen; j++) {
			cmws.qq[i][j] = 0;
			cmws.vv[i][j] = 0;
			cmws.rr[i][j] = 0;
			cmws.xx[i][j] = 0;
			cmws.yy[i][j] = 0;
		}
	}
};

exports.ConvertSubroutineType = {
	TakiSimple: 'TakiSimple',         // per Taki eq 5.3-4,
	TakiSmallAngle: 'TakiSmallAngle', // per Taki eq 5.3-2,
	TakiIterative: 'TakiIterative',   // per Taki eq 5.3-5/6,
	BellIterative: 'BellIterative',   // Bell apparent alt, iterative az,
	BellTaki: 'BellTaki'              // Bell apparent alt, Taki iterative az
};

exports.InitType = {
	equatorial: 'equatorial',
	altazimuth: 'altazimuth',
	star: 'star',
	none: 'none'
};

exports.ConvertMatrixWorkingStorage = function () {
	var FabErrors = exports.FabErrors,
	    MeridianFlip = exports.MeridianFlip,
	    Position = exports.Position,
	    create2DArray = exports.create2DArray,
	    zeroArrays = exports.zeroArrays,
		ConvertSubroutineType = exports.ConvertSubroutineType,
		InitType = exports.InitType;

	// constant
	this.dimen = 3;

	// count of iterations needed in z12FuncTakilIterative()
	this.subrTCount = 0;
	// count of iterations needed in z12FuncBellIterative()
	this.subrLCount = 0;
	// initialization state, ie, # of init positions
	this.init = 0;
	// working vars
	this.pri = 0;
	this.sec = 0;
	this.determinate = 0;
	// arrays needed for matrix transforms: 0th element is 1st star, 1st element is 2nd star, 2nd element is 3rd star
	this.qq = create2DArray(this.dimen);
	this.vv = create2DArray(this.dimen);
	this.rr = create2DArray(this.dimen);
	this.xx = create2DArray(this.dimen);
	this.yy = create2DArray(this.dimen);
	zeroArrays(this);

	this.current = new Position();
	this.one = new Position();
	this.two = new Position();
	this.three = new Position();
	this.initMatrixPosition = new Position();
	this.recoverInitPosition = new Position();
	this.initAltazResultsPosition = new Position();
	this.presetPosition = new Position();
	this.calcPointingErrorPosition = new Position();
	this.bestZ12Position = new Position();

	this.fabErrors = new FabErrors();
	this.meridianFlip = new MeridianFlip();
	this.convertSubrType = ConvertSubroutineType.BellTaki;
	this.initType = InitType.none;
};

/*
per Larry Bell's derivation;
FabErrors.z1 rotation done between alt and az rotations so no closed algebraic solution, instead, search iteratively;
apparent coordinates are what the encoders see, and are our goal;
*/
exports.getApparentAlt = function (cosZ1, cosZ2, sinZ1, sinZ2, cmws) {
	var v1 = (Math.sin(cmws.sec) - sinZ1 * sinZ2) * cosZ1 * (cosZ2 / ((sinZ1 * sinZ1 - 1) * (sinZ2 * sinZ2 - 1)));

	return Math.asin(v1);
};

exports.angleSubr = function (cmws) {
	var uom = MLB.sharedLib.uom,
	    validRev = exports.validRev,
		reverseRev = exports.reverseRev,
	    c = Math.sqrt(cmws.yy[0][1] * cmws.yy[0][1] + cmws.yy[1][1] * cmws.yy[1][1]);

	if (c === 0 && cmws.yy[2][1] > 0) {
		cmws.sec = uom.qtrRev;
	} else if (c === 0 && cmws.yy[2][1] < 0) {
		cmws.sec = -uom.qtrRev;
	} else if (c !== 0) {
		cmws.sec = Math.atan(cmws.yy[2][1] / c);
	} else {
		cmws.sec = 0;
		//throw new Error('undetermined cmws.sec in angleSubr()');
	}

	if (c === 0) {
		// cmws.pri should be indeterminate: Taki program listing is cmws.pri = 1000 degrees (maybe to note this situation?)
		cmws.pri = 0;
		//throw new Error('undetermined cmws.pri in angleSubr()');
	} else if (c !== 0 && cmws.yy[0][1] === 0 && cmws.yy[1][1] > 0) {
		cmws.pri = uom.qtrRev;
	} else if (c !== 0 && cmws.yy[0][1] === 0 && cmws.yy[1][1] < 0) {
		cmws.pri = reverseRev(uom.qtrRev);
	} else if (cmws.yy[0][1] > 0) {
		cmws.pri = Math.atan(cmws.yy[1][1] / cmws.yy[0][1]);
	} else if (cmws.yy[0][1] < 0) {
		cmws.pri = Math.atan(cmws.yy[1][1] / cmws.yy[0][1]) + uom.halfRev;
	} else {
		cmws.pri = 0;
		//throw new Error('undetermined cmws.pri in angleSubr()');
	}

	cmws.pri = validRev(cmws.pri);
};

//per Taki's eq 5.3-4
exports.z12FuncTakiSimple = function (cosPri, cosSec, sinPri, sinSec, cmws) {
	cmws.yy[0][1] = cosSec * cosPri + cmws.fabErrors.z2 * sinPri - cmws.fabErrors.z1 * sinSec * sinPri;
	cmws.yy[1][1] = cosSec * sinPri - cmws.fabErrors.z2 * cosPri - cmws.fabErrors.z1 * sinSec * cosPri;
	cmws.yy[2][1] = sinSec;
};

//per Taki's eq 5.3-2
exports.z12FuncTakiSmallAngle = function (cosPri, cosSec, sinPri, sinSec, cosZ1, cosZ2, sinZ1, sinZ2, cmws) {
	cmws.yy[0][1] = (cosSec * cosPri + sinPri * cosZ1 * sinZ2 - sinSec * sinPri * sinZ1 * cosZ2) / cosZ2;
	cmws.yy[1][1] = (cosSec * sinPri - cosPri * cosZ1 * sinZ2 + sinSec * cosPri * sinZ1 * cosZ2) / cosZ2;
	cmws.yy[2][1] = (sinSec - sinZ1 * sinZ2) / (cosZ1 * cosZ2);
};

//from Larry Bell's derivation of iterative solution to FabErrors.z1, FabErrors.z2
exports.z12FuncBellIterative = function (cosPri, cosSec, sinPri, sinSec, cosZ1, cosZ2, sinZ1, sinZ2, cmws) {
	var trueAz,
	    tanTrueAz,
		apparentAlt,
		apparentAz,
		bestApparentAz,
		cosApparentSec,
		sinApparentSec,
		g,
		h,
		goalSeek,
		holdGoalSeek,
		incr,
		minIncr,
		dir,
		subrLCount,
		uom = MLB.sharedLib.uom,
		getApparentAlt = exports.getApparentAlt,
		angleSubr = exports.angleSubr,
		z12FuncTakiSmallAngle = exports.z12FuncTakiSmallAngle;

	trueAz = cmws.pri;
	tanTrueAz = Math.tan(trueAz);

	apparentAlt = getApparentAlt(cosZ1, cosZ2, sinZ1, sinZ2, cmws);

	cosApparentSec = Math.cos(apparentAlt);
	sinApparentSec = Math.sin(apparentAlt);
	g = cosZ2 * sinZ1 * sinApparentSec * tanTrueAz - tanTrueAz * sinZ2 * cosZ1 - cosZ2 * cosApparentSec;
	h = sinZ2 * cosZ1 - cosZ2 * sinZ1 * sinApparentSec - tanTrueAz * cosZ2 * cosApparentSec;

	// start with best guess using Taki's subroutine 'B' for apparentAz
	z12FuncTakiSmallAngle(cosPri, cosSec, sinPri, sinSec, cosZ1, cosZ2, sinZ1, sinZ2, cmws);
	angleSubr(cmws);
	apparentAz = cmws.pri;

	// iteratively solve for best apparent azimuth by searching for a goal of 0 for goalSeek
	bestApparentAz = apparentAz;
	holdGoalSeek = Number.MAX_VALUE;
	incr = uom.arcminToRad;
	minIncr = uom.arcsecToRad;
	dir = true;
	subrLCount = 0;
	do {
		if (dir) {
			apparentAz += incr;
		} else {
			apparentAz -= incr;
		}

		goalSeek = g * Math.sin(apparentAz) - h * Math.cos(apparentAz);

		if (Math.abs(goalSeek) <= Math.abs(holdGoalSeek)) {
			bestApparentAz = apparentAz;
		} else {
			// GoakSeek getting worse, so reverse direction and cut increment by half
			incr /= 2;
			dir = !dir;
		}
		holdGoalSeek = goalSeek;
		subrLCount++;
	} while (incr >= minIncr);

	cosPri = Math.cos(bestApparentAz);
	sinPri = Math.sin(bestApparentAz);
	cosSec = Math.cos(apparentAlt);
	sinSec = Math.sin(apparentAlt);

	cmws.yy[0][1] = cosPri * cosSec;
	cmws.yy[1][1] = sinPri * cosSec;
	cmws.yy[2][1] = sinSec;
};

/*
per Taki's eq 5.3-5/6 (Taki says 2 loops sufficient for z errors of 1 deg),
FabErrors.z1=1, FabErrors.z2=-1, Faberrors.z3=1, alt/az=88/100 loops needed 6 FabErrors.z1=2, FabErrors.z2=-2, Faberrors.z3=0, alt/az=90/100 loops needed 22
will not converge if .dec or .alt = 90 deg and FabErrors.z12 non-zero and equat init adopted (could be because of poor initial guess by z12FuncTakiSmallAngle())
*/
exports.z12FuncTakilIterative = function (cosPri, cosSec, sinPri, sinSec, cosZ1, cosZ2, sinZ1, sinZ2, cmws) {
	var MaxLoopCount,
	    holdPri,
		holdSec,
		last,
		err,
		subrTCount,
		cosF1,
		sinF1,
		uom = MLB.sharedLib.uom,
		AxisTuple = exports.AxisTuple,
		angleSubr = exports.angleSubr,
		z12FuncTakiSmallAngle = exports.z12FuncTakiSmallAngle,
		z12FuncBellIterative = exports.z12FuncBellIterative;

	MaxLoopCount = 25;
	holdPri = cmws.pri;
	holdSec = cmws.sec;
	last = new AxisTuple();
	err = new AxisTuple();

	// so as to not make the error be invalid later
	last.sec = Number.MAX_VALUE / 2;
	last.pri = Number.MAX_VALUE / 2;
	subrTCount = 0;

	// start with best guess using Taki's 'subroutine b'
	z12FuncTakiSmallAngle(cosPri, cosSec, sinPri, sinSec, cosZ1, cosZ2, sinZ1, sinZ2, cmws);
	do {
		angleSubr(cmws);

		err.sec = Math.abs(last.sec - cmws.sec);
		err.pri = Math.abs(last.pri - cmws.pri);

		last.sec = cmws.sec;
		last.pri = cmws.pri;

		cosF1 = Math.cos(cmws.pri);
		sinF1 = Math.sin(cmws.pri);

		cmws.yy[0][1] = (cosSec * cosPri + sinF1 * cosZ1 * sinZ2 - (sinSec - sinZ1 * sinZ2) * sinF1 * sinZ1 / cosZ1) / cosZ2;
		cmws.yy[1][1] = (cosSec * sinPri - cosF1 * cosZ1 * sinZ2 + (sinSec - sinZ1 * sinZ2) * cosF1 * sinZ1 / cosZ1) / cosZ2;
		cmws.yy[2][1] = (sinSec - sinZ1 * sinZ2) / (cosZ1 * cosZ2);

		subrTCount++;
		if (subrTCount > MaxLoopCount) {
			//switching from z12FuncTakilIterative() to z12FuncBellIterative()...
			cmws.pri = holdPri;
			cmws.sec = holdSec;
			z12FuncBellIterative(cosPri, cosSec, sinPri, sinSec, cosZ1, cosZ2, sinZ1, sinZ2, cmws);
		}
	} while (err.sec > uom.TenthsArcsecToRad || err.pri > uom.TenthsArcsecToRad);
};

//use apparent alt derivation from Larry Bell, apparent az from Taki's iterative solution
exports.z12FuncBellTakilIterative = function (cosPri, cosSec, sinPri, sinSec, cosZ1, cosZ2, sinZ1, sinZ2, cmws) {
	var getApparentAlt = exports.getApparentAlt,
	    angleSubr = exports.angleSubr,
		z12FuncTakilIterative = exports.z12FuncTakilIterative,
	    apparentAlt = getApparentAlt(cosZ1, cosZ2, sinZ1, sinZ2, cmws);

	z12FuncTakilIterative(cosPri, cosSec, sinPri, sinSec, cosZ1, cosZ2, sinZ1, sinZ2, cmws);
	angleSubr(cmws);

	cosSec = Math.cos(apparentAlt);
	sinSec = Math.sin(apparentAlt);
	cosPri = Math.cos(cmws.pri);
	sinPri = Math.sin(cmws.pri);

	cmws.yy[0][1] = cosPri * cosSec;
	cmws.yy[1][1] = sinPri * cosSec;
	cmws.yy[2][1] = sinSec;
};

exports.subrSwitcher = function (cmws) {
	var cosPri,
	    cosSec,
		sinPri,
		sinSec,
		cosZ1,
		cosZ2,
		sinZ1,
		sinZ2,
		ConvertSubroutineType = exports.ConvertSubroutineType,
		z12FuncTakiSimple = exports.z12FuncTakiSimple,
		z12FuncTakiSmallAngle = exports.z12FuncTakiSmallAngle,
		z12FuncBellIterative = exports.z12FuncBellIterative,
		z12FuncTakilIterative = exports.z12FuncTakilIterative,
		z12FuncBellTakilIterative = exports.z12FuncBellTakilIterative;

	cosPri = Math.cos(cmws.pri);
	cosSec = Math.cos(cmws.sec);
	sinPri = Math.sin(cmws.pri);
	sinSec = Math.sin(cmws.sec);

	if (cmws.fabErrors.z1 === 0 && cmws.fabErrors.z2 === 0) {
		cmws.yy[0][1] = cosPri * cosSec;
		cmws.yy[1][1] = sinPri * cosSec;
		cmws.yy[2][1] = sinSec;
	} else {
		cosZ1 = Math.cos(cmws.fabErrors.z1);
		cosZ2 = Math.cos(cmws.fabErrors.z2);
		sinZ1 = Math.sin(cmws.fabErrors.z1);
		sinZ2 = Math.sin(cmws.fabErrors.z2);
		if (cmws.convertSubrType === ConvertSubroutineType.TakiSimple) {
			z12FuncTakiSimple(cosPri, cosSec, sinPri, sinSec, cmws);
		} else if (cmws.convertSubrType === ConvertSubroutineType.TakiSmallAngle) {
			z12FuncTakiSmallAngle(cosPri, cosSec, sinPri, sinSec, cosZ1, cosZ2, sinZ1, sinZ2, cmws);
		} else if (cmws.convertSubrType === ConvertSubroutineType.BellIterative) {
			z12FuncBellIterative(cosPri, cosSec, sinPri, sinSec, cosZ1, cosZ2, sinZ1, sinZ2, cmws);
		} else if (cmws.convertSubrType === ConvertSubroutineType.TakiIterative) {
			z12FuncTakilIterative(cosPri, cosSec, sinPri, sinSec, cosZ1, cosZ2, sinZ1, sinZ2, cmws);
		} else if (cmws.convertSubrType === ConvertSubroutineType.BellTaki) {
			z12FuncBellTakilIterative(cosPri, cosSec, sinPri, sinSec, cosZ1, cosZ2, sinZ1, sinZ2, cmws);
		}
	}
};

exports.determinateSubr = function (cmws) {
	cmws.determinate = cmws.vv[0][1] * cmws.vv[1][2] * cmws.vv[2][3] + cmws.vv[0][2] * cmws.vv[1][3] * cmws.vv[2][1]
		+ cmws.vv[0][3] * cmws.vv[2][2] * cmws.vv[1][1] - cmws.vv[0][3] * cmws.vv[1][2] * cmws.vv[2][1]
		- cmws.vv[0][1] * cmws.vv[2][2] * cmws.vv[1][3] - cmws.vv[0][2] * cmws.vv[1][1] * cmws.vv[2][3];
};

exports.calcRectCoords = function (cmws) {
	var cosPri,
	    cosSec,
		cosZ1,
		cosZ2,
		sinPri,
		sinSec,
		sinZ1,
		sinZ2;

	cosPri = Math.cos(cmws.pri);
	cosSec = Math.cos(cmws.sec);
	sinPri = Math.sin(cmws.pri);
	sinSec = Math.sin(cmws.sec);

	if (cmws.fabErrors.z1 === 0 && cmws.fabErrors.z2 === 0) {
		cmws.yy[0][0] = cosPri * cosSec;
		cmws.yy[1][0] = sinPri * cosSec;
		cmws.yy[2][0] = sinSec;
	} else {
		cosZ1 = Math.cos(cmws.fabErrors.z1);
		cosZ2 = Math.cos(cmws.fabErrors.z2);
		sinZ1 = Math.sin(cmws.fabErrors.z1);
		sinZ2 = Math.sin(cmws.fabErrors.z2);
		cmws.yy[0][0] = cosPri * cosSec * cosZ2 - sinPri * cosZ1 * sinZ2 + sinPri * sinSec * sinZ1 * cosZ2;
		cmws.yy[1][0] = sinPri * cosSec * cosZ2 + cosPri * sinZ2 * cosZ1 - cosPri * sinSec * sinZ1 * cosZ2;
		cmws.yy[2][0] = sinSec * cosZ1 * cosZ2 + sinZ1 * sinZ2;
	}
};

exports.getAltazMatrix = function (cmws) {
	var i,
	    j,
		HA,
		cosDec,
		sinDec,
		validRev = exports.validRev,
		reverseRev = exports.reverseRev,
		translateAltazAcrossPoleBasedOnMeridianFlip = exports.translateAltazAcrossPoleBasedOnMeridianFlip,
		angleSubr = exports.angleSubr,
		subrSwitcher = exports.subrSwitcher;

	// HA is CCW so this HA formula is written backwards
	HA = cmws.current.RA - cmws.current.SidT;
	// convert to rectangular coordinates and put in xx

	cosDec = Math.cos(cmws.current.Dec);
	sinDec = Math.sin(cmws.current.Dec);
	cmws.xx[0][1] = cosDec * Math.cos(HA);
	cmws.xx[1][1] = cosDec * Math.sin(HA);
	cmws.xx[2][1] = sinDec;
	cmws.yy[0][1] = 0;
	cmws.yy[1][1] = 0;
	cmws.yy[2][1] = 0;
	// mutiply xx by transform matrix rr to get equatorial rectangular coordinates
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			cmws.yy[i][1] += cmws.rr[i][j + 1] * cmws.xx[j][1];
		}
	}
	// convert to celestial coordinates
	angleSubr(cmws);
	// modify for non-zero z1z2z3 mount error values
	subrSwitcher(cmws);
	angleSubr(cmws);
	cmws.current.alt = cmws.sec;
	// convert azimuth from CCW to CW
	cmws.current.az = reverseRev(validRev(cmws.pri));
	// if flipped then restore true or actual altaz coordinates since input equat and the subsequent
	// coordinate translation is not aware of meridian flip and always results in not flipped coordinate values
	translateAltazAcrossPoleBasedOnMeridianFlip(cmws.current, cmws.meridianFlip);
	// adjust altitude: this should occur after meridian flip adjustment - see notes in Mounting.MeridianFlip
	cmws.current.alt -= cmws.fabErrors.z3;
};

exports.getEquatMatrix = function (cmws) {
	var i,
	    j,
		holdAlt,
		holdAz,
		validRev = exports.validRev,
		reverseRev = exports.reverseRev,
		translateAltazAcrossPoleBasedOnMeridianFlip = exports.translateAltazAcrossPoleBasedOnMeridianFlip,
		angleSubr = exports.angleSubr,
		calcRectCoords = exports.calcRectCoords;

	holdAlt = cmws.current.alt;
	holdAz = cmws.current.az;
	cmws.current.alt += cmws.fabErrors.z3;
	// return to equivalent not flipped altaz values for purposes of coordinate translation
	translateAltazAcrossPoleBasedOnMeridianFlip(cmws.current, cmws.meridianFlip);
	cmws.sec = cmws.current.alt;
	// convert from CW to CCW az
	cmws.pri = reverseRev(cmws.current.az);
	cmws.current.alt = holdAlt;
	cmws.current.az = holdAz;

	calcRectCoords(cmws);
	cmws.xx[0][1] = cmws.yy[0][0];
	cmws.xx[1][1] = cmws.yy[1][0];
	cmws.xx[2][1] = cmws.yy[2][0];
	cmws.yy[0][1] = 0;
	cmws.yy[1][1] = 0;
	cmws.yy[2][1] = 0;
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			cmws.yy[i][1] += (cmws.qq[i][j + 1] * cmws.xx[j][1]);
		}
	}
	angleSubr(cmws);
	cmws.pri += cmws.current.SidT;
	cmws.current.RA = validRev(cmws.pri);
	cmws.current.Dec = cmws.sec;
};

exports.arrayAssignInit = function (init, cmws) {
	var HA,
	    cosDec,
		sinDec,
		reverseRev = exports.reverseRev,
		zeroArrays = exports.zeroArrays,
		calcRectCoords = exports.calcRectCoords;

	if (init === 1) {
		zeroArrays(cmws);
	}

	// HA is CCW so ha formula backwards
	HA = cmws.current.RA - cmws.current.SidT;
	cosDec = Math.cos(cmws.current.Dec);
	sinDec = Math.sin(cmws.current.Dec);
	// xx is telescope matrix convert parameters into rectangular (cartesian) coordinates
	cmws.xx[0][init] = cosDec * Math.cos(HA);
	cmws.xx[1][init] = cosDec * Math.sin(HA);
	cmws.xx[2][init] = sinDec;
	// pri axis is CCW
	cmws.pri = reverseRev(cmws.current.az);
	cmws.sec = cmws.current.alt + cmws.fabErrors.z3;
	calcRectCoords(cmws);
	// yy is celestial matrix convert parameters into rectangular (cartesian) coordinates
	cmws.yy[0][init] = cmws.yy[0][0];
	cmws.yy[1][init] = cmws.yy[1][0];
	cmws.yy[2][init] = cmws.yy[2][0];
};

exports.generateThirdInit = function (cmws) {
	var i,
	    a;

	// generate 3rd initialization point from the first two using vector product formula
	cmws.xx[0][3] = cmws.xx[1][1] * cmws.xx[2][2] - cmws.xx[2][1] * cmws.xx[1][2];
	cmws.xx[1][3] = cmws.xx[2][1] * cmws.xx[0][2] - cmws.xx[0][1] * cmws.xx[2][2];
	cmws.xx[2][3] = cmws.xx[0][1] * cmws.xx[1][2] - cmws.xx[1][1] * cmws.xx[0][2];
	a = Math.sqrt(cmws.xx[0][3] * cmws.xx[0][3] + cmws.xx[1][3] * cmws.xx[1][3] + cmws.xx[2][3] * cmws.xx[2][3]);
	for (i = 0; i < 3; i++) {
		if (a === 0) {
			cmws.xx[i][3] = Number.Max_Value;
		} else {
			cmws.xx[i][3] /= a;
		}
	}

	cmws.yy[0][3] = cmws.yy[1][1] * cmws.yy[2][2] - cmws.yy[2][1] * cmws.yy[1][2];
	cmws.yy[1][3] = cmws.yy[2][1] * cmws.yy[0][2] - cmws.yy[0][1] * cmws.yy[2][2];
	cmws.yy[2][3] = cmws.yy[0][1] * cmws.yy[1][2] - cmws.yy[1][1] * cmws.yy[0][2];
	a = Math.sqrt(cmws.yy[0][3] * cmws.yy[0][3] + cmws.yy[1][3] * cmws.yy[1][3] + cmws.yy[2][3] * cmws.yy[2][3]);
	for (i = 0; i < 3; i++) {
		if (a === 0) {
			cmws.yy[i][3] = Number.Max_Value;
		} else {
			cmws.yy[i][3] /= a;
		}
	}
};

exports.transformMatrix = function (cmws) {
	var i,
	    j,
		l,
		m,
		n,
		e,
		determinateSubr = exports.determinateSubr;

	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			cmws.vv[i][j + 1] = cmws.xx[i][j + 1];
		}
	}

	// get determinate from copied into array cmws.vv
	determinateSubr(cmws);
	// save it
	e = cmws.determinate;

	for (m = 0; m < 3; m++) {
		for (i = 0; i < 3; i++) {
			for (j = 0; j < 3; j++) {
				cmws.vv[i][j + 1] = cmws.xx[i][j + 1];
			}
		}
		for (n = 0; n < 3; n++) {
			cmws.vv[0][m + 1] = 0;
			cmws.vv[1][m + 1] = 0;
			cmws.vv[2][m + 1] = 0;
			cmws.vv[n][m + 1] = 1;
			determinateSubr(cmws);
			if (e === 0) {
				cmws.qq[m][n + 1] = Number.MAX_VALUE;
			} else {
				cmws.qq[m][n + 1] = cmws.determinate / e;
			}
		}
	}

	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			cmws.rr[i][j + 1] = 0;
		}
	}

	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			for (l = 0; l < 3; l++) {
				cmws.rr[i][j + 1] += (cmws.yy[i][l + 1] * cmws.qq[l][j + 1]);
			}
		}
	}

	for (m = 0; m < 3; m++) {
		for (i = 0; i < 3; i++) {
			for (j = 0; j < 3; j++) {
				cmws.vv[i][j + 1] = cmws.rr[i][j + 1];
			}
		}
		determinateSubr(cmws);
		e = cmws.determinate;
		for (n = 0; n < 3; n++) {
			cmws.vv[0][m + 1] = 0;
			cmws.vv[1][m + 1] = 0;
			cmws.vv[2][m + 1] = 0;
			cmws.vv[n][m + 1] = 1;
			determinateSubr(cmws);
			if (e === 0) {
				cmws.qq[m][n + 1] = Number.MAX_VALUE;
			} else {
				cmws.qq[m][n + 1] = cmws.determinate / e;
			}
		}
	}
};

exports.initMatrix = function (init, cmws) {
	var copyPosition = exports.copyPosition,
	    arrayAssignInit = exports.arrayAssignInit,
		generateThirdInit = exports.generateThirdInit,
		transformMatrix = exports.transformMatrix;

	if (init === 3 && cmws.init === 2) {
		copyPosition(cmws.current, cmws.initMatrixPosition);
		copyPosition(cmws.one, cmws.current);
		arrayAssignInit(1, cmws);
		copyPosition(cmws.two, cmws.current);
		arrayAssignInit(2, cmws);
		copyPosition(cmws.initMatrixPosition, cmws.current);
		arrayAssignInit(3, cmws);
		transformMatrix(cmws);
		copyPosition(cmws.current, cmws.three);
		cmws.init = 3;
	} else if (init === 2 && cmws.init === 3) {
		copyPosition(cmws.current, cmws.initMatrixPosition);
		copyPosition(cmws.one, cmws.current);
		arrayAssignInit(1, cmws);
		copyPosition(cmws.initMatrixPosition, cmws.current);
		arrayAssignInit(2, cmws);
		copyPosition(cmws.current, cmws.initMatrixPosition);
		copyPosition(cmws.three, cmws.current);
		arrayAssignInit(3, cmws);
		copyPosition(cmws.initMatrixPosition, cmws.current);
		transformMatrix(cmws);
		copyPosition(cmws.current, cmws.two);
	} else if (init === 2 && (cmws.init === 1 || cmws.init === 2)) {
		copyPosition(cmws.current, cmws.initMatrixPosition);
		copyPosition(cmws.one, cmws.current);
		arrayAssignInit(1, cmws);
		copyPosition(cmws.initMatrixPosition, cmws.current);
		arrayAssignInit(2, cmws);
		generateThirdInit(cmws);
		transformMatrix(cmws);
		copyPosition(cmws.current, cmws.two);
		cmws.init = 2;
	} else if (init === 1 && cmws.init === 3) {
		arrayAssignInit(1, cmws);
		copyPosition(cmws.current, cmws.initMatrixPosition);
		copyPosition(cmws.two, cmws.current);
		arrayAssignInit(2, cmws);
		copyPosition(cmws.three, cmws.current);
		arrayAssignInit(3, cmws);
		copyPosition(cmws.initMatrixPosition, cmws.current);
		transformMatrix(cmws);
		copyPosition(cmws.current, cmws.one);
	} else if (init === 1 && cmws.init === 2) {
		arrayAssignInit(1, cmws);
		copyPosition(cmws.current, cmws.initMatrixPosition);
		copyPosition(cmws.two, cmws.current);
		arrayAssignInit(2, cmws);
		copyPosition(cmws.initMatrixPosition, cmws.current);
		generateThirdInit(cmws);
		transformMatrix(cmws);
		copyPosition(cmws.current, cmws.one);
	} else if (init === 1 && (cmws.init === 0 || cmws.init === 1)) {
		arrayAssignInit(1, cmws);
		copyPosition(cmws.current, cmws.one);
		cmws.init = 1;
	} else {
		throw new Error('initMatrix() failure: init=' + init + ', cmws.init=' + cmws.init);
	}
};

exports.initMatrixFacade = function (cmws, init) {
	var copyPosition = exports.copyPosition,
	    initMatrix = exports.initMatrix;

	cmws.init = init;
	copyPosition(cmws.current, cmws.presetPosition);
	copyPosition(cmws.one, cmws.current);
	initMatrix(1, cmws);
	copyPosition(cmws.presetPosition, cmws.current);
};

exports.Hemisphere = {
	northern: 'northern',
	southern: 'southern'
};

exports.presetEquat = function (hemisphere, cmws) {
	var setPositionDeg = exports.setPositionDeg,
	    clearPosition = exports.clearPosition,
	    InitType = exports.InitType,
		initMatrixFacade = exports.initMatrixFacade,
		Hemisphere = exports.Hemisphere,
	    decDeg = hemisphere === Hemisphere.northern ? 90 : -90;

	setPositionDeg(0, decDeg, 0, 90, 0, 0, cmws.one);
	setPositionDeg(0, 0, 180, 0, 0, 0, cmws.two);
	clearPosition(cmws.three);

	initMatrixFacade(cmws, 2);
	cmws.initType = InitType.equatorial;
};

exports.presetAltaz = function (latitudeRad, cmws) {
	var decDeg,
	    absLatitudeDeg,
		uom = MLB.sharedLib.uom,
		setPositionDeg = exports.setPositionDeg,
		clearPosition = exports.clearPosition,
		InitType = exports.InitType,
		initMatrixFacade = exports.initMatrixFacade;

	decDeg = latitudeRad > 0 ? 90 : -90;
	absLatitudeDeg = Math.abs(latitudeRad / uom.degToRad);

	setPositionDeg(0, decDeg, 0, absLatitudeDeg, 0, 0, cmws.one);
	setPositionDeg(0, 0, 180, 90 - absLatitudeDeg, 0, 0, cmws.two);
	clearPosition(cmws.three);

	initMatrixFacade(cmws, 2);
	cmws.initType = InitType.altazimuth;
};

exports.initAltazResults = function (hemisphere, cmws) {
	var azOffset,
	    HAOffset,
		latitude,
		uom = MLB.sharedLib.uom,
		validHalfRev = exports.validHalfRev,
		copyPosition = exports.copyPosition,
		getAltazMatrix = exports.getAltazMatrix,
		getEquatMatrix = exports.getEquatMatrix,
		Hemisphere = exports.Hemisphere;

	// point to equatorial pole then get altaz where az = az offset;
	// ignore altitude = latitude: latitude derived from code below using the altazimuth pole more accurate and useful because azimuth motions become extreme near the altazimuth pole
	// (translating from RA/Dec back into altazimuth numbers will be more accurate if Dec derived from altazimuth pole)
	copyPosition(cmws.current, cmws.initAltazResultsPosition);
	cmws.current.RA = 0;
	cmws.current.Dec = hemisphere === Hemisphere.northern ? uom.qtrRev : -uom.qtrRev;
	getAltazMatrix(cmws);
	azOffset = validHalfRev(cmws.current.az);

	// point to altazimuth pole then get equat where SidT - RA = HA offset, Dec = latitude; original SidT preserved in initAltazResultsPosition;
	// remember that alt readings are offset by the altOffset, so adjust
	cmws.current.alt = uom.qtrRev - cmws.fabErrors.z3;
	cmws.current.az = 0;
	cmws.current.SidT = 0;
	getEquatMatrix(cmws);
	HAOffset = validHalfRev(cmws.initAltazResultsPosition.SidT - cmws.current.RA);
	latitude = cmws.current.Dec;

	copyPosition(cmws.initAltazResultsPosition, cmws.current);

	return {
		initLatitude: latitude,
		azOffset: azOffset,
		HAOffset: HAOffset
	};
};

exports.ConvertStyle = {
	trig: 'trig',
	matrix: 'matrix'
};

// angular separation of two equatorial coordinates should = the angular separation of the corresponding altazimuth coordinates;
// for target altitudes that cross the equator of their coordinate system, there are two solutions;
// formula from Dave Ek <ekdave@earthlink.net>
exports.calcAltOffsetDirectly = function (a, z) {
	var n,
	    m,
		x,
		a1,
		a2,
		altOffset,
		calcAngularSepUsingEquat = exports.calcAngularSepUsingEquat;

	n = Math.cos(a.az - z.az);
	m = Math.cos(calcAngularSepUsingEquat(a, z));
	x = (2 * m - (n + 1) * Math.cos(a.alt - z.alt)) / (n - 1);
	// acos(x) may be invalid: likely causes are azimuths not separate enough resulting in n-1 term being too small, or, variation from ideal numbers in other variables
	a1 = 0.5 * (+Math.acos(x) - a.alt - z.alt);
	a2 = 0.5 * (-Math.acos(x) - a.alt - z.alt);
	if (Math.abs(a1) < Math.abs(a2)) {
		altOffset = a1;
	} else {
		altOffset = a2;
	}
	return altOffset;
};

// when angular separation of altaz values closest to that of equat values, best altitude offset found;
// +- 45 deg search range
exports.calcAltOffsetIteratively = function (a, z) {
	var searchRangeDeg,
	    aAlt,
		zAlt,
		altIncr,
		ix,
		maxIx,
		diff,
		lastDiff,
		bestDiff,
		bestAltOff,
		uom = MLB.sharedLib.uom,
		angSepDiff = exports.angSepDiff;

	searchRangeDeg = 45;
	altIncr = uom.arcsecToRad;
	maxIx = searchRangeDeg * uom.degToRad / altIncr;

	// start from zero offset and increment offset until difference starts to get worse
	aAlt = a.alt;
	zAlt = z.alt;
	bestDiff = Number.MAX_VALUE;
	lastDiff = Number.MAX_VALUE;
	ix = 0;
	while (ix < maxIx) {
		diff = angSepDiff(a, z);
		if (diff < bestDiff) {
			bestDiff = diff;
			bestAltOff = a.alt - aAlt;
		}
		if (diff > lastDiff) {
			break;
		} else {
			lastDiff = diff;
		}
		ix++;
		a.alt += altIncr;
		z.alt += altIncr;
	}

	// again, start from zero offset, but this time decrement offset
	a.alt = aAlt;
	z.alt = zAlt;
	lastDiff = Number.MAX_VALUE;
	ix = 0;
	while (ix < maxIx) {
		diff = angSepDiff(a, z);
		if (diff < bestDiff) {
			bestDiff = diff;
			bestAltOff = a.alt - aAlt;
		}
		if (diff > lastDiff) {
			break;
		} else {
			lastDiff = diff;
		}
		ix++;
		a.alt -= altIncr;
		z.alt -= altIncr;
	}

	a.alt = aAlt;
	z.alt = zAlt;

	return bestAltOff;
};

// if equatorially aligned, then always points to north pole (angle of 0); in southern hemisphere angle is 180
exports.calcFieldRotationAngle = function (position, HAOffset, latitude) {
	var a,
	    sinHA,
		FR,
		uom = MLB.sharedLib.uom,
		validRev = exports.validRev;

	position.HA = position.SidT - HAOffset - position.RA;
	sinHA = Math.sin(position.HA);
	a = (Math.sin(latitude) / Math.cos(latitude)) * Math.cos(position.Dec) - Math.sin(position.Dec) * Math.cos(position.HA);
	if (a < 0) {
		FR = Math.atan(sinHA / a) + uom.halfRev;
	} else if (a === 0) {
		if (sinHA < 0) {
			FR = -uom.halfRev;
		} else if (sinHA === 0) {
			FR = 0;
		} else {
			FR = uom.halfRev;
		}
	} else {
		FR = Math.atan(sinHA / a);
	}
	return validRev(FR);
};

// single point of change to swap altitude offset calculators
exports.getAltOffset = function (a, z) {
	var calcAltOffsetDirectly = exports.calcAltOffsetDirectly,
	    calcAltOffsetIteratively = exports.calcAltOffsetIteratively;

	try {
		// faster, but may blow up if bad data
		return calcAltOffsetDirectly(a, z);
	} catch (err) {
		return calcAltOffsetIteratively(a, z);
	}
};

exports.bestZ3 = function (positions, cmws) {
	var accumAltOffset,
	    count,
		positionsLength,
		ix,
		ixB,
		getAltOffset = exports.getAltOffset;

	accumAltOffset = getAltOffset(cmws.one, cmws.two);
	count = 1;
	if (cmws.init === 3) {
		accumAltOffset += getAltOffset(cmws.one, cmws.three);
		count++;
		accumAltOffset += getAltOffset(cmws.two, cmws.three);
		count++;
	}
	positionsLength = positions.length;
	for (ix = 0; ix < positionsLength; ix++) {
		accumAltOffset += getAltOffset(positions[ix], cmws.one);
		count++;
		accumAltOffset += getAltOffset(positions[ix], cmws.two);
		count++;
		if (cmws.init === 3) {
			accumAltOffset += getAltOffset(positions[ix], cmws.three);
			count++;
		}
		positionsLength = positions.length;
		for (ixB = 0; ixB < positionsLength; ixB++) {
			if (ixB > ix) {
				accumAltOffset += getAltOffset(positions[ix], positions[ixB]);
				count++;
			}
		}
	}
	return accumAltOffset / count;
};

exports.calcPointingError = function (positions, cmws) {
	var pointingErrorRMS,
	    positionsLength,
		ix,
		aError,
		zError,
		pointingErrorRMSTotal,
		copyPosition = exports.copyPosition,
		getAltazMatrix = exports.getAltazMatrix;

	copyPosition(cmws.current, cmws.calcPointingErrorPosition);
	pointingErrorRMSTotal = 0;
	positionsLength = positions.length;
	for (ix = 0; ix < positionsLength; ix++) {
		copyPosition(positions[ix], cmws.current);
		// get altaz with trial Z123, compare to given altaz
		getAltazMatrix(cmws);
		// if scope aimed higher or more CW than what it should be, then it is defined as a positive error
		aError = positions[ix].alt - cmws.current.alt;
		// azimuth errors in terms of true field decrease towards the zenith
		zError = (positions[ix].az - cmws.current.az) * Math.cos(positions[ix].alt);
		pointingErrorRMS = Math.sqrt(aError * aError + zError * zError);
		pointingErrorRMSTotal += pointingErrorRMS;
	}
	copyPosition(cmws.calcPointingErrorPosition, cmws.current);

	return pointingErrorRMSTotal / positions.length;
};

exports.bestZ12Core = function (cmws, combinedPositions, z1, z2, bestZ1, bestZ2, bestZ3, bestPointingErrorRMS) {
	var pointingErrorRMS,
		uom = MLB.sharedLib.uom,
		copyPosition = exports.copyPosition,
		initMatrix = exports.initMatrix,
		calcPointingError = exports.calcPointingError;

	cmws.fabErrors.z1 = z1;
	cmws.fabErrors.z2 = z2;
	cmws.fabErrors.z3 = bestZ3;
	// similar to initMatrixFacade()
	copyPosition(cmws.one, cmws.current);
	initMatrix(1, cmws);
	// calcPointingError uses getAltaz()
	pointingErrorRMS = calcPointingError(combinedPositions, cmws);
	// adopt new best values if warranted: don't let very tiny errors impact best values, therefore use a threshold to beat
	if (pointingErrorRMS < bestPointingErrorRMS[0] - uom.arcsecToRad) {
		bestPointingErrorRMS[0] = pointingErrorRMS;
		bestZ1[0] = z1;
		bestZ2[0] = z2;
	}
};

exports.bestZ12 = function (positions, cmws, fabErrors, bestZ3, z1Range, z2Range, resolution) {
	var ix,
	    combinedPositions,
		positionsLength,
		startZ1,
		startZ2,
		bestZ1 = [],
		bestZ2 = [],
		z1,
		z2,
		bestPointingErrorRMS = [],
		uom = MLB.sharedLib.uom,
		copyPosition = exports.copyPosition,
		initMatrix = exports.initMatrix,
		bestZ12Core = exports.bestZ12Core;

	z1Range = uom.degToRad;
	z2Range = uom.degToRad;

	fabErrors.z1 = cmws.fabErrors.z1;
	fabErrors.z2 = cmws.fabErrors.z2;
	fabErrors.z3 = cmws.fabErrors.z3;
	copyPosition(cmws.current, cmws.bestZ12Position);
	bestPointingErrorRMS[0] = Number.MAX_VALUE;
	startZ1 = 0;
	startZ2 = 0;
	bestZ1[0] = 0;
	bestZ2[0] = 0;

	combinedPositions = [];
	combinedPositions.push(cmws.one);
	combinedPositions.push(cmws.two);
	if (cmws.init === 3) {
		combinedPositions.push(cmws.three);
	}
	positionsLength = positions.length;
	for (ix = 0; ix < positionsLength; ix++) {
		combinedPositions.push(positions[ix]);
	}

	for (z1 = 0; z1 < z2Range; z1 += resolution) {
		for (z2 = 0; z2 < z1Range; z2 += resolution) {
			bestZ12Core(cmws, combinedPositions, z1, z2, bestZ1, bestZ2, bestZ3, bestPointingErrorRMS);
		}
	}
	for (z1 = 0; z1 < z2Range; z1 += resolution) {
		for (z2 = 0; z2 < z1Range; z2 += resolution) {
			bestZ12Core(cmws, combinedPositions, z1, z2, bestZ1, bestZ2, bestZ3, bestPointingErrorRMS);
		}
	}

	// restore starting convert matrix, z123 values
	cmws.fabErrors.z1 = fabErrors.z1;
	cmws.fabErrors.z2 = fabErrors.z2;
	cmws.fabErrors.z3 = fabErrors.z3;
	// see above: similar to initMatrixFacade()
	copyPosition(cmws.one, cmws.current);
	initMatrix(1, cmws);
	copyPosition(cmws.bestZ12Position, cmws.current);

	return {
		z1: bestZ1[0],
		z2: bestZ2[0],
		z3: bestZ3
	};
};

// +- 1 deg for z12, +- 45 deg for z3, resolution = arcmin
exports.bestZ123 = function (positions, cmws, fabErrors) {
	var uom = MLB.sharedLib.uom,
	    bestZ3 = exports.bestZ3,
		bestZ12 = exports.bestZ12,
	    z1Range = uom.degToRad,
	    z2Range = uom.degToRad,
		resolution = uom.arcminToRad,
	    bestZ3Result = bestZ3(positions, cmws);

	return bestZ12(positions, cmws, fabErrors, bestZ3Result, z1Range, z2Range, resolution);
};

// +- 5 deg for axis misalignment (z1), +- 45 deg for alt offset (z3), resolution = arcmin
// use an az offset (z2) of 0, since az positions are typically relative
exports.bestZ13 = function (positions, cmws, fabErrors) {
	var uom = MLB.sharedLib.uom,
	    bestZ3 = exports.bestZ3,
		bestZ12 = exports.bestZ12,
	    z1Range = 5 * uom.degToRad,
	    z2Range = 0,
		resolution = uom.arcminToRad,
	    bestZ3Result = bestZ3(positions, cmws);

	return bestZ12(positions, cmws, fabErrors, bestZ3Result, z1Range, z2Range, resolution);
};

// returns north or south pole if equatorially aligned, else returns latitude
exports.getLatitude = function (initType, latitude) {
	var uom = MLB.sharedLib.uom,
	    InitType = exports.InitType;

	if (initType === InitType.equatorial) {
		if (latitude >= 0) {
			return uom.qtrRev;
		}
		return -uom.qtrRev;
	}
	return latitude;
};

exports.getHemisphere = function (latitude) {
	var Hemisphere = exports.Hemisphere;

	if (latitude >= 0) {
		return Hemisphere.northern;
	}
	return Hemisphere.southern;
};

// bridge trig and matrix conversion routines into a single interface
exports.XForm = function (convertStyle, latitude) {
	this.convertStyle = convertStyle;
	this.latitude = latitude;

	// functions that do not require coordinate conversions and therefore aren't aware of trig or matrix conversion strategies
	this.calcFieldRotationAngle = function (HAOffset) {
		var calcFieldRotationAngle = exports.calcFieldRotationAngle,
		    getLatitude = exports.getLatitude;

		return calcFieldRotationAngle(this.position, HAOffset, getLatitude(this.initType, this.latitude));
	};
	this.setMeridianFlip = function (canFlip, isFlipped) {
		var setFlipState = exports.setFlipState;

		this.meridianFlip.canFlip = canFlip;
		setFlipState(isFlipped, this.meridianFlip);
	};

	this.setConvertStyle = function (convertStyle) {
		var FabErrors = exports.FabErrors,
		    MeridianFlip = exports.MeridianFlip,
		    Position = exports.Position,
			ConvertMatrixWorkingStorage = exports.ConvertMatrixWorkingStorage,
			ConvertStyle = exports.ConvertStyle;

		// convert using trig math
		if (convertStyle === ConvertStyle.trig) {
			// vars in common
			this.position = new Position();
			this.meridianFlip = new MeridianFlip();

			// common functions
			this.presetEquat = function () {
				var InitType = exports.InitType;

				this.initType = InitType.equatorial;
			};
			this.presetAltaz = function () {
				var InitType = exports.InitType;

				this.initType = InitType.altazimuth;
			};
			this.getEquat = function () {
				var getEquatTrig = exports.getEquatTrig,
				    getLatitude = exports.getLatitude;

				getEquatTrig(this.position, this.meridianFlip, getLatitude(this.initType, this.latitude));
			};
			this.getAltaz = function () {
				var getAltazTrig = exports.getAltazTrig,
				    getLatitude = exports.getLatitude;

				getAltazTrig(this.position, this.meridianFlip, getLatitude(this.initType, this.latitude));
			};

		// convert using matrix math
		} else if (convertStyle === ConvertStyle.matrix) {
			this.cmws = new ConvertMatrixWorkingStorage();
			this.bestZ123FabErrors = new FabErrors();

			// vars in common
			this.position = this.cmws.current;
			this.meridianFlip = this.cmws.meridianFlip;

			// common functions
			this.presetEquat = function () {
				var InitType = exports.InitType,
				    presetEquat = exports.presetEquat,
					getHemisphere = exports.getHemisphere;

				this.initType = InitType.equatorial;
				// sets cmws.initType
				presetEquat(getHemisphere(this.latitude), this.cmws);
			};
			this.presetAltaz = function () {
				var InitType = exports.InitType,
				    presetAltaz = exports.presetAltaz;

				this.initType = InitType.altazimuth;
				// sets cmws.initType
				presetAltaz(this.latitude, this.cmws);
			};
			this.getEquat = function () {
				var getEquatMatrix = exports.getEquatMatrix;

				getEquatMatrix(this.cmws);
			};
			this.getAltaz = function () {
				var getAltazMatrix = exports.getAltazMatrix;

				getAltazMatrix(this.cmws);
			};

			// functions unique to matrix math
			this.setFabErrorsDeg = function (z1, z2, z3) {
				var setFabErrorsDeg = exports.setFabErrorsDeg;

				setFabErrorsDeg(z1, z2, z3, this.cmws.fabErrors);
			};
			this.initAltazResults = function () {
				var initAltazResults = exports.initAltazResults,
				    getHemisphere = exports.getHemisphere;

				return initAltazResults(getHemisphere(this.latitude), this.cmws);
			};
			this.bestZ123 = function (positions) {
				var bestZ123 = exports.bestZ123;

				return bestZ123(positions, this.cmws, this.bestZ123FabErrors);
			};
			this.bestZ13 = function (positions) {
				var bestZ13 = exports.bestZ13;

				return bestZ13(positions, this.cmws, this.bestZ123FabErrors);
			};
		}
	};
	this.setConvertStyle(convertStyle);
};

/*
from http://www.brayebrookobservatory.org/BrayObsWebSite/HOMEPAGE/BRAYOBS%20PUBLICATIONS.html;
King Rate is 1436.46 at temperate latitudes, +- 2 hrs meridian, -10 to 30deg dec, from refracted pole;
for comparision, sidereal tracking rate = 1440/units.SidRate = 1436.068;
*/
exports.calcKingRateMinutesPerDay = function (RA, Dec, SidT, latitude) {
	var cosLat,
	    sinLat,
		cotLat,
		cosDec,
		sinDec,
		tanDec,
		cosHA,
		rate,
		cot = MLB.sharedLib.cot,
		uom = MLB.sharedLib.uom;

	cosLat = Math.cos(latitude);
	sinLat = Math.sin(latitude);
	cotLat = cot(latitude);
	cosDec = Math.cos(Dec);
	sinDec = Math.sin(Dec);
	tanDec = Math.tan(Dec);
	cosHA = Math.cos(RA - SidT);

	rate = (1436.46 + 0.4 * (cosLat / cosDec * (cosLat * cosDec + sinLat * sinDec * cosHA) / Math.pow(sinLat * sinDec + cosLat * cosDec * cosHA, 2) - cotLat * tanDec * cosHA));
	// to convert from sidereal seconds to civil time seconds
	rate /= uom.sidRate;
	if (latitude < 0) {
		rate = -rate;
	}
	return rate;
};

exports.calcKingRateArcsecPerSec = function (KingRateMinutesPerDay) {
	var uom = MLB.sharedLib.uom;

	// 21600 = # of 15" in 24 hrs; equation is equivalent of 1440/sidRate/KingRate*15
	return 21600 / uom.sidRate / KingRateMinutesPerDay;
};

/*
From: "MLThiebaux" <mlt@ns.sympatico.ca>
	We can think of this system as an equatorial mounting with the polar axis grossly misoriented.
	Say the polar axis is pointing at a fixed point t in the sky with declination t. (If Dobsonian, read azimuth axis instead of polar axis). 
	Suppose we are tracking a star with declination s.  Let h be the hour angle of the star minus the fixed hour angle of t.
	h is increasing at the constant rate w = 15 deg/hr.
	Let a = cos(s)tan(t) and b = sin(s).
	Then the apparent rotation rate in the field of view of the telescope is w(b-a*cosH)/((sinH)^2 +(a-b*cosH)^2)).
	Note that the rate is time-varying.
	A positive rate corresponds to a clockwise rotation in a 2-mirror telescope.
	
note: this function matches with large hour angle offsets of the scope's zenith and very high altitude angles if latitude is found by pointing at scope's zenith as opposed to pointing at scope's equatorial pole;
equatorial coordinates must be set ahead of time;
*/
exports.calcFieldRotationRateSidTrackFormula = function (position, meridianFlip, latitude, HAOffset) {
	var initialDec,
	    hourAngle,
		DecAtZenith,
		tanDecAtZenith,
		a,
		b,
		cosHA,
		sinHA,
		intermed,
		FRRateToSidTrackRateRatio,
		uom = MLB.sharedLib.uom,
		validRev = exports.validRev,
		getEquatTrig = exports.getEquatTrig;

	// get hour angle of target
	initialDec = position.Dec;
	hourAngle = validRev(position.SidT - position.RA - HAOffset);

	// get Dec of scope's zenith point (hour angle of scope's initialDec zenith point already calculated: HAOffset)
	position.alt = uom.qtrRev;
	position.az = 0;
	getEquatTrig(position, meridianFlip, latitude);
	DecAtZenith = position.Dec;
	tanDecAtZenith = Math.tan(DecAtZenith);
	// tan of 90 or -90 is infinite; fix if DecAtNight is 90/-90
	if (!isFinite(tanDecAtZenith)) {
		tanDecAtZenith = Number.MAX_VALUE;
	}
	a = Math.cos(initialDec) * tanDecAtZenith;
	b = Math.sin(initialDec);

	cosHA = Math.cos(hourAngle);
	sinHA = Math.sin(hourAngle);
	intermed = a - b * cosHA;
	// formula produces rotation rate as a ratio compared to sidereal tracking rate, ie, 1.0 = 15.04"/sec, 2.0 is twice sidereal track rate
	FRRateToSidTrackRateRatio = (b - a * cosHA) / (sinHA * sinHA + intermed * intermed);
	// secToRad is the correct conversion as 15" = 1 sec: this converts from sidereal tracking rate ratio to radians;
	// rate needs to be speeded up by sidRate because rate calculated for 15deg/hr which is too slow, needs to be calculated for 15.04deg/hr
	return -FRRateToSidTrackRateRatio * uom.sidRate * uom.secToRad;
};

// distance traveled during 1 second of civil time
exports.calcAltazTrackingRates = function (az, alt, latitude) {
	var azRate,
	    altRate,
		cot = MLB.sharedLib.cot,
		uom = MLB.sharedLib.uom;

	azRate = 15 * uom.sidRate * uom.arcsecToRad * (Math.sin(latitude) + (cot(uom.qtrRev - alt)) * Math.cos(uom.halfRev - az) * Math.cos(latitude));
	altRate = 15 * uom.sidRate * uom.arcsecToRad * (Math.sin(uom.halfRev - az) * Math.cos(latitude));

	return {
		azRate: azRate,
		altRate: altRate
	};
};

// use RA, Dec and site latitude to get site az, alt, then add refraction to site alt in order to get refracted RA, Dec;
// needs a getAltaz and a getEquat
exports.calcRefractionFromTrueEquatorialCorrection = function (RA, Dec, SidT, latitude, refractPosition) {
	var az,
	    alt,
		refraction,
		deltaRA,
		deltaDec,
		meridianFlip = null,
		uom = MLB.sharedLib.uom,
		calcRefractionFromTrue = exports.calcRefractionFromTrue,
		getAltazTrig = exports.getAltazTrig,
		getEquatTrig = exports.getEquatTrig;

	refractPosition.RA = RA;
	refractPosition.Dec = Dec;
	refractPosition.SidT = SidT;
	getAltazTrig(refractPosition, meridianFlip, latitude);
	az = refractPosition.az;
	alt = refractPosition.alt;
	refraction = calcRefractionFromTrue(refractPosition.alt / uom.degToRad);

	refractPosition.alt += refraction;
	getEquatTrig(refractPosition, meridianFlip, latitude);
	deltaRA = refractPosition.RA - RA;
	deltaDec = refractPosition.Dec - Dec;

	return {
		az: az,
		alt: alt,
		refraction: refraction,
		refractedAlt: refractPosition.alt,
		deltaRA: deltaRA,
		deltaDec: deltaDec
	};
};

exports.calcRefractionFromApparentEquatorialCorrection = function (RA, Dec, SidT, latitude, refractPosition) {
	var az,
	    alt,
		refraction,
		deltaRA,
		deltaDec,
		meridianFlip = null,
		uom = MLB.sharedLib.uom,
		calcRefractionFromApparent = exports.calcRefractionFromApparent,
		getAltazTrig = exports.getAltazTrig,
		getEquatTrig = exports.getEquatTrig;

	refractPosition.RA = RA;
	refractPosition.Dec = Dec;
	refractPosition.SidT = SidT;
	getAltazTrig(refractPosition, meridianFlip, latitude);
	az = refractPosition.az;
	alt = refractPosition.alt;
	refraction = calcRefractionFromApparent(refractPosition.alt / uom.degToRad);

	refractPosition.alt -= refraction;
	getEquatTrig(refractPosition, meridianFlip, latitude);
	deltaRA = refractPosition.RA - RA;
	deltaDec = refractPosition.Dec - Dec;

	return {
		az: az,
		alt: refractPosition.alt,
		refraction: refraction,
		refractedAlt: alt,
		deltaRA: deltaRA,
		deltaDec: deltaDec
	};
};

exports.undefinedRefraction = function () {
	return {az: undefined, alt: undefined, refraction: undefined, refractedAlt: undefined, deltaRA: undefined, deltaDec: undefined};
};

// get refracted RA, Dec, from site's coordinates, then use them to calculate xform new altaz values
exports.trackingRatesGetAltazCheckRefraction = function (xform, FR, HAOffset, savePosition, refractPosition) {
	var calcRefractionFromTrueEquatorialCorrection = exports.calcRefractionFromTrueEquatorialCorrection,
	    refractionValues = calcRefractionFromTrueEquatorialCorrection(xform.position.RA, xform.position.Dec, xform.position.SidT, xform.latitude, refractPosition);

	xform.position.RA += refractionValues.deltaRA;
	xform.position.Dec += refractionValues.deltaDec;
	xform.getAltaz();
	// get FR angle from refracted RA, Dec
	FR[0] = xform.calcFieldRotationAngle(HAOffset);
	// reset equatorial as it was
	xform.position.RA = savePosition.RA;
	xform.position.Dec = savePosition.Dec;

	// xform's altaz coordinates updated; return (site) az, alt, refraction, refractedAlt, deltaRA, deltaDec
	return refractionValues;
};

// pass in arrays so that the changed values can be read by the calling function
exports.setAzAltFRRates = function (position2, position1, FR2, FR1, timeIntervalSecs, azRate, altRate, FRRate) {
	var validHalfRev = exports.validHalfRev;

	azRate[0] = validHalfRev(position2.az - position1.az) / timeIntervalSecs;
	altRate[0] = validHalfRev(position2.alt - position1.alt) / timeIntervalSecs;
	FRRate[0] = validHalfRev(FR2[0] - FR1[0]) / timeIntervalSecs;
};

exports.calcTimeIntervalSidT = function (timeIntervalSecs) {
	var uom = MLB.sharedLib.uom;

	return timeIntervalSecs * uom.sidRate * uom.secToRad;
};

exports.TrackingRatesStrategy = {
	deltaTimeWithRefraction: 'deltaTime: refraction',
	deltaTimeNoRefraction: 'deltaTime: no refraction',
	KingRates: 'KingRates',
	formulae: 'formulae'
};

// distance of arcseconds per time in seconds (civil time seconds, not sidereal time seconds);
// sidereal time runs faster than civil time therefore sidereal tracking rate is approx 15.04 arcsec/second;
// set xform.position's RA, Dec, SidT before calling
exports.TrackingRates = function () {
	var constantTrackRateAzError,
		constantTrackRateAltError,
		savePosition = new exports.Position(),
	    workPositions = [];

	MLB.underscoreLib._(5).times(function () {
		workPositions.push(new exports.Position());
	});

	this.trackingRatesStrategy = undefined;

	this.setConstantTrackRateTimeRad = function (constantTrackRateTimeRad) {
		this.constantTrackRateTimeRad = constantTrackRateTimeRad;
	};

	// uses three getAltaz(); with refraction, uses 3 getAltaz(), 3 getAltazTrig(), 3 getEquatTrig() (not counting 5 min track error calc)
	this.getRatesViaDeltaTime = function (xform, timeIntervalSecs, HAOffset, includeRefraction) {
		var uom = MLB.sharedLib.uom,
		    timeIntervalSidT,
		    refractionResults,
			// use arrays so that the changed values can be read by the calling function
			azRate1 = [],
			altRate1 = [],
			FRRate1 = [],
			azRate2 = [],
			altRate2 = [],
			FRRate2 = [],
			FR1 = [],
			FR2 = [],
			FR3 = [],
			FR4 = [],
			changeAz,
			changeAlt,
			changeFR,
			siteAz,
			siteAlt,
		    validHalfRev = exports.validHalfRev,
			getAltazTrig = exports.getAltazTrig,
			copyPosition = exports.copyPosition,
			undefinedRefraction = exports.undefinedRefraction,
			trackingRatesGetAltazCheckRefraction = exports.trackingRatesGetAltazCheckRefraction,
			setAzAltFRRates = exports.setAzAltFRRates,
			calcTimeIntervalSidT = exports.calcTimeIntervalSidT,
			TrackingRatesStrategy = exports.TrackingRatesStrategy;

		timeIntervalSidT = calcTimeIntervalSidT(timeIntervalSecs);
		copyPosition(xform.position, savePosition);

		if (includeRefraction) {
			this.trackingRatesStrategy = TrackingRatesStrategy.deltaTimeWithRefraction;
			// updates xform's altaz coordinates and returns (site) az, alt, refraction, refractedAlt, deltaRA, deltaDec
			refractionResults = trackingRatesGetAltazCheckRefraction(xform, FR1, HAOffset, savePosition, workPositions[0]);
			copyPosition(xform.position, workPositions[1]);

			xform.position.SidT += timeIntervalSidT;
			trackingRatesGetAltazCheckRefraction(xform, FR2, HAOffset, savePosition, workPositions[0]);
			copyPosition(xform.position, workPositions[2]);

			xform.position.SidT += timeIntervalSidT;
			trackingRatesGetAltazCheckRefraction(xform, FR3, HAOffset, savePosition, workPositions[0]);
			copyPosition(xform.position, workPositions[3]);

			xform.position.SidT = workPositions[1].SidT + this.constantTrackRateTimeRad;
			trackingRatesGetAltazCheckRefraction(xform, FR4, HAOffset, savePosition, workPositions[0]);
			copyPosition(xform.position, workPositions[4]);
		} else {
			this.trackingRatesStrategy = TrackingRatesStrategy.deltaTimeNoRefraction;
			// site altaz
			copyPosition(xform.position, workPositions[0]);
			getAltazTrig(workPositions[0], null, xform.latitude);

			xform.getAltaz();
			FR1[0] = xform.calcFieldRotationAngle(HAOffset);
			copyPosition(xform.position, workPositions[1]);

			xform.position.SidT += timeIntervalSidT;
			xform.getAltaz();
			FR2[0] = xform.calcFieldRotationAngle(HAOffset);
			copyPosition(xform.position, workPositions[2]);

			xform.position.SidT += timeIntervalSidT;
			xform.getAltaz();
			FR3[0] = xform.calcFieldRotationAngle(HAOffset);
			copyPosition(xform.position, workPositions[3]);

			xform.position.SidT = workPositions[1].SidT + this.constantTrackRateTimeRad;
			xform.getAltaz();
			copyPosition(xform.position, workPositions[4]);
		}

		// pass in arrays so that the changed values can be read by the calling function
		setAzAltFRRates(workPositions[2], workPositions[1], FR2, FR1, timeIntervalSecs, azRate1, altRate1, FRRate1);
		setAzAltFRRates(workPositions[3], workPositions[2], FR3, FR2, timeIntervalSecs, azRate2, altRate2, FRRate2);
		// rates and net difference is in radians/sec, so difference in rates needs to be divided by the time interval to obtain radians/sec/sec
		changeAz = (azRate2[0] - azRate1[0]) / timeIntervalSecs;
		changeAlt = (altRate2[0] - altRate1[0]) / timeIntervalSecs;
		changeFR = (FRRate2[0] - FRRate1[0]) / timeIntervalSecs;

		// tracking rate is rad per sidereal second: slow down by sidereal time ratio and convert from seconds to rad;
		// if error is negative, then rates predicted more tracking motion than required, eg, near horizon w/ refraction where tracking slows down
		constantTrackRateAzError = validHalfRev(workPositions[4].az - workPositions[1].az - azRate1[0] / uom.sidRate / uom.secToRad * this.constantTrackRateTimeRad);
		constantTrackRateAltError = validHalfRev(workPositions[4].alt - workPositions[1].alt - altRate1[0] / uom.sidRate / uom.secToRad * this.constantTrackRateTimeRad);

		copyPosition(savePosition, xform.position);

		if (refractionResults === undefined) {
			refractionResults = undefinedRefraction();
			siteAz = workPositions[0].az;
			siteAlt = workPositions[0].alt;
		} else {
			siteAz = refractionResults.az;
			siteAlt = refractionResults.alt;
		}

		return {
			initialAz: workPositions[1].az,
			// includes refraction, if turned on
			initialAlt: workPositions[1].alt,
			initialFR: FR1[0],
			azRate: azRate1[0],
			altRate: altRate1[0],
			FRRate: FRRate1[0],
			changeAzRate: changeAz,
			changeAltRate: changeAlt,
			changeFRRate: changeFR,
			constantTrackRateAzError: constantTrackRateAzError,
			constantTrackRateAltError: constantTrackRateAltError,
			refractionResults: refractionResults,
			siteAz: siteAz,
			// does not include refraction
			siteAlt: siteAlt,
			trackingRatesStrategy: this.trackingRatesStrategy
		};
	};

	this.getKingRates = function (xform, timeIntervalSecs) {
		var sitePosition = workPositions[0],
		    latitude,
			scopePosition = workPositions[1],
			refractionResults,
			refractPosition = workPositions[2],
			timeIntervalSidT,
			KingRate0,
			KingRate1,
			rateChange,
			uom = MLB.sharedLib.uom,
		    validHalfRev = exports.validHalfRev,
			copyPosition = exports.copyPosition,
			getAltazTrig = exports.getAltazTrig,
			getLatitude = exports.getLatitude,
			calcKingRateMinutesPerDay = exports.calcKingRateMinutesPerDay,
			calcKingRateArcsecPerSec = exports.calcKingRateArcsecPerSec,
			calcRefractionFromTrueEquatorialCorrection = exports.calcRefractionFromTrueEquatorialCorrection,
			calcTimeIntervalSidT = exports.calcTimeIntervalSidT,
			TrackingRatesStrategy = exports.TrackingRatesStrategy;

		this.trackingRatesStrategy = TrackingRatesStrategy.KingRates;
		copyPosition(xform.position, savePosition);

		// site altaz
		copyPosition(xform.position, sitePosition);
		getAltazTrig(sitePosition, null, xform.latitude);

		// scope position
		latitude = getLatitude(xform.initType, xform.latitude);
		getAltazTrig(xform.position, xform.meridianFlip, latitude);
		copyPosition(xform.position, scopePosition);

		// refraction
		refractionResults = calcRefractionFromTrueEquatorialCorrection(xform.position.RA, xform.position.Dec, xform.position.SidT, xform.latitude, refractPosition);

		// KingRates
		timeIntervalSidT = calcTimeIntervalSidT(timeIntervalSecs);
		KingRate0 = calcKingRateArcsecPerSec(calcKingRateMinutesPerDay(xform.position.RA, xform.position.Dec, xform.position.SidT, xform.latitude));
		KingRate1 = calcKingRateArcsecPerSec(calcKingRateMinutesPerDay(xform.position.RA, xform.position.Dec, xform.position.SidT + timeIntervalSidT, xform.latitude));

		// change in rates
		rateChange = (KingRate1 - KingRate0) / timeIntervalSecs;

		// tracking rate is rad per sidereal second: convert arcsec to rad, slow down by sidereal time ratio and convert from seconds to rad;
		// if error is negative, then rates predicted more tracking motion than required, eg, near horizon w/ refraction where tracking slows down
		copyPosition(scopePosition, xform.position);
		xform.position.SidT += this.constantTrackRateTimeRad;
		getAltazTrig(xform.position, xform.meridianFlip, latitude);
		constantTrackRateAzError = validHalfRev(xform.position.az - scopePosition.az - (KingRate0 + KingRate1) / 2 * uom.arcsecToRad / uom.sidRate / uom.secToRad * this.constantTrackRateTimeRad);

		copyPosition(savePosition, xform.position);

		return {
			initialAz: scopePosition.az,
			initialAlt: scopePosition.alt,
			initialFR: 0,
			azRate: KingRate0 * uom.arcsecToRad,
			altRate: 0,
			FRRate: 0,
			changeAzRate: rateChange * uom.arcsecToRad,
			changeAltRate: 0,
			changeFRRate: 0,
			constantTrackRateAzError: constantTrackRateAzError,
			constantTrackRateAltError: 0,
			refractionResults: refractionResults,
			siteAz: sitePosition.az,
			siteAlt: sitePosition.alt,
			trackingRatesStrategy: this.trackingRatesStrategy
		};
	};

	// uses two getAltazTrig(), two getEquatTrig() (not counting 5 min track error calc)
	this.getRatesViaFormulaeNoRefraction = function (xform, timeIntervalSecs, HAOffset) {
		var sitePosition = workPositions[0],
		    latitude,
			initialFR,
			altazRates1,
			FRRate1,
			altazRates2,
			FRRate2,
			changeAz,
			changeAlt,
			changeFR,
			uom = MLB.sharedLib.uom,
		    validHalfRev = exports.validHalfRev,
			copyPosition = exports.copyPosition,
			getAltazTrig = exports.getAltazTrig,
			getLatitude = exports.getLatitude,
			calcFieldRotationRateSidTrackFormula = exports.calcFieldRotationRateSidTrackFormula,
			calcAltazTrackingRates = exports.calcAltazTrackingRates,
			undefinedRefraction = exports.undefinedRefraction,
			TrackingRatesStrategy = exports.TrackingRatesStrategy;

		this.trackingRatesStrategy = TrackingRatesStrategy.formulae;
		copyPosition(xform.position, savePosition);

		// site altaz
		copyPosition(xform.position, sitePosition);
		getAltazTrig(sitePosition, null, xform.latitude);

		// 1st set of rates
		latitude = getLatitude(xform.initType, xform.latitude);
		initialFR = xform.calcFieldRotationAngle(HAOffset);
		getAltazTrig(xform.position, xform.meridianFlip, latitude);
		altazRates1 = calcAltazTrackingRates(xform.position.az, xform.position.alt, latitude);
		copyPosition(xform.position, workPositions[1]);
		FRRate1 = calcFieldRotationRateSidTrackFormula(xform.position, xform.meridianFlip, latitude, HAOffset);
		copyPosition(workPositions[1], xform.position);

		// 2nd set of rates
		xform.position.SidT += timeIntervalSecs * uom.sidRate * uom.secToRad;
		getAltazTrig(xform.position, xform.meridianFlip, latitude);
		altazRates2 = calcAltazTrackingRates(xform.position.az, xform.position.alt, latitude);
		FRRate2 = calcFieldRotationRateSidTrackFormula(xform.position, xform.meridianFlip, latitude, HAOffset);

		// get change in rates
		changeAz = (altazRates2.azRate - altazRates1.azRate) / timeIntervalSecs;
		changeAlt = (altazRates2.altRate - altazRates1.altRate) / timeIntervalSecs;
		changeFR = (FRRate2 - FRRate1) / timeIntervalSecs;

		// tracking rate is arcsec per sidereal second: convert arcsec to rad, slow down by sidereal time ratio and convert from seconds to rad;
		// if error is negative, then rates predicted more tracking motion than required, eg, near horizon w/ refraction where tracking slows down
		copyPosition(workPositions[1], xform.position);
		xform.position.SidT += this.constantTrackRateTimeRad;
		getAltazTrig(xform.position, xform.meridianFlip, latitude);

		constantTrackRateAzError = validHalfRev(xform.position.az - workPositions[1].az - altazRates1.azRate / uom.sidRate / uom.secToRad * this.constantTrackRateTimeRad);
		constantTrackRateAltError = validHalfRev(xform.position.alt - workPositions[1].alt - altazRates1.altRate / uom.sidRate / uom.secToRad * this.constantTrackRateTimeRad);

		copyPosition(savePosition, xform.position);

		return {
			initialAz: workPositions[1].az,
			initialAlt: workPositions[1].alt,
			initialFR: initialFR,
			azRate: altazRates1.azRate,
			altRate: altazRates1.altRate,
			FRRate: FRRate1,
			changeAzRate: changeAz,
			changeAltRate: changeAlt,
			changeFRRate: changeFR,
			constantTrackRateAzError: constantTrackRateAzError,
			constantTrackRateAltError: constantTrackRateAltError,
			refractionResults: undefinedRefraction(),
			siteAz: sitePosition.az,
			siteAlt: sitePosition.alt,
			trackingRatesStrategy: this.trackingRatesStrategy
		};
	};

	// includes refraction correction when equatorially aligned using King rates; otherwise there is no refraction correction
	this.getRatesViaFormulae = function (xform, timeIntervalSecs, HAOffset, includeRefraction) {
		var InitType = exports.InitType;

		if (xform.initType === InitType.equatorial && includeRefraction) {
			return this.getKingRates(xform, timeIntervalSecs);
		}
		return this.getRatesViaFormulaeNoRefraction(xform, timeIntervalSecs, HAOffset);
	};
};

/*
A three axis mount, 'alt-alt-a', can be configured as an horizontal yoke mount on a platter. The platter is parallel to the ground with its axis pointing at the zenith. On the platter is the 'alt-alt' or 'el-el' portion that is most commonly constructed as a yoke mounted horizontally, sometimes with each yoke end in the shape of a horseshoe so that the telescope can swing down to the horizon. The primary axis of the yoke points to the horizon and the secondary axis rotates between the yoke's arms. By calculating the field rotation angle at the focal plane using a standard altazimuth conversion, then setting the platter's rotational angle to the field rotation, then using a new altazimuth conversion where the altazimuth's primary axis points at the horizon (think of a Dobsonian tipped over 90 degrees) and finally taking these azimuth and altitude angles to set the 'alt-alt' or 'el-el' horizonal yoke, the 3-axis telescope can be made to point at the object and maintain a constant field rotation angle at the focal plane.
*/
exports.ThreeAxis = function () {
	var XForm = exports.XForm,
	    Position = exports.Position,
		uom = MLB.sharedLib.uom,
		ConvertStyle = exports.ConvertStyle;

	this.XFormSite = new XForm(ConvertStyle.matrix, null);
	this.XFormYoke = new XForm(ConvertStyle.matrix, uom.qtrRev);
	this.yokePolePosition = new Position();
	this.yokeSiteZenithPosition = new Position();

	this.getAltAltAz = function (latitude, RA, Dec, SidT, HAOffset) {
		var setPositionDeg = exports.setPositionDeg,
			a1,
			a2,
			a3;

		this.XFormSite.latitude = latitude;
		this.XFormSite.presetAltaz();

		setPositionDeg(RA / uom.degToRad, Dec / uom.degToRad, 0, 0, SidT / uom.degToRad, 0, this.XFormSite.position);
		a1 = this.XFormSite.calcFieldRotationAngle(HAOffset);

		this.initYoke(a1, SidT);

		setPositionDeg(RA / uom.degToRad, Dec / uom.degToRad, 0, 0, SidT / uom.degToRad, 0, this.XFormYoke.position);
		this.XFormYoke.getAltaz();
		a2 = this.XFormYoke.position.az;
		a3 = this.XFormYoke.position.alt;

		return {
			a1: a1,
			a2: a2,
			a3: a3
		};
	};

	this.getEquat = function (latitude, a1, a2, a3, SidT) {
		var setPositionDeg = exports.setPositionDeg,
			RA,
			Dec;

		this.XFormSite.latitude = latitude;
		this.XFormSite.presetAltaz();

		this.initYoke(a1, SidT);

		setPositionDeg(0, 0, a2 / uom.degToRad, a3 / uom.degToRad, SidT / uom.degToRad, 0, this.XFormYoke.position);
		this.XFormYoke.getEquat();
		RA = this.XFormYoke.position.RA;
		Dec = this.XFormYoke.position.Dec;

		return {
			RA: RA,
			Dec: Dec
		};
	};

	this.initYoke = function (a1, SidT) {
		var setPositionDeg = exports.setPositionDeg,
		    initMatrixFacade = exports.initMatrixFacade,
			copyPosition = exports.copyPosition,
		    yokePoleRADeg,
			yokePoleDecDeg,
			yokeSiteZenithRADeg,
			yokeSiteZenithDecDeg;

		// init position #1: yoke's pole which points at horizon offset from north by a1
		setPositionDeg(0, 0, a1 / uom.degToRad, 0, SidT / uom.degToRad, 0, this.XFormSite.position);
		this.XFormSite.getEquat();
		yokePoleRADeg = this.XFormSite.position.RA / uom.degToRad;
		yokePoleDecDeg = this.XFormSite.position.Dec / uom.degToRad;
		setPositionDeg(yokePoleRADeg, yokePoleDecDeg, 0, 90, SidT / uom.degToRad, 0, this.yokePolePosition);

		// init position #2: yoke pointing at site's zenith
		setPositionDeg(0, 0, a1 / uom.degToRad, 90, SidT / uom.degToRad, 0, this.XFormSite.position);
		this.XFormSite.getEquat();
		yokeSiteZenithRADeg = this.XFormSite.position.RA / uom.degToRad;
		yokeSiteZenithDecDeg = this.XFormSite.position.Dec / uom.degToRad;
		setPositionDeg(yokeSiteZenithRADeg, yokeSiteZenithDecDeg, 0, 0, SidT / uom.degToRad, 0, this.yokeSiteZenithPosition);

		// init yoke
		copyPosition(this.yokePolePosition, this.XFormYoke.cmws.one);
		copyPosition(this.yokeSiteZenithPosition, this.XFormYoke.cmws.two);
		initMatrixFacade(this.XFormYoke.cmws, 2);
	};

	this.getAltAltAzTrackingRates = function (latitude, RA, Dec, SidT, HAOffset, timeIntervalSecs) {
		var validRev = exports.validRev,
		    validHalfRev = exports.validHalfRev,
		    calcTimeIntervalSidT = exports.calcTimeIntervalSidT,
		    timeIntervalSidT = calcTimeIntervalSidT(timeIntervalSecs),
		    begin = this.getAltAltAz(latitude, RA, Dec, SidT, HAOffset),
		    end = this.getAltAltAz(latitude, RA, Dec, validRev(SidT + timeIntervalSidT), HAOffset);

		return {
			a1: begin.a1,
			a2: begin.a2,
			a3: begin.a3,
			changeA1: validHalfRev(end.a1 - begin.a1) / timeIntervalSecs,
			changeA2: validHalfRev(end.a2 - begin.a2) / timeIntervalSecs,
			changeA3: validHalfRev(end.a3 - begin.a3) / timeIntervalSecs
		};
	};
};

// end of file
