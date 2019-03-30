var gdal = require('../lib/gdal.js');
var assert = require('chai').assert;

describe('gdal.VrtSourcedRasterBand', function() {
	afterEach(gc);

	it('should be exposed', function() {
		assert.ok(gdal.VrtSourcedRasterBand);
	});

	it('should not be instantiable', function() {
		assert.throws(function() {
			new gdal.VrtSourcedRasterBand();
		});
	});

	describe('toString()', function() {
		assert.ok('test');
	});
});