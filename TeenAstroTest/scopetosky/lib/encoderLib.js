// copyright Mel Bartels, 2015
// see encoderLib unitTests.htm for unit tests; also the encoder calculator, encoderCalc.htm
// turn on jslint's Tolerate ++ and --

'use strict';

//MLB.encoderLib = {};

exports.encoder = function () {
	this.name = undefined;
	this.minCount = undefined;
	this.maxCount = undefined;
	this.totalCounts = undefined;
	this.currentCount = undefined;
	this.lastCount = undefined;
	this.gearReduction = undefined;
	this.offsetAngle = undefined;
	this.direction = undefined;
	this.rotations = undefined;
	this.encoderAngle = undefined;
	this.gearAngle = undefined;
	this.gearAngleWithOffset = undefined;
};

exports.encoderBuilder = function (name, minCount, maxCount, gearReduction) {
	var encoder = new exports.encoder();

	encoder.name = name;
	encoder.minCount = minCount;
	encoder.maxCount = maxCount;
    // the lowest number is a valid encoder count, eg, min=0,max=9 results in a total count of 10 (0,1,2,3,4,5,6,7,8,9)
    encoder.totalCounts = encoder.maxCount - encoder.minCount + 1;
	encoder.gearReduction = gearReduction;
	encoder.offsetAngle = 0;

	encoder.currentCount = 0;
	encoder.lastCount = 0;
	encoder.direction = 0;
	encoder.rotations = 0;
	encoder.encoderAngle = 0;
	encoder.gearAngle = 0;
	encoder.gearAngleWithOffset = 0;

	return encoder;
};

exports.calcMotorEncoderTicksResolution = function (encoder) {
	var uom = MLB.sharedLib.uom;

	return {
		countsPerRev: encoder.totalCounts,
		countsPerDegree: encoder.totalCounts * uom.degToRev,
		countsPerArcsec: encoder.totalCounts * uom.arcsecToRev
	};
};

/* 
given encoder range is -5 to 4; 
    increasing counts: 
		current/last of 3/2
		current/last of -2/-3
		current/last of -4/4
		current/last of 1/-1
	decreasing counts the opposite of above
*/
exports.getDirection = function (encoder) {
	var diff = encoder.currentCount - encoder.lastCount;

	if (diff === 0) {
		encoder.direction = 0;
		return;
	}

	if (diff > 0) {
		if (diff <= encoder.totalCounts / 2) {
			encoder.direction = 1;
			return;
		}
		encoder.direction = -1;
		return;
	}

	// else diff must be negative
	if (-diff <= encoder.totalCounts / 2) {
		encoder.direction = -1;
		return;
	}
	encoder.direction = 1;
};

exports.setRotations = function (encoder) {
	if (encoder.direction === 1 && encoder.currentCount < encoder.lastCount) {
		encoder.rotations += 1;
		return;
	}
	if (encoder.direction === -1 && encoder.currentCount > encoder.lastCount) {
		encoder.rotations -= 1;
	}
};

exports.setAngles = function (encoder) {
	var uom = MLB.sharedLib.uom,
	    validRev = MLB.coordLib.validRev,
	    gearRotation,
		encoderRotation = (encoder.currentCount - encoder.minCount) / encoder.totalCounts;

	if (isNaN(encoder.gearReduction) || encoder.gearReduction === 0) {
		gearRotation = encoderRotation;
	} else {
		gearRotation = (encoderRotation + encoder.rotations) / encoder.gearReduction;
	}

	encoder.encoderAngle = encoderRotation * uom.oneRev;
	encoder.gearAngle = validRev(gearRotation * uom.oneRev);
	encoder.gearAngleWithOffset = validRev(encoder.gearAngle + encoder.offsetAngle);
};

exports.update = function (encoder, count) {
	var getDirection = exports.getDirection,
	    setRotations = exports.setRotations,
		setAngles = exports.setAngles;

	encoder.lastCount = encoder.currentCount;
	encoder.currentCount = count;

	getDirection(encoder);
	setRotations(encoder);
	setAngles(encoder);
};

exports.reset = function (encoder) {
	var validRev = MLB.coordLib.validRev;

	validRev(encoder.offsetAngle = encoder.gearAngleWithOffset);

	encoder.currentCount = 0;
	encoder.lastCount = 0;
	encoder.direction = 0;
	encoder.rotations = 0;
	encoder.encoderAngle = 0;
	encoder.gearAngle = 0;
	encoder.gearAngleWithOffset = 0;
};

exports.setToAngle = function (encoder, angleRad) {
	var uom = MLB.sharedLib.uom,
	    int = MLB.sharedLib.int,
	    encoderAngleRad = (angleRad - encoder.offsetAngle) * encoder.gearReduction,
	    rotations = MLB.sharedLib.int(encoderAngleRad / uom.oneRev),
		netRotation = encoderAngleRad / uom.oneRev - rotations,
		setAngles = exports.setAngles;

	encoder.lastCount = encoder.currentCount;
	// no rounding because encoder ticks occur precisely on the count; the next tick won't occur until the full tick angle has been traversed
	encoder.currentCount = int(encoder.totalCounts * netRotation + encoder.minCount);
	encoder.rotations = rotations;
	setAngles(encoder);
};

// end of file
