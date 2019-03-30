#include "../gdal_common.hpp"
#include "../gdal_majorobject.hpp"
#include "../gdal_driver.hpp"
#include "../gdal_spatial_reference.hpp"
#include "../gdal_layer.hpp"
#include "../gdal_geometry.hpp"
#include "gdal_vrtdataset.hpp"
#include "vrtdataset_bands.hpp"
#include "../collections/dataset_layers.hpp"

namespace node_gdal {

Nan::Persistent<FunctionTemplate> VrtDataset::constructor;
ObjectCache<VRTDataset, VrtDataset> VrtDataset::vrtdataset_cache;

void VrtDataset::Initialize(Local<Object> target)
{
	Nan::HandleScope scope;

	Local<FunctionTemplate> lcons = Nan::New<FunctionTemplate>(VrtDataset::New);
	lcons->Inherit(Nan::New(Dataset::constructor)); 
	lcons->InstanceTemplate()->SetInternalFieldCount(1);
	lcons->SetClassName(Nan::New("VrtDataset").ToLocalChecked());
	
	Nan::SetPrototypeMethod(lcons, "toString", toString);
	// Nan::SetPrototypeMethod(lcons, "setGCPs", setGCPs);
	// Nan::SetPrototypeMethod(lcons, "getGCPs", getGCPs);
	// Nan::SetPrototypeMethod(lcons, "getGCPProjection", getGCPProjection);
	// Nan::SetPrototypeMethod(lcons, "getFileList", getFileList);
	// Nan::SetPrototypeMethod(lcons, "flush", flush);
	// Nan::SetPrototypeMethod(lcons, "close", close);
	// Nan::SetPrototypeMethod(lcons, "getMetadata", getMetadata);
	// Nan::SetPrototypeMethod(lcons, "testCapability", testCapability);
	// Nan::SetPrototypeMethod(lcons, "executeSQL", executeSQL);
	// Nan::SetPrototypeMethod(lcons, "buildOverviews", buildOverviews);

	ATTR_DONT_ENUM(lcons, "_uid", uidGetter, READ_ONLY_SETTER);
	// ATTR(lcons, "description", descriptionGetter, READ_ONLY_SETTER);
	// ATTR(lcons, "bands", bandsGetter, READ_ONLY_SETTER);
	// ATTR(lcons, "layers", layersGetter, READ_ONLY_SETTER);
	// ATTR(lcons, "rasterSize", rasterSizeGetter, READ_ONLY_SETTER);
	// ATTR(lcons, "driver", driverGetter, READ_ONLY_SETTER);
	// ATTR(lcons, "srs", srsGetter, srsSetter);
	// ATTR(lcons, "geoTransform", geoTransformGetter, geoTransformSetter);

	target->Set(Nan::New("VrtDataset").ToLocalChecked(), lcons->GetFunction());

	constructor.Reset(lcons);
}

VrtDataset::VrtDataset(VRTDataset *ds)
	: Nan::ObjectWrap(),
	  uid(0),
	  this_dataset(ds)
{
	LOG("Created VrtDataset [%p]", ds);
	LOG("Dataset type: %s", ds->GetDriver()->GetDescription());
}

VrtDataset::~VrtDataset()
{
	//Destroy at garbage collection time if not already explicitly destroyed
	dispose();
}

void VrtDataset::dispose()
{

	if (this_dataset) {
		LOG("Disposing VrtDataset [%p]", this_dataset);

		ptr_manager.dispose(uid);

		LOG("Disposed VrtDataset [%p]", this_dataset);

		this_dataset = NULL;
	}
}

/**
 * A set of associated raster bands and/or vector layers, usually from one file.
 *
 * ```
 * // raster VrtDataset:
 * VrtDataset = gdal.open('file.tif');
 * bands = VrtDataset.bands;
 *
 * // vector VrtDataset:
 * VrtDataset = gdal.open('file.shp');
 * layers = VrtDataset.layers;```
 *
 * @class gdal.VrtDataset
 */
NAN_METHOD(VrtDataset::New)
{
	Nan::HandleScope scope;
	VrtDataset *f;

	LOG("NAN_METHOD(VrtDataset::New)");

	if (!info.IsConstructCall()) {
		Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
		return;
	}
	if (info[0]->IsExternal()) {
		Local<External> ext = info[0].As<External>();
		void* ptr = ext->Value();
		f =  static_cast<VrtDataset *>(ptr);
	} else {
		int xSize;
		int ySize;

		NODE_ARG_INT(0, "xSize", xSize);
		NODE_ARG_INT(1, "ySize", ySize);

		VRTDataset *ds = new VRTDataset(xSize, ySize);
		LOG("VRTDataset xSize: %ld", ds->GetRasterXSize());
		LOG("VRTDataset ySize: %ld", ds->GetRasterYSize());

		f = new VrtDataset(ds);

		LOG("NAN_METHOD(VrtDataset::New) => UID: [%ld]", f->uid);
	}


	Local<Value> bands = VrtDatasetBands::New(info.This());
	Nan::SetPrivate(info.This(), Nan::New("bands_").ToLocalChecked(), bands);

	Local<Value> layers = DatasetLayers::New(info.This());
	Nan::SetPrivate(info.This(), Nan::New("layers_").ToLocalChecked(), layers);

	f->Wrap(info.This());
	info.GetReturnValue().Set(info.This());
}

Local<Value> VrtDataset::New(VRTDataset *raw)
{
	LOG("Local<Value> VrtDataset::New(VRTDataset *raw)");

	Nan::EscapableHandleScope scope;

	if (!raw) {
		return scope.Escape(Nan::Null());
	}

	if (vrtdataset_cache.has(raw)) {
		LOG("Local<Value> VrtDataset::New(VRTDataset *raw) => Cache hit");
		return scope.Escape(vrtdataset_cache.get(raw));
	}

	VrtDataset *wrapped = new VrtDataset(raw);

	Local<Value> ext = Nan::New<External>(wrapped);
	Local<Object> obj = Nan::NewInstance(Nan::New(VrtDataset::constructor)->GetFunction(), 1, &ext).ToLocalChecked();

	vrtdataset_cache.add(raw, obj);
	wrapped->uid = ptr_manager.add(raw);

	return scope.Escape(obj);
}

NAN_METHOD(VrtDataset::toString)
{
	Nan::HandleScope scope;
	info.GetReturnValue().Set(Nan::New("VrtDataset").ToLocalChecked());
}

// /**
//  * Fetch metadata.
//  *
//  * @method getMetadata
//  * @param {string} [domain]
//  * @return {Object}
//  */
// NAN_METHOD(VrtDataset::getMetadata)
// {
// 	Nan::HandleScope scope;
// 	VrtDataset *ds = Nan::ObjectWrap::Unwrap<VrtDataset>(info.This());

// 	if(!ds->isAlive()){
// 		Nan::ThrowError("VrtDataset object has already been destroyed");
// 		return;
// 	}

// 	VRTDataset* raw = ds->getDataset();
// 	std::string domain("");
// 	NODE_ARG_OPT_STR(0, "domain", domain);
// 	info.GetReturnValue().Set(MajorObject::getMetadata(raw, domain.empty() ? NULL : domain.c_str()));
// }

// /**
//  * Determines if the VrtDataset supports the indicated operation.
//  *
//  * @method testCapability
//  * @param {string} capability (see {{#crossLink "Constants (ODsC)"}}capability list{{/crossLink}})
//  * @return {Boolean}
//  */
// NAN_METHOD(VrtDataset::testCapability)
// {
// 	Nan::HandleScope scope;
// 	VrtDataset *ds = Nan::ObjectWrap::Unwrap<VrtDataset>(info.This());

// 	if(!ds->isAlive()){
// 		Nan::ThrowError("VrtDataset object has already been destroyed");
// 		return;
// 	}

// 	VRTDataset *raw = ds->getDataset();

// 	std::string capability("");
// 	NODE_ARG_STR(0, "capability", capability);

// 	info.GetReturnValue().Set(Nan::New<Boolean>(raw->TestCapability(capability.c_str())));
// }

// /**
//  * Get output projection for GCPs.
//  *
//  * @method getGCPProjection
//  * @return {String}
//  */
// NAN_METHOD(VrtDataset::getGCPProjection)
// {
// 	Nan::HandleScope scope;
// 	VrtDataset *ds = Nan::ObjectWrap::Unwrap<VrtDataset>(info.This());

// 	if(!ds->isAlive()){
// 		Nan::ThrowError("VrtDataset object has already been destroyed");
// 		return;
// 	}

// 	VRTDataset* raw = ds->getDataset();
// 	info.GetReturnValue().Set(SafeString::New(raw->GetGCPProjection()));
// }

// /**
//  * Closes the VrtDataset to further operations.
//  *
//  * @method close
//  */
// NAN_METHOD(VrtDataset::close)
// {
// 	VrtDataset *ds = Nan::ObjectWrap::Unwrap<VrtDataset>(info.This());

// 	if(!ds->isAlive()){
// 		Nan::ThrowError("VrtDataset object has already been destroyed");
// 		return;
// 	}

// 	ds->dispose();

// 	return;
// }

// /**
//  * Flushes all changes to disk.
//  *
//  * @throws Error
//  * @method flush
//  */
// NAN_METHOD(VrtDataset::flush)
// {
// 	Nan::HandleScope scope;
// 	VrtDataset *ds = Nan::ObjectWrap::Unwrap<VrtDataset>(info.This());

// 	if(!ds->isAlive()){
// 		Nan::ThrowError("VrtDataset object has already been destroyed");
// 		return;
// 	}

// 	VRTDataset* raw = ds->getDataset();
// 	if (!raw) {
// 		Nan::ThrowError("VrtDataset object has already been destroyed");
// 		return;
// 	}
// 	raw->FlushCache();

// 	return;
// }

// /**
//  * Execute an SQL statement against the data store.
//  *
//  * @throws Error
//  * @method executeSQL
//  * @param {String} statement SQL statement to execute.
//  * @param {gdal.Geometry} [spatial_filter=null] Geometry which represents a spatial filter.
//  * @param {String} [dialect=null] Allows control of the statement dialect. If set to `null`, the OGR SQL engine will be used, except for RDBMS drivers that will use their dedicated SQL engine, unless `"OGRSQL"` is explicitely passed as the dialect. Starting with OGR 1.10, the `"SQLITE"` dialect can also be used.
//  * @return {gdal.Layer}
//  */
// NAN_METHOD(VrtDataset::executeSQL)
// {
// 	Nan::HandleScope scope;
// 	VrtDataset *ds = Nan::ObjectWrap::Unwrap<VrtDataset>(info.This());

// 	if(!ds->isAlive()){
// 		Nan::ThrowError("VrtDataset object has already been destroyed");
// 		return;
// 	}

// 	VRTDataset* raw = ds->getDataset();

// 	std::string sql;
// 	std::string sql_dialect;
// 	Geometry *spatial_filter = NULL;

// 	NODE_ARG_STR(0, "sql text", sql);
// 	NODE_ARG_WRAPPED_OPT(1, "spatial filter geometry", Geometry, spatial_filter);
// 	NODE_ARG_OPT_STR(2, "sql dialect", sql_dialect);

// 	OGRLayer *layer = raw->ExecuteSQL(sql.c_str(),
// 											spatial_filter ? spatial_filter->get() : NULL,
// 											sql_dialect.empty() ? NULL : sql_dialect.c_str());

// 	if (layer) {
// 		info.GetReturnValue().Set(Layer::New(layer, raw, true));
// 		return;
// 	} else {
// 		Nan::ThrowError("Error executing SQL");
// 		return;
// 	}
// }

// /**
//  * Fetch files forming VrtDataset.
//  *
//  * Returns a list of files believed to be part of this VrtDataset. If it returns an
//  * empty list of files it means there is believed to be no local file system files
//  * associated with the VrtDataset (for instance a virtual VrtDataset).
//  *
//  * Returns an empty array for vector datasets if GDAL version is below 2.0
//  *
//  * @method getFileList
//  * @return {String[]}
//  */
// NAN_METHOD(VrtDataset::getFileList)
// {
// 	Nan::HandleScope scope;
// 	VrtDataset *ds = Nan::ObjectWrap::Unwrap<VrtDataset>(info.This());

// 	Local<Array> results = Nan::New<Array>(0);

// 	if(!ds->isAlive()){
// 		Nan::ThrowError("VrtDataset object has already been destroyed");
// 		return;
// 	}

// 	VRTDataset* raw = ds->getDataset();
// 	if (!raw) {
// 		Nan::ThrowError("VrtDataset object has already been destroyed");
// 		return;
// 	}

// 	char **list = raw->GetFileList();
// 	if (!list) {
// 		info.GetReturnValue().Set(results);
// 		return;
// 	}

// 	int i = 0;
// 	while (list[i]) {
// 		results->Set(i, SafeString::New(list[i]));
// 		i++;
// 	}

// 	CSLDestroy(list);

// 	info.GetReturnValue().Set(results);
// }

// /**
//  * Fetches GCPs.
//  *
//  * @method getGCPs
//  * @return {Object[]}
//  */
// NAN_METHOD(VrtDataset::getGCPs)
// {
// 	Nan::HandleScope scope;
// 	VrtDataset *ds = Nan::ObjectWrap::Unwrap<VrtDataset>(info.This());

// 	Local<Array> results = Nan::New<Array>(0);

// 	if(!ds->isAlive()){
// 		Nan::ThrowError("VrtDataset object has already been destroyed");
// 		return;
// 	}

// 	VRTDataset* raw = ds->getDataset();
// 	if (!raw) {
// 		Nan::ThrowError("VrtDataset object has already been destroyed");
// 		return;
// 	}

// 	int n = raw->GetGCPCount();
// 	const GDAL_GCP *gcps = raw->GetGCPs();

// 	if (!gcps) {
// 		info.GetReturnValue().Set(results);
// 		return;
// 	}

// 	for (int i = 0; i < n; i++) {
// 		GDAL_GCP gcp = gcps[i];
// 		Local<Object> obj = Nan::New<Object>();
// 		obj->Set(Nan::New("pszId").ToLocalChecked(), Nan::New(gcp.pszId).ToLocalChecked());
// 		obj->Set(Nan::New("pszInfo").ToLocalChecked(), Nan::New(gcp.pszInfo).ToLocalChecked());
// 		obj->Set(Nan::New("dfGCPPixel").ToLocalChecked(), Nan::New<Number>(gcp.dfGCPPixel));
// 		obj->Set(Nan::New("dfGCPLine").ToLocalChecked(), Nan::New<Number>(gcp.dfGCPLine));
// 		obj->Set(Nan::New("dfGCPX").ToLocalChecked(), Nan::New<Number>(gcp.dfGCPX));
// 		obj->Set(Nan::New("dfGCPY").ToLocalChecked(), Nan::New<Number>(gcp.dfGCPY));
// 		obj->Set(Nan::New("dfGCPZ").ToLocalChecked(), Nan::New<Number>(gcp.dfGCPZ));
// 		results->Set(i, obj);
// 	}

// 	info.GetReturnValue().Set(results);
// }

// /**
//  * Sets GCPs.
//  *
//  * @throws Error
//  * @method setGCPs
//  * @param {Object[]} gcps
//  * @param {String} projection
//  */
// NAN_METHOD(VrtDataset::setGCPs)
// {
// 	Nan::HandleScope scope;
// 	VrtDataset *ds = Nan::ObjectWrap::Unwrap<VrtDataset>(info.This());

// 	if(!ds->isAlive()){
// 		Nan::ThrowError("VrtDataset object has already been destroyed");
// 		return;
// 	}

// 	VRTDataset* raw = ds->getDataset();
// 	if (!raw) {
// 		Nan::ThrowError("VrtDataset object has already been destroyed");
// 		return;
// 	}

// 	Local<Array> gcps;
// 	std::string projection("");
// 	NODE_ARG_ARRAY(0, "gcps", gcps);
// 	NODE_ARG_OPT_STR(1, "projection", projection);

// 	GDAL_GCP *list = new GDAL_GCP [gcps->Length()];
// 	std::string *pszId_list = new std::string [gcps->Length()];
// 	std::string *pszInfo_list = new std::string [gcps->Length()];
// 	GDAL_GCP *gcp = list;
// 	for (unsigned int i = 0; i < gcps->Length(); ++i) {
// 		Local<Value> val = gcps->Get(i);
// 		if (!val->IsObject()) {
// 			if (list) {
// 				delete [] list;
// 				delete [] pszId_list;
// 				delete [] pszInfo_list;
// 			}
// 			Nan::ThrowError("GCP array must only include objects");
// 			return;
// 		}
// 		Local<Object> obj = val.As<Object>();

// 		NODE_DOUBLE_FROM_OBJ(obj, "dfGCPPixel", gcp->dfGCPPixel);
// 		NODE_DOUBLE_FROM_OBJ(obj, "dfGCPLine", gcp->dfGCPLine);
// 		NODE_DOUBLE_FROM_OBJ(obj, "dfGCPX", gcp->dfGCPX);
// 		NODE_DOUBLE_FROM_OBJ(obj, "dfGCPY", gcp->dfGCPY);
// 		NODE_DOUBLE_FROM_OBJ_OPT(obj, "dfGCPZ", gcp->dfGCPZ);
// 		NODE_STR_FROM_OBJ_OPT(obj, "pszId", pszId_list[i]);
// 		NODE_STR_FROM_OBJ_OPT(obj, "pszInfo", pszInfo_list[i]);

// 		gcp->pszId = (char*) pszId_list[i].c_str();
// 		gcp->pszInfo = (char*) pszInfo_list[i].c_str();

// 		gcp++;
// 	}

// 	CPLErr err = raw->SetGCPs(gcps->Length(), list, projection.c_str());

// 	if (list) {
// 		delete [] list;
// 		delete [] pszId_list;
// 		delete [] pszInfo_list;
// 	}

// 	if (err) {
// 		NODE_THROW_CPLERR(err);
// 		return;
// 	}

// 	return;
// }

// /**
//  * Builds VrtDataset overviews.
//  *
//  * @throws Error
//  * @method buildOverviews
//  * @param {String} resampling `"NEAREST"`, `"GAUSS"`, `"CUBIC"`, `"AVERAGE"`, `"MODE"`, `"AVERAGE_MAGPHASE"` or `"NONE"`
//  * @param {Integer[]} overviews
//  * @param {Integer[]} [bands] Note: Generation of overviews in external TIFF currently only supported when operating on all bands.
//  */
// NAN_METHOD(VrtDataset::buildOverviews)
// {
// 	Nan::HandleScope scope;
// 	VrtDataset *ds = Nan::ObjectWrap::Unwrap<VrtDataset>(info.This());

// 	if(!ds->isAlive()){
// 		Nan::ThrowError("VrtDataset object has already been destroyed");
// 		return;
// 	}

// 	VRTDataset* raw = ds->getDataset();
// 	std::string resampling = "";
// 	Local<Array> overviews;
// 	Local<Array> bands;

// 	NODE_ARG_STR(0, "resampling", resampling);
// 	NODE_ARG_ARRAY(1, "overviews", overviews);
// 	NODE_ARG_ARRAY_OPT(2, "bands", bands);

// 	int *o, *b = NULL;
// 	int n_overviews = overviews->Length();
// 	int i, n_bands = 0;

// 	o = new int[n_overviews];
// 	for(i = 0; i<n_overviews; i++){
// 		Local<Value> val = overviews->Get(i);
// 		if(!val->IsNumber()) {
// 			delete [] o;
// 			Nan::ThrowError("overviews array must only contain numbers");
// 			return;
// 		}
// 		o[i] = val->Int32Value();
// 	}

// 	if(!bands.IsEmpty()){
// 		n_bands = bands->Length();
// 		b = new int[n_bands];
// 		for(i = 0; i<n_bands; i++){
// 			Local<Value> val = bands->Get(i);
// 			if(!val->IsNumber()) {
// 				delete [] o;
// 				delete [] b;
// 				Nan::ThrowError("band array must only contain numbers");
// 				return;
// 			}
// 			b[i] = val->Int32Value();
// 			if(b[i] > raw->GetRasterCount() || b[i] < 1) {
// 				//BuildOverviews prints an error but segfaults before returning
// 				delete [] o;
// 				delete [] b;
// 				Nan::ThrowError("invalid band id");
// 				return;
// 			}
// 		}
// 	}

// 	CPLErr err = raw->BuildOverviews(resampling.c_str(), n_overviews, o, n_bands, b, NULL, NULL);

// 	delete [] o;
// 	if(b) delete [] b;

// 	if(err) {
// 		NODE_THROW_CPLERR(err);
// 		return;
// 	}

// 	return;
// }

// /**
//  * @readOnly
//  * @attribute description
//  * @type String
//  */
// NAN_GETTER(VrtDataset::descriptionGetter)
// {
// 	Nan::HandleScope scope;
// 	VrtDataset *ds = Nan::ObjectWrap::Unwrap<VrtDataset>(info.This());

// 	if(!ds->isAlive()){
// 		Nan::ThrowError("VrtDataset object has already been destroyed");
// 		return;
// 	}

// 	#if GDAL_VERSION_MAJOR < 2
// 	if (ds->uses_ogr) {
// 		OGRDataSource* raw = ds->getDatasource();
// 		info.GetReturnValue().Set(SafeString::New(raw->GetName()));
// 		return;
// 	}
// 	#endif

// 	VRTDataset* raw = ds->getDataset();
// 	if (!raw) {
// 		Nan::ThrowError("VrtDataset object has already been destroyed");
// 		return;
// 	}
// 	info.GetReturnValue().Set(SafeString::New(raw->GetDescription()));
// }

// /**
//  * Raster dimensions. An object containing `x` and `y` properties.
//  *
//  * @readOnly
//  * @attribute rasterSize
//  * @type Object
//  */
// NAN_GETTER(VrtDataset::rasterSizeGetter)
// {
// 	Nan::HandleScope scope;
// 	VrtDataset *ds = Nan::ObjectWrap::Unwrap<VrtDataset>(info.This());

// 	if(!ds->isAlive()){
// 		Nan::ThrowError("VrtDataset object has already been destroyed");
// 		return;
// 	}

// 	#if GDAL_VERSION_MAJOR < 2
// 	if (ds->uses_ogr) {
// 		info.GetReturnValue().Set(Nan::Null());
// 		return;
// 	}
// 	#endif

// 	VRTDataset* raw = ds->getDataset();

// 	//GDAL 2.x will return 512x512 for vector datasets... which doesn't really make sense in JS where we can return null instead of a number
// 	//https://github.com/OSGeo/gdal/blob/beef45c130cc2778dcc56d85aed1104a9b31f7e6/gdal/gcore/gdaldataset.cpp#L173-L174
// 	#if GDAL_VERSION_MAJOR >= 2
// 	if(!raw->GetDriver()->GetMetadataItem(GDAL_DCAP_RASTER)){
// 		info.GetReturnValue().Set(Nan::Null());
// 		return;
// 	}
// 	#endif

// 	Local<Object> result = Nan::New<Object>();
// 	result->Set(Nan::New("x").ToLocalChecked(), Nan::New<Integer>(raw->GetRasterXSize()));
// 	result->Set(Nan::New("y").ToLocalChecked(), Nan::New<Integer>(raw->GetRasterYSize()));
// 	info.GetReturnValue().Set(result);
// }

// /**
//  * Spatial reference associated with raster VrtDataset
//  *
//  * @throws Error
//  * @attribute srs
//  * @type {gdal.SpatialReference}
//  */
// NAN_GETTER(VrtDataset::srsGetter)
// {
// 	Nan::HandleScope scope;
// 	VrtDataset *ds = Nan::ObjectWrap::Unwrap<VrtDataset>(info.This());

// 	if(!ds->isAlive()){
// 		Nan::ThrowError("VrtDataset object has already been destroyed");
// 		return;
// 	}

// 	#if GDAL_VERSION_MAJOR < 2
// 	if (ds->uses_ogr) {
// 		info.GetReturnValue().Set(Nan::Null());
// 		return;
// 	}
// 	#endif

// 	VRTDataset* raw = ds->getDataset();
// 	//get projection wkt and return null if not set
// 	char* wkt = (char*) raw->GetProjectionRef();
// 	if (*wkt == '\0') {
// 		//getProjectionRef returns string of length 0 if no srs set
// 		info.GetReturnValue().Set(Nan::Null());
// 		return;
// 	}
// 	//otherwise construct and return SpatialReference from wkt
// 	OGRSpatialReference *srs = new OGRSpatialReference();
// 	int err = srs->importFromWkt(&wkt);

// 	if(err) {
// 		NODE_THROW_OGRERR(err);
// 		return;
// 	}

// 	info.GetReturnValue().Set(SpatialReference::New(srs, true));
// }

// /**
//  * An affine transform which maps pixel/line coordinates into georeferenced space using the following relationship:
//  *
//  * @example
//  * ```
//  * var GT = VrtDataset.geoTransform;
//  * var Xgeo = GT[0] + Xpixel*GT[1] + Yline*GT[2];
//  * var Ygeo = GT[3] + Xpixel*GT[4] + Yline*GT[5];```
//  *
//  * @attribute geoTransform
//  * @type {Array}
//  */
// NAN_GETTER(VrtDataset::geoTransformGetter)
// {
// 	Nan::HandleScope scope;
// 	VrtDataset *ds = Nan::ObjectWrap::Unwrap<VrtDataset>(info.This());

// 	if(!ds->isAlive()){
// 		Nan::ThrowError("VrtDataset object has already been destroyed");
// 		return;
// 	}

// 	#if GDAL_VERSION_MAJOR < 2
// 	if (ds->uses_ogr) {
// 		info.GetReturnValue().Set(Nan::Null());
// 		return;
// 	}
// 	#endif

// 	VRTDataset* raw = ds->getDataset();
// 	double transform[6];
// 	CPLErr err = raw->GetGeoTransform(transform);
// 	if(err) {
// 		// This is mostly (always?) a sign that it has not been set
// 		info.GetReturnValue().Set(Nan::Null());
// 		return;
// 		//NODE_THROW_CPLERR(err);
// 	}

// 	Local<Array> result = Nan::New<Array>(6);
// 	result->Set(0, Nan::New<Number>(transform[0]));
// 	result->Set(1, Nan::New<Number>(transform[1]));
// 	result->Set(2, Nan::New<Number>(transform[2]));
// 	result->Set(3, Nan::New<Number>(transform[3]));
// 	result->Set(4, Nan::New<Number>(transform[4]));
// 	result->Set(5, Nan::New<Number>(transform[5]));

// 	info.GetReturnValue().Set(result);
// }

// /**
//  * @readOnly
//  * @attribute driver
//  * @type {gdal.Driver}
//  */
// NAN_GETTER(VrtDataset::driverGetter)
// {
// 	Nan::HandleScope scope;
// 	VrtDataset *ds = Nan::ObjectWrap::Unwrap<VrtDataset>(info.This());

// 	if(!ds->isAlive()){
// 		Nan::ThrowError("VrtDataset object has already been destroyed");
// 		return;
// 	}

// 	#if GDAL_VERSION_MAJOR < 2
// 	if (ds->uses_ogr) {
// 		OGRDataSource* raw = ds->getDatasource();
// 		info.GetReturnValue().Set(Driver::New(raw->GetDriver()));
// 		return;
// 	}
// 	#endif

// 	VRTDataset* raw = ds->getDataset();
// 	info.GetReturnValue().Set(Driver::New(raw->GetDriver()));
// }

// NAN_SETTER(VrtDataset::srsSetter)
// {
// 	Nan::HandleScope scope;
// 	VrtDataset *ds = Nan::ObjectWrap::Unwrap<VrtDataset>(info.This());

// 	if(!ds->isAlive()){
// 		Nan::ThrowError("VrtDataset object has already been destroyed");
// 		return;
// 	}

// 	VRTDataset* raw = ds->getDataset();
// 	std::string wkt("");
// 	if (IS_WRAPPED(value, SpatialReference)) {

// 		SpatialReference *srs_obj = Nan::ObjectWrap::Unwrap<SpatialReference>(value.As<Object>());
// 		OGRSpatialReference *srs = srs_obj->get();
// 		//Get wkt from OGRSpatialReference
// 		char* str;
// 		if (srs->exportToWkt(&str)) {
// 			Nan::ThrowError("Error exporting srs to wkt");
// 			return;
// 		}
// 		wkt = str; //copy string
// 		CPLFree(str);

// 	} else if (!value->IsNull() && !value->IsUndefined()) {
// 		Nan::ThrowError("srs must be SpatialReference object");
// 		return;
// 	}

// 	CPLErr err = raw->SetProjection(wkt.c_str());

// 	if(err) {
// 		NODE_THROW_CPLERR(err);
// 	}
// }

// NAN_SETTER(VrtDataset::geoTransformSetter)
// {
// 	Nan::HandleScope scope;
// 	VrtDataset *ds = Nan::ObjectWrap::Unwrap<VrtDataset>(info.This());

// 	if(!ds->isAlive()){
// 		Nan::ThrowError("VrtDataset object has already been destroyed");
// 		return;
// 	}

// 	VRTDataset* raw = ds->getDataset();

// 	if (!value->IsArray()) {
// 		Nan::ThrowError("Transform must be an array");
// 		return;
// 	}
// 	Local<Array> transform = value.As<Array>();

// 	if (transform->Length() != 6) {
// 		Nan::ThrowError("Transform array must have 6 elements");
// 		return;
// 	}

// 	double buffer[6];
// 	for (int i = 0; i < 6; i++) {
// 		Local<Value> val = transform->Get(i);
// 		if (!val->IsNumber()) {
// 			Nan::ThrowError("Transform array must only contain numbers");
// 			return;
// 		}
// 		buffer[i] = val->NumberValue();
// 	}

// 	CPLErr err = raw->SetGeoTransform(buffer);
// 	if(err) {
// 		NODE_THROW_CPLERR(err);
// 	}
// }

// /**
//  * @readOnly
//  * @attribute bands
//  * @type {gdal.DatasetBands}
//  */
// NAN_GETTER(VrtDataset::bandsGetter)
// {
// 	Nan::HandleScope scope;
// 	info.GetReturnValue().Set(Nan::GetPrivate(info.This(), Nan::New("bands_").ToLocalChecked()).ToLocalChecked());
// }

// /**
//  * @readOnly
//  * @attribute layers
//  * @type {gdal.DatasetLayers}
//  */
// NAN_GETTER(VrtDataset::layersGetter)
// {
// 	Nan::HandleScope scope;
// 	info.GetReturnValue().Set(Nan::GetPrivate(info.This(), Nan::New("layers_").ToLocalChecked()).ToLocalChecked());
// }


NAN_GETTER(VrtDataset::uidGetter)
{
	Nan::HandleScope scope;
	VrtDataset *ds = Nan::ObjectWrap::Unwrap<VrtDataset>(info.This());
	info.GetReturnValue().Set(Nan::New((int)ds->uid));
}

} // namespace node_gdal
