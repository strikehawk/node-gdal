var fs = require('fs');
var gdal = require('../lib/gdal.js');
var path = require('path');
var assert = require('chai').assert;
var fileUtils = require('./utils/file.js');

describe('gdal.VrtSimpleSource', function() {
	afterEach(gc);

	it('should be exposed', function() {
		assert.ok(gdal.VrtSimpleSource);
	});
	describe('should be instantiable', function() {
		it('should instantiate with no args', function() {
			assert.ok(new gdal.VrtSimpleSource());
		});

		it('should instantiate with args', function() {
			var src = new gdal.VrtSimpleSource();
			assert.ok(new gdal.VrtSimpleSource(src, 2, 1));
		});
	});

	describe('setSrcBand()', function() {
		it('should set the source band', function() {
			var ds   = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte);
			var band = ds.bands.get(1);
			var src = new gdal.VrtSimpleSource();
			src.setSrcBand(band);

			//TODO: Check actual srcBand
		});
	});

	describe('setSrcMaskBand()', function() {
		it('should set the source mask band', function() {
			var ds   = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte);
			var band = ds.bands.get(1);
			var src = new gdal.VrtSimpleSource();
			src.setSrcMaskBand(band);

			//TODO: Check actual srcMaskBand
		});
	});
	
	describe('setSrcWindow()', function() {
		it('should set the source window', function() {
			var src = new gdal.VrtSimpleSource();
			src.setSrcWindow(20, 10, 200, 100);

			//TODO: Check actual srcWindow
		});
	});
		
	describe('setDstWindow()', function() {
		it('should set the destination window', function() {
			var src = new gdal.VrtSimpleSource();
			src.setDstWindow(20, 10, 200, 100);

			//TODO: Check actual dstWindow
		});
	});
});