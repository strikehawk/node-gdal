var gdal = require('../lib/gdal.js');
var assert = require('chai').assert;

describe('gdal.translate', function () {
    afterEach(gc);

    const srcFilePath = "C:\\Geo\\Oman\\Sources\\ASTGTM2_N23E057_dem.tif";
    const targetProjection = "EPSG:3857";

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
                gdal.translate("", targetProjection, [0,0,0,0], 1, 1);
            });
        });

        it('validates target projection', function () {
            assert.throws(function () {
                // empty projection
                gdal.translate(srcFilePath, "", [0,0,0,0], 1, 1);
            });

            assert.throws(function () {
                // invalid projection
                gdal.translate(srcFilePath, "this won't work", [0,0,0,0], 1, 1);
            });
        });

        it('validates bbox', function () {
            assert.throws(function () {
                // incorrect number of args
                gdal.translate(srcFilePath, targetProjection, [0], 1);
            });

            assert.throws(function () {
                // wrong type of args
                gdal.translate(srcFilePath, targetProjection, [{},{},{},{}], 1, 1);
            });
        });
        
        it('validates width', function () {
            assert.throws(function () {
                // negative width
                gdal.translate(srcFilePath, targetProjection, [0,0,0,0], -1, 1);
            });
        });
                
        it('validates height', function () {
            assert.throws(function () {
                // negative height
                gdal.translate(srcFilePath, targetProjection, [0,0,0,0], 1, -1);
            });
        });

        it('accepts correct args', function() {
            // gdal.startLogging("C:\\Projects\\GitHub\\custom.log");
            try {
                const ds = gdal.translate(srcFilePath, targetProjection, [6345210.98,2692598.22,6400870.72,2632018.64], 400, 400);
                assert.ok(ds);
                assert.instanceOf(ds, gdal.Dataset);
            } catch (e) { 
                console.log(e);
            } finally {
                // gdal.stopLogging();
            }
        });
        
        it('accepts OMAN test BBOX', function() {
            gdal.startLogging("C:\\Projects\\GitHub\\custom.log");
            try {
                const vrtPath = "C:\\Geo\\Oman\\Sentinel\\muscat.vrt";
                const ds = gdal.translate(vrtPath, "EPSG:4326", [58.1,23.7,58.4,23.5], 400, 400);
                assert.ok(ds);
                assert.instanceOf(ds, gdal.Dataset);
            } catch (e) { 
                console.log(e);
            } finally {
                gdal.stopLogging();
            }
        });
    });
});