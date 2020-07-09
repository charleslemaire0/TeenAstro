// copyright Mel Bartels, 2011-2020
// see calcLib unitTests.htm for unit tests
// turn on jslint's Tolerate ++ and --

'use strict';

//MLB.calcLib = {};

// Ronchi

// from a Sky and Telescope article
exports.wavefrontError = function (mirrorDia, radiusOfCurvature, correctionFactor) {
    var mirrorRadius = mirrorDia / 2,
        zones = [0.3, 0.7, 0.93],
        zonalRadii = [],
        readings = [],
        deviations = [],
        errors = [],
        surfaceErrors = [],
        length = zones.length,
        wavelengthLight = 0.000022,
        ix,
        A,
        B,
        C,
        zr;

    // md = 20, rc = 200, cf = 0.86: zonalRadii = [3, 7, 9.3]
    for (ix = 0; ix < length; ix++) {
        zonalRadii[ix] = zones[ix] * mirrorRadius;
    }

    // for fixed light source
    // md = 20, rc = 200, cf = 0.86: readings = [0.045, 0.245, 0.43245000000000006]
    for (ix = 0; ix < length; ix++) {
        readings[ix] = zonalRadii[ix] * zonalRadii[ix] / radiusOfCurvature;
    }

    // md = 20, rc = 200, cf = 0.86: deviations = [0.0063, 0.034300000000000004, 0.060543000000000013]
    for (ix = 0; ix < length; ix++) {
        deviations[ix] = readings[ix] * (1 - correctionFactor);
    }

    // md = 20, rc = 200, cf = 0.86: errors = [0.010738636363636363, 0.13642045454545457, 0.31991471590909104]
    for (ix = 0; ix < length; ix++) {
        errors[ix] = deviations[ix] * zonalRadii[ix] / (2 * radiusOfCurvature * radiusOfCurvature * wavelengthLight);
    }

    // md = 20, rc = 200, cf = 0.86: A = 0.0001012748943181821, B = 0.000015981517045452475, C = -0.010287304602272734
    A = (3.31 * errors[0] - 3.88 * errors[1] + 1.86 * errors[2]) / (mirrorRadius * mirrorRadius * mirrorRadius);
    B = (-7.19 * errors[0] + 6.37 * errors[1] - 2.47 * errors[2]) / (mirrorRadius * mirrorRadius);
    C = -mirrorRadius * (mirrorRadius * A + B);

    // md = 20, rc = 200, cf = 0.86: wavefronts = [-0.08395097402045464, -0.2554352439068186, -0.11930524737190273]
    for (ix = 0; ix < length; ix++) {
        zr = zonalRadii[ix];
        surfaceErrors[ix] = A * zr * zr * zr * zr + B * zr * zr * zr + C * zr * zr;
    }

    // 70% zone will have greatest deviation when smoothly under/over corrected (center and edge will have zero error);
    // wavefront error is twice that of surface error
    return Math.abs(2 * surfaceErrors[1]);
};

// parabolic correction tolerances: focalRatio 1: 98.3%; focalRatio 2: 96.7%; focalRatio 3: 95%; focalRatio 4: 93.3%; focalRatio 5: 91.7%; focalRatio 6: 90%; focalRatio 7: 88.4%; focalRatio 8: 86.7%; focalRatio 9: 85%; focalRatio 10: 83.4%;
exports.findAllowableCorrection = function (mirrorDia, radiusOfCurvature) {
    var wavefrontError = exports.wavefrontError,
        iterations = 0,
        maxIterations = 8,
        lastWavefrontError,
        currentWavefrontError,
        direction = -1,
        stepSize = radiusOfCurvature / mirrorDia / 2 / 100, // make an initial guess
        correction = 1 - stepSize;

    while (iterations < maxIterations) {
        currentWavefrontError = wavefrontError(mirrorDia, radiusOfCurvature, correction);
        if (lastWavefrontError !== undefined && currentWavefrontError > lastWavefrontError) {
            direction *= -1;
            stepSize /= 2;
        } else {
            lastWavefrontError = currentWavefrontError;
        }
        correction += stepSize * direction;
        iterations++;
    }

    return correction;
};

exports.calcAllowableParabolicDeviationForQuarterWavefront = function (focalRatio) {
    return focalRatio * 0.0167;
};

// sagitta

exports.calcSagittaSpherical = function (mirrorDia, focalRatio) {
    var RoC,
        mirrorRadius;

    RoC = mirrorDia * focalRatio * 2;
    mirrorRadius = mirrorDia / 2;
    return RoC - Math.sqrt(RoC * RoC  - mirrorRadius * mirrorRadius);
};

exports.calcSagittaParabolic = function (mirrorDia, focalRatio) {
    var mirrorRadius = mirrorDia / 2,
        focalLength = mirrorDia * focalRatio;

    return mirrorRadius * mirrorRadius / (4 * focalLength);
};

exports.calcFocalRatio = function (mirrorDia, sagitta) {
    var mirrorRadius = mirrorDia / 2;

    return (sagitta * sagitta + mirrorRadius * mirrorRadius) / (8 * sagitta * mirrorRadius);
};

exports.calcFocalRatioFromSphericalSagitta = function (mirrorDia, sphericalSagitta) {
    return mirrorDia / (16 * sphericalSagitta);
};

// http://www.1728.org/sphere.htm and http://www.had2know.com/academics/spherical-cap-volume-surface-area-calculator.html
exports.calcSagittalVolume = function (mirrorDia, focalRatio) {
    var sagitta = exports.calcSagittaSpherical(mirrorDia, focalRatio),
        RoC = mirrorDia * focalRatio * 2;

    return Math.PI * sagitta * sagitta * (3 * RoC - sagitta) / 3;
};

exports.calcSagittalVolumeParabolic = function (mirrorDia, focalRatio) {
    var sagittaParabolic = exports.calcSagittaParabolic(mirrorDia, focalRatio),
        sagittaSpherical = exports.calcSagittaSpherical(mirrorDia, focalRatio),
        // the parabola touches the mirror center at the parabolic sagitta and touches the mirror edge at the spherical sagitta, so the sagitta to use will be between these two sagitti
        sagitta = ( sagittaSpherical + sagittaParabolic) / 2,
        RoC = mirrorDia * focalRatio * 2;

    return Math.PI * sagitta * sagitta * (3 * RoC - sagitta) / 3;
};

exports.calcSagittalVolumeRemovedDuringParabolization = function (mirrorDia, focalRatio) {
    var calcSagittalVolume = exports.calcSagittalVolume,
        calcSagittalVolumeParabolic = exports.calcSagittalVolumeParabolic,
        sphericalVolume = calcSagittalVolume(mirrorDia, focalRatio),
        parabolicVolume = calcSagittalVolumeParabolic(mirrorDia, focalRatio);

    return sphericalVolume - parabolicVolume;
};

// in cubic inches; 0.000393701 inches = 10 microns
// standard formula to calculate spherical captureEvents
exports.glassRemovalDuringPolishingFrom10MicronAluminumOxideCubicInches = function (mirrorDia, focalRatio) {
    var mirrorRadius = mirrorDia / 2,
        RoC = mirrorDia * focalRatio * 2;

    return 2 * Math.PI * RoC * RoC * (1 - Math.cos(mirrorRadius / RoC)) * 0.000393701;
};

// for wavelength of green light;
// conversion between nanometers and inches is 3.9370078740157E-8; green light is 550 nanometers
exports.inchesToWavesGreenLight = function (sphereParabolaDifferenceInches) {
    return sphereParabolaDifferenceInches / 0.000022;
};

// see https://en.wikipedia.org/wiki/Rotating_furnace
exports.calcRotatingFurnaceRPM = function (focalLengthMeters) {
    return Math.sqrt(447 / focalLengthMeters);
};

// diagonal off-axis illumination; sources are mid-70's Sky and Telescope article on diagonal size and Telescope Making #9, pg. 11 on diagonal offset.

exports.calcOffAxisIllumination = function (mirrorDia, focalLen, diagSize, diagToFocalPlaneDistance, offAxisDistance) {
    var r,
        x,
        a,
        c;

    r = diagSize * focalLen / (diagToFocalPlaneDistance * mirrorDia);
    x = 2 * offAxisDistance * (focalLen - diagToFocalPlaneDistance) / (diagToFocalPlaneDistance * mirrorDia);
    a = (x * x + 1 - r * r) / (2 * x);
    if (a < -1) {
        return 1;
    }
    if (a > 1) {
        return 0;
    }
    c = (x * x + r * r - 1) / (2 * x * r);
    return (Math.acos(a) - x * Math.sqrt(1 - a * a) + r * r * Math.acos(c)) / Math.PI;
};

// < 1.0 illumination results in a positive magnitude drop
exports.getMagnitudeFromIllum = function (illumination) {
    var log10 = MLB.sharedLib.log10;

    return -2.5 * log10(illumination);
};

exports.getIllumFromMagnitude = function (magnitude) {
    return Math.pow(10, -0.4 * magnitude);
};

exports.addMagnitudes = function (magnitude1, magnitude2) {
    var getIllumFromMagnitude = exports.getIllumFromMagnitude,
        getMagnitudeFromIllum = exports.getMagnitudeFromIllum;

    return getMagnitudeFromIllum(getIllumFromMagnitude(magnitude1) + getIllumFromMagnitude(magnitude2));
};

exports.addArrayMagnitudes = function (magnitudes) {
    var getIllumFromMagnitude = exports.getIllumFromMagnitude,
        getMagnitudeFromIllum = exports.getMagnitudeFromIllum,
        totalIllum = 0;

    if (magnitudes === undefined || magnitudes.length === 0) {
        return undefined;
    }

    magnitudes.forEach(function(value, ix) {
        totalIllum += getIllumFromMagnitude(value) ;
    });
    return getMagnitudeFromIllum(totalIllum);
};

// aperture2 > aperture1 results in a negative magnitude difference
exports.magnitudeDifferenceBetweenApertures = function (aperture1, aperture2) {
    return exports.getMagnitudeFromIllum(aperture2 * aperture2 / aperture1 / aperture1);
};

exports.diagObstructionArea = function (mirrorDia, diagSize) {
    return diagSize / mirrorDia * diagSize / mirrorDia;
};

// diagonal offset at right angle to focal plane: need to offset away from focuser and again towards the primary mirror;
// from http://www.telescope-optics.net/newtonian.htm
exports.calcDiagOffsetUsingFocalPoint = function (mirrorDia, focalLen, diagSize, diagToFocalPlaneDistance) {
    return (mirrorDia - diagSize) * diagSize / (4 * (focalLen - diagToFocalPlaneDistance));
};

// from an old Sky and Telescope magazine article (note that the offset is negative)
exports.calcDiagOffsetUsingFullIllumField = function (mirrorDia, focalLen, diagSize, diagToFocalPlaneDistance) {
    var sagitta,
        focalRatio,
        fullyIllumFieldDia,
        p,
        q,
        r,
        n,
        f,
        calcSagittaParabolic = exports.calcSagittaParabolic;

    sagitta = calcSagittaParabolic(mirrorDia, focalLen / mirrorDia);
    focalRatio = focalLen / mirrorDia;
    fullyIllumFieldDia = diagSize - diagToFocalPlaneDistance / focalRatio;
    p = fullyIllumFieldDia * (focalLen - sagitta) + diagToFocalPlaneDistance * (mirrorDia - fullyIllumFieldDia);
    q = 2 * (focalLen - sagitta) - (mirrorDia - fullyIllumFieldDia);
    r = 2 * (focalLen - sagitta) + (mirrorDia - fullyIllumFieldDia);
    n = p / q;
    f = p / r;
    return (f - n) / 2;
};

// adjusts offset for field diameter by using the diagonal to eye distance;
// since diagToEyeDistance > diagToFocalPlaneDistance, the offset will shrink as the diagonal appears closer to the primary mirror;
// from http://www.telescope-optics.net/newtonian.htm
exports.calcDiagOffsetUsingEyeToDiagDistance = function (diagMinorAxis, diagToEyeDistance) {
    return -diagMinorAxis * diagMinorAxis / 4 / diagToEyeDistance;
};

/* from my offset based on field edge study:
    for equal angles, the vertical/horizontal ratio of the
    lefthand side will equal that of the righthand side
    lefthand side: vertical = l - ma/2 - offset
    righthand side: vertical = l + ma/2 + offset
    lefthand side: horizontal = ma/2 - fieldDia/2 - offset
    righthand side: horizontal = ma/2 - fieldDia/2 + offset
    x = l - ma/2; y = l + ma/2; z = ma/2 - fieldDia/2;
    (x-o)/(z-o)=(y+o)/(z+o) ; (z+o)(x-o) = (y+o)(z-o);
    zx - zo + xo - o^2 = yz - yo + zo - o^2;
    xz - zo + xo = yz - yo + zo;
    -zo + xo + yo - zo = yz - xz;
    o (y - 2z + x) = yz - xz;
    o = z(y - x) / (y - 2z + x)
    note that y - x = ma, therefore o also = z * ma / (y - 2z + x)
    ma = 2.828; ma/2 = 1.414; fieldDia/2 = 1; l = 3
    x = 1.586; y = 4.414; z = 0.414
    o = 0.414*2.828/(4.414-2*0.414+1.586)
    o =1.171/5.172=0.226
*/
exports.calcDiagOffsetUsingFieldEdge = function(diagSize, diagToFocalPlaneDistance, fieldDia) {
    var diagSizeHalf = diagSize / 2,
        x = diagToFocalPlaneDistance - diagSizeHalf,
        y = diagToFocalPlaneDistance + diagSizeHalf,
        z = diagSizeHalf - fieldDia / 2;

    return  z * (y - x) / (y - 2 * z + x);
};

exports.getDiagIllumArray = function (mirrorDia, focalLen, diagSize, diagToFocalPlaneDistance, increment, maxField) {
    var roundedMaxField,
        fieldRadius,
        steps,
        illumArray,
        ix,
        offAxisDistance,
        offAxisIllumination,
        int = MLB.sharedLib.int,
        resolveNumberToPrecision = MLB.sharedLib.resolveNumberToPrecision,
        calcOffAxisIllumination = exports.calcOffAxisIllumination;

    roundedMaxField = resolveNumberToPrecision(maxField + increment, increment);
    fieldRadius = roundedMaxField / 2;
    steps = int(fieldRadius / increment);
    illumArray = [];
    for (ix = 0; ix <= steps; ix++) {
        offAxisDistance = increment * ix;
        offAxisIllumination = calcOffAxisIllumination(mirrorDia, focalLen, diagSize, diagToFocalPlaneDistance, offAxisDistance);
        illumArray[steps + ix] = [offAxisDistance, offAxisIllumination];
        illumArray[steps - ix] = [-offAxisDistance, offAxisIllumination];
    }
    return illumArray;
};

// http://stackoverflow.com/questions/4247889/area-of-intersection-between-two-circles
exports.areaIntersectingCircles = function (x0, y0, r0, x1, y1, r1) {
    var rr0 = r0 * r0,
        rr1 = r1 * r1,
        d = Math.sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0)),
        phi,
        theta,
        area1,
        area2;

    // Circles do not overlap
    if (d > r1 + r0) {
        return 0;
    }

    // Circle1 is completely inside circle0
    if (d <= Math.abs(r0 - r1) && r0 >= r1) {
        // Return area of circle1
        return Math.PI * rr1;
    }

    // Circle0 is completely inside circle1
    if (d <= Math.abs(r0 - r1) && r0 < r1) {
        // Return area of circle0
        return Math.PI * rr0;
    }

    // Circles partially overlap
    phi = (Math.acos((rr0 + (d * d) - rr1) / (2 * r0 * d))) * 2;
    theta = (Math.acos((rr1 + (d * d) - rr0) / (2 * r1 * d))) * 2;
    area1 = 0.5 * theta * rr1 - 0.5 * rr1 * Math.sin(theta);
    area2 = 0.5 * phi * rr0 - 0.5 * rr0 * Math.sin(phi);

    // Return area of intersection
    return area1 + area2;
};

// binoscope secondary and tiertiary mirrors calculator; see binoLayoutForNewtDesigner.jpg

exports.calcBinoscope = function (mirrorDia, focalRatio, tertiaryOffsetFromEdgeOfPrimary, focalPointToTertiaryDistance, focalPointToSecondaryDistance) {
    var focalLength,
        primaryToSecondaryDistance,
        secondaryToTiertiaryDistance,
        secondaryToTiertiaryVerticalLength,
        secondaryToTiertiaryHorizontalLength,
        elbowAngleRad,
        elbowAngleDeg,
        anglePrimaryMirrorEdgeToFocalPointRad,
        anglePrimaryMirrorEdgeToFocalPointDeg,
        anglePrimaryMirrorEdgeToFocalPointAndVerticalRad,
        anglePrimaryMirrorEdgeToFocalPointAndVerticalDeg,
        anglePrimaryMirrorEdgeToFocalPointAndVerticalOutsideRad,
        anglePrimaryMirrorEdgeToFocalPointAndVerticalOutsideDeg,
        secondaryCenterToPrimaryMirrorEdgeRayLength,
        angleSecondaryFaceToVerticalRad,
        angleSecondaryFaceToVerticalDeg,
        angle3UpperRad,
        angle3UpperDeg,
        angle3LowerRad,
        angle3LowerDeg,
        secondaryUpperLength,
        secondaryLowerLength,
        secondaryMajorAxis,
        secondaryMinorAxis,
        secondaryOffset,
        secondaryUpperPointXLength,
        secondaryUpperPointYLength,
        secondaryLowerPointXLength,
        secondaryLowerPointYLength,
        tiertiaryToSecondaryRatio,
        tiertiaryUpperLength,
        tiertiaryLowerLength,
        tiertiaryMajorAxis,
        tiertiaryMinorAxis,
        tiertiaryOffset,
        tiertiaryUpperPointXLength,
        tiertiaryUpperPointYLength,
        tiertiaryLowerPointXLength,
        tiertiaryLowerPointYLength,
        uom = MLB.sharedLib.uom;

    focalLength = mirrorDia * focalRatio;
    primaryToSecondaryDistance = focalLength - focalPointToSecondaryDistance;
    secondaryToTiertiaryDistance = focalPointToSecondaryDistance - focalPointToTertiaryDistance;
    secondaryToTiertiaryVerticalLength = mirrorDia / 2 + tertiaryOffsetFromEdgeOfPrimary;
    secondaryToTiertiaryHorizontalLength = Math.sqrt(Math.pow(secondaryToTiertiaryDistance, 2) - Math.pow(secondaryToTiertiaryVerticalLength, 2));

    // if no elbow, then angle is 90 deg
    elbowAngleRad = Math.acos(secondaryToTiertiaryHorizontalLength / secondaryToTiertiaryDistance);
    elbowAngleDeg = elbowAngleRad / uom.degToRad;

    // get primary mirror edge to focal point angle and to vertical line
    anglePrimaryMirrorEdgeToFocalPointRad = Math.atan(mirrorDia / 2 / focalLength);
    anglePrimaryMirrorEdgeToFocalPointDeg = anglePrimaryMirrorEdgeToFocalPointRad / uom.degToRad;
    anglePrimaryMirrorEdgeToFocalPointAndVerticalRad = uom.qtrRev - anglePrimaryMirrorEdgeToFocalPointRad;
    anglePrimaryMirrorEdgeToFocalPointAndVerticalDeg = anglePrimaryMirrorEdgeToFocalPointAndVerticalRad / uom.degToRad;
    anglePrimaryMirrorEdgeToFocalPointAndVerticalOutsideRad = uom.qtrRev + anglePrimaryMirrorEdgeToFocalPointRad;
    anglePrimaryMirrorEdgeToFocalPointAndVerticalOutsideDeg = anglePrimaryMirrorEdgeToFocalPointAndVerticalOutsideRad / uom.degToRad;

    // distance from secondary center to top of ray from primary mirror edge to focal point
    secondaryCenterToPrimaryMirrorEdgeRayLength = focalPointToSecondaryDistance / focalRatio / 2;

    // angle from secondary face to a vertical line is half that of the elbow angle
    angleSecondaryFaceToVerticalRad = elbowAngleRad / 2;
    angleSecondaryFaceToVerticalDeg = angleSecondaryFaceToVerticalRad / uom.degToRad;

    // solve for upward half of secondary size
    angle3UpperRad = uom.halfRev - anglePrimaryMirrorEdgeToFocalPointAndVerticalRad - angleSecondaryFaceToVerticalRad;
    angle3UpperDeg = angle3UpperRad / uom.degToRad;
    secondaryUpperLength = Math.sin(anglePrimaryMirrorEdgeToFocalPointAndVerticalRad) * secondaryCenterToPrimaryMirrorEdgeRayLength / Math.sin(angle3UpperRad);

    // solve for lower half of secondary size
    angle3LowerRad = uom.halfRev - anglePrimaryMirrorEdgeToFocalPointAndVerticalOutsideRad - angleSecondaryFaceToVerticalRad;
    angle3LowerDeg = angle3UpperRad / uom.degToRad;
    secondaryLowerLength = Math.sin(anglePrimaryMirrorEdgeToFocalPointAndVerticalOutsideRad) * secondaryCenterToPrimaryMirrorEdgeRayLength / Math.sin(angle3LowerRad);

    // secondary size and offset
    secondaryMajorAxis = secondaryUpperLength + secondaryLowerLength;
    secondaryMinorAxis = secondaryCenterToPrimaryMirrorEdgeRayLength * 2;
    secondaryOffset = secondaryLowerLength - secondaryMajorAxis / 2;

    // secondary end point distances
    secondaryUpperPointYLength = Math.cos(angleSecondaryFaceToVerticalRad) * secondaryUpperLength;
    secondaryUpperPointXLength = Math.sin(angleSecondaryFaceToVerticalRad) * secondaryUpperLength;
    secondaryLowerPointYLength = Math.cos(angleSecondaryFaceToVerticalRad) * secondaryLowerLength;
    secondaryLowerPointXLength = Math.sin(angleSecondaryFaceToVerticalRad) * secondaryLowerLength;

    // tiertiary is a smaller version of the secondary, since the finishing optical axis is parallel to the primary mirror axis
    tiertiaryToSecondaryRatio = focalPointToTertiaryDistance / focalPointToSecondaryDistance;
    tiertiaryUpperLength = secondaryUpperLength * tiertiaryToSecondaryRatio;
    tiertiaryLowerLength = secondaryLowerLength * tiertiaryToSecondaryRatio;
    tiertiaryMajorAxis = secondaryMajorAxis * tiertiaryToSecondaryRatio;
    tiertiaryMinorAxis = secondaryMinorAxis * tiertiaryToSecondaryRatio;
    tiertiaryOffset = secondaryOffset * tiertiaryToSecondaryRatio;
    tiertiaryUpperPointXLength = secondaryUpperPointXLength * tiertiaryToSecondaryRatio;
    tiertiaryUpperPointYLength = secondaryUpperPointYLength * tiertiaryToSecondaryRatio;
    tiertiaryLowerPointXLength = secondaryLowerPointXLength * tiertiaryToSecondaryRatio;
    tiertiaryLowerPointYLength = secondaryLowerPointYLength * tiertiaryToSecondaryRatio;

    return {
        focalLength: focalLength,
        primaryToSecondaryDistance: primaryToSecondaryDistance,
        secondaryToTiertiaryDistance: secondaryToTiertiaryDistance,
        secondaryToTiertiaryVerticalLength: secondaryToTiertiaryVerticalLength,
        secondaryToTiertiaryHorizontalLength: secondaryToTiertiaryHorizontalLength,
        elbowAngleRad: elbowAngleRad,
        elbowAngleDeg: elbowAngleDeg,
        anglePrimaryMirrorEdgeToFocalPointRad: anglePrimaryMirrorEdgeToFocalPointRad,
        anglePrimaryMirrorEdgeToFocalPointDeg: anglePrimaryMirrorEdgeToFocalPointDeg,
        anglePrimaryMirrorEdgeToFocalPointAndVerticalRad: anglePrimaryMirrorEdgeToFocalPointAndVerticalRad,
        anglePrimaryMirrorEdgeToFocalPointAndVerticalDeg: anglePrimaryMirrorEdgeToFocalPointAndVerticalDeg,
        secondaryCenterToPrimaryMirrorEdgeRayLength: secondaryCenterToPrimaryMirrorEdgeRayLength,
        angleSecondaryFaceToVerticalRad: angleSecondaryFaceToVerticalRad,
        angleSecondaryFaceToVerticalDeg: angleSecondaryFaceToVerticalDeg,
        secondaryUpperLength: secondaryUpperLength,
        secondaryLowerLength: secondaryLowerLength,
        secondaryMajorAxis: secondaryMajorAxis,
        secondaryMinorAxis: secondaryMinorAxis,
        secondaryOffset: secondaryOffset,
        secondaryUpperPointYLength: secondaryUpperPointYLength,
        secondaryUpperPointXLength: secondaryUpperPointXLength,
        secondaryLowerPointYLength: secondaryLowerPointYLength,
        secondaryLowerPointXLength: secondaryLowerPointXLength,
        // same angle as secondary since finishing optical axis is parallel to primary mirror axis
        angleTiertiaryFaceToVerticalRad: angleSecondaryFaceToVerticalRad,
        angleTiertiaryFaceToVerticalDeg: angleSecondaryFaceToVerticalDeg,
        tiertiaryUpperLength: tiertiaryUpperLength,
        tiertiaryLowerLength: tiertiaryLowerLength,
        tiertiaryMajorAxis: tiertiaryMajorAxis,
        tiertiaryMinorAxis: tiertiaryMinorAxis,
        tiertiaryOffset: tiertiaryOffset,
        tiertiaryUpperPointYLength: tiertiaryUpperPointYLength,
        tiertiaryUpperPointXLength: tiertiaryUpperPointXLength,
        tiertiaryLowerPointYLength: tiertiaryLowerPointYLength,
        tiertiaryLowerPointXLength: tiertiaryLowerPointXLength
    };
};

// folded Newtonian

// focalPointToDiagDistance is optional parm
exports.calcFoldedNewtonian = function (mirrorDia, focalRatio, diagSize, tertiaryOffsetFromEdgeOfPrimary, focalPointToTertiaryDistance, focalPointToDiagDistance) {
    var diagToPrimaryMirrorDistance,
        focalPointToPrimaryMirrorDistance,
        bentLightPathLength,
        bentLightPathVerticalLength,
        bentLightPathHorizontalLength,
        elbowAngleRad,
        elbowAngleDeg,
        diagMajorAxisSize,
        uom = MLB.sharedLib.uom;

    if (focalPointToDiagDistance === undefined) {
        focalPointToDiagDistance = focalRatio * diagSize;
    }
    diagToPrimaryMirrorDistance = focalRatio * mirrorDia - focalPointToDiagDistance;

    // the distance along the primary optical axis from the focal point projected onto this axis to the diagonal or folding mirror (excluding the focalPointToTertiaryDistance) that the light can be 'folded backwards' is one side of a triangle with the hypotenuse = bentLightPathLength and other side the bentLightPathVerticalLength;
    bentLightPathLength = focalPointToDiagDistance - focalPointToTertiaryDistance;
    bentLightPathVerticalLength = mirrorDia / 2 + tertiaryOffsetFromEdgeOfPrimary;
    bentLightPathHorizontalLength = Math.sqrt(Math.pow(bentLightPathLength, 2) - Math.pow(bentLightPathVerticalLength, 2));
    focalPointToPrimaryMirrorDistance = diagToPrimaryMirrorDistance - bentLightPathHorizontalLength;
    /*
        canvas x is the horizontal coordinate, canvas y is the vertical coordinate;
        atan2(y,x): y is the horizontal coordinate, x is the vertical coordinate
        atan2(1,0) aims to the right; atan2(0,1) aims to the top; atan2(0,-1) aims to the bottom; atan2(-1,-1) aims to the left;
        or, 0deg aims to the right, 90deg aims to the top, -90deg aims to the bottom and 180/180deg aims to the left;
        using the atan2 coordinate system (y to the right, x to the top, 0 deg to the right, grows counter-clockwise:
    */
    elbowAngleRad = Math.atan2(bentLightPathVerticalLength, bentLightPathHorizontalLength);
    elbowAngleDeg = elbowAngleRad / uom.degToRad;
    diagMajorAxisSize = diagSize / Math.cos(elbowAngleRad / 2);

    return {
        diagToPrimaryMirrorDistance: diagToPrimaryMirrorDistance,
        focalPointToPrimaryMirrorDistance: focalPointToPrimaryMirrorDistance,
        focalPointToDiagDistance: focalPointToDiagDistance,
        elbowAngleDeg: elbowAngleDeg,
        diagMajorAxisSize: diagMajorAxisSize
    };
};

exports.getFoldedNewtonianScalingFactor = function (width, height, focalRatio, diagSize, focalPointToTertiaryDistance, tertiaryOffsetFromEdgeOfPrimary, diagToMirrorDistance) {
    var maxWidth,
        maxHeight,
        modelWidthToHeightRatio,
        graphicsWidthToHeightRatio,
        scalingFactor,
        margin;

    maxWidth = diagToMirrorDistance;
    maxHeight = (diagSize * focalRatio + focalPointToTertiaryDistance + tertiaryOffsetFromEdgeOfPrimary) * 2;
    modelWidthToHeightRatio = maxWidth / maxHeight;
    graphicsWidthToHeightRatio = width / height;
    margin = 0.1;

    if (maxWidth === 0 || maxHeight === 0) {
        scalingFactor = 0;
    } else if (modelWidthToHeightRatio > graphicsWidthToHeightRatio) {
        scalingFactor = width / maxWidth;
    } else {
        scalingFactor = height / maxHeight;
    }
    scalingFactor *= 1 - margin * 2;
    return scalingFactor;
};

// visual detection calculator

exports.limitingMagnitude = function (apertureIn) {
    var log10 = MLB.sharedLib.log10;

    // from http://adsabs.harvard.edu/full/1947PASP...59..253B
    // assumes 2 mag gain due to high magnification; no gain in limiting magnitude below 1.8mm exit pupil, 'RFT' exit pupil lowers limiting magnitude ~1
    return 10.8 + 5 * log10(apertureIn);
};

exports.apertureInchesFromMagnitude = function (magnitude) {
    return Math.pow(10, (magnitude - 10.8) / 5);
};

/* see http://www.users.on.net/~dbenn/ECMAScript/surface_brightness.html
       view-source:http://www.users.on.net/~dbenn/ECMAScript/surface_brightness.html
   note that object surface brightness above is in magnitude per square arc-minute whereas the Blackwell data requires object surface brightness in square arc-seconds
   magnitude difference between square arc-seconds and square arc-minutes is (using circular area) 8.6283145212238; if using a square area it is 8.89075625191822 - see MLB.sharedLib.uom and calcLib unit tests
*/

exports.calcSurfaceBrightnessFromArea = function (objMag, minObjArcmin, maxObjArcmin) {
    var uom = MLB.sharedLib.uom,
        getMagnitudeFromIllum = exports.getMagnitudeFromIllum,
        getIllumFromMagnitude = exports.getIllumFromMagnitude;

    return getMagnitudeFromIllum(getIllumFromMagnitude(objMag) / (minObjArcmin * maxObjArcmin * uom.sqrArcminToSqrArcsec));
};

exports.calcMagnitudeFromSurfaceBrightnessAndArea = function (objSurfaceBrightness, minObjArcmin, maxObjArcmin) {
    var uom = MLB.sharedLib.uom,
         getMagnitudeFromIllum = exports.getMagnitudeFromIllum,
        getIllumFromMagnitude = exports.getIllumFromMagnitude;

    return getMagnitudeFromIllum(getIllumFromMagnitude(objSurfaceBrightness) * minObjArcmin * maxObjArcmin * uom.sqrArcminToSqrArcsec);
};

// same answer as above function

exports.calcMagnitudePerArcMinSquaredFromSurfaceBrightnessAndEllipticalArea = function (objSurfaceBrightness, minObjArcmin, maxObjArcmin) {
    var getMagnitudeFromIllum = exports.getMagnitudeFromIllum,
        getIllumFromMagnitude = exports.getIllumFromMagnitude;

    return getMagnitudeFromIllum(getIllumFromMagnitude(objSurfaceBrightness) * minObjArcmin / 2 * maxObjArcmin / 2 * Math.PI);
};

// area of aperture times area of field of view in cm^2deg^2
exports.calcEtendue = function (apertureCm, FOVDeg) {
    return apertureCm * apertureCm * FOVDeg * FOVDeg * Math.PI * Math.PI / 16;
};

exports.VisualDetectCalcParms = function () {
    this.apertureIn = 0;
    this.bkgndBrightEye = 0;
    this.objName = '';
    this.objMag = 0;
    this.maxObjArcmin = 0;
    this.minObjArcmin = 0;
    this.eyepieceExitPupilmm = 0;
    this.apparentFoV = 0;
    this.eyeLimitMag = 6;
    this.exitPupilmm = 7;
    this.scopeTrans = 0.8;
    this.singleEyeFactor = 0.5;

    this.copyFrom = function () {
        var to = new exports.VisualDetectCalcParms(),
            v;
        for (v in this) {
            if (this.hasOwnProperty(v)) {
                to[v] = this[v];
            }
        }
        return to;
    };
};

exports.visualDetectCalcData = {
    angleSize: 7,
    ltcSize: 24,
    firstRowBkgndBright: 4,
    lastRowBkgndBright: 27,
    eyeLimitMagGainAtX: 2.5,

    logAngle: [-0.2255, 0.5563, 0.9859, 1.260, 1.742, 2.083, 2.556],

    // log threshold contrast as function of background brightness for angles of: 0.595, 3.60, 9.68, 18.2, 55.2, 121, 360
    // 0 to 23: first row is brightness of 4, last row is brightness of 27
    ltc: [
        [-0.3769, -1.8064, -2.3368, -2.4601, -2.5469, -2.5610, -2.5660],
        [-0.3315, -1.7747, -2.3337, -2.4608, -2.5465, -2.5607, -2.5658],
        [-0.2682, -1.7345, -2.3310, -2.4605, -2.5467, -2.5608, -2.5658],
        [-0.1982, -1.6851, -2.3140, -2.4572, -2.5481, -2.5615, -2.5665],
        [-0.1238, -1.6252, -2.2791, -2.4462, -2.5463, -2.5597, -2.5646],
        [-0.0424, -1.5529, -2.2297, -2.4214, -2.5343, -2.5501, -2.5552],
        [0.0498, -1.4655, -2.1659, -2.3763, -2.5047, -2.5269, -2.5333],
        [0.1596, -1.3581, -2.0810, -2.3036, -2.4499, -2.4823, -2.4937],
        [0.2934, -1.2256, -1.9674, -2.1965, -2.3631, -2.4092, -2.4318],
        [0.4557, -1.0673, -1.8186, -2.0531, -2.2445, -2.3083, -2.3491],
        [0.6500, -0.8841, -1.6292, -1.8741, -2.0989, -2.1848, -2.2505],
        [0.8808, -0.6687, -1.3967, -1.6611, -1.9284, -2.0411, -2.1375],
        [1.1558, -0.3952, -1.1264, -1.4176, -1.7300, -1.8727, -2.0034],
        [1.4822, -0.0419, -0.8243, -1.1475, -1.5021, -1.6768, -1.8420],
        [1.8559, 0.3458, -0.4924, -0.8561, -1.2661, -1.4721, -1.6624],
        [2.2669, 0.6960, -0.1315, -0.5510, -1.0562, -1.2892, -1.4827],
        [2.6760, 1.0880, 0.2060, -0.3210, -0.8800, -1.1370, -1.3620],
        [2.7766, 1.2065, 0.3467, -0.1377, -0.7361, -0.9964, -1.2439],
        [2.9304, 1.3821, 0.5353, 0.0328, -0.5605, -0.8606, -1.1187],
        [3.1634, 1.6107, 0.7708, 0.2531, -0.3895, -0.7030, -0.9681],
        [3.4643, 1.9034, 1.0338, 0.4943, -0.2033, -0.5259, -0.8288],
        [3.8211, 2.2564, 1.3265, 0.7605, 0.0172, -0.2992, -0.6394],
        [4.2210, 2.6320, 1.6990, 1.1320, 0.2860, -0.0510, -0.4080],
        [4.6100, 3.0660, 2.1320, 1.5850, 0.6520, 0.2410, -0.1210]
    ]
};

exports.VisualDetectCalc = function () {
    var x,
        minX,
        maxXMaxObj,
        maxXMinObj,
        maxXBasedOn1MmExitPupil,
        actualFoV,
        fitsFoV,
        surfBrightObj,
        objPlusBkgnd,
        contrastPercent,
        logContrastObject,
        bkgndBrightAtX,
        illumReductionAtX,
        objSizeDegAtX,
        surfBrightObjAtX,
        surfBrightObjPlusBkgndAtX,
        apparentAngle,
        logApparentAngle,
        bbX,
        intbbX,
        bbIxA,
        bbIxB,
        ix,
        interpAngle,
        interpA,
        interpB,
        logContrastRequired,
        logContrastDiff,
        detectable,
        int = MLB.sharedLib.int,
        log10 = MLB.sharedLib.log10,
        roundToDecimal = MLB.sharedLib.roundToDecimal,
        calcSurfaceBrightnessFromArea = exports.calcSurfaceBrightnessFromArea,
        getMagnitudeFromIllum = exports.getMagnitudeFromIllum,
        getIllumFromMagnitude = exports.getIllumFromMagnitude,
        addArrayMagnitudes = exports.addArrayMagnitudes,
        visualDetectCalcData = exports.visualDetectCalcData;

    this.data = visualDetectCalcData;

    this.calc = function (parms) {
        // magnifications
        x = parms.apertureIn * 25.4 / parms.eyepieceExitPupilmm;
        minX = parms.apertureIn * 25.4 / parms.exitPupilmm;
        maxXMaxObj = parms.apparentFoV / (parms.minObjArcmin / 60);
        maxXMinObj = parms.apparentFoV / (parms.maxObjArcmin / 60);
        maxXBasedOn1MmExitPupil = 25.4 * parms.apertureIn;
        if (maxXMaxObj > maxXBasedOn1MmExitPupil) {
            maxXMaxObj = maxXBasedOn1MmExitPupil;
        }
        if (maxXMinObj > maxXBasedOn1MmExitPupil) {
            maxXMinObj = maxXBasedOn1MmExitPupil;
        }
        // fields of view
        actualFoV = parms.apparentFoV / x;
        fitsFoV = x <= maxXMaxObj;

        // size
        objSizeDegAtX = roundToDecimal(x * parms.minObjArcmin / 60, 1) + ' x ' + roundToDecimal(x * parms.maxObjArcmin / 60, 1);

        // object brightness
        surfBrightObj = calcSurfaceBrightnessFromArea(parms.objMag, parms.minObjArcmin, parms.maxObjArcmin);
        objPlusBkgnd = addArrayMagnitudes([surfBrightObj, parms.bkgndBrightEye]);
        contrastPercent = (getIllumFromMagnitude(objPlusBkgnd - parms.bkgndBrightEye) - 1) * 100;

        // at magnification of X
        illumReductionAtX = (minX / x) * parms.singleEyeFactor * parms.scopeTrans;
        surfBrightObjAtX = getMagnitudeFromIllum(getIllumFromMagnitude(surfBrightObj) * illumReductionAtX);
        bkgndBrightAtX = getMagnitudeFromIllum(getIllumFromMagnitude(parms.bkgndBrightEye) * illumReductionAtX);
        surfBrightObjPlusBkgndAtX = addArrayMagnitudes([surfBrightObjAtX, bkgndBrightAtX]);

        // log of the magnitude difference between object and background;
        // question is which one was data built on?
        // these two following expressions give the same result...
        logContrastObject = -0.4 * (surfBrightObj - parms.bkgndBrightEye);
        //logContrastObject = -0.4 * (surfBrightObjAtX - bkgndBrightAtX);
        // ditto for these two...
        //logContrastObject = -0.4 * (objPlusBkgnd - parms.bkgndBrightEye);
        //logContrastObject = -0.4 * (surfBrightObjPlusBkgndAtX - bkgndBrightAtX);

        // 2 dimensional interpolation of LTC array
        apparentAngle = x * parms.minObjArcmin;
        logApparentAngle = log10(apparentAngle);
        bbX = bkgndBrightAtX;
        // int of background brightness
        intbbX = int(bbX);
        // background brightness index A
        bbIxA = intbbX - this.data.firstRowBkgndBright;
        // min index must be at least 0
        if (bbIxA < 0) {
            bbIxA = 0;
        }
        // max bbIxA index cannot > 22 to leave room for bbIxB
        if (bbIxA > this.data.ltcSize - 2) {
            bbIxA = this.data.ltcSize - 2;
        }
        // background brightness index B
        bbIxB = bbIxA + 1;
        ix = 0;
        while (ix < this.data.angleSize && logApparentAngle > this.data.logAngle[ix]) {
            ix++;
        }
        // found 1st Angle[] value > logApparentAngle, so back up 2
        ix -= 2;
        if (ix < 0) {
            ix = 0;
            logApparentAngle = this.data.logAngle[0];
        }
        // eg, if LogApparentAngle = 4 and Angle[ix] = 3 and Angle[ix+1] = 5: * InterpAngle = .5, or .5 of the way between Angle[ix] and Angle[ix+1]
        interpAngle = (logApparentAngle - this.data.logAngle[ix]) / (this.data.logAngle[ix + 1] - this.data.logAngle[ix]);
        interpA = this.data.ltc[bbIxA][ix] + interpAngle * (this.data.ltc[bbIxA][ix + 1] - this.data.ltc[bbIxA][ix]);
        interpB = this.data.ltc[bbIxB][ix] + interpAngle * (this.data.ltc[bbIxB][ix + 1] - this.data.ltc[bbIxB][ix]);
        // log contrast
        if (bbX < this.data.firstRowBkgndBright) {
            bbX = this.data.firstRowBkgndBright;
        }
        if (intbbX >= this.data.lastRowBkgndBright) {
            logContrastRequired = interpB + (bbX - this.data.lastRowBkgndBright) * (interpB - interpA);
        } else {
            logContrastRequired = interpA + (bbX - intbbX) * (interpB - interpA);
        }
        logContrastDiff = logContrastObject - logContrastRequired;
        detectable = logContrastDiff > 0;

        return {
            parms: parms,
            contrastPercent: contrastPercent,
            logContrastDiff: logContrastDiff,
            logContrastObject: logContrastObject,
            logContrastRequired: logContrastRequired,
            detectable: detectable,
            x: x,
            minX: minX,
            maxXMaxObj: maxXMaxObj,
            maxXMinObj: maxXMinObj,
            fitsFoV: fitsFoV,
            actualFoV: actualFoV,
            surfBrightObj: surfBrightObj,
            objPlusBkgnd: objPlusBkgnd,
            objSizeDegAtX: objSizeDegAtX,
            illumReductionAtX: illumReductionAtX,
            bkgndBrightAtX: bkgndBrightAtX,
            surfBrightObjAtX: surfBrightObjAtX,
            surfBrightObjPlusBkgndAtX: surfBrightObjPlusBkgndAtX
        };
    };

    this.includeResultAsString = function (result) {
        var r,
            p,
            s;

        r = result;
        p = r.parms;
        s = 'input values:\n' +
            '  aperture (in): ' + p.apertureIn + '\n' +
            '  eye limiting magnitude: ' + p.eyeLimitMag + '\n' +
            '  eye max exit pupil mm :' + p.exitPupilmm + '\n' +
            '  sky background brightness: ' + p.bkgndBrightEye + '\n' +
            '  object name: ' + p.objName + '\n' +
            '  object integrated magnitude: ' + p.objMag + '\n' +
            '  object dimensions arcmin: ' + p.maxObjArcmin + ' x ' + p.minObjArcmin + '\n' +
            '  eyepiece apparent field deg: ' + p.apparentFoV + '\n' +
            'calculated values:\n' +
            '  magnification: ' + Math.round(r.x) + '\n' +
            '  actual field deg: ' + r.actualFoV + '\n' +
            '  object fits FoV? ' + r.fitsFoV + '\n' +
            '  object surface brightness MPAS: ' + r.surfBrightObj + '\n' +
            '  object + sky brightness MPAS: ' + r.objPlusBkgnd + '\n' +
            '  at X:\n' +
            '    object size deg: ' + r.objSizeDegAtX + '\n' +
            '    brightness reduction: ' + r.illumReductionAtX + '\n' +
            '    object surface brightness MPAS: ' + r.surfBrightObjAtX + '\n' +
            '    sky MPAS: ' + r.bkgndBrightAtX + '\n' +
            '    object+sky surface brightness MPAS: ' + r.surfBrightObjPlusBkgndAtX + '\n' +
            '  contrast of object+sky to sky %: ' + r.contrastPercent + '\n' +
            '  log (object - sky): ' + r.logContrastObject + '\n' +
            '  log required: ' + r.logContrastRequired + '\n' +
            '  log difference: ' + r.logContrastDiff + '\n' +
            '  detectable? ' + r.detectable + '\n';

        return {
            parms: p,
            result: r,
            text: s
        };
    };

    this.includeResultAsJSON = function (result) {
        var rnd = function (num) {
                return MLB.sharedLib.roundToDecimal(num, 2);
            },
            r,
            p,
            json;

        r = result;
        p = r.parms;
        json = [
            {label: 'aperture (inches - mm)', result: rnd(p.apertureIn) + ' - ' + rnd(p.apertureIn * 25.4)},
            {label: 'is the object detectable?', result: r.detectable},
            {label: 'best  detection magnification \'X\'', result: Math.round(r.x)},
            {label: 'eye pupil (mm)', result: rnd(p.apertureIn / r.x * 25.4)},
            {label: '..', result: ''},
            {label: 'illumination multipler at \'X\'', result: rnd(r.illumReductionAtX)},
            {label: 'log (object-sky)', result: rnd(r.logContrastObject)},
            {label: 'log required', result: rnd(r.logContrastRequired)},
            {label: 'log difference', result: rnd(r.logContrastDiff)},
            {label: 'contrast of object+sky to sky %', result: rnd(r.contrastPercent)},
            {label: '..', result: ''},
            {label: 'eyepiece apparent field (degrees)', result: p.apparentFoV},
            {label: 'actual field (degrees)', result: rnd(r.actualFoV)},
            {label: 'Does the object fit into the field? ', result: r.fitsFoV},
            {label: 'object apparent size at X (degrees)', result: r.objSizeDegAtX},
            {label: '..', result: ''},
            {label: 'object surface brightness MPAS', result: rnd(r.surfBrightObj)},
            {label: 'sky background brightness MPAS', result: p.bkgndBrightEye},
            {label: 'object+sky brightness MPAS', result: rnd(r.objPlusBkgnd)},
            {label: '..', result: ''},
            {label: 'object brightness at \'X\' MPAS', result: rnd(r.surfBrightObjAtX)},
            {label: 'sky background brightness at \'X\' MPAS', result: rnd(r.bkgndBrightAtX)},
            {label: 'object+sky at \'X\' MPAS', result: rnd(r.surfBrightObjPlusBkgndAtX)}
        ];

        return {
            parms: p,
            result: r,
            json: json
        };
    };

    this.includeResultAsJSON_NewtDesigner = function (result, telescopeFLmm) {
        var rnd = function (num) {
                return MLB.sharedLib.roundToDecimal(num, 2);
            },
            r,
            p,
            json;

        r = result;
        p = r.parms;
        json = [
            {label: 'Is the object detectable?', result: r.detectable},
            {label: 'Contrast of object+sky to sky', result: rnd(r.contrastPercent) + '%'},
            {label: 'Best detection magnification \'X\'', result: Math.round(r.x) + 'x'},
            {label: 'Eye pupil', result: rnd(p.apertureIn / r.x * 25.4) + ' mm'},
            {label: 'Best eyepiece FL', result: rnd(telescopeFLmm / r.x) + ' mm'},
            {label: 'Object apparent size (at X)', result: r.objSizeDegAtX + ' degrees'},
            {label: 'Object surface brightness (at X)', result: rnd(r.surfBrightObj) + ' (' + rnd(r.surfBrightObjAtX) + ') MPAS'},
            {label: 'Sky background brightness (at X)', result: p.bkgndBrightEye + ' (' + rnd(r.bkgndBrightAtX) + ') MPAS'},
            {label: 'Object+sky brightness (at X)', result: rnd(r.objPlusBkgnd) + ' (' + rnd(r.surfBrightObjPlusBkgndAtX) + ') MPAS'},
            {label: 'Illumination multipler (at X)', result: rnd(r.illumReductionAtX)},
            {label: 'Log difference (object-sky) - required', result: '(' + rnd(r.logContrastObject) + ') - (' + rnd(r.logContrastRequired) + ') = (' + rnd(r.logContrastDiff) + ') positive value means detectable'}
        ];

        return {
            parms: p,
            result: r,
            json: json
        };
    };
};

exports.VisualDetectCalcExitPupils = function (parms) {
    var epCalcs,
        vdc,
        eyepieceExitPupilmm,
        newParms,
        VisualDetectCalc = exports.VisualDetectCalc;

    epCalcs = [];
    vdc = new VisualDetectCalc();
    for (eyepieceExitPupilmm = 1; eyepieceExitPupilmm <= parms.eyepieceExitPupilmm; eyepieceExitPupilmm++) {
        newParms = parms.copyFrom();
        newParms.eyepieceExitPupilmm = eyepieceExitPupilmm;
        epCalcs.push(vdc.calc(newParms));
    }
    return epCalcs;
};

exports.VisualDetectCalcApertures = function (parms) {
    var apertureCalcs,
        dblApertureParms,
        halfApertureParms,
        VisualDetectCalcExitPupils = exports.VisualDetectCalcExitPupils;

    apertureCalcs = [];

    dblApertureParms = parms.copyFrom();
    dblApertureParms.apertureIn *= 2;
    apertureCalcs.push(new VisualDetectCalcExitPupils(dblApertureParms));

    apertureCalcs.push(new VisualDetectCalcExitPupils(parms));

    halfApertureParms = parms.copyFrom();
    halfApertureParms.apertureIn /= 2;
    apertureCalcs.push(new VisualDetectCalcExitPupils(halfApertureParms));

    return apertureCalcs;
};

// Newtonian baffles: focuser, diagonal and primary mirrors

exports.calcNewtBaffle = function (focalPlaneDia, focuserBarrelBottomToFocalPlaneDistance, focuserBarrelID, diagSizeMinorAxis, diagToFocalPlaneDistance, diagtoFocuserBaffleDistance, diagToOppositeSideBaffleDistance, primaryMirrorFocalLength, primaryToBaffleDistance, tubeID) {
    var focuserBaffleToFocalPlaneDistance,
        focuserBaffleOD,
        focuserBaffleID,
        diagonalBaffleOD,
        primaryBaffleOD,
        tubeExtension;

    focuserBaffleToFocalPlaneDistance = diagToFocalPlaneDistance - diagtoFocuserBaffleDistance;
    focuserBaffleOD = (focalPlaneDia + focuserBarrelID) / focuserBarrelBottomToFocalPlaneDistance * (focuserBaffleToFocalPlaneDistance - focuserBarrelBottomToFocalPlaneDistance) + focuserBarrelID;
    focuserBaffleID = (diagSizeMinorAxis - focalPlaneDia) / diagToFocalPlaneDistance * focuserBaffleToFocalPlaneDistance + focalPlaneDia;
    diagonalBaffleOD = (focalPlaneDia + focuserBaffleID) / focuserBaffleToFocalPlaneDistance * (diagtoFocuserBaffleDistance + diagToOppositeSideBaffleDistance) + focuserBaffleID;
    primaryBaffleOD = (diagSizeMinorAxis + focalPlaneDia) * (primaryMirrorFocalLength - primaryToBaffleDistance) / diagToFocalPlaneDistance - focalPlaneDia;
    tubeExtension = primaryMirrorFocalLength - (diagToFocalPlaneDistance * (focalPlaneDia + tubeID) / (diagSizeMinorAxis + focalPlaneDia));

    return {
        focuserBaffleID: focuserBaffleID,
        focuserBaffleOD: focuserBaffleOD,
        diagonalBaffleOD: diagonalBaffleOD,
        primaryBaffleOD: primaryBaffleOD,
        tubeExtension: tubeExtension
    };
};

// Z12 mounting errors in azimuth calculator

exports.MeasurementType = {
    real: 'real',
    apparent: 'apparent'
};

exports.initZ12Calc = function () {
    var initLatitudeDeg,
        altStepSizeDeg,
        positions,
        altDeg,
        position,
        uom = MLB.sharedLib.uom,
        ConvertStyle = MLB.coordLib.ConvertStyle,
        XForm = MLB.coordLib.XForm;

    initLatitudeDeg = 40;
    altStepSizeDeg = 2;
    positions = [];

    for (altDeg = -(90 - altStepSizeDeg); altDeg < 90; altDeg += altStepSizeDeg) {
        position = new MLB.coordLib.Position();
        position.alt = altDeg * uom.degToRad;
        positions.push(position);
    }
    return {
        xform: new XForm(ConvertStyle.matrix, initLatitudeDeg * uom.degToRad),
        positions: positions,
        azErrors: [],
        z1Errors: [],
        z2Errors: []
    };
};

exports.InitZ12Calc = {
    init: exports.initZ12Calc()
};

exports.setAlignment = function (xform, initType) {
    var InitType = MLB.coordLib.InitType;

    if (initType === InitType.altazimuth) {
        xform.presetAltaz();
    } else if (initType === InitType.equatorial) {
        xform.presetEquat();
    } else {
        throw ('unprocessed initType of ' + initType + ' in setAlignment');
    }
};

exports.generateZ12ErrorValues = function (z1Deg, z2Deg, latDeg, azDeg, xform, positions, azErrors, initType) {
    var az,
        positionsLength,
        ix,
        uom = MLB.sharedLib.uom,
        setAlignment = exports.setAlignment;

    xform.latitude = latDeg * uom.degToRad;
    xform.setFabErrorsDeg(0, 0, 0);
    setAlignment(xform, initType);
    xform.position.SidT = 0;
    az = azDeg * uom.degToRad;
    xform.position.az = az;

    // getEquat() with z123 = 0 and preset az
    positionsLength = positions.length;
    for (ix = 0; ix < positionsLength; ix++) {
        xform.position.alt = positions[ix].alt;
        xform.getEquat();
        positions[ix].RA = xform.position.RA;
        positions[ix].Dec = xform.position.Dec;
    }

    // get altaz with input z12 and RA+Dec from above;
    // az error is difference between this az and starting az above
    xform.setFabErrorsDeg(z1Deg, z2Deg, 0);
    setAlignment(xform, initType);
    positionsLength = positions.length;
    for (ix = 0; ix < positionsLength; ix++) {
        xform.position.RA = positions[ix].RA;
        xform.position.Dec = positions[ix].Dec;
        xform.getAltaz();
        azErrors[ix] = az - xform.position.az;
    }
};

exports.getZ12ErrorValues = function (z1Deg, z2Deg, latDeg, azDeg, initType) {
    var init,
        positionsLength,
        ix,
        InitZ12Calc = exports.InitZ12Calc,
        generateZ12ErrorValues = exports.generateZ12ErrorValues;

    init = InitZ12Calc.init;
    generateZ12ErrorValues(z1Deg, 0, latDeg, azDeg, init.xform, init.positions, init.azErrors, initType);
    positionsLength = init.positions.length;
    for (ix = 0; ix < positionsLength; ix++) {
        init.z1Errors[ix] = init.azErrors[ix];
    }
    generateZ12ErrorValues(0, z2Deg, latDeg, azDeg, init.xform, init.positions, init.azErrors, initType);
    positionsLength = init.positions.length;
    for (ix = 0; ix < positionsLength; ix++) {
        init.z2Errors[ix] = init.azErrors[ix];
    }
};

exports.setSecAxisDeg = function (position, initType) {
    var uom = MLB.sharedLib.uom,
        InitType = MLB.coordLib.InitType;

    return initType === (InitType.altazimuth || initType === InitType.star)
        ? position.alt / uom.degToRad
        : position.Dec / uom.degToRad;
};

exports.includePriAxisError = function (position, initType) {
    var InitType = MLB.coordLib.InitType;

    return initType === InitType.equatorial || ((initType === InitType.altazimuth || initType === InitType.star) && position.alt >= 0);
};

exports.buildZ12AzErrors = function (positions, zErrors, measurementType, initType) {
    var zPoints,
        positionsLength,
        ix,
        azErrorArcmin,
        uom = MLB.sharedLib.uom,
        setSecAxisDeg = exports.setSecAxisDeg,
        includePriAxisError = exports.includePriAxisError,
        MeasurementType = exports.MeasurementType;

    zPoints = [];
    positionsLength = positions.length;
    for (ix = 0; ix < positionsLength; ix++) {
        if (includePriAxisError(positions[ix], initType)) {
            azErrorArcmin = zErrors[ix] / uom.arcminToRad;
            if (measurementType === MeasurementType.apparent) {
                azErrorArcmin *= Math.cos(positions[ix].alt);
            }
            zPoints.push([azErrorArcmin, setSecAxisDeg(positions[ix], initType)]);
        }
    }
    return zPoints;
};

/*
Generate pointing errors across the sky for an altazimuth initialized telescope given random initialization errors.
Conclusions based on init star separation:
    bad for 0-20 and 160-180 degrees separation (error doubled from the init error),
    marginal for 20-45 and 135-160 degrees separation (error 30% greater of the init error),
    good for 40-140 degrees separation (error the same as the init error)
*/

exports.getRandomErrorDeg = function (maxErrorDeg) {
    return Math.random() * maxErrorDeg;
};

exports.createPerfectPositions = function (latitudeRad, spacingDeg) {
    var uom = MLB.sharedLib.uom,
        Position = MLB.coordLib.Position,
        convertStyle = MLB.coordLib.ConvertStyle.matrix,
        XForm = MLB.coordLib.XForm,
        xform,
        perfectPositions = [],
        altDeg,
        azSpacingDeg,
        azDeg;

    xform = new XForm(convertStyle, latitudeRad);
    xform.presetAltaz();

    for (altDeg = 0; altDeg < 90; altDeg += spacingDeg) {
        azSpacingDeg = spacingDeg / Math.cos(altDeg * uom.degToRad);
        for (azDeg = 0; azDeg < 360; azDeg += azSpacingDeg) {
            xform.position.SidT = 0;
            xform.position.az = azDeg * uom.degToRad;
            xform.position.alt = altDeg * uom.degToRad;
            xform.getEquat();
            perfectPositions.push(new Position(xform.position.RA, xform.position.Dec, xform.position.az, xform.position.alt, 0, 0));
        }
    }
    return perfectPositions;
};

exports.initErrorsPlot = function (latitudeRad, spacingDeg, numberTrials, maxErrorDeg, position1, position2, z1deg, z2deg) {
    var uom = MLB.sharedLib.uom,
        validHalfRev = MLB.coordLib.validHalfRev,
        copyPosition = MLB.coordLib.copyPosition,
        ConvertStyle = MLB.coordLib.ConvertStyle,
        XForm = MLB.coordLib.XForm,
        clearPosition = MLB.coordLib.clearPosition,
        initMatrixFacade = MLB.coordLib.initMatrixFacade,
        createPerfectPositions = exports.createPerfectPositions,
        getRandomErrorDeg = exports.getRandomErrorDeg,
        xform,
        cmws,
        variances,
        trials,
        ppIx,
        azError,
        altError,
        totalError,
        errors = [],
        plotData = [],
        perfectPositions = createPerfectPositions(latitudeRad, spacingDeg),
        lengthPerfectPositions = perfectPositions.length;

    // main loop
    for (trials = 0; trials < numberTrials; trials++) {
        xform = new XForm(ConvertStyle.matrix, latitudeRad);
        cmws = xform.cmws;
        variances = [getRandomErrorDeg(maxErrorDeg), getRandomErrorDeg(maxErrorDeg), getRandomErrorDeg(maxErrorDeg), getRandomErrorDeg(maxErrorDeg)];
        copyPosition(position1, cmws.one);
        copyPosition(position2, cmws.two);
        // increase az per altitude
        cmws.one.az += variances[0] * uom.degToRad / Math.cos(cmws.one.alt);
        cmws.one.alt += variances[1] * uom.degToRad;
        cmws.two.az += variances[2] * uom.degToRad / Math.cos(cmws.two.alt);
        cmws.two.alt += variances[3] * uom.degToRad;
        clearPosition(cmws.three);
        initMatrixFacade(cmws, 2);
        // post-init setting of z1 equivalent to pre-init setting of z1+changed init positions:
        // see test module 'z1 (axis twist or non-perpendicularity) initialization study' in coordLib unitTests.htm
        xform.setFabErrorsDeg(z1deg, z2deg, 0);

        // for each position in perfectPositions, grab equat coords and convert to altaz, comparing converted altaz to perfect altaz
        for (ppIx = 0; ppIx < lengthPerfectPositions; ppIx++) {
            copyPosition(perfectPositions[ppIx], xform.position);
            xform.getAltaz();
            // shrink az per altitude
            azError = Math.abs(validHalfRev(xform.position.az - perfectPositions[ppIx].az)) * Math.cos(xform.position.alt);
            altError = Math.abs(xform.position.alt - perfectPositions[ppIx].alt);
            totalError = Math.sqrt(azError * azError + altError * altError);

            if (isNaN(errors[ppIx])) {
                errors[ppIx] = 0;
            }
            errors[ppIx] += totalError / numberTrials;
        }
    }

    // create plot data
    for (ppIx = 0; ppIx < lengthPerfectPositions; ppIx++) {
        plotData[ppIx] = {error: errors[ppIx], azimuth: perfectPositions[ppIx].az, altitude: perfectPositions[ppIx].alt};
    }
    return plotData;
};

/*
data engine for altazimuth tracking errors that accrue from constant motions
*/

exports.createAltazConstantMotionTrackingErrors = function (latitudeRad, constantTrackRateTimeRad, spacingDeg) {
    var uom = MLB.sharedLib.uom,
        convertStyle = MLB.coordLib.ConvertStyle.trig,
        XForm = MLB.coordLib.XForm,
        xform,
        trackingRates,
        altDeg,
        azSpacingDeg,
        azDeg,
        timeIntervalSec = 30,
        HAOffset = 0,
        includeRefraction = true,
        rates,
        adjustedConstantTrackRateAzError,
        rmsErrorRad,
        data = [];

    xform = new XForm(convertStyle, latitudeRad);
    xform.presetAltaz();
    trackingRates = new MLB.coordLib.TrackingRates();
    trackingRates.setConstantTrackRateTimeRad(constantTrackRateTimeRad);

    for (altDeg = 0; altDeg < 90; altDeg += spacingDeg) {
        azSpacingDeg = spacingDeg / Math.cos(altDeg * uom.degToRad);
        for (azDeg = 0; azDeg < 360; azDeg += azSpacingDeg) {
            xform.position.SidT = 0;
            xform.position.az = azDeg * uom.degToRad;
            xform.position.alt = altDeg * uom.degToRad;
            xform.getEquat();
            rates = trackingRates.getRatesViaDeltaTime(xform, timeIntervalSec, HAOffset, includeRefraction);
            // use apparent az error as seen in the eyepiece: adjust by the cosine of the altitude
            adjustedConstantTrackRateAzError = rates.constantTrackRateAzError * Math.cos(rates.initialAz);
            rmsErrorRad = Math.sqrt(adjustedConstantTrackRateAzError * adjustedConstantTrackRateAzError + rates.constantTrackRateAltError * rates.constantTrackRateAltError);
            data.push([xform.position.az, xform.position.alt, rmsErrorRad]);
        }
    }
    // ex, data[5][2] is the rms error in radians for the 6th entry
    return data;
};

/*
How much is the entrance aperture reduced when a mirror is slumped over a mold?
Circle: perimeter = 2* PI * radius;
if radius = RC (radius of curvature), which is MD (mirror dia) * FR (focal ratio) *2,
then MD = 1/(2*FR) part of a radius and 1/(2*FR*2*PI)=1/(4*FR*PI) portion of a circle,
eg, MD=30, FR=2: therefore mirror covers 1/(8*PI)= 0.0398 portion of a circle;
a circle is 360 degrees or 2*PI radians, so in this example, the angle from the RC point to opposite edges of the mirror =
    360/(8*PI)=14.32 degrees or 2*PI/(8*PI)=0.25 radians,
formula for mirror diameter subtended angle in radians = 2*PI/(4*FR*PI)=1/(2*FR),
angle from mirror's optical path centerline to mirror edge is half the subtended angle or 1/(4*FR);
to get entrance aperture, we know the hypotenuse and the angle: half of entrance aperture = RC*sin(1/4*FR) = MD*FR*2*sin(1/(4*FR)),
and full entrance aperture = 2*RC*2*sin(1/4*FR) = MD*FR*4*sin(1/(4*FR)),
eg, MD=30, FR=2: entrance aperture = 2*30*2*2*sin(1/(4*2)) = 29.92;
*/
exports.calcMirrorSlumpingParms = function (mirrorDia, focalRatio) {
    var angle = 1 / (4 * focalRatio),
        calcSagittaParabolic = exports.calcSagittaParabolic,
        effectiveDia = mirrorDia * focalRatio * 4 * Math.sin(angle),
        sphericalSagitta = calcSagittaParabolic(mirrorDia, focalRatio),
        uom = MLB.sharedLib.uom;

    return {
        effectiveDia: effectiveDia,
        sphericalSagitta: sphericalSagitta,
        edgeAngleDeg: angle / uom.degToRad
    };
};

// from Thompson's Making Your Own Telescope, page 76
exports.calcSphereParabolaDifference = function (mirrorDia, focalRatio) {
    var mirrorRad = mirrorDia / 2,
        radiusOfCurvature = mirrorDia * focalRatio * 2;

    return Math.pow(mirrorRad, 4) / (8 * Math.pow(radiusOfCurvature, 3));
};

/*
from Telescope Making #8, pg 36-, Richard Berry
    16 inch f5
    AZ radius 9, weight 120 lbs, 45deg bearing angle, moment arm 40inches, f=0.083
    0.088*120*9/(cos(45)*40)=3.2
    ALT radius 4, weight 80 lbs, 60deg bearing angle, moment arm 40inches, f=0.088
    1.3*80*(1/cos(60))*4/40=2.1
*/
exports.calcDobFriction = function (azimuthFrictionCoefficient, altitudeFrictionCoefficient, momentArm, azWeight, altWeight, azBearingRadius, altBearingRadius, altBearingAngleDegFromVertical, altitudeAngleDegFromHorizontal) {
    var uom = MLB.sharedLib.uom,
        azFriction = azimuthFrictionCoefficient * azWeight * azBearingRadius / (momentArm * Math.cos(altitudeAngleDegFromHorizontal * uom.degToRad)),
        altFriction = altitudeFrictionCoefficient * altWeight * (1 / Math.cos(altBearingAngleDegFromVertical * uom.degToRad)) * altBearingRadius / momentArm;

    return {
        az: azFriction,
        alt: altFriction
    };
};

// SQM (Sky Quality Meter) to NELM (Naked-Eye Limiting Magnitude) converter
// http://www.unihedron.com/projects/darksky/NELM2BCalc.html taken from http://adsbit.harvard.edu/cgi-bin/nph-iarticle_query?bibcode=1990PASP..102..212S
// Formula: B_mpsas = 21.58 - 5 log(10^(1.586-NELM/5)-1)
// Formula: NELM=7.93-5*log(10^(4.316-(Bmpsas/5))+1)
// mag 6 ~= 20.8 skies

exports.SQMtoNELMconverter = function (SQMreading) {
    return 7.93 - 5 * MLB.sharedLib.log10(Math.pow(10, 4.316 - (SQMreading / 5)) + 1);
};

exports.NELMtoSQMconverter = function (NELM) {
    return 21.58 - 5 * MLB.sharedLib.log10(Math.pow(10, 1.586 - (NELM / 5)) - 1);
};

/*
old formulae from Cloudy Nights discussion - not accurate for brighter skies

// SQM (Sky Quality Meter) to NELM (Naked-Eye Limiting Magnitude) converter
exports.SQMtoNELMconverter = function (SQMreading) {
    return (SQMreading - 8.89) / 2 + 0.5;
};

exports.NELMtoSQMconverter = function (NELM) {
    return 2 * (NELM - 0.5) + 8.89;
};
*/

// from http://web.telia.com/~u41105032/misc/nulltest.htm
exports.artificialStarDistanceMM = function (mirrorDiameter, focalLength) {
    return MLB.sharedLib.int(14 * Math.pow(mirrorDiameter, 4) / Math.pow(focalLength, 2));
};

exports.artificialStarDistanceInches = function (mirrorDiameter, focalLength) {
    return MLB.sharedLib.int(25.4 * 14 * Math.pow(mirrorDiameter, 4) / Math.pow(focalLength, 2));
};

/*
Five fundamental parameters for telescope design:
    aperture, true field of view, eyepiece focal length, eyepiece field stop, eye pupil

From my 13.2 inch F3.0 ZipDob article, http://www.bbastrodesigns.com/ZipDob/ZipDob.html and my Richest Field Telescopes article, http://bbastrodesigns.com/rft.html
Formula is: mirror diameter = eyepiece field stop * exit pupil * 57.3 / (true field of view * eyepiece focal length * 25.4)
From: true field of view = eyepiece field stop / telescope focal length * 57.3; focal length = focal ratio * mirror diameter; eyepiece focal length / exit pupil = focal ratio

ex: 21mm Ethos with 36.2mm field stop, coma corrector with 1.15x factor, desired FOV of 1.8 deg and 13.2 inch primary mirror at F3 with focal length = 13*3.2=39.6:
exit pupil = 21/3/1.15=6.1, therefore aperture = 2.256 * 36.2 * 6.1 / 1.8 / 21 = 13.2;

coma corrector comments:
    without coma corrector, focal ratio increases from F3 to F3.5, focal length increases from 40 to 46 and magnification remains unchanged (or put another way, using a coma corrector with the above numbers means that focal ratio shrinks, focal length therefore shrinks, but magnification does not change);
    the following equations ignore any coma corrector magnification because they do not use focal ratio or focal length
*/

exports.calcApertureFromFOV_EyepieceFL_EyepieceFieldStop_EyePupil = function (FOVdeg, eyepieceFocalLengthmm, eyepieceFieldStopmm, eyePupilmm) {
    return 2.256 * eyepieceFieldStopmm * eyePupilmm / FOVdeg / eyepieceFocalLengthmm;
};

exports.calcFOVFromAperture_EyepieceFL_EyepieceFieldStop_EyePupil = function (apertureInches, eyepieceFocalLengthmm, eyepieceFieldStopmm, eyePupilmm) {
    return 2.256 * eyepieceFieldStopmm * eyePupilmm / apertureInches / eyepieceFocalLengthmm;
};

exports.calcEyePupilFromAperture_FOV_EyepieceFL_EyepieceFieldStop = function (apertureInches, FOVdeg, eyepieceFocalLengthmm, eyepieceFieldStopmm) {
    return apertureInches * FOVdeg * eyepieceFocalLengthmm / eyepieceFieldStopmm / 2.256;
};

exports.calcEyepieceFieldStopFromAperture_FOV_EyepieceFL_EyePupil = function (apertureInches, FOVdeg, eyepieceFocalLengthmm, eyePupilmm) {
    return apertureInches * FOVdeg * eyepieceFocalLengthmm / eyePupilmm / 2.256;
};

exports.calcEyepieceFLFromAperture_FOV_EyepieceFieldStop_EyePupil = function (apertureInches, FOVdeg, eyepieceFieldStopmm, eyePupilmm) {
    return 2.256 * eyepieceFieldStopmm * eyePupilmm / apertureInches / FOVdeg;
};

/*
If focal ratio known from eyepiece focal length and eye pupil, then can reduce to four fundamental parameters for telescope design:
    aperture, true field of view, eyepiece field stop, focal ratio

From: aperture = eyepiece field stop / (true field of view * focal ratio * 25.4 / 57.3)
*/

exports.calcApertureFromFOV_FocalRatio_EyepieceFieldStop = function (FOVdeg, focalRatio, eyepieceFieldStopmm) {
    return 2.256 * eyepieceFieldStopmm / FOVdeg / focalRatio;
};

exports.calcFOVFromAperture_FocalRatio_EyepieceFieldStop = function (apertureInches, focalRatio, eyepieceFieldStopmm) {
    return 2.256 * eyepieceFieldStopmm / apertureInches / focalRatio;
};

exports.calcFocalRatioFromAperture_FOV_EyepieceFieldStop = function (apertureInches, FOVdeg, eyepieceFieldStopmm) {
    return 2.256 * eyepieceFieldStopmm / apertureInches / FOVdeg;
};

exports.calcEyepieceFieldStopFromAperture_FOV_FocalRatio = function (apertureInches, FOVdeg, focalRatio) {
    return apertureInches * FOVdeg * focalRatio / 2.256;
};

/*
adding in a coma corrector with magnifying power results in:
aperture = eyepiece field stop / (true field of view * focal ratio * comaCorrectorFactor * 25.4 / 57.3)
*/

exports.calcApertureFromFOV_FocalRatio_EyepieceFieldStop_ComaCorrectorFactor = function (FOVdeg, focalRatio, eyepieceFieldStopmm, comaCorrectorFactor) {
    var calcApertureFromFOV_FocalRatio_EyepieceFieldStop = exports.calcApertureFromFOV_FocalRatio_EyepieceFieldStop,
        apertureInches = calcApertureFromFOV_FocalRatio_EyepieceFieldStop(FOVdeg, focalRatio, eyepieceFieldStopmm);

    if (!isNaN(comaCorrectorFactor)) {
        apertureInches /= comaCorrectorFactor;
    }
    return apertureInches;
};

exports.calcFOVFromAperture_FocalRatio_EyepieceFieldStop_ComaCorrectorFactor = function (apertureInches, focalRatio, eyepieceFieldStopmm, comaCorrectorFactor) {
    var calcFOVFromAperture_FocalRatio_EyepieceFieldStop = exports.calcFOVFromAperture_FocalRatio_EyepieceFieldStop,
        FOVdeg = calcFOVFromAperture_FocalRatio_EyepieceFieldStop(apertureInches, focalRatio, eyepieceFieldStopmm);

    if (!isNaN(comaCorrectorFactor)) {
        FOVdeg /= comaCorrectorFactor;
    }
    return FOVdeg;
};

exports.calcFocalRatioFromAperture_FOV_EyepieceFieldStop_ComaCorrectorFactor = function (apertureInches, FOVdeg, eyepieceFieldStopmm, comaCorrectorFactor) {
    var calcFocalRatioFromAperture_FOV_EyepieceFieldStop = exports.calcFocalRatioFromAperture_FOV_EyepieceFieldStop,
        focalRatio = calcFocalRatioFromAperture_FOV_EyepieceFieldStop(apertureInches, FOVdeg, eyepieceFieldStopmm);

    if (!isNaN(comaCorrectorFactor)) {
        focalRatio /= comaCorrectorFactor;
    }
    return focalRatio;
};

exports.calcEyepieceFieldStopFromAperture_FOV_FocalRatio_ComaCorrectorFactor = function (apertureInches, FOVdeg, focalRatio, comaCorrectorFactor) {
    var calcEyepieceFieldStopFromAperture_FOV_FocalRatio = exports.calcEyepieceFieldStopFromAperture_FOV_FocalRatio,
        eyepieceFieldStopmm = calcEyepieceFieldStopFromAperture_FOV_FocalRatio(apertureInches, FOVdeg, focalRatio);

    if (!isNaN(comaCorrectorFactor)) {
        eyepieceFieldStopmm *= comaCorrectorFactor;
    }
    return eyepieceFieldStopmm;
};

// based on unaided eye barely resolving Epsilon Lyrae
exports.resolutionFromAperture_Magnification = function (apertureInches, magnification) {
    var DawesLimit = 4.6 / apertureInches,
        resolution = 240 / magnification;

    if (resolution < DawesLimit) {
        return DawesLimit;
    }
    return resolution;
};

// ie, Dawes' Limit
exports.calcTheoreticalResolutionArcsec = function (apertureInches) {
    return 4.6 / apertureInches;
};

exports.calcAiryDiskInches = function (focalRatio) {
    return focalRatio * 2.44 * 0.000022;
};

exports.calcProjectedFocuserBaffleRadius = function (eyepieceFieldStop, barrelTubeID, focalPlaneToFocuserBarrelBottomDistance, focalPlaneToDiagDistance, telescopeTubeOD, telescopeTubeThickness) {
    var focuserBaffleSlope = (eyepieceFieldStop + barrelTubeID) / 2 / focalPlaneToFocuserBarrelBottomDistance;

    return (focalPlaneToDiagDistance + telescopeTubeOD / 2 - telescopeTubeThickness) * focuserBaffleSlope - eyepieceFieldStop / 2;
};

exports.scalingFactor = function (maxWidth, maxHeight, modelWidth, modelHeight, border) {
    var widthFactor = (maxWidth - border * 2) / modelWidth,
        heightFactor = (maxHeight - border * 2) / modelHeight,
        scalingFactor = widthFactor < heightFactor
            ? widthFactor
            : heightFactor;

    return {
        scalingFactor: scalingFactor,
        width: modelWidth * scalingFactor + border * 2,
        height: modelHeight * scalingFactor + border * 2
    };
};

exports.calcMaxMagnification = function (apertureInches) {
    return 27 * apertureInches;
};

exports.calcMinMagnification = function (apertureInches) {
    return 3.6 * apertureInches;
};

exports.calcComaFreeDiaInches = function (focalRatio) {
    return 0.0007 * Math.pow(focalRatio, 3);
};

exports.calcDiopter = function (focalLengthInches) {
    return 39.37 / focalLengthInches;
};

exports.calcFocalLengthInches = function (diopter) {
    return 39.37 / diopter;
};

/*
    closest near point that eye can focus is assumed to be 25cm;
    if object is placed at focal point of lens then power is 0.25m * diopter;
    if lens is placed very close to the eye and object is placed closer to lens than its focal point, then add 1 more to power (lens changes the diopter of the eye making it myopic so that object can be placed closer to the eye resulting in 1x more magnification);
*/
exports.calcMagnifyingLensPower = function (diopter) {
    var assumedNearPointcm = 25,
        power = assumedNearPointcm / 100 * diopter;

    return {
        lensHeldAwayFromEye: power,
        lensHeldCloseToEye: power + 1
    };
};

// http://www.rfroyce.com/standards.htm

exports.calcStrehlFromRMS = function (rms) {
    return Math.pow((1 - 2 * Math.PI * Math.PI * rms * rms), 2);
};

exports.calcRMSFromStrehl = function (Strehl) {
    return Math.sqrt((Math.sqrt(Strehl) - 1) / (-2 * Math.PI * Math.PI));
};

// 14.05 / 4 = 3.51

exports.calcPVFromRMS = function (rms) {
    return rms * 3.51;
};

exports.calcRMSFromPV = function (PV) {
    return PV / 3.51;
};

// aperture = FOV / objectSize * pupil
// for luminance, units are aperture units squared * deg^2, also see MLB.telescopeCriteriaCalc.calcLuminance
exports.calcApertureFromEyepieceApparentFOVdegs_Pupil_ObjectApparentSize = function (eyepieceApparentFOVdegs, pupil, objectSizeArcmin1, objectSizeArcmin2) {
    var magnification,
        aperture,
        luminance,
        apertures = [],
        objectSizeArcmin = objectSizeArcmin1 > objectSizeArcmin2
            ? objectSizeArcmin1
            : objectSizeArcmin2;

    eyepieceApparentFOVdegs.forEach(function (FOVdeg) {
        magnification = FOVdeg / objectSizeArcmin * 60;
        aperture = magnification * pupil;
        luminance = aperture * aperture * objectSizeArcmin / 60 * objectSizeArcmin / 60;
        apertures.push({'apparentFOVdegs': FOVdeg, 'aperture': aperture, 'luminance': luminance});
    });

    return apertures;
};

/*
Nils Olaf comment on ATM list, January 15, 2011:
coma in wavelengths RMS (550 nm) is 6.7*h/F^3, where h is distance from optical center.
For low power EPs, you have to consider linear coma: 3*h/(16*F^2).
*/
exports.calcComa = function (eyepieceFieldStopmm, focalRatio) {
    return {
        lowOrderComaRMS: 6.7 * (eyepieceFieldStopmm / 2) / (focalRatio * focalRatio * focalRatio),
        linearComaRMS: 3 * (eyepieceFieldStopmm / 2) / (focalRatio * focalRatio) // central 1/3 visible
    };
};

exports.calcGreaterComa = function (eyepieceFieldStopmm, focalRatio) {
    var coma = exports.calcComa(eyepieceFieldStopmm, focalRatio);

    return coma.lowOrderComaRMS > coma.linearComaRMS
        ? coma.lowOrderComaRMS
        : coma.linearComaRMS;
};

exports.calcGreaterComaWithComaCorrector = function (eyepieceFieldStopmm, focalRatio, useComaCorrector) {
    return useComaCorrector
        // per TeleVue's F3 coma reduced to that of an F12
        ? exports.calcGreaterComa(eyepieceFieldStopmm, focalRatio * 4)
        : exports.calcGreaterComa(eyepieceFieldStopmm, focalRatio);
};

exports.calcCollimationToleranceInches = function (focalRatio) {
    return 0.0007 * focalRatio * focalRatio * focalRatio;
};

exports.calcMinimumMirrorSupportPoints = function (apertureInches) {
    var maxMirrorAreaSquareInchesPerSupportPoint = 35,
        mirrorSupportsArray = [3, 6, 9, 12, 18, 27, 54],
        mirrorSupportsArrayLength = mirrorSupportsArray.length,
        apertureAreaInches = apertureInches * apertureInches * Math.PI / 4,
        calculatedPoints = apertureAreaInches / maxMirrorAreaSquareInchesPerSupportPoint,
        ix = 0;

    while (ix < mirrorSupportsArrayLength) {
        if (Math.floor(calculatedPoints) < mirrorSupportsArray[ix]) {
            return mirrorSupportsArray[ix];
        }
        ix += 1;
    }
    return undefined;
};

/*
For 3 point support, minimum bending before refocusing occurs at the 70% radius while after refocusing minimum bending occurs at the 40% radius. See https://www.davidlewistoronto.com/plop/ David Lewis' discussion and further references to Luc Arnold and Richard Schwartz.
For 6 point supports, the ring that the 6 points form is best placed at a radius of 57% according to PLOP.
For 9 point supports, place the inner ring at the 33% radius and the outer ring at the 72% radius. If x is outer radius and y is inner radius then balance point = 2/3(cos(30)x-y) + y) = 2/3*cos(30)x + y/3 = 0.577x + y/3 = 52.6%.
For 18 point supports, place the radii at 38% and 76%. If x is outer radius and y is inner radius then balance point = 2/3(cos(15)x-y) + y) = 2/3*cos(15)x + y/3 = 0.644x + y/3 = 61.6%. Using similar math, place the bar centers at the 53.4% radius.
*/
exports.calcMirrorCell3pt = function (radius) {
    return {
        radius: radius * 0.4
    };
};

exports.calcMirrorCell6pt = function (radius) {
    return {
        radius: radius * 0.57,
        balanceRadius: radius * 0.4936
    };
};

exports.calcMirrorCell9pt = function (radius) {
    return {
        innerRadius: radius * 0.33,
        outerRadius: radius * 0.72,
        balanceRadius: radius * 0.5257
    };
};

exports.calcMirrorCell12pt = function (radius) {
    return {
        innerRadius: radius * 0.19,
        outerRadius: radius * 0.72,
        collimationRadius: radius * 0.53,
        innerBarLength: radius * 0.55,
        midBarLength: radius * 0.64,
        outerBarLength: radius * 0.49,
        innerBarBalanceAlongBar: 0.64,
        midBarBalanceAlongBar: 0.56,
        outerBarBalanceAlongBar: 0.5,
        // uses ...AlongBar factors
        innerBarBalance: radius * 0.55 * 0.64,
        midBarBalance: radius * 0.64 * 0.56,
        outerBarBalance: radius * 0.49 * 0.5
    };
};

exports.calcMirrorCell18pt = function (radius) {
    return {
        innerRadius: radius * 0.38,
        outerRadius: radius * 0.76,
        triangleBalanceRadius: radius * 0.6161,
        pivotBarBalanceRadius: radius * 0.5336
    };
};

exports.calcMirrorCell27pt = function (radius) {
    var uom = MLB.sharedLib.uom;

    return {
        inner6Radius: radius * 0.331,
        mid9Radius: radius * 0.644,
        outer12Radius: radius * 0.849,
        baseTriangleBalanceRadius: radius * 0.562,
        innerTriangleBalanceRadius: radius * 0.404,
        outerTriangleBalanceRadius: radius * 0.760,
        // points are not equidistance, instead angles are 32.7, 87 and 152.7 degrees from vertical
        outerTriangleBalancePtAngles: _.map([32.7, 87, 152.7, 32.7 + 180, 87 + 180, 152.7 + 180], function (s) {return s * uom.degToRad;})
    };
};

exports.calcMirrorCellDimensions = function (supportPoints, radius) {
    var calcMirrorCell3pt = exports.calcMirrorCell3pt,
        calcMirrorCell6pt = exports.calcMirrorCell6pt,
        calcMirrorCell9pt = exports.calcMirrorCell9pt,
        calcMirrorCell12pt = exports.calcMirrorCell12pt,
        calcMirrorCell18pt = exports.calcMirrorCell18pt,
        calcMirrorCell27pt = exports.calcMirrorCell27pt;

    switch (supportPoints) {
        case 3:
            return calcMirrorCell3pt(radius);
        case 6:
            return calcMirrorCell6pt(radius);
        case 9:
            return calcMirrorCell9pt(radius);
        case 12:
            return calcMirrorCell12pt(radius);
        case 18:
            return calcMirrorCell18pt(radius);
        case 27:
            return calcMirrorCell27pt(radius);
        // 54 and larger not defined
        default:
            return undefined;
    }
};

// from https://www.telescope-optics.net/obstruction.htm
exports.calcRMSCausedByCentralObstruction = function (centralObstructionByDiameter) {
    return 0.21 * centralObstructionByDiameter;
};

exports.findWeightedCenterOfPoints = function (points) {
    var pointsLength = points.length,
        sumX = 0,
        sumY = 0;

    _.map(points, function (s) {sumX += s.x; sumY += s.y;});
    return {
        x: sumX / pointsLength,
        y: sumY / pointsLength
    };
};

exports.findOffsetCenterBetweenTwoPoints = function (pointA, pointB, offset) {
    return {
        x: (pointB.x - pointA.x) * offset + pointA.x,
        y: (pointB.y - pointA.y) * offset + pointA.y
    };
};

// end of file
