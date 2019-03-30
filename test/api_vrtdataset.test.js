var fs = require('fs');
var gdal = require('../lib/gdal.js');
var path = require('path');
var assert = require('chai').assert;
var fileUtils = require('./utils/file.js');

describe('gdal.VrtDataset', function () {
  afterEach(gc);

  it('should be exposed', function () {
    assert.ok(gdal.VrtDataset);
  });

  // it('gdal.createVrt should be exposed', function () {
  //   assert.ok(gdal.createVrt);
  // });

  it('should be instantiable', function () {
    const vrt = new gdal.VrtDataset(200, 50);
    assert.ok(vrt);
    assert.strictEqual(vrt.driver.description, "VRT");

    assert.instanceOf(vrt, gdal.Dataset);
    assert.instanceOf(vrt, gdal.VrtDataset);
  });

  describe('check inheritance', function() {
    it('should have bands', function () {
      const vrt = new gdal.VrtDataset(200, 50);
      assert.ok(vrt.bands);
    });

    it('should have a driver', function () {
      const vrt = new gdal.VrtDataset(200, 50);
      assert.ok(vrt.driver);
    });
  });

  describe('bands', function() {
    it('should be of type VrtDatasetBands', function () {
      const vrt = new gdal.VrtDataset(200, 50);
      assert.instanceOf(vrt.bands, gdal.VrtDatasetBands);
    });

    // it('creates instances of VrtSourcedRasterBands', function () {
    //   gdal.startLogging("C:\\Projects\\GitHub\\node_gdal.log");
    //   gdal.log("test de log");
    //   const vrt = new gdal.VrtDataset(200, 50);
    //   console.log("Dataset UID: " + vrt._uid);
    //   gdal.stopLogging();

    //   const band = vrt.bands.create(undefined);
    // });
  });
});