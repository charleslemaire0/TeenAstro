// copyright Mel Bartels, 2011-2020
// see sharedLib unitTests.htm for unit tests
// turn on jslint's Tolerate ++ and --

'use strict';

exports.withinRange = function (actual, expected, range) {
    return actual <= expected + range && actual >= expected - range;
};

// error handling

exports.int = function (n) {
    return (n | 0) + (n % -1 < 0 ? -1 : 0);
};

exports.isInt = function (n) {
    return n === +n && n === (n | 0);
};

exports.isFloat = function (n) {
    return n === +n && n !== (n | 0);
};

exports.resolveNumberToPrecision = function (number, precision) {
    var absNumber, absNumberToPrecision, int = exports.int;

    absNumber = number < 0
        ? -number
        : number;
    absNumberToPrecision = int((absNumber + precision / 2 + 0.000000000000001) / precision) * precision;
    if (number >= 0) {
        return absNumberToPrecision;
    }
    return -absNumberToPrecision;
};

exports.limitDecimalPlaces = function (number, decimalPlaces) {
    var split = number.toString().split('.');

    if (Math.abs(number) < Math.pow(10, -decimalPlaces)) {
        return '0';
    }
    if (split.length > 1) {
        return split[0] + '.' + split[1].substr(0, decimalPlaces);
    }
    return number.toString();
};

exports.roundToDecimal = function (number, decimalPlaces) {
    var resolveNumberToPrecision = exports.resolveNumberToPrecision,
        limitDecimalPlaces = exports.limitDecimalPlaces;

    return limitDecimalPlaces(resolveNumberToPrecision(number, Math.pow(10, -decimalPlaces)), decimalPlaces);
};

exports.log10 = function (n) {
    return Math.log(n) / Math.log(10);
};

exports.cot = function (n) {
    return 1 / Math.tan(n);
};

exports.point = function (width, height) {
    return {x: Math.round(width), y: Math.round(height)};
};

exports.rect = function (x, y, width, height) {
    return {
        x: x,
        y: y,
        width: width,
        height: height,
        endX: x + width,
        endY: y + height
    };
};

exports.drawLine = function (context, strokeStyle, width, point1, point2) {
    context.beginPath();
    context.moveTo(point1.x, point1.y);
    context.lineTo(point2.x, point2.y);
    context.strokeStyle = strokeStyle;
    context.lineWidth = width;
    context.stroke();
};

exports.drawRect = function (context, strokeStyle, lineWidth, rect) {
    context.strokeStyle = strokeStyle;
    context.lineWidth = lineWidth;
    context.strokeRect(Math.round(rect.x), Math.round(rect.y), Math.round(rect.width), Math.round(rect.height));
};

exports.drawCircle = function (context, center, radius, width, strokeStyle) {
    context.beginPath();
    context.arc(center.x, center.y, radius, 0, 2 * Math.PI);
    context.strokeStyle = strokeStyle;
    context.lineWidth = width;
    context.stroke();
};

exports.fillCircle = function (context, center, radius, fillStyle) {
    context.fillStyle = fillStyle;
    context.beginPath();
    context.arc(center.x, center.y, radius, 0, Math.PI * 2, true);
    context.fill();
};

exports.fillPolygon = function (context, points, fillStyle) {
    var ix, length;

    context.fillStyle = fillStyle;
    context.beginPath();
    context.moveTo(points[0].x, points[0].y);
    length = points.length;
    for (ix = 1; ix < length; ix++) {
        context.lineTo(points[ix].x, points[ix].y);
    }
    context.closePath();
    context.fill();
};

exports.clearCanvas = function (canvas) {
    canvas.getContext('2d').clearRect(0, 0, canvas.width, canvas.height);
};

exports.drawHorizDimen = function (context, text, dimensionY, dimensionLeftX, dimensionRightX, markHeight, lineWidth, strokeStyle) {
    var point = exports.point,
        drawLine = exports.drawLine,
        dimensionRightPt = point(dimensionRightX, dimensionY),
        dimensionLeftPt = point(dimensionLeftX, dimensionY);

    drawLine(context, strokeStyle, lineWidth, dimensionLeftPt, dimensionRightPt);
    drawLine(context, strokeStyle, lineWidth, point(dimensionLeftPt.x, dimensionLeftPt.y - markHeight), point(dimensionLeftPt.x, dimensionLeftPt.y + markHeight));
    drawLine(context, strokeStyle, lineWidth, point(dimensionRightPt.x, dimensionRightPt.y - markHeight), point(dimensionRightPt.x, dimensionRightPt.y + markHeight));
    context.fillStyle = strokeStyle;
    context.fillText(text, dimensionLeftPt.x + markHeight, dimensionY - markHeight);
};

exports.drawVertDimen = function (context, text, dimensionX, dimensionBottomY, dimensionTopY, markHeight, lineWidth, strokeStyle) {
    var point = exports.point,
        drawLine = exports.drawLine,
        dimensionTopPt = point(dimensionX, dimensionTopY),
        dimensionBottomPt = point(dimensionX, dimensionBottomY);

    drawLine(context, strokeStyle, lineWidth, dimensionBottomPt, dimensionTopPt);
    drawLine(context, strokeStyle, lineWidth, point(dimensionBottomPt.x - markHeight, dimensionBottomPt.y), point(dimensionBottomPt.x + markHeight, dimensionBottomPt.y));
    drawLine(context, strokeStyle, lineWidth, point(dimensionTopPt.x - markHeight, dimensionTopPt.y), point(dimensionTopPt.x + markHeight, dimensionTopPt.y));
    context.fillStyle = strokeStyle;
    context.fillText(text, dimensionBottomPt.x + markHeight, (dimensionBottomY + dimensionTopY) / 2);
};

// 'a' is opaqueness with 255 or 0xff opaque, 0 transparent
exports.setPixel = function (imageData, x, y, r, g, b, a) {
    var ix = (x + y * imageData.width) * 4;
    imageData.data[ix] = r;
    imageData.data[ix + 1] = g;
    imageData.data[ix + 2] = b;
    imageData.data[ix + 3] = a;
};

exports.getDistance = function (pointA, pointB) {
    return Math.sqrt(Math.pow(pointA.x - pointB.x, 2) + Math.pow(pointA.y - pointB.y, 2));
};

exports.seriesLabel = function (label) {
    return {
        label: label,
        showMarker: false
    };
};

exports.seriesLabelDiamondMarker = function (label) {
    return {
        label: label,
        markerOptions: {
            style: 'dimaond'
        }
    };
};

exports.buildCanvasElement = function (id, width, height) {
    $('#' + id).remove();

    return $('<canvas/>', {
        'id': id
    }).prop({
        width: width,
        height: height
    });
};

// unit of measurements constants

exports.uom = {
    sidRate: 1.002737909,

    //JD on Greenwich Jan 1 2000 noon
    JD2000: 2451545,
    // number of seconds since Jan 1 1970
    secondsAtYr2000: 946627200,
    daysInYear: 365.25,

    // 1 revolution: 2 Pi Radians: 360 Degrees: 24 Hours
    oneRev: Math.PI * 2,
    threeQtrsRev: Math.PI * 3 / 2,
    halfRev: Math.PI,
    qtrRev: Math.PI / 2,

    hrToRev: 1 / 24,
    hrToRad: Math.PI / 12,
    minToRev: 1 / 1440,
    minToRad: Math.PI / 720,
    secToRev: 1 / 86400,
    secToRad: Math.PI / 43200,

    degToRev: 1 / 360,
    degToRad: Math.PI / 180,
    arcminToRev: 1 / 21600,
    arcminToRad: Math.PI / 10800,
    arcsecToRev: 1 / 1296000,
    arcsecToRad: Math.PI / 648000,

    // based on comparison of circular areas; area change from arc-seconds to arc-minutes is 30*30*Pi = 2827.4334
    sqrArcminToSqrArcsec: 30 * 30 * Math.PI,
    // using the above value to get magnitude change; if using a square area (not circular) then the value is 8.89075625191822
    sqrArcminToSqrArcsecCircularMagnitudeChange: 8.628480955333647,
    sqrArcminToSqrArcsecSquareMagnitudeChange: 8.89075625191822,

    plus: '+',
    minus: '-',
    deg: '°',
    min: '\'',
    sec: '"'
};

// regex library http://www.regexlib.com/DisplayPatterns.aspx?cattabindex=2&categoryId=3&AspxAutoDetectCookieSupport=1
// matches 1, 34.5, .3, 5., 10.5:1, 10.5:2 (returns 5.25), 10 2 (returns 5)
exports.getReduction = function (val) {
    var numbers = val.match(/([0-9]+\.[0-9]*)|([0-9]*\.[0-9]+)|([0-9]+)\s*/g),
        numbersLength = numbers === null
            ? 0
            : numbers.length,
        denominator,
        reduction;

    if (numbersLength === 1) {
        reduction = numbers[0];
    } else if (numbersLength === 2) {
        denominator = parseFloat(numbers[1]);
        // check for divide by 0
        if (denominator === 0) {
            reduction = undefined;
        } else {
            reduction = numbers[0] / denominator;
        }
    } else {
        reduction = undefined;
    }
    return reduction;
};

// end of file
