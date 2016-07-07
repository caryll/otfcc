var path = require('path');
var fs = require('fs');

function check(fn, desc) {
	if (fn) {
		process.stderr.write('\x1b[32;1m[PASS]\x1b[39;49m ' + desc + '\n');
	} else {
		process.stderr.write('\x1b[31;1m[FAIL]\x1b[39;49m ' + desc + '\n');
		process.exit(1);
	}
}

var c1 = fs.readFileSync(process.argv[2], 'utf-8');
var c2 = fs.readFileSync(process.argv[2], 'utf-8');
check(c1 === c2, "Roundtrip passed.");
