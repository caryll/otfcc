var path = require('path');
var fs = require('fs');

var ROUNDING_ERROR = 0.001;
var pSlice = Array.prototype.slice;
var objectKeys = Object.keys;
var deepEqual = function (actual, expected, opts) {
	if (actual === expected) {
		return true;
	} else if (typeof actual == 'number' && typeof expected == 'number') {
		return Math.abs(actual - expected) < ROUNDING_ERROR;
	} else if (!actual || !expected || typeof actual != 'object' && typeof expected != 'object') {
		return actual === expected;
	} else {
		return objEquiv(actual, expected, opts);
	}
}

function isUndefinedOrNull(value) {
	return value === null || value === undefined;
}

function isBuffer(x) {
	if (!x || typeof x !== 'object' || typeof x.length !== 'number') return false;
	if (typeof x.copy !== 'function' || typeof x.slice !== 'function') {
		return false;
	}
	if (x.length > 0 && typeof x[0] !== 'number') return false;
	return true;
}

function objEquiv(a, b, opts) {
	var i, key;
	if (isUndefinedOrNull(a) || isUndefinedOrNull(b))
		return false;
	if (a.prototype !== b.prototype) return false;
	try {
		var ka = objectKeys(a),
			kb = objectKeys(b);
	} catch (e) {
		return false;
	}
	if (ka.length != kb.length)
		return false;
	ka.sort();
	kb.sort();
	for (i = ka.length - 1; i >= 0; i--) {
		if (ka[i] != kb[i])
			return false;
	}
	for (i = ka.length - 1; i >= 0; i--) {
		key = ka[i];
		if (!deepEqual(a[key], b[key], opts)) return false;
	}
	return typeof a === typeof b;
}

function check(fn, desc) {
	if (fn) {
		process.stderr.write('\x1b[32;1m[PASS]\x1b[39;49m ' + desc + '\n');
	} else {
		process.stderr.write('\x1b[31;1m[FAIL]\x1b[39;49m ' + desc + '\n');
		process.exit(1);
	}
}

var c1 = fs.readFileSync(process.argv[2], 'utf-8');
var c2 = fs.readFileSync(process.argv[3], 'utf-8');
check(deepEqual(JSON.parse(c1), JSON.parse(c2)), "Roundtrip passed.");
