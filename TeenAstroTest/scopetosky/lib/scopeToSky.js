// copyright Mel Bartels, 2012-2015

'use strict';

MLB.scopeToSky = {};


exports.scopeToSkyState = {
	limitToHundredthsArcsec: true,
	limitToTenthsArcsec: true,
	positionDecimals: 4,
	rateDecimals: 4,
	changeDecimals: 6,

	properMotion: 0,
	HAOffset: 0,
	timeIntervalSecs: 1,
	notAvailable: 'n/a',

	analysisPositionSubString: 'analysisPosition',
	analysisPositionsSize: 3,
	analysisPositions: [new MLB.coordLib.Position(), new MLB.coordLib.Position(), new MLB.coordLib.Position()],

	JD: undefined,
	SidT: undefined,

	date: new Date(),

	xform: undefined,
	xformTrig: new MLB.coordLib.XForm(),
	xformMatrix: new MLB.coordLib.XForm(),

	trackingRates: new MLB.coordLib.TrackingRates(),
	refractPosition: new MLB.coordLib.Position(),
	lastLatitude: undefined,
	lastConversionStyle: undefined,
	lastAlignment: undefined,
	lastTrackingRatesAlgorithm: undefined,
	initCount: 0,
	initAnalysis: undefined,
	bestZ123: undefined,

	// Controls in dialog box
	// hard-coded for testing, replace by file or sockets later
	
	// fabrication errors
	inOutZ1: function () {
		return ('0');
	},
	inOutZ2: function () {
		return ('0');
	},
	inOutZ3: function () {
		return ('0');
	},
	// alignment positions 1,2,3
	inOutOne: function () {
		return ('RA: 0 Dec: 90 Az: 0 Alt: 90 SidT: 0');
	},
	inOutTwo: function () {
		return ('RA: 0 Dec: 0 Az: 180 Alt: 0 SidT: 0');
	},
	inOutThree: function () {
		return ('');
	},
	outputFabErrors: function () {
		return ('');
	},
	outputNotes1: function () {
		return ('');
	},
	outputNotes1_html: function(text) {
		console.log (text);		
	},
	outputNotes2: function () {
		return ('');
	},
	outputNotes2_html: function(text) {
		console.log (text);		
	},
	outputNotes3: function () {
		return ('');
	},
	outputNotes3_html: function(text) {
		console.log (text);		
	}
};



// immediately build out the two XForms...
(function () {
	exports.scopeToSkyState.xformTrig.setConvertStyle('trig');
	exports.scopeToSkyState.xformMatrix.setConvertStyle('matrix');
}());

exports.setXForm = function () {
	var scopeToSkyState = exports.scopeToSkyState,
	    inputConversionStyle = scopeToSkyState.conversionStyle;

	if (inputConversionStyle === 'trig') {
		scopeToSkyState.xform = scopeToSkyState.xformTrig;
	}
	if (inputConversionStyle === 'matrix') {
		scopeToSkyState.xform = scopeToSkyState.xformMatrix;
	}
	return scopeToSkyState.xform;
};

exports.initUserDefinedMatrix = function (xform) {
	var scopeToSkyState = exports.scopeToSkyState,
	    z1,
	    z2,
		z3,
		position1,
	    position2,
	    position3,
		init = 2,
		inOutZ1 = scopeToSkyState.inOutZ1,
		inOutZ2 = scopeToSkyState.inOutZ2,
		inOutZ3 = scopeToSkyState.inOutZ3,
		inOutOne = scopeToSkyState.inOutOne,
		inOutTwo = scopeToSkyState.inOutTwo,
		inOutThree = scopeToSkyState.inOutThree,
		uom = MLB.sharedLib.uom,
		parseCoordinateGetValueInRadians = MLB.coordLib.parseCoordinateGetValueInRadians,
		setPositionDegFromString = MLB.coordLib.setPositionDegFromString,
		isValidPosition = MLB.coordLib.isValidPosition,
	    InitType = MLB.coordLib.InitType,
		initMatrixFacade = MLB.coordLib.initMatrixFacade;

	z1 = inOutZ1();
	z2 = inOutZ2();
	z3 = inOutZ3();
	if (isNaN(z1) || isNaN(z2) || isNaN(z3)) {
		alert('Fabrication numbers must be valid');
		return;
	}
	xform.setFabErrorsDeg(parseCoordinateGetValueInRadians(z1).radians / uom.degToRad, parseCoordinateGetValueInRadians(z2).radians / uom.degToRad, parseCoordinateGetValueInRadians(z3).radians / uom.degToRad);

	position1 = inOutOne();
	setPositionDegFromString(position1, xform.cmws.one);
	if (!isValidPosition(xform.cmws.one)) {
		console.log('Initialized position #1 must be valid');
		return;
	}
	position2 = inOutTwo();
	setPositionDegFromString(position2, xform.cmws.two);
	if (!isValidPosition(xform.cmws.two)) {
		console.log('Initialized position #2 must be valid');
		return;
	}
	position3 = inOutThree().trim();
	if (position3.length > 0) {
		setPositionDegFromString(position3, xform.cmws.three);
		if (!isValidPosition(xform.cmws.three)) {
			console.log('Initialized position #3 must be valid');
			return;
		}
		init = 3;
	}

	initMatrixFacade(xform.cmws, init);
	xform.initType = InitType.star;
};

exports.setXFormAlignment = function (xform, inputAlignment) {
	var scopeToSkyState = exports.scopeToSkyState,
	  matrix = scopeToSkyState.matrix,
		inputConversionStyle = scopeToSkyState.conversionStyle,
	  initUserDefinedMatrix = exports.initUserDefinedMatrix,
		notAvailable = scopeToSkyState.notAvailable,
		outputNotes1 = scopeToSkyState.outputNotes1,
		outputNotes3 = scopeToSkyState.outputNotes3,
		outputFabErrors = scopeToSkyState.outputFabErrors;

	if (inputAlignment === 'equatorial') {
		xform.presetEquat();
	}
	if (inputAlignment === 'altazimuth') {
		xform.presetAltaz();
	}
	if (inputAlignment === 'star') {
		initUserDefinedMatrix(xform);
	}

	scopeToSkyState.initCount = ++scopeToSkyState.initCount;
	if (inputConversionStyle == 'matrix') {
		scopeToSkyState.nbInitPoints =  xform.cmws.init;
	} 
	scopeToSkyState.bestZ123 = undefined;
};
/*
exports.latitudeChanged = function (xform) {
	var scopeToSkyState = exports.scopeToSkyState,
	    lastLatitude = scopeToSkyState.lastLatitude;

	return lastLatitude !== xform.latitude;
};

exports.conversionStyleChanged = function () {
	var scopeToSkyState = exports.scopeToSkyState,
		lastConversionStyle = scopeToSkyState.lastConversionStyle,
	    inputConversionStyle = scopeToSkyState.inputConversionStyle;

	return (lastConversionStyle === 'trig' && inputConversionStyle == 'matrix' || 
					(lastConversionStyle === 'matrix' && inputConversionStyle === 'trig'));
};

exports.alignmentChanged = function () {
	var scopeToSkyState = exports.scopeToSkyState,
		forceAligment = scopeToSkyState.forceAlignment,
		lastAlignment = scopeToSkyState.lastAlignment,
		inputAlignment = scopeToSkyState.alignment;
console.log(inputAlignment, lastAlignment);
	return (lastAlignment === 'forceAlignment') || 
					(lastAlignment === 'altazimuth' && !inputAlignment === 'altazimuth') || 
					(lastAlignment === 'equatorial' && !inputAlignment === 'equatorial') || 
					(lastAlignment === 'star' && ! inputAlignment === 'star');
};

exports.trackingRatesAlgorithmChanged = function () {
	var scopeToSkyState = exports.scopeToSkyState,
	    deltaTime = scopeToSkyState.deltaTime,
	    formulae = scopeToSkyState.formulae,
	    lastTrackingRatesAlgorithm = scopeToSkyState.lastTrackingRatesAlgorithm,
	    inputTrackingRatesAlgorithm = scopeToSkyState.inputTrackingRatesAlgorithm;

	return (lastTrackingRatesAlgorithm === 'deltaTime' && !inputTrackingRatesAlgorithm === 'deltaTime') || 
					(lastTrackingRatesAlgorithm === 'formulae' && !inputTrackingRatesAlgorithm === 'formulae');
};

exports.updateLastInitializationNeeded = function (xform) {
	var scopeToSkyState = exports.scopeToSkyState;

	scopeToSkyState.lastLatitude = xform.latitude;
	scopeToSkyState.lastConversionStyle = scopeToSkyState.inputConversionStyle;
	scopeToSkyState.lastAlignment = scopeToSkyState.alignment;
	scopeToSkyState.lastTrackingRatesAlgorithm = scopeToSkyState.inputTrackingRatesAlgorithm;
};

exports.initializationNeeded = function (xform) {
	var latitudeChanged = exports.latitudeChanged,
	    conversionStyleChanged = exports.conversionStyleChanged,
	    alignmentChanged = exports.alignmentChanged,
	    trackingRatesAlgorithmChanged = exports.trackingRatesAlgorithmChanged;
	return latitudeChanged(xform) || conversionStyleChanged() || alignmentChanged() || trackingRatesAlgorithmChanged();
};
*/
exports.setXFormMeridianFlip = function () {
	var scopeToSkyState = exports.scopeToSkyState,
		inputFlippedState = scopeToSkyState.flippedState,
		inputFlipped = scopeToSkyState.flipped,
		xform = scopeToSkyState.xform,
		MeridianFlipStates = MLB.coordLib.MeridianFlipStates,
		setFlipState = MLB.coordLib.setFlipState;

	xform.meridianFlip.canFlip = scopeToSkyState.canFlipMeridian;
	if (inputFlippedState == 'onWestSidePointingEast') {
		xform.meridianFlip.inputFlippedState = 'onWestSidePointingEast';
	} else if (inputFlippedState == 'onEastSidePointingWest') {
		xform.meridianFlip.inputFlippedState = 'onEastSidePointingWest';
	}
	setFlipState(inputFlipped, xform.meridianFlip);
};

exports.setupXForm = function (latitude) {
	var xform,
	  scopeToSkyState = exports.scopeToSkyState,
		setXForm = exports.setXForm,
		setXFormAlignment = exports.setXFormAlignment,
		updateLastInitializationNeeded = exports.updateLastInitializationNeeded,
		initializationNeeded = exports.initializationNeeded,
		setXFormMeridianFlip = exports.setXFormMeridianFlip,
		inputAlignment = scopeToSkyState.alignment,
	  inputConversionStyle = scopeToSkyState.conversionStyle;

	xform = setXForm();
	setXFormMeridianFlip();

	xform.latitude = latitude;
	setXFormAlignment(xform, inputAlignment);
/*
	if (initializationNeeded(xform)) {
		setXFormAlignment(xform, inputAlignment);
		if (inputConversionStyle === 'matrix') {
			scopeToSkyState.initAnalysis = xform.initAltazResults();
		}
		updateLastInitializationNeeded(xform);
	}
*/
	return xform;
};

exports.updateCorrections = function (pnaCorrections, correctedRA, correctedDec) {
	var outputPrecessionRA,
		outputPrecessionDEC,
		outputNutationRA,
		outputNutationDec,
		outputAnnualAberrationRA,
		outputAnnualAberrationDec,
		outputCorrectedRA,
		outputCorrectedDec,
		scopeToSkyState = exports.scopeToSkyState,
		limitToHundredthsArcsec = scopeToSkyState.limitToHundredthsArcsec,
		limitToTenthsArcsec = scopeToSkyState.limitToTenthsArcsec,
		notAvailable = scopeToSkyState.notAvailable,
	  inputIncludeCorrections = scopeToSkyState.includeCorrections,
		convertRadiansToHMSMString = MLB.coordLib.convertRadiansToHMSMString,
		convertRadiansToDMSMString = MLB.coordLib.convertRadiansToDMSMString;

	if (inputIncludeCorrections) {
		/*
		outputPrecessionRA.html(convertRadiansToHMSMString(pnaCorrections.precession.deltaRA, limitToHundredthsArcsec));
		outputPrecessionDEC.html(convertRadiansToDMSMString(pnaCorrections.precession.deltaDec, limitToTenthsArcsec));
		outputNutationRA.html(convertRadiansToHMSMString(pnaCorrections.nutation.deltaRA, limitToHundredthsArcsec));
		outputNutationDec.html(convertRadiansToDMSMString(pnaCorrections.nutation.deltaDec, limitToTenthsArcsec));
		outputAnnualAberrationRA.html(convertRadiansToHMSMString(pnaCorrections.annualAberration.deltaRA, limitToHundredthsArcsec));
		outputAnnualAberrationDec.html(convertRadiansToDMSMString(pnaCorrections.annualAberration.deltaDec, limitToTenthsArcsec));
		outputCorrectedRA.html(convertRadiansToHMSMString(correctedRA, limitToHundredthsArcsec));
		outputCorrectedDec.html(convertRadiansToDMSMString(correctedDec, limitToTenthsArcsec));
		*/
	} else {
	}
};

exports.updateRefraction = function (includeRefraction, refractionResults) {
	var outputRefraction,
		outputRefractedAltitude,
		outputRefractionRA,
		outputRefractionDecs,
		scopeToSkyState = exports.scopeToSkyState,
	    limitToHundredthsArcsec = scopeToSkyState.limitToHundredthsArcsec,
		limitToTenthsArcsec = scopeToSkyState.limitToTenthsArcsec,
		positionDecimals = scopeToSkyState.positionDecimals,
		notAvailable = scopeToSkyState.notAvailable,
		roundToDecimal = MLB.sharedLib.roundToDecimal,
		uom = MLB.sharedLib.uom,
		convertRadiansToHMSMString = MLB.coordLib.convertRadiansToHMSMString,
		convertRadiansToDMSMString = MLB.coordLib.convertRadiansToDMSMString;

	if (includeRefraction) {
		/*
		outputRefraction.html(roundToDecimal(refractionResults.refraction / uom.degToRad, positionDecimals));
		outputRefractedAltitude.html(roundToDecimal(refractionResults.refractedAlt / uom.degToRad, positionDecimals));
		outputRefractionRA.html(convertRadiansToHMSMString(refractionResults.deltaRA, limitToHundredthsArcsec));
		outputRefractionDec.html(convertRadiansToDMSMString(refractionResults.deltaDec, limitToTenthsArcsec));
		*/
	} else {
/*
		outputRefraction.html(notAvailable);
		outputRefractedAltitude.html(notAvailable);
		outputRefractionRA.html(notAvailable);
		outputRefractionDec.html(notAvailable);
		*/
	}
};

exports.updateSiteAltaz = function (siteAz, siteAlt) {
	var 
	    scopeToSkyState = exports.scopeToSkyState,
	    positionDecimals = scopeToSkyState.positionDecimals,
		roundToDecimal = MLB.sharedLib.roundToDecimal,
		uom = MLB.sharedLib.uom;

	scopeToSkyState.azimuth = roundToDecimal(siteAz / uom.degToRad, positionDecimals);
	scopeToSkyState.altitude = roundToDecimal(siteAlt / uom.degToRad, positionDecimals);
};

exports.updateTrackingRates = function (rates) {
	var outputScopeAzimuthRate,
		outputScopeAltitudeRate,
		outputScopeFRRate,
		outputScopeAzimuthRateChange,
		outputScopeAltitudeRateChange,
		outputScopeFRRateChange,
		outputScopeAzFiveMinuteError,
		outputScopeAltFiveMinuteError,
		scopeToSkyState = exports.scopeToSkyState,
	    rateDecimals = scopeToSkyState.rateDecimals,
		changeDecimals = scopeToSkyState.changeDecimals,
		outputNotes2 = scopeToSkyState.outputNotes2,
		roundToDecimal = MLB.sharedLib.roundToDecimal,
		uom = MLB.sharedLib.uom;
/*
	outputScopeAzimuthRate.html(roundToDecimal(rates.azRate / uom.arcsecToRad, rateDecimals));
	outputScopeAltitudeRate.html(roundToDecimal(rates.altRate / uom.arcsecToRad, rateDecimals));
	outputScopeFRRate.html(roundToDecimal(rates.FRRate / uom.arcsecToRad, rateDecimals));

	outputScopeAzimuthRateChange.html(roundToDecimal(rates.changeAzRate / uom.arcsecToRad, changeDecimals));
	outputScopeAltitudeRateChange.html(roundToDecimal(rates.changeAltRate / uom.arcsecToRad, changeDecimals));
	outputScopeFRRateChange.html(roundToDecimal(rates.changeFRRate / uom.arcsecToRad, changeDecimals));

	outputNotes2().html('tracking strategy: ' + rates.trackingRatesStrategy);

	// apparent errors
	outputScopeAzFiveMinuteError.html(roundToDecimal(rates.constantTrackRateAzError / uom.arcsecToRad * Math.cos(rates.initialAlt), rateDecimals));
	outputScopeAltFiveMinuteError.html(roundToDecimal(rates.constantTrackRateAltError / uom.arcsecToRad, rateDecimals));
*/
};

exports.starAligned = function (xform) {
	var scopeToSkyState = exports.scopeToSkyState,
	    inputConversionStyle = scopeToSkyState.inputConversionStyle,
		matrix = scopeToSkyState.matrix;

	return inputConversionStyle() && xform.cmws !== undefined;
};

exports.updateMatrix = function (xform) {
	var outputInitAnalysis,
	    fabErrors,
	    scopeToSkyState = exports.scopeToSkyState,
		starAligned = exports.starAligned,
	    notAvailable = scopeToSkyState.notAvailable,
	    positionDecimals = scopeToSkyState.positionDecimals,
		initAnalysis = scopeToSkyState.initAnalysis,
		inOutZ1 = scopeToSkyState.inOutZ1,
		inOutZ2 = scopeToSkyState.inOutZ2,
		inOutZ3 = scopeToSkyState.inOutZ3,
		inOutOne = scopeToSkyState.inOutOne,
		inOutTwo = scopeToSkyState.inOutTwo,
		inOutThree = scopeToSkyState.inOutThree,
		roundToDecimal = MLB.sharedLib.roundToDecimal,
		uom = MLB.sharedLib.uom,
		positionDegToString = MLB.coordLib.positionDegToString;
/*
	if (starAligned(xform)) {
		fabErrors = xform.cmws.fabErrors;
		inOutZ1().val(roundToDecimal(fabErrors.z1 / uom.degToRad, positionDecimals));
		inOutZ2().val(roundToDecimal(fabErrors.z2 / uom.degToRad, positionDecimals));
		inOutZ3().val(roundToDecimal(fabErrors.z3 / uom.degToRad, positionDecimals));
		inOutOne().val(positionDegToString(xform.cmws.one));
		inOutTwo().val(positionDegToString(xform.cmws.two));
		if (xform.cmws.init === 3) {
			inOutThree().val(positionDegToString(xform.cmws.three));
		} else {
			inOutThree().val('');
		}
		outputInitAnalysis.html('latitude: ' + roundToDecimal(initAnalysis.initLatitude / uom.degToRad, positionDecimals) + ', azimuth offset: ' + roundToDecimal(initAnalysis.azOffset / uom.degToRad, positionDecimals) + ', hour angle offset: ' + roundToDecimal(initAnalysis.HAOffset / uom.degToRad, positionDecimals));
	} else {
		inOutZ1().val(notAvailable);
		inOutZ2().val(notAvailable);
		inOutZ3().val(notAvailable);
		inOutOne().val(notAvailable);
		inOutTwo().val(notAvailable);
		inOutThree().val('');
		outputInitAnalysis.html(notAvailable);
	}
	*/
};


exports.computeScope = function (config) {
	var latitude,
	  startingRAHA,
		startingRA,
		startingDec,
		coordJD,
		pnaCorrections,
		correctedRA,
		correctedDec,
		xform,
		rates,
		scopeToSkyState = exports.scopeToSkyState,
		setupXForm = exports.setupXForm,
		updateCorrections = exports.updateCorrections,
		updateRefraction = exports.updateRefraction,
		updateSiteAltaz = exports.updateSiteAltaz,
		updateTrackingRates = exports.updateTrackingRates,
		updateMatrix = exports.updateMatrix,
		updateInputIncludeCorrectionsBasedOnInputRAorHA = exports.updateInputIncludeCorrectionsBasedOnInputRAorHA,
		getIncludeRefractionUpdateButton = exports.getIncludeRefractionUpdateButton,
		inputRAorHA = scopeToSkyState.RAorHA,
		deltaTime = scopeToSkyState.deltaTime,
	  formulae = scopeToSkyState.formulae,
		positionDecimals = scopeToSkyState.positionDecimals,
		properMotion = scopeToSkyState.properMotion,
		HAOffset = scopeToSkyState.HAOffset,
		timeIntervalSecs = scopeToSkyState.timeIntervalSecs,
		JD = scopeToSkyState.JD,
		SidT = scopeToSkyState.SidT,
		trackingRates = scopeToSkyState.trackingRates,
	  inputIncludeCorrections = scopeToSkyState.includeCorrections,
		inOutScopeAzimuth = scopeToSkyState.inOutScopeAzimuth,
		inOutScopeAltitude = scopeToSkyState.inOutScopeAltitude,
		outputScopeFR = scopeToSkyState.outputScopeFR,
		roundToDecimal = MLB.sharedLib.roundToDecimal,
		uom = MLB.sharedLib.uom,
		validRev = MLB.coordLib.validRev,
		calcJDFromJulianYear = MLB.coordLib.calcJDFromJulianYear,
		calcProperMotionPrecessionNutationAnnualAberration = MLB.coordLib.calcProperMotionPrecessionNutationAnnualAberration,
		parseCoordinateGetValueInRadians = MLB.coordLib.parseCoordinateGetValueInRadians;

	// read UI
	startingRAHA = parseCoordinateGetValueInRadians(config.RAHA, true).radians;
	if (inputRAorHA === 'HA') {
		// SidT set in processDateTime() which is called from the button's click event
		startingRA = validRev(SidT - startingRAHA);
	} else {
		startingRA = startingRAHA;
	}
	startingDec = parseCoordinateGetValueInRadians(config.dec).radians;
	latitude = parseCoordinateGetValueInRadians(config.latitude).radians;
	coordJD = calcJDFromJulianYear(+config.coordinateYear);

	if (inputRAorHA === 'HA') 
		scopeToSkyState.HA = config.RAHA;
	else
		scopeToSkyState.RA = config.RAHA;
	scopeToSkyState.dec = config.dec;
	scopeToSkyState.latitude = config.latitude;
	scopeToSkyState.longitude = config.longitude;

	// coordinate corrections
	correctedRA = startingRA;
	correctedDec = startingDec;
	if (inputIncludeCorrections) {
		pnaCorrections = calcProperMotionPrecessionNutationAnnualAberration(startingRA, startingDec, coordJD, JD, properMotion, properMotion);
		correctedRA += pnaCorrections.total.deltaRA;
		correctedDec += pnaCorrections.total.deltaDec;
	}

	// set xform
	xform = setupXForm(latitude);
	xform.position.SidT = SidT;
	xform.position.RA = correctedRA;
	xform.position.Dec = correctedDec;

	// scope tracking rates
	if (config.trackingRatesAlgorithm === 'deltaTime') {
		rates = trackingRates.getRatesViaDeltaTime(xform, timeIntervalSecs, HAOffset, config.includeRefraction);
	} else if (config.trackingRatesAlgorithm === 'formulae') {
		rates = trackingRates.getRatesViaFormulae(xform, timeIntervalSecs, HAOffset, config.includeRefraction);
	}

	// update UI
	updateCorrections(pnaCorrections, correctedRA, correctedDec);
	updateRefraction(config.includeRefraction, rates.refractionResults);
	updateSiteAltaz(rates.siteAz, rates.siteAlt);
	updateTrackingRates(rates);
	updateMatrix(xform);

	scopeToSkyState.primaryAxis = roundToDecimal(rates.initialAz / uom.degToRad, positionDecimals);
	scopeToSkyState.secondaryAxis = roundToDecimal(rates.initialAlt / uom.degToRad, positionDecimals);
	scopeToSkyState.tertiaryAxis = roundToDecimal(rates.initialFR / uom.degToRad, positionDecimals);

};

exports.computeEquatCoords = function (config) {
	var includeRefraction,
		latitude,
		coordJD,
		pnaCorrections,
		refractionResults,
		refractedRA,
		refractedDec,
		startingRA,
		startingDec,
		xform,
		rates,
		scopeToSkyState = exports.scopeToSkyState,
    setupXForm = exports.setupXForm,
    updateCorrections = exports.updateCorrections,
    updateRefraction = exports.updateRefraction,
    updateSiteAltaz = exports.updateSiteAltaz,
    updateTrackingRates = exports.updateTrackingRates,
    updateMatrix = exports.updateMatrix,
		updateInputIncludeCorrectionsBasedOnInputRAorHA = exports.updateInputIncludeCorrectionsBasedOnInputRAorHA,
		inputRAorHA = scopeToSkyState.RAorHA,
		limitToHundredthsArcsec = scopeToSkyState.limitToHundredthsArcsec,
		limitToTenthsArcsec = scopeToSkyState.limitToTenthsArcsec,
		positionDecimals = scopeToSkyState.positionDecimals,
		properMotion = scopeToSkyState.properMotion,
		HAOffset = scopeToSkyState.HAOffset,
		timeIntervalSecs = scopeToSkyState.timeIntervalSecs,
		JD = scopeToSkyState.JD,
		SidT = scopeToSkyState.SidT,
		trackingRates = scopeToSkyState.trackingRates,
    refractPosition = scopeToSkyState.refractPosition,
		inputIncludeCorrections = scopeToSkyState.includeCorrections,
		inputLatitude = scopeToSkyState.latitude,
	  inputTrackingRatesAlgorithm = scopeToSkyState.trackingRatesAlgorithm,
		inputRAHA = scopeToSkyState.RAHA,
		inputDec = scopeToSkyState.dec,
		inputCoordinateYear = scopeToSkyState.coordinateYear,
		inOutScopeAzimuth = scopeToSkyState.scopeAzimuth,
		inOutScopeAltitude = scopeToSkyState.scopeAltitude,
		roundToDecimal = MLB.sharedLib.roundToDecimal,
		uom = MLB.sharedLib.uom,
		validRev = MLB.coordLib.validRev,
		convertRadiansToHMSMString = MLB.coordLib.convertRadiansToHMSMString,
		convertRadiansToDMSMString = MLB.coordLib.convertRadiansToDMSMString,
		calcJDFromJulianYear = MLB.coordLib.calcJDFromJulianYear,
		calcProperMotionPrecessionNutationAnnualAberration = MLB.coordLib.calcProperMotionPrecessionNutationAnnualAberration,
		parseCoordinateGetValueInRadians = MLB.coordLib.parseCoordinateGetValueInRadians,
		calcRefractionFromApparentEquatorialCorrection = MLB.coordLib.calcRefractionFromApparentEquatorialCorrection;

	// update/read UI
	includeRefraction = config.includeRefraction;

	// read UI
	latitude = parseCoordinateGetValueInRadians(config.latitude).radians;
	coordJD = calcJDFromJulianYear(+config.coordinateYear);

	// set xform
	xform = setupXForm(latitude);
	xform.position.az = parseCoordinateGetValueInRadians(config.primaryAxis).radians;
	xform.position.alt = parseCoordinateGetValueInRadians(config.secondaryAxis).radians;
	xform.position.SidT = SidT;

	// get equatorial coordinates
	xform.getEquat();

	// refraction
	if (includeRefraction) {
		refractionResults = calcRefractionFromApparentEquatorialCorrection(xform.position.RA, xform.position.Dec, xform.position.SidT, xform.latitude, refractPosition);
		xform.position.RA += refractionResults.deltaRA;
		xform.position.Dec += refractionResults.deltaDec;
	}

	// coordinate corrections
	refractedRA = xform.position.RA;
	refractedDec = xform.position.Dec;
	startingRA = xform.position.RA;
	startingDec = xform.position.Dec;
	if (inputIncludeCorrections){
		pnaCorrections = calcProperMotionPrecessionNutationAnnualAberration(refractedRA, refractedDec, coordJD, JD, properMotion, properMotion);
		startingRA -= pnaCorrections.total.deltaRA;
		startingDec -= pnaCorrections.total.deltaDec;
	}

	// scope tracking rates
	// xform set to refracted equatorial position
	if (inputTrackingRatesAlgorithm === 'deltaTime') {
		rates = trackingRates.getRatesViaDeltaTime(xform, timeIntervalSecs, HAOffset, includeRefraction);
	} else if (inputTrackingRatesAlgorithm === 'formulae') {
		rates = trackingRates.getRatesViaFormulae(xform, timeIntervalSecs, HAOffset);
	}

	// update UI
	// RA, Dec comes from the scope axes positions so do not use startingRA/Dec, instead use refractedRA/Dec
	updateCorrections(pnaCorrections, refractedRA, refractedDec);
	updateRefraction(includeRefraction, rates.refractionResults);
	updateSiteAltaz(rates.siteAz, rates.siteAlt);
	updateTrackingRates(rates);
	updateMatrix(xform);
	// update UI individual fields
	console.log("Tertiary Axis: %f", roundToDecimal(rates.initialFR / uom.degToRad, positionDecimals));

	if (inputRAorHA === 'HA') {
		console.log("RA: %s",validRev(SidT - startingRA), limitToHundredthsArcsec);
	} else {
		console.log("HA: %s",convertRadiansToHMSMString(startingRA, limitToHundredthsArcsec));
	}
	console.log("Dec: %s", convertRadiansToDMSMString(startingDec, limitToTenthsArcsec));
};



exports.processDateTime = function (config) {
	var scopeToSkyState = exports.scopeToSkyState,
		date = scopeToSkyState.date,
		convertRadiansToHMSString = MLB.coordLib.convertRadiansToHMSString,
		dateFromString = MLB.coordLib.dateFromString,
		dateToString = MLB.coordLib.dateToString,
		calcJD = MLB.coordLib.calcJD,
		calcSidTFromJD = MLB.coordLib.calcSidTFromJD;

	// setup: datetime display
	if (config.currentDateTime)
	{
			config.dateTime = new Date();	
	}

	date = dateFromString(config.dateTime);
	scopeToSkyState.JD = calcJD(date.getFullYear(), date.getMonth() + 1, date.getDate(), date.getHours(), date.getMinutes(), date.getSeconds(), date.getMilliseconds(), config.timeZone);
	scopeToSkyState.SidT = calcSidTFromJD(scopeToSkyState.JD, +config.longitude);
	
};

exports.setInitialValues = function (config) {
	var	scopeToSkyState = exports.scopeToSkyState;

	scopeToSkyState.alignment = config.alignment;
	scopeToSkyState.trackingRatesAlgorithm = config.trackingRatesAlgorithm;
	scopeToSkyState.RAorHA = config.RAorHA;
	scopeToSkyState.conversionStyle = config.conversionStyle;
	scopeToSkyState.canFlipMeridian = config.canFlipMeridian;
	scopeToSkyState.flipped = config.flipped;
	scopeToSkyState.flippedState = config.flippedState;
	scopeToSkyState.includeCorrections = config.includeCorrections;
	scopeToSkyState.includeRefraction = config.includeRefraction;
}



// end of file
