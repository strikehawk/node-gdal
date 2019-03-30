var fs = require('fs');
var gdal = require('../lib/gdal.js');
var path = require('path');
var assert = require('chai').assert;

describe('gdal.translate', function () {
    afterEach(gc);

    const srcFilePath = "C:\\Geo\\Oman\\Sources\\ASTGTM2_N23E057_dem.tif";

    it('should be exposed', function () {
        assert.ok(gdal.translate);
    });

    describe('validates the inputs', function () {
        it('expects args', function () {
            assert.throws(function () {
                // no args
                gdal.translate();
            });
        });

        it('validates source file path', function () {
            assert.throws(function () {
                // negative resolution
                gdal.translate("", [0,0,0,0], -1);
            });
        });

        it('validates bbox', function () {
            assert.throws(function () {
                // incorrect number of args
                gdal.translate(srcFilePath, [0], 1);
            });

            assert.throws(function () {
                // wrong type of args
                gdal.translate(srcFilePath, [{},{},{},{}], 1);
            });
        });
        
        it('validates resolution', function () {
            assert.throws(function () {
                // negative resolution
                gdal.translate(srcFilePath, [0,0,0,0], -1);
            });
        });

        it('accepts correct args', function() {
            gdal.startLogging("C:\\Projects\\GitHub\\custom.log");
            const res = gdal.translate(srcFilePath, [57,23,57.5,23.5], 400);
            console.log("Translate result: " + res);
            assert.ok(res);
            gdal.stopLogging();
        });
    });
});